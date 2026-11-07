#include "3D/OpenGLBuffers.hpp"
#include "3D/Mesh3D.hpp"
#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <limits>
#include "DebugLog.hpp"

namespace Anubis {

	MeshGeometry::~MeshGeometry() {
		if (_vertexArray != 0)
			GL::deleteVertexArrays(1, &_vertexArray);

		if (_vertexBuffer != 0)
			GL::deleteBuffers(1, &_vertexBuffer);

		if (_indexBuffer != 0)
			GL::deleteBuffers(1, &_indexBuffer);
	}

	Mesh3D::Mesh3D() {

	}

	Mesh3D::Mesh3D(const Mesh3D& other) {
		*this = other;
	}

	Mesh3D& Mesh3D::operator=(const Mesh3D& other) {
		if (this == &other)
			return *this;

		_geometry = other._geometry;
		_texture = other._texture;
		_nodeIndex = other._nodeIndex;

		if (_jointTexture != 0)
			glDeleteTextures(1, &_jointTexture);

		if (_jointBuffer != 0)
			GL::deleteBuffers(1, &_jointBuffer);

		_jointTexture = 0;
		_jointBuffer = 0;
		_gpuJointCount = 0;

		return *this;
	}

	Mesh3D::~Mesh3D() {

		if (_jointTexture != 0)
			glDeleteTextures(1, &_jointTexture);

		if (_jointBuffer != 0)
			GL::deleteBuffers(1, &_jointBuffer);
	}

	void Mesh3D::setTexture(std::shared_ptr<Texture> texture) {
		_texture = texture;
	}

	bool Mesh3D::uploadJointMatrices(std::vector<Matrix4>& matrices) {
		static_assert(sizeof(Matrix4) == 16 * sizeof(float));

		if (!GL::genBuffers || !GL::bindBuffer || !GL::bufferData || !GL::texBuffer) {
			DebugError(L"OpenGL joint buffer functions are not initialized");
			return false;
		}

		if (matrices.empty()) {
			DebugError(L"Joint matrix array is empty");
			return false;
		}

		GLint maxTexels = 0;
		glGetIntegerv(GL::MaxTextureBufferSize, &maxTexels);

		// Each matrix occupies four RGBA32F texels.
		if (maxTexels <= 0 || matrices.size() > (std::size_t)maxTexels / 4) {
			DebugError(L"Joint matrices exceed the GPU texture buffer limit");
			return false;
		}

		std::size_t maxBytes = (std::size_t)std::numeric_limits<std::ptrdiff_t>::max();

		if (matrices.size() > maxBytes / sizeof(Matrix4)) {
			DebugError(L"Joint matrix buffer is too large");
			return false;
		}

		for (auto& matrix : matrices) {
			for (float value : matrix) {
				if (!std::isfinite(value)) {
					DebugError(L"Joint matrix contains a non-finite value");
					return false;
				}
			}
		}

		GLint previousBuffer = 0;
		GLint previousTexture = 0;

		glGetIntegerv(GL::TextureBuffer, &previousBuffer);
		glGetIntegerv(GL::TextureBindingBuffer, &previousTexture);

		if (_jointBuffer == 0)
			GL::genBuffers(1, &_jointBuffer);

		if (_jointTexture == 0)
			glGenTextures(1, &_jointTexture);

		if (_jointBuffer == 0 || _jointTexture == 0) {
			DebugError(L"Failed to create joint buffer objects");
			return false;
		}

		GL::bindBuffer(GL::TextureBuffer, _jointBuffer);

		GL::bufferData(
			GL::TextureBuffer,
			std::ptrdiff_t(matrices.size() * sizeof(Matrix4)),
			matrices.data(),
			GL::DynamicDraw
		);

		glBindTexture(GL::TextureBuffer, _jointTexture);
		GL::texBuffer(GL::TextureBuffer, GL::RGBA32F, _jointBuffer);

		// Restore the previous buffer and texture bindings.
		glBindTexture(GL::TextureBuffer, (GLuint)previousTexture);

		GL::bindBuffer(GL::TextureBuffer, (GLuint)previousBuffer);

		_gpuJointCount = matrices.size();
		return true;
	}

