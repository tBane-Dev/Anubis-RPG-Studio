#include "3D/Scene.hpp"
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>
#include "DebugLog.hpp"
#include "RenderWindow.hpp"
#include "Cursor.hpp"
#include "Time.hpp"
#include "3D/OpenGLBuffers.hpp"
#include <unordered_set>
#include <algorithm>
#include <functional>

namespace Anubis {

	sf::Vector3f shadowNormalize(sf::Vector3f v) {
		float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		return length > 0.f ? v / length : v;
	}

	sf::Vector3f shadowCross(sf::Vector3f a, sf::Vector3f b) {
		return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
	}

	float shadowDot(sf::Vector3f a, sf::Vector3f b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	// a * b, column-major.
	std::array<float, 16> shadowMultiply(std::array<float, 16> a, std::array<float, 16> b) {
		std::array<float, 16> result{};

		for (int column = 0; column < 4; column++)
			for (int row = 0; row < 4; row++)
				for (int k = 0; k < 4; k++)
					result[column * 4 + row] += a[k * 4 + row] * b[column * 4 + k];

		return result;
	}

	Scene::Scene() {
		_cameraXAngle = 45.f;
		_cameraYAngle = 0.f;
		_cameraRadius = 500.f;

		_camera = std::make_shared<Camera>();

		_camera->_rotation = Quaternion(
			std::cos(_cameraXAngle * 3.14159265f / 180.f * 0.5f),
			std::sin(_cameraXAngle * 3.14159265f / 180.f * 0.5f),
			0.f,
			0.f
		);

		_lastCameraMouse = sf::Vector2i(0, 0);

		float horizontalRadius = std::cos(_cameraXAngle * 3.14159265f / 180.f) * _cameraRadius;

		_camera->_position.x = std::sin(_cameraYAngle) * horizontalRadius;
		_camera->_position.y = std::sin(_cameraXAngle * 3.14159265f / 180.f) * _cameraRadius;
		_camera->_position.z = std::cos(_cameraYAngle) * horizontalRadius;

		_cameraZoom = 1.0f;

		_isPressedMiddle = false;
		_panPlaneY = 0.f;
		_cameraIsMoving = false;

		_pickingFBO = 0;
		_pickingColor = 0;
		_pickingDepth = 0;
		_pickingWidth = 0;
		_pickingHeight = 0;

		_fovY = 60.f;
		_nearPlane = 0.1f;
		_farPlane = 10000.f;

		_cameraSpeed = 400.f;

		_shadowMapSize = 2048;
		_shadowDepthComponent = 0x81A7;
		_shadowSamples = 3;
		_maxShadowRadius = 4000.f;
		_shadowFBO = 0;
		_shadowDepth = 0;
		_sunDirection = { 0.1f, -1.0f, -0.1f };
	}

	Scene::~Scene() {

		if (_pickingFBO != 0)
			GL::deleteFramebuffers(1, &_pickingFBO);

		if (_pickingColor != 0)
			glDeleteTextures(1, &_pickingColor);

		if (_pickingDepth != 0)
			glDeleteTextures(1, &_pickingDepth);

		if (_shadowFBO != 0)
			GL::deleteFramebuffers(1, &_shadowFBO);

		if (_shadowDepth != 0)
			glDeleteTextures(1, &_shadowDepth);
	}

	bool Scene::initializeModel3DShaders() {

		if (!sf::Shader::isAvailable()) {
			DebugError(L"Shaders are not supported");
			return false;
		}

		if (!_modelShader.loadFromFile("assets\\shr\\model3d.vert", "assets\\shr\\model3d.frag")) {
			DebugError(L"Failed to load model3D shaders");
			return false;
		}

		// Use separate texture units for different sampler types.
		_modelShader.setUniform("uTexture", 0);
		_modelShader.setUniform("uJointMatrices", 1);

		// Start with static mesh rendering.
		_modelShader.setUniform("uSkinned", false);
		_modelShader.setUniform("uPicking", false);
		_modelShader.setUniform("uTextured", false);
		_modelShader.setUniform("uTint", sf::Glsl::Vec4(1.f, 1.f, 1.f, 1.f));
		_modelShader.setUniform("uPickColor", sf::Glsl::Vec3(0.f, 0.f, 0.f));

		// Uniform locations for the fast drawing path (beginModelPass / drawModelFast).
		GLuint program = _modelShader.getNativeHandle();

		if (GL::getUniformLocation && GL::uniformMatrix4fv && GL::uniform1i && GL::uniform3f && GL::uniform4f && program != 0) {
			_locModel = GL::getUniformLocation(program, "uModel");
			_locView = GL::getUniformLocation(program, "uView");
			_locProjection = GL::getUniformLocation(program, "uProjection");
			_locSkinned = GL::getUniformLocation(program, "uSkinned");
			_locPicking = GL::getUniformLocation(program, "uPicking");
			_locTextured = GL::getUniformLocation(program, "uTextured");
			_locTint = GL::getUniformLocation(program, "uTint");
			_locPickColor = GL::getUniformLocation(program, "uPickColor");

			_fastPathReady = _locModel >= 0 && _locView >= 0 && _locProjection >= 0;
		}

		if (!_fastPathReady) {
			DebugError(L"Model3D shader uniforms are unavailable");
			return false;
		}

		DebugLog(L"Model3D shaders loaded");
		return true;
	}

	void Scene::setViewport() {
		glViewport(0, 0, Anubis::render_window->getSize().x, Anubis::render_window->getSize().y);
	}

	void Scene::setProjection() {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		gluPerspective(
			_fovY,
			(float)render_window->getSize().x / (float)render_window->getSize().y,
			_nearPlane,
			_farPlane
		);
	}

	void Scene::setCamera() {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		std::array<float, 16> rotation = _camera->_rotation.matrix();
		glMultMatrixf(rotation.data());

		glTranslatef(-_camera->_position.x, -_camera->_position.y, -_camera->_position.z);
	}

	void Scene::addModel(std::shared_ptr<Model3D> model) {

		if (!model)
			return;

		if (std::find_if(_models.begin(), _models.end(),
			[&](std::shared_ptr<Model3D>& m) {
				return m.get() == model.get();
			}) == _models.end()) {

			model->setScene(this->shared_from_this());

			_models.push_back(model);
		}
	}

	void Scene::removeModel(std::shared_ptr<Model3D> model) {

		if (!model)
			return;

		size_t before = _models.size();

		std::erase(_hovered, model);
		std::erase(_pressed, model);
		std::erase(_selected, model);
		std::erase(_models, model);

		size_t after = _models.size();

		DebugStat(L"Models before: " + std::to_wstring(before));
		DebugStat(L"Models after: " + std::to_wstring(after));
		DebugStat(L"Removed: " + std::to_wstring(before - after));
	}

	void Scene::drawModels() {

		GLfloat view[16];
		GLfloat projection[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);

		if (!beginModelPass(view, projection, false, false))
			return;

		// Highlight lookups once per frame instead of 3 x std::find per model.
		std::unordered_set<std::shared_ptr<Model3D>> hovered;
		std::unordered_set<std::shared_ptr<Model3D>> highlighted;

		for (auto& model : _hovered) hovered.insert(model);
		for (auto& model : _pressed) highlighted.insert(model);
		for (auto& model : _selected) highlighted.insert(model);

		// Same colors as Model3D::draw().
		GLfloat normalColor[4] = { 191.f / 255.f, 191.f / 255.f, 191.f / 255.f, 1.f };
		GLfloat hoveredColor[4] = { 1.f, 1.f, 1.f, 1.f };
		GLfloat highlightedColor[4] = { 231.f / 255.f, 231.f / 255.f, 231.f / 255.f, 1.f };

		for (auto& model : _models) {
			if (!model)
				continue;

			GLfloat* color = normalColor;

			if (hovered.count(model))
				color = hoveredColor;
			else if (highlighted.count(model))
				color = highlightedColor;

			drawModelFast(*model, color, nullptr);
		}

		endModelPass();
	}

	bool Scene::beginModelPass(GLfloat* view, GLfloat* projection, bool picking, bool cullMeshes) {

		GLuint program = _modelShader.getNativeHandle();

		if (!_fastPathReady || program == 0 || !GL::bindVertexArray || !GL::activeTexture || !GL::useProgram)
			return false;

		std::copy(view, view + 16, _passView);
		std::copy(projection, projection + 16, _passProjection);
		_passPicking = picking;
		_passCullMeshes = cullMeshes;

		// Saved once per pass (not once per mesh) and restored in endModelPass().
		GLenum currentProgram = 0x8B8D;
		GLenum vertexArrayBinding = 0x85B5;
		GLenum activeTextureBinding = 0x84E0;

		glGetIntegerv(currentProgram, &_passPreviousProgram);
		glGetIntegerv(vertexArrayBinding, &_passPreviousVertexArray);
		glGetIntegerv(activeTextureBinding, &_passPreviousActiveTexture);

		GL::activeTexture(0x84C0);		// GL_TEXTURE0
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &_passPreviousTexture);

		GL::useProgram(program);

		GL::uniformMatrix4fv(_locView, 1, GL_FALSE, _passView);
		GL::uniformMatrix4fv(_locProjection, 1, GL_FALSE, _passProjection);
		GL::uniform1i(_locPicking, picking ? 1 : 0);
		GL::uniform1i(_locTextured, 0);
		GL::uniform1i(_locSkinned, 0);

		_passTextured = false;
		_passSkinned = false;
		_passTexture = 0;
		glBindTexture(GL_TEXTURE_2D, 0);

		return true;
	}

