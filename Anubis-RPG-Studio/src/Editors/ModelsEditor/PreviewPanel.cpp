#include "Editors/ModelsEditor/PreviewPanel.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"
#include "Objects/Monster.hpp"
#include "PrefabsManager.hpp"
#include "Windows/FileDialog/FileDialog.hpp"
#include "ShadersManager.hpp"
#include "PrefabsManager.hpp"
#include "Cursor.hpp"
#include "SFML/OpenGL.hpp"

namespace ModelsEditor {

	PreviewPanel::PreviewPanel(sf::Vector2i margin) : Panel(sf::Vector2i(912, 712 + 60), sf::Vector2i(420 + margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y + 128)) {

		_title = std::make_unique<sf::Text>(basicFont, L"Preview", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));
	
		

		_sceneRect.size = sf::Vector2i(512 + 256 + 64, 512 + 128 + 32);
		_sceneRect.position = sf::Vector2i(getPosition().x + 32, getPosition().y + _title->getFont().getLineSpacing(20) + 24 + 16);
		
		_scene = std::make_shared<Anubis::Scene>();
		_scene->initializeModel3DShaders();

		sf::Vector2i topLeft = Anubis::render_window->mapCoordsToPixel(sf::Vector2f(_sceneRect.position));
		sf::Vector2i bottomRight = Anubis::render_window->mapCoordsToPixel(sf::Vector2f(_sceneRect.position + _sceneRect.size));
		_scenePixelRect = sf::IntRect(topLeft, bottomRight - topLeft);
		_scene->_viewport = _scenePixelRect;