	void Mesh3D::uploadToGPU(std::vector<Vertex3D>& vertices, std::vector<std::uint32_t>& indices) {

		//float verticesMB = float(vertices.size() * sizeof(Vertex3D)) / (1024.f * 1024.f);
		//float indicesMB = float(indices.size() * sizeof(std::uint32_t)) / (1024.f * 1024.f);
		//DebugStat(L"GPU Mesh: " + std::to_wstring(verticesMB + indicesMB) + L" MiB");

		static_assert(std::is_standard_layout_v<Vertex3D>);

		if (!GL::genBuffers || !GL::bindBuffer || !GL::bufferData ||
			!GL::genVertexArrays || !GL::bindVertexArray ||
			!GL::enableVertexAttribArray ||
			!GL::vertexAttribPointer || !GL::vertexAttribIPointer) {
			DebugError(L"OpenGL vertex buffer functions are not initialized");
			return;
		}

		if (indices.size() > (std::size_t)std::numeric_limits<GLsizei>::max()) {
			DebugError(L"Too many indices");
			return;
		}

		std::size_t maxBufferBytes = (std::size_t)std::numeric_limits<std::ptrdiff_t>::max();

		if (vertices.size() > maxBufferBytes / sizeof(Vertex3D) ||
			indices.size() > maxBufferBytes / sizeof(std::uint32_t)) {
			DebugError(L"Mesh buffer is too large");
			return;
		}

		for (auto index : indices) {
			if (index >= vertices.size()) {
				DebugError(L"Mesh index exceeds vertex count");
				return;
			}
		}

		// keep the current GL state
		GLenum vertexArrayBinding = 0x85B5;
		GLenum arrayBufferBinding = 0x8894;

		GLint previousVertexArray = 0;
		GLint previousArrayBuffer = 0;

		glGetIntegerv(vertexArrayBinding, &previousVertexArray);
		glGetIntegerv(arrayBufferBinding, &previousArrayBuffer);

		std::shared_ptr<MeshGeometry> geometry = std::make_shared<MeshGeometry>();

		GL::genVertexArrays(1, &geometry->_vertexArray);
		GL::genBuffers(1, &geometry->_vertexBuffer);
		GL::genBuffers(1, &geometry->_indexBuffer);

		if (geometry->_vertexArray == 0 || geometry->_vertexBuffer == 0 || geometry->_indexBuffer == 0) {
			DebugError(L"Failed to create mesh buffer objects");
			return;
		}

		GL::bindVertexArray(geometry->_vertexArray);

		GL::bindBuffer(GL::ArrayBuffer, geometry->_vertexBuffer);

		GL::bufferData(
			GL::ArrayBuffer,
			std::ptrdiff_t(vertices.size() * sizeof(Vertex3D)),
			vertices.data(),
			GL::StaticDraw
		);

		GL::bindBuffer(GL::ElementArrayBuffer, geometry->_indexBuffer);

		GL::bufferData(
			GL::ElementArrayBuffer,
			std::ptrdiff_t(indices.size() * sizeof(std::uint32_t)),
			indices.data(),
			GL::StaticDraw
		);

		GLsizei stride = (GLsizei)sizeof(Vertex3D);

		// 0 - positions
		GL::enableVertexAttribArray(0);
		GL::vertexAttribPointer(
			0, 3, GL_FLOAT, GL_FALSE, stride,
			(void*)offsetof(Vertex3D, _x)
		);

		// 1 - normals
		GL::enableVertexAttribArray(1);
		GL::vertexAttribPointer(
			1, 3, GL_FLOAT, GL_FALSE, stride,
			(void*)offsetof(Vertex3D, _nx)
		);

		// 2 - uv tex coords
		GL::enableVertexAttribArray(2);
		GL::vertexAttribPointer(
			2, 2, GL_FLOAT, GL_FALSE, stride,
			(void*)offsetof(Vertex3D, _u)
		);

		// 3 - bones indices
		GL::enableVertexAttribArray(3);
		GL::vertexAttribIPointer(
			3, 4, GL_UNSIGNED_SHORT, stride,
			(void*)offsetof(Vertex3D, _joints)
		);

		// 4 - bones weights
		GL::enableVertexAttribArray(4);
		GL::vertexAttribPointer(
			4, 4, GL_FLOAT, GL_FALSE, stride,
			(void*)offsetof(Vertex3D, _weights)
		);

		// Restore the previous VAO and vertex buffer.
		GL::bindVertexArray((GLuint)previousVertexArray);
		GL::bindBuffer(GL::ArrayBuffer, (GLuint)previousArrayBuffer);

		geometry->_indexCount = (GLsizei)indices.size();

		// Local bounding box, used by isOutsideView() to skip meshes outside the view / scissor.
		if (!vertices.empty()) {
			geometry->_boundsMin = sf::Vector3f(vertices[0]._x, vertices[0]._y, vertices[0]._z);
			geometry->_boundsMax = geometry->_boundsMin;

			for (auto& v : vertices) {
				geometry->_boundsMin.x = std::min(geometry->_boundsMin.x, v._x);
				geometry->_boundsMin.y = std::min(geometry->_boundsMin.y, v._y);
				geometry->_boundsMin.z = std::min(geometry->_boundsMin.z, v._z);
				geometry->_boundsMax.x = std::max(geometry->_boundsMax.x, v._x);
				geometry->_boundsMax.y = std::max(geometry->_boundsMax.y, v._y);
				geometry->_boundsMax.z = std::max(geometry->_boundsMax.z, v._z);
			}

			geometry->_hasBounds = true;
		}

		_geometry = std::move(geometry);
	}