	void Scene::drawModelFast(Model3D& model, GLfloat* tint, GLfloat* pickColor) {

		// uModel = T * R * S (column-major).
		std::array<float, 16> r = model._rotation.matrix();
		float s = model._scale;

		GLfloat m[16] = {
			r[0] * s, r[1] * s, r[2] * s,  0.f,
			r[4] * s, r[5] * s, r[6] * s,  0.f,
			r[8] * s, r[9] * s, r[10] * s, 0.f,
			model._position.x, model._position.y, model._position.z, 1.f
		};

		GL::uniformMatrix4fv(_locModel, 1, GL_FALSE, m);

		if (tint)
			GL::uniform4f(_locTint, tint[0], tint[1], tint[2], tint[3]);

		if (pickColor)
			GL::uniform3f(_locPickColor, pickColor[0], pickColor[1], pickColor[2]);

		// modelView for Mesh3D::isOutsideView (frustum + scissor culling in picking passes).
		GLfloat modelView[16];

		if (_passCullMeshes) {
			for (int c = 0; c < 4; c++)
				for (int row = 0; row < 4; row++)
					modelView[c * 4 + row] =
						_passView[0 * 4 + row] * m[c * 4 + 0] +
						_passView[1 * 4 + row] * m[c * 4 + 1] +
						_passView[2 * 4 + row] * m[c * 4 + 2] +
						_passView[3 * 4 + row] * m[c * 4 + 3];
		}

		GLenum textureUnit0 = 0x84C0;
		GLenum textureUnit1 = 0x84C1;		// uJointMatrices

		for (auto& mesh : model._meshes) {
			if (!mesh || !mesh->_geometry || mesh->_geometry->_vertexArray == 0 || mesh->_geometry->_indexCount == 0)
				continue;

			if (_passCullMeshes && mesh->isOutsideView(modelView, _passProjection))
				continue;

			bool skinned = mesh->_jointTexture != 0 && mesh->_gpuJointCount > 0;

			if (skinned != _passSkinned) {
				GL::uniform1i(_locSkinned, skinned ? 1 : 0);
				_passSkinned = skinned;
			}

			if (skinned) {
				GL::activeTexture(textureUnit1);
				glBindTexture(GL::TextureBuffer, mesh->_jointTexture);
				GL::activeTexture(textureUnit0);
			}

			if (!_passPicking) {
				bool hasTexture = mesh->_texture && mesh->_texture->_texture;

				if (hasTexture != _passTextured) {
					GL::uniform1i(_locTextured, hasTexture ? 1 : 0);
					_passTextured = hasTexture;
				}

				GLuint texture = hasTexture ? mesh->_texture->_texture->getNativeHandle() : 0;

				if (texture != _passTexture) {
					glBindTexture(GL_TEXTURE_2D, texture);
					_passTexture = texture;
				}
			}

			GL::bindVertexArray(mesh->_geometry->_vertexArray);
			glDrawElements(GL_TRIANGLES, mesh->_geometry->_indexCount, GL_UNSIGNED_INT, nullptr);
		}
	}