		loadModel(); // TO-DO - must be the laodModel
	}

	PreviewPanel::~PreviewPanel() {

	}

	void PreviewPanel::loadModel() {

		// TO-DO
		//std::ostringstream ss;
		//ss << std::fixed << std::setprecision(3) << 0.125f;
		//std::string text = ss.str();
		//std::wstring intervalText(text.begin(), text.end());
		//_i->setText(intervalText);

		_scene->_models.clear();

		_model = nullptr;
		_modelRadius = 1.f;

		if (Anubis::prefabs_manager->_prefabs.empty()) return;

		std::shared_ptr<Anubis::Model3D> model = std::make_shared<Anubis::Model3D>(*Anubis::prefabs_manager->getPrefab(L"dziobak")->_model.lock());

		sf::Vector3f boundsMin;
		sf::Vector3f boundsMax;
		bool hasBounds = false;

		for (auto& mesh : model->_meshes) {
			if (!mesh || !mesh->_geometry || !mesh->_geometry->_hasBounds)
				continue;

			sf::Vector3f mn = mesh->_geometry->_boundsMin;
			sf::Vector3f mx = mesh->_geometry->_boundsMax;

			if (!hasBounds) {
				boundsMin = mn;
				boundsMax = mx;
				hasBounds = true;
				continue;
			}

			boundsMin = sf::Vector3f(std::min(boundsMin.x, mn.x), std::min(boundsMin.y, mn.y), std::min(boundsMin.z, mn.z));
			boundsMax = sf::Vector3f(std::max(boundsMax.x, mx.x), std::max(boundsMax.y, mx.y), std::max(boundsMax.z, mx.z));
		}

		if (!hasBounds)
			return;

		_model = model;
		_scene->addModel(_model);

		_model->setScene(_scene);
		_model->_isSelectable = true;
		_model->_scale = 1.f;
		_model->_rotation = Anubis::Quaternion(1.f, 0.f, 0.f, 0.f);

		if (!_model->_animations.empty())
			_model->playAnimation(0, true);

		// skinned model: move the bind pose bounds by the average joint matrix (global * inverseBind)
		if (!_model->_skins.empty()) {
			Anubis::Skin3D& skin = _model->_skins[0];
			Anubis::Matrix4 average{};
			int count = 0;

			for (std::size_t i = 0; i < skin._joints.size() && i < skin._inverseBindMatrices.size(); i++) {
				Anubis::Matrix4& global = _model->_nodes[skin._joints[i]]._globalMatrix;
				Anubis::Matrix4& inverseBind = skin._inverseBindMatrices[i];

				for (int column = 0; column < 4; column++)
					for (int row = 0; row < 4; row++)
						for (int k = 0; k < 4; k++)
							average[column * 4 + row] += global[k * 4 + row] * inverseBind[column * 4 + k];

				count++;
			}

			if (count > 0) {
				for (int i = 0; i < 16; i++)
					average[i] /= (float)count;

				sf::Vector3f newMin(1e30f, 1e30f, 1e30f);
				sf::Vector3f newMax(-1e30f, -1e30f, -1e30f);

				for (int i = 0; i < 8; i++) {
					float x = (i & 1) ? boundsMax.x : boundsMin.x;
					float y = (i & 2) ? boundsMax.y : boundsMin.y;
					float z = (i & 4) ? boundsMax.z : boundsMin.z;

					sf::Vector3f p(
						average[0] * x + average[4] * y + average[8] * z + average[12],
						average[1] * x + average[5] * y + average[9] * z + average[13],
						average[2] * x + average[6] * y + average[10] * z + average[14]
					);

					newMin = sf::Vector3f(std::min(newMin.x, p.x), std::min(newMin.y, p.y), std::min(newMin.z, p.z));
					newMax = sf::Vector3f(std::max(newMax.x, p.x), std::max(newMax.y, p.y), std::max(newMax.z, p.z));
				}

				boundsMin = newMin;
				boundsMax = newMax;
			}
		}

		sf::Vector3f center = (boundsMin + boundsMax) * 0.5f;
		sf::Vector3f size = boundsMax - boundsMin;

		_modelRadius = 0.5f * std::sqrt(size.x * size.x + size.y * size.y + size.z * size.z);
		if (_modelRadius <= 0.f) _modelRadius = 1.f;

		_model->_position = -center;

		float distance = _modelRadius * 0.6f / std::sin(_scene->_fovY * 0.5f * 3.14159265f / 180.f);

		float cameraXAngle = 15.0f;
		_scene->_camera->_rotation = Anubis::Quaternion(
			std::cos(cameraXAngle * 3.14159265f / 180.f * 0.5f),
			std::sin(cameraXAngle * 3.14159265f / 180.f * 0.5f),
			0.f,
			0.f
		);

		std::array<float, 16> m = _scene->_camera->_rotation.matrix();

		_scene->_camera->_position = sf::Vector3f(m[2], m[6], m[10]) * distance;

		_scene->_cameraRadius = distance;
		_scene->_cameraZoom = 1.f;
		_scene->_panPlaneY = 0.f;

	}

	void PreviewPanel::drawModel() {

		if (!_model)
			return;

		if (_scenePixelRect.size.x <= 0 || _scenePixelRect.size.y <= 0)
			return;

		int glY = (int)Anubis::render_window->getSize().y - (_scenePixelRect.position.y + _scenePixelRect.size.y);

		glEnable(GL_SCISSOR_TEST);
		glScissor(_scenePixelRect.position.x, glY, _scenePixelRect.size.x, _scenePixelRect.size.y);
		glViewport(_scenePixelRect.position.x, glY, _scenePixelRect.size.x, _scenePixelRect.size.y);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		float aspect = (float)_scenePixelRect.size.x / (float)_scenePixelRect.size.y;
		float halfHeight = _scene->_nearPlane * std::tan(_scene->_fovY * 0.5f * 3.14159265f / 180.f);
		float halfWidth = halfHeight * aspect;

		glFrustum(-halfWidth, halfWidth, -halfHeight, halfHeight, _scene->_nearPlane, _scene->_farPlane);

		_scene->setCamera();
		_scene->draw();

		glDisable(GL_SCISSOR_TEST);
		Anubis::render_window->resetGLStates();
	}

	void PreviewPanel::setTextInputsRange() {

	}

	void PreviewPanel::cursorHover() {
		Components::Panel::cursorHover();

		//_loadBtn->cursorHover();
		_scene->cursorHover();

	}

	void PreviewPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		//_loadBtn->handleEvent(event);

		std::array<float, 16> m = _scene->_camera->_rotation.matrix();
		sf::Vector3f right(m[0], m[4], m[8]);
		sf::Vector3f up(m[1], m[5], m[9]);
		sf::Vector3f back(m[2], m[6], m[10]);

		bool isOnPreview = _scene->getViewport().contains(Anubis::cursor->_position);

		if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>(); scroll && isOnPreview) {
			float oldDistance = _scene->_cameraRadius / _scene->_cameraZoom;

			if (scroll->delta > 0 && _scene->_cameraZoom < 4.0f) _scene->_cameraZoom += 0.25f;
			if (scroll->delta < 0 && _scene->_cameraZoom > 0.25f) _scene->_cameraZoom -= 0.25f;

			float distance = _scene->_cameraRadius / _scene->_cameraZoom;

			_scene->_camera->_position += back * (distance - oldDistance);
			_scene->_cameraIsMoving = true;
			return;
		}

		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && pressed->button == sf::Mouse::Button::Middle && isOnPreview) {
			_scene->_lastCameraMouse = pressed->position;
			_scene->_isPressedMiddle = true;
			return;
		}

		if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>(); released && released->button == sf::Mouse::Button::Middle) {
			_scene->_isPressedMiddle = false;
			return;
		}

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>(); moved && _scene->_isPressedMiddle) {
			sf::Vector2i delta = moved->position - _scene->_lastCameraMouse;

			float distance = _scene->_cameraRadius / _scene->_cameraZoom;
			float unitsPerPixel = 2.f * distance * std::tan(_scene->_fovY * 0.5f * 3.14159265f / 180.f) / (float)_scene->getViewport().size.y;

			_scene->_camera->_position -= right * ((float)delta.x * unitsPerPixel);
			_scene->_camera->_position += up * ((float)delta.y * unitsPerPixel);

			_scene->_lastCameraMouse = moved->position;
			_scene->_cameraIsMoving = true;
			return;
		}
	}
	void PreviewPanel::update() {
		Components::Panel::update();

		//_loadBtn->update();

		_scene->update();
		_model->update();

	}

	void PreviewPanel::draw() {
		Components::Panel::draw();
		
		// draw title
		Anubis::render_window->draw(*_title);

		// draw load button
		//_loadBtn->draw();

		// draw checkerboard
		sf::RectangleShape rect(sf::Vector2f(_sceneRect.size));
		rect.setFillColor(sf::Color::White);
		rect.setPosition(sf::Vector2f(_sceneRect.position));
		sf::RenderStates states;
		states.shader = &*Anubis::shaders_manager->checkerboard_shader;
		Anubis::shaders_manager->checkerboard_shader->setUniform("rectPos", rect.getPosition());
		Anubis::render_window->draw(rect, states);

		// draw model
		drawModel();
	}
}