	bool Mesh3D::isOutsideView(GLfloat* modelView, GLfloat* projection) {

		if (!_geometry || !_geometry->_hasBounds)
			return false;

		// Skinned vertices can move outside the bind-pose box.
		if (_jointTexture != 0 && _gpuJointCount > 0)
			return false;

		// mvp = projection * modelView (column-major, like OpenGL).
		GLfloat mvp[16];
		for (int c = 0; c < 4; c++)
			for (int r = 0; r < 4; r++)
				mvp[c * 4 + r] =
					projection[0 * 4 + r] * modelView[c * 4 + 0] +
					projection[1 * 4 + r] * modelView[c * 4 + 1] +
					projection[2 * 4 + r] * modelView[c * 4 + 2] +
					projection[3 * 4 + r] * modelView[c * 4 + 3];

		sf::Vector3f mn = _geometry->_boundsMin;
		sf::Vector3f mx = _geometry->_boundsMax;

		GLfloat clip[8][4];
		int outside[6] = { 0, 0, 0, 0, 0, 0 };
		bool allInFront = true;

		for (int i = 0; i < 8; i++) {
			float x = (i & 1) ? mx.x : mn.x;
			float y = (i & 2) ? mx.y : mn.y;
			float z = (i & 4) ? mx.z : mn.z;

			for (int r = 0; r < 4; r++)
				clip[i][r] = mvp[r] * x + mvp[4 + r] * y + mvp[8 + r] * z + mvp[12 + r];

			float cx = clip[i][0], cy = clip[i][1], cz = clip[i][2], cw = clip[i][3];

			if (cx < -cw) outside[0]++;
			if (cx > cw)  outside[1]++;
			if (cy < -cw) outside[2]++;
			if (cy > cw)  outside[3]++;
			if (cz < -cw) outside[4]++;
			if (cz > cw)  outside[5]++;

			if (cw <= 0.f)
				allInFront = false;
		}

		// Frustum: all 8 corners behind the same plane.
		for (int k = 0; k < 6; k++)
			if (outside[k] == 8)
				return true;

		// Scissor: picking draws only a few pixels, so skip meshes that do not touch the scissor box.
		if (!allInFront || !glIsEnabled(GL_SCISSOR_TEST))
			return false;

		GLint viewport[4];
		GLint scissor[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		glGetIntegerv(GL_SCISSOR_BOX, scissor);

		float minX = std::numeric_limits<float>::max();
		float minY = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float maxY = std::numeric_limits<float>::lowest();

		for (int i = 0; i < 8; i++) {
			float wx = (float)viewport[0] + (clip[i][0] / clip[i][3] * 0.5f + 0.5f) * (float)viewport[2];
			float wy = (float)viewport[1] + (clip[i][1] / clip[i][3] * 0.5f + 0.5f) * (float)viewport[3];

			minX = std::min(minX, wx); maxX = std::max(maxX, wx);
			minY = std::min(minY, wy); maxY = std::max(maxY, wy);
		}

		// 1 pixel margin for rasterization rounding.
		return
			maxX < (float)scissor[0] - 1.f ||
			minX > float(scissor[0] + scissor[2]) + 1.f ||
			maxY < (float)scissor[1] - 1.f ||
			minY > float(scissor[1] + scissor[3]) + 1.f;
	}

	std::shared_ptr<Mesh3D> generateCube(float size) {
		std::shared_ptr<Mesh3D> cube = std::make_shared<Mesh3D>();
		std::vector<Vertex3D> vertices;
		// Front face
		vertices.emplace_back(-size, -size, size, 0.f, 0.f, 1.f, 0.f, 0.f);
		vertices.emplace_back(size, -size, size, 0.f, 0.f, 1.f, 1.f, 0.f);
		vertices.emplace_back(size, size, size, 0.f, 0.f, 1.f, 1.f, 1.f);
		vertices.emplace_back(-size, -size, size, 0.f, 0.f, 1.f, 0.f, 0.f);
		vertices.emplace_back(size, size, size, 0.f, 0.f, 1.f, 1.f, 1.f);
		vertices.emplace_back(-size, size, size, 0.f, 0.f, 1.f, 0.f, 1.f);
		// Back face
		vertices.emplace_back(-size, -size, -size, 0.f, 0.f, -1.f, 1.f, 0.f);
		vertices.emplace_back(size, -size, -size, 0.f, 0.f, -1.f, 0.f, 0.f);
		vertices.emplace_back(size, size, -size, 0.f, 0.f, -1.f, 0.f, 1.f);
		vertices.emplace_back(-size, -size, -size, 0.f, 0.f, -1.f, 1.f, 0.f);
		vertices.emplace_back(size, size, -size, 0.f, 0.f, -1.f, 0.f, 1.f);
		vertices.emplace_back(-size, size, -size, 0.f, 0.f, -1.f, 1.f, 1.f);
		// Left face
		vertices.emplace_back(-size, -size, -size, -1.f, 0.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(-size, -size, size, -1.f, 0.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(-size, size, size, -1.f, 0.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(-size, -size, -size, -1.f, 0.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(-size, size, size, -1.f, 0.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(-size, size, -size, -1.f, 0.f, 0.f, 0.f, 1.f);
		// Right face
		vertices.emplace_back(size, -size, -size, 1.f, 0.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(size, -size, size, 1.f, 0.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(size, size, size, 1.f, 0.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(size, -size, -size, 1.f, 0.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(size, size, size, 1.f, 0.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(size, size, -size, 1.f, 0.f, 0.f, 1.f, 1.f);
		// Top face
		vertices.emplace_back(-size, size, -size, 0.f, 1.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(-size, size, size, 0.f, 1.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(size, size, size, 0.f, 1.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(-size, size, -size, 0.f, 1.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(size, size, size, 0.f, 1.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(size, size, -size, 0.f, 1.f, 0.f, 1.f, 1.f);
		// Bottom face
		vertices.emplace_back(-size, -size, -size, 0.f, -1.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(-size, -size, size, 0.f, -1.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(size, -size, size, 0.f, -1.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(-size, -size, -size, 0.f, -1.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(size, -size, size, 0.f, -1.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(size, -size, -size, 0.f, -1.f, 0.f, 0.f, 1.f);

		std::vector<std::uint32_t> indices = {
			0, 1, 2, 3, 4, 5,        // Front
			6, 7, 8, 9, 10, 11,      // Back
			12, 13, 14, 15, 16, 17,  // Left
			18, 19, 20, 21, 22, 23,  // Right
			24, 25, 26, 27, 28, 29,  // Top
			30, 31, 32, 33, 34, 35   // Bottom
		};

		cube->uploadToGPU(vertices, indices);
		return cube;
	}

	std::shared_ptr<Mesh3D> generateCube(sf::Vector3f size) {
		std::shared_ptr<Mesh3D> cube = std::make_shared<Mesh3D>();
		std::vector<Vertex3D> vertices;

		float x = size.x * 0.5f;
		float y = size.y * 0.5f;
		float z = size.z * 0.5f;

		// Front face (+Z)
		vertices.emplace_back(-x, -y, z, 0.f, 0.f, 1.f, 0.f, 0.f);
		vertices.emplace_back(x, -y, z, 0.f, 0.f, 1.f, 1.f, 0.f);
		vertices.emplace_back(x, y, z, 0.f, 0.f, 1.f, 1.f, 1.f);
		vertices.emplace_back(-x, -y, z, 0.f, 0.f, 1.f, 0.f, 0.f);
		vertices.emplace_back(x, y, z, 0.f, 0.f, 1.f, 1.f, 1.f);
		vertices.emplace_back(-x, y, z, 0.f, 0.f, 1.f, 0.f, 1.f);

		// Back face (-Z)
		vertices.emplace_back(-x, -y, -z, 0.f, 0.f, -1.f, 1.f, 0.f);
		vertices.emplace_back(x, y, -z, 0.f, 0.f, -1.f, 0.f, 1.f);
		vertices.emplace_back(x, -y, -z, 0.f, 0.f, -1.f, 0.f, 0.f);
		vertices.emplace_back(-x, -y, -z, 0.f, 0.f, -1.f, 1.f, 0.f);
		vertices.emplace_back(-x, y, -z, 0.f, 0.f, -1.f, 1.f, 1.f);
		vertices.emplace_back(x, y, -z, 0.f, 0.f, -1.f, 0.f, 1.f);

		// Left face (-X)
		vertices.emplace_back(-x, -y, -z, -1.f, 0.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(-x, -y, z, -1.f, 0.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(-x, y, z, -1.f, 0.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(-x, -y, -z, -1.f, 0.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(-x, y, z, -1.f, 0.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(-x, y, -z, -1.f, 0.f, 0.f, 0.f, 1.f);

		// Right face (+X)
		vertices.emplace_back(x, -y, -z, 1.f, 0.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(x, y, z, 1.f, 0.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(x, -y, z, 1.f, 0.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(x, -y, -z, 1.f, 0.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(x, y, -z, 1.f, 0.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(x, y, z, 1.f, 0.f, 0.f, 0.f, 1.f);

		// Top face (+Y)
		vertices.emplace_back(-x, y, -z, 0.f, 1.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(-x, y, z, 0.f, 1.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(x, y, z, 0.f, 1.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(-x, y, -z, 0.f, 1.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(x, y, z, 0.f, 1.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(x, y, -z, 0.f, 1.f, 0.f, 1.f, 1.f);

		// Bottom face (-Y)
		vertices.emplace_back(-x, -y, -z, 0.f, -1.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(x, -y, z, 0.f, -1.f, 0.f, 0.f, 0.f);
		vertices.emplace_back(-x, -y, z, 0.f, -1.f, 0.f, 1.f, 0.f);
		vertices.emplace_back(-x, -y, -z, 0.f, -1.f, 0.f, 1.f, 1.f);
		vertices.emplace_back(x, -y, -z, 0.f, -1.f, 0.f, 0.f, 1.f);
		vertices.emplace_back(x, -y, z, 0.f, -1.f, 0.f, 0.f, 0.f);

		std::vector<std::uint32_t> indices = {
			0, 1, 2, 3, 4, 5,        // Front
			6, 7, 8, 9, 10, 11,      // Back
			12, 13, 14, 15, 16, 17,  // Left
			18, 19, 20, 21, 22, 23,  // Right
			24, 25, 26, 27, 28, 29,  // Top
			30, 31, 32, 33, 34, 35   // Bottom
		};

		cube->uploadToGPU(vertices, indices);
		return cube;
	}
}