	void Scene::endModelPass() {

		GL::bindVertexArray((GLuint)_passPreviousVertexArray);

		GL::activeTexture(0x84C1);
		glBindTexture(GL::TextureBuffer, 0);

		GL::activeTexture(0x84C0);
		glBindTexture(GL_TEXTURE_2D, (GLuint)_passPreviousTexture);

		GL::activeTexture((GLenum)_passPreviousActiveTexture);
		GL::useProgram((GLuint)_passPreviousProgram);
	}

	bool Scene::createShadowBuffer() {

		if (_shadowFBO != 0)
			return true;

		GLenum clampToEdge = 0x812F;
		GLenum textureWrapS = 0x2802;
		GLenum textureWrapT = 0x2803;
		GLenum none = 0;

		glGenTextures(1, &_shadowDepth);
		glBindTexture(GL_TEXTURE_2D, _shadowDepth);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, textureWrapS, clampToEdge);
		glTexParameteri(GL_TEXTURE_2D, textureWrapT, clampToEdge);

		glTexImage2D(
			GL_TEXTURE_2D, 0, _shadowDepthComponent,
			_shadowMapSize, _shadowMapSize, 0,
			GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr
		);

		GL::genFramebuffers(1, &_shadowFBO);
		GL::bindFramebuffer(GL::Framebuffer, _shadowFBO);
		GL::framebufferTexture2D(GL::Framebuffer, GL::DepthAttachment, GL_TEXTURE_2D, _shadowDepth, 0);

		// Depth only, no color attachment.
		glDrawBuffer(none);
		glReadBuffer(none);

		bool complete = GL::checkFramebufferStatus(GL::Framebuffer) == GL::FramebufferComplete;

		GL::bindFramebuffer(GL::Framebuffer, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (!complete) {
			DebugError(L"Shadow framebuffer is incomplete");
			GL::deleteFramebuffers(1, &_shadowFBO);
			glDeleteTextures(1, &_shadowDepth);
			_shadowFBO = 0;
			_shadowDepth = 0;
			return false;
		}

		return true;
	}

	void Scene::renderShadowMap() {

		if (!createShadowBuffer())
			return;

		sf::Vector2u size = render_window->getSize();

		if (size.x == 0 || size.y == 0)
			return;

		setProjection();
		setCamera();

		// 1. Ground area visible on the screen: rays through the screen corners and the center hit the plane y = 0.
		sf::Vector2i pixels[] = {
			{ 0, 0 }, { (int)size.x - 1, 0 }, { 0, (int)size.y - 1 }, { (int)size.x - 1, (int)size.y - 1 }, { (int)size.x / 2, (int)size.y / 2 }
		};

		float minX = 1e30f, maxX = -1e30f, minZ = 1e30f, maxZ = -1e30f;

		for (auto& pixel : pixels) {
			sf::Vector3f origin, dir;
			if (!screenToRay(pixel, origin, dir))
				continue;

			sf::Vector3f hit;

			if (dir.y < -1e-6f)
				hit = origin + dir * (-origin.y / dir.y);
			else {
				// Ray above the horizon: take the farthest point that still gets shadows.
				sf::Vector3f flat = shadowNormalize(sf::Vector3f(dir.x, 0.f, dir.z));
				hit = sf::Vector3f(origin.x, 0.f, origin.z) + flat * (2.f * _maxShadowRadius);
			}

			minX = std::min(minX, hit.x); maxX = std::max(maxX, hit.x);
			minZ = std::min(minZ, hit.z); maxZ = std::max(maxZ, hit.z);
		}

		if (minX > maxX)
			return;

		sf::Vector3f center((minX + maxX) * 0.5f, 0.f, (minZ + maxZ) * 0.5f);
		float radius = 0.5f * std::sqrt((maxX - minX) * (maxX - minX) + (maxZ - minZ) * (maxZ - minZ));

		// Change the size in steps, so shadows do not shimmer on every small camera move.
		radius = std::min(std::ceil(radius / 256.f) * 256.f, _maxShadowRadius);

		// 2. Sun camera: orthographic, looking along _sunDirection at the visible area.
		sf::Vector3f forward = shadowNormalize(_sunDirection);
		sf::Vector3f worldUp = std::abs(forward.y) > 0.99f ? sf::Vector3f(0.f, 0.f, 1.f) : sf::Vector3f(0.f, 1.f, 0.f);
		sf::Vector3f side = shadowNormalize(shadowCross(forward, worldUp));
		sf::Vector3f up = shadowCross(side, forward);

		// Snap the center to shadow map texels, so shadow edges stay still while the camera pans.
		float texel = 2.f * radius / (float)_shadowMapSize;
		float centerSide = shadowDot(center, side);
		float centerUp = shadowDot(center, up);
		center += side * (std::round(centerSide / texel) * texel - centerSide);
		center += up * (std::round(centerUp / texel) * texel - centerUp);

		float heightMargin = 1000.f;						// room for tall models between the sun and the ground
		float depthRange = 2.f * (radius + heightMargin);
		sf::Vector3f eye = center - forward * (depthRange * 0.5f);

		std::array<float, 16> lightView = {
			side.x, up.x, -forward.x, 0.f,
			side.y, up.y, -forward.y, 0.f,
			side.z, up.z, -forward.z, 0.f,
			-shadowDot(side, eye), -shadowDot(up, eye), shadowDot(forward, eye), 1.f
		};

		std::array<float, 16> lightProjection = {
			1.f / radius, 0.f,          0.f,                0.f,
			0.f,          1.f / radius, 0.f,                0.f,
			0.f,          0.f,          -2.f / depthRange,  0.f,
			0.f,          0.f,          -1.f,               1.f
		};

		_lightViewProjection = shadowMultiply(lightProjection, lightView);

		// 3. Draw the models' depth from the sun.
		GL::bindFramebuffer(GL::Framebuffer, _shadowFBO);
		glViewport(0, 0, _shadowMapSize, _shadowMapSize);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(lightProjection.data());
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(lightView.data());

		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// Pushes the stored depth a bit away from the sun against shadow acne.
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(2.f, 4.f);

		glClearDepth(1.0);
		glClear(GL_DEPTH_BUFFER_BIT);

		drawShadowCasters(lightView, lightProjection, center, side, up, radius);

		glDisable(GL_POLYGON_OFFSET_FILL);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		GL::bindFramebuffer(GL::Framebuffer, 0);

		// Back to the player's camera.
		setViewport();
		setProjection();
		setCamera();
	}

	void Scene::drawShadowCasters(std::array<float, 16> lightView, std::array<float, 16> lightProjection, sf::Vector3f center, sf::Vector3f side, sf::Vector3f up, float radius) {

		// Models a bit outside the shadow map square can still throw a shadow into it.
		float casterLimit = radius + 300.f;

		std::function<bool(std::shared_ptr<Model3D>&)> isCaster = [&](std::shared_ptr<Model3D>& model) {
			if (!model)
				return false;

			// Light space: the shadow map covers |side|, |up| <= radius around the center.
			sf::Vector3f d = model->_position - center;
			return std::abs(shadowDot(d, side)) <= casterLimit && std::abs(shadowDot(d, up)) <= casterLimit;
		};

		if (!beginModelPass(lightView.data(), lightProjection.data(), true, false))
			return;

		for (auto& model : _models)
			if (isCaster(model))
				drawModelFast(*model, nullptr, nullptr);

		endModelPass();
	}

	bool Scene::createPickingBuffer(unsigned int width, unsigned int height) {
		if (width == 0 || height == 0)
			return false;

		if (_pickingFBO != 0 && _pickingWidth == width && _pickingHeight == height)
			return true;

		if (_pickingFBO == 0)
			GL::genFramebuffers(1, &_pickingFBO);

		if (_pickingColor == 0)
			glGenTextures(1, &_pickingColor);

		if (_pickingDepth == 0)
			glGenTextures(1, &_pickingDepth);

		glBindTexture(GL_TEXTURE_2D, _pickingColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA8,
			width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr
		);

		GLenum depthComponent24 = 0x81A6;

		glBindTexture(GL_TEXTURE_2D, _pickingDepth);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(
			GL_TEXTURE_2D, 0, depthComponent24,		// picking always uses 24 bits (pickTerrain precision), independent of the shadow quality
			width, height, 0,
			GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr
		);

		GL::bindFramebuffer(GL::Framebuffer, _pickingFBO);

		GL::framebufferTexture2D(
			GL::Framebuffer, GL::ColorAttachment0,
			GL_TEXTURE_2D, _pickingColor, 0
		);

		GL::framebufferTexture2D(
			GL::Framebuffer, GL::DepthAttachment,
			GL_TEXTURE_2D, _pickingDepth, 0
		);

		glDrawBuffer(GL::ColorAttachment0);
		glReadBuffer(GL::ColorAttachment0);

		bool complete = GL::checkFramebufferStatus(GL::Framebuffer) == GL::FramebufferComplete;

		GL::bindFramebuffer(GL::Framebuffer, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (!complete) {
			DebugError(L"Picking framebuffer is incomplete");
			_pickingWidth = 0;
			_pickingHeight = 0;
			return false;
		}

		_pickingWidth = width;
		_pickingHeight = height;

		DebugStat(L"Picking framebuffer created");
		return true;
	}

	bool Scene::screenToRay(sf::Vector2i pixel, sf::Vector3f& origin, sf::Vector3f& dir) {
		// Center of the pixel.
		return screenToRay(sf::Vector2f((float)pixel.x + 0.5f, (float)pixel.y + 0.5f), origin, dir);
	}

	bool Scene::screenToRay(sf::Vector2f pixel, sf::Vector3f& origin, sf::Vector3f& dir) {
		sf::Vector2u size = render_window->getSize();

		if (size.x == 0 || size.y == 0 || !_camera)
			return false;

		float tanHalfFov = std::tan(_fovY * 0.5f * 3.14159265f / 180.f);
		float aspect = (float)size.x / (float)size.y;

		// Normalized device coordinates.
		float ndcX = 2.f * pixel.x / (float)size.x - 1.f;
		float ndcY = 1.f - 2.f * pixel.y / (float)size.y;

		// Ray direction in camera space (camera looks along -Z, eye z = -1).
		sf::Vector3f eye(ndcX * tanHalfFov * aspect, ndcY * tanHalfFov, -1.f);

		// setCamera() uses view = R * T(-position), so world direction = R^T * eye.
		std::array<float, 16> m = _camera->_rotation.matrix(); // column-major: R(i, j) = m[j * 4 + i]

		dir = sf::Vector3f(
			m[0] * eye.x + m[1] * eye.y + m[2] * eye.z,
			m[4] * eye.x + m[5] * eye.y + m[6] * eye.z,
			m[8] * eye.x + m[9] * eye.y + m[10] * eye.z
		);

		origin = _camera->_position;
		return true;
	}

	bool Scene::panByPixels(sf::Vector2f anchorPixel, sf::Vector2f pixelDelta) {

		sf::Vector3f oldOrigin, oldDir, newOrigin, newDir;

		if (!screenToRay(anchorPixel, oldOrigin, oldDir) || !screenToRay(anchorPixel + pixelDelta, newOrigin, newDir))
			return false;

		if (std::abs(oldDir.y) < 1e-6f || std::abs(newDir.y) < 1e-6f)
			return false;

		float oldT = (_panPlaneY - oldOrigin.y) / oldDir.y;
		float newT = (_panPlaneY - newOrigin.y) / newDir.y;

		if (oldT <= 0.f || newT <= 0.f)
			return false;

		sf::Vector3f oldPoint = oldOrigin + oldDir * oldT;
		sf::Vector3f newPoint = newOrigin + newDir * newT;

		// Camera only translates, so the ground point follows the pixel exactly.
		_camera->_position.x -= newPoint.x - oldPoint.x;
		_camera->_position.z -= newPoint.z - oldPoint.z;
		_cameraIsMoving = true;
		return true;
	}

	bool Scene::unprojectDepth(sf::Vector2i pixel, float depth, sf::Vector3f& hit) {

		// depth == 1 means nothing was drawn under the pixel.
		if (depth <= 0.f || depth >= 1.f)
			return false;

		sf::Vector3f origin, dir;
		if (!screenToRay(pixel, origin, dir))
			return false;

		// Depth -> NDC z -> distance along the view axis (inverse of gluPerspective).
		float n = _nearPlane;
		float f = _farPlane;
		float ndcZ = 2.f * depth - 1.f;
		float viewDistance = (2.f * f * n) / ((f + n) - ndcZ * (f - n));

		// dir has eye z = -1, so scaling it by the view distance lands on the surface.
		hit = origin + dir * viewDistance;
		return true;
	}

	int Scene::getHoveredModelId() {
		return getHoveredModelId(Anubis::cursor->_position);
	}

	int Scene::getHoveredModelId(sf::Vector2i screenPixel) {
		sf::Vector2u size = render_window->getSize();
		sf::Vector2i mouse = screenPixel;

		if (size.x == 0 || size.y == 0)
			return -1;

		if (mouse.x < 0 || mouse.y < 0 || mouse.x >= (int)size.x || mouse.y >= (int)size.y)
			return -1;

		if (!createPickingBuffer(size.x, size.y))
			return -1;

		int pixelX = mouse.x;
		int pixelY = (int)size.y - mouse.y - 1;

		// Preserve OpenGL state.
		GLint previousMatrixMode = 0;
		glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		GL::bindFramebuffer(GL::Framebuffer, _pickingFBO);

		glDrawBuffer(GL::ColorAttachment0);
		glReadBuffer(GL::ColorAttachment0);

		setViewport();
		setProjection();
		setCamera();

		// Preserve exact ID colors.
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glDisable(GL_DITHER);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_FOG);
		glDisable(GL_COLOR_LOGIC_OP);

		GLenum framebufferSRGB = 0x8DB9;
		GLenum multisample = 0x809D;

		glDisable(framebufferSRGB);
		glDisable(multisample);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Only the pixel under the cursor needs a result.
		glEnable(GL_SCISSOR_TEST);
		glScissor(pixelX, pixelY, 1, 1);

		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GLfloat view[16];
		GLfloat projection[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);

		// Meshes outside the 1x1 scissor are skipped by Mesh3D::isOutsideView.
		if (beginModelPass(view, projection, true, true)) {

			for (auto& model : _models) {
				if (!model || model->_id < 0 || model->_id > 0xFFFFFE || !model->_isSelectable)
					continue;

				// Model id + 1 encoded in RGB (0 = nothing under the cursor).
				GLuint encodedId = (GLuint)model->_id + 1u;

				GLfloat pickColor[3] = {
					float(encodedId & 0xFF) / 255.f,
					float((encodedId >> 8) & 0xFF) / 255.f,
					float((encodedId >> 16) & 0xFF) / 255.f
				};

				drawModelFast(*model, nullptr, pickColor);
			}

			endModelPass();
		}

		GLubyte pixel[4] = {};

		GLenum pixelPackBuffer = 0x88EB;
		GL::bindBuffer(pixelPackBuffer, 0);

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ROW_LENGTH, 0);
		glPixelStorei(GL_PACK_SKIP_ROWS, 0);
		glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

		glReadPixels(
			pixelX, pixelY,
			1, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			pixel
		);

		GL::bindFramebuffer(GL::Framebuffer, 0);

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(previousMatrixMode);

		glPopClientAttrib();
		glPopAttrib();

		GLuint encodedId = (GLuint)pixel[0] | ((GLuint)pixel[1] << 8) | ((GLuint)pixel[2] << 16);

		if (encodedId == 0)
			return -1;

		return int(encodedId - 1u);
	}

	std::vector<std::shared_ptr<Model3D>> Scene::getModelsInRect(sf::IntRect rect) {

		std::vector<std::shared_ptr<Model3D>> selected;

		sf::Vector2u size = Anubis::render_window->getSize();

		if (size.x == 0 || size.y == 0)
			return selected;

		int width = (int)size.x;
		int height = (int)size.y;

		sf::Vector2i start = rect.position;
		sf::Vector2i end = rect.position + rect.size;

		// At least 1 pixel, so a rect started by a tiny mouse move still covers the pixel under the cursor.
		int left = std::clamp(std::min(start.x, end.x), 0, width);
		int right = std::clamp(std::max(std::max(start.x, end.x), left + 1), 0, width);
		int top = std::clamp(std::min(start.y, end.y), 0, height);
		int bottom = std::clamp(std::max(std::max(start.y, end.y), top + 1), 0, height);

		if (left == right || top == bottom)
			return selected;

		if (!createPickingBuffer(size.x, size.y))
			return selected;

		// Keep one query for each model.
		std::vector<GLuint> queries(_models.size(), 0);
		selected.reserve(_models.size());

		GLint previousMatrixMode = 0;
		glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		GL::bindFramebuffer(GL::Framebuffer, _pickingFBO);

		setViewport();
		setProjection();
		setCamera();

		glEnable(GL_SCISSOR_TEST);
		glScissor(left, height - bottom, right - left, bottom - top);

		// Include models hidden behind other models.
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_POLYGON_STIPPLE);

		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Only count samples; do not modify the framebuffer.
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);

		GLfloat view[16];
		GLfloat projection[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);

		// Meshes outside the selection rect (scissor) are skipped by Mesh3D::isOutsideView.
		if (beginModelPass(view, projection, true, true)) {

			// Submit every query before reading any results.
			for (std::size_t i = 0; i < _models.size(); i++) {
				if (!_models[i])
					continue;

				GL::genQueries(1, &queries[i]);

				GL::beginQuery(GL::AnySamplesPassed, queries[i]);
				drawModelFast(*_models[i], nullptr, nullptr);
				GL::endQuery(GL::AnySamplesPassed);
			}

			endModelPass();
		}

		// Restore rendering state.
		GL::bindFramebuffer(GL::Framebuffer, 0);

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(previousMatrixMode);
		glPopAttrib();

		// Read results. This can wait for the GPU.
		for (std::size_t i = 0; i < _models.size(); i++) {
			if (queries[i] == 0)
				continue;

			GLuint passed = GL_FALSE;

			GL::getQueryObjectuiv(
				queries[i],
				GL::QueryResult,
				&passed
			);

			if (passed != GL_FALSE && _models[i]->_isSelectable)
				selected.push_back(_models[i]);

			GL::deleteQueries(1, &queries[i]);
		}

		return selected;
	}

	void Scene::cursorHover() {

		_hovered.clear();

		if (cursor->_rect.size.x == 0 && cursor->_rect.size.y == 0) {
			int id = getHoveredModelId();

			if (id >= 0) {
				std::vector<std::shared_ptr<Model3D>>::iterator it = std::find_if(_models.begin(), _models.end(),
					[id](std::shared_ptr<Model3D>& model) {
						return model->_id == id;
					});

				if (it != _models.end())
					_hovered.push_back(*it);
			}
		}
		else
			_hovered = getModelsInRect(cursor->_rect);
	}

	void Scene::handleEvent(const sf::Event& event) {

		if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>(); scroll) {
			float oldDistance = _cameraRadius / _cameraZoom;

			if (scroll->delta > 0 && _cameraZoom < 4.0f) _cameraZoom += 0.25f;
			if (scroll->delta < 0 && _cameraZoom > 0.25f) _cameraZoom -= 0.25f;

			float distance = _cameraRadius / _cameraZoom;
			float distanceDelta = distance - oldDistance;


			_camera->_position.y += std::sin(_cameraXAngle * 3.14159265f / 180.f) * distanceDelta;
			_camera->_position.z += std::cos(_cameraXAngle * 3.14159265f / 180.f) * distanceDelta;
			_cameraIsMoving = true;
			return;
		}

		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && pressed->button == sf::Mouse::Button::Middle) {
			_lastCameraMouse = pressed->position;
			_isPressedMiddle = true;
			return;
		}

		if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>(); released && released->button == sf::Mouse::Button::Middle) {
			_isPressedMiddle = false;
			return;
		}

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>(); moved && _isPressedMiddle) {
			// The grabbed point stays under the cursor at every zoom level.
			sf::Vector2f last((float)_lastCameraMouse.x + 0.5f, (float)_lastCameraMouse.y + 0.5f);
			panByPixels(last, sf::Vector2f(moved->position - _lastCameraMouse));

			_lastCameraMouse = moved->position;
			return;
		}

		// Selecting and moving models is handled by MapEditor::CursorOnMap.
	}

	void Scene::update() {

		_cameraIsMoving = false;

		// WASD / arrows: the map moves by a constant number of pixels per second at every zoom level.
		sf::Vector2f pixelDelta(0.f, 0.f);
		float step = _cameraSpeed * Anubis::deltaTime.asSeconds();

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
			pixelDelta.y += step;		// camera forward = map moves down on the screen

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
			pixelDelta.y -= step;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
			pixelDelta.x += step;		// camera left = map moves right on the screen

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
			pixelDelta.x -= step;

		if (pixelDelta.x != 0.f || pixelDelta.y != 0.f) {
			sf::Vector2u size = render_window->getSize();
			sf::Vector2f center((float)size.x * 0.5f, (float)size.y * 0.5f);
			panByPixels(center, pixelDelta);
		}

		//DebugLog(L"models: " + std::to_wstring(_models.size()));

		for (auto& model : _models)
			model->update();
	}

	void Scene::draw() {
		drawModels();
	}
}