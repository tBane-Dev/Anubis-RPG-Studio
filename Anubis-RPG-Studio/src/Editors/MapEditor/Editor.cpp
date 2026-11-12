#include "Editors/MapEditor/Editor.hpp"
#include "DebugLog.hpp"
#include "Objects/Monster.hpp"
#include "Objects/Nature.hpp"
#include "PrefabsManager.hpp"
#include "WindowsManager.hpp"
#include "History.hpp"
#include <SFML/OpenGL.hpp>
#include "3D/Model3D.hpp"
#include "3D/Mesh3D.hpp"
#include "ShadersManager.hpp"

namespace MapEditor {

	Editor::Editor() {

	}

	Editor::~Editor() {

	}

	void Editor::createScene() {

		_scene = std::make_shared<Anubis::Scene>();
	
		auto size = Anubis::render_window->getSize();
		if (!_scene->createPickingBuffer(size.x, size.y)) {
			DebugError(L"Failed to create Picking Buffer");
			exit(0);
		}

		if (!_scene->initializeModel3DShaders()) {
			DebugError(L"Failed to Initialize Model3D Shaders");
			exit(0);
		}
	}

	void Editor::createMap(int width, int height) {

		_map = std::make_shared<Map>();

		_map->create(width, height);

		_map->drawCircle(sf::Vector2i(24, 28), 16, 4);
		_map->drawCircle(sf::Vector2i(48, 16), 16, 4);
		_map->drawCircle(sf::Vector2i(42, 28), 16, 4);
	}	

	void Editor::createCursorOnMap() {
		_cursor_on_map = std::make_shared<CursorOnMap>();
	}

	void Editor::createGameObjects() {
		_game_objects = std::make_shared<PlacedGameObjects>();
	}

	void Editor::createCamera() {

		_camera = std::make_shared<CameraOnMap>();

		_camera->_position = sf::Vector2f((float)(_map->getRect().size.x / 2), (float)(_map->getRect().size.y / 2));
		_camera->update();
	}

	void Editor::setVisibleChunks() {
		_map->setVisibleChunks();
		
		auto& visibleObjects = MapEditor::editor->_game_objects->_visiblePlacedGameObjects;

		for (const auto& selectedObject : MapEditor::editor->_cursor_on_map->_selectedObjects) {

			auto object = selectedObject->_object;
			if (!object)
				continue;

			if (std::find(visibleObjects.begin(), visibleObjects.end(), object) == visibleObjects.end()) {
				visibleObjects.emplace_back(selectedObject->_object);
			}
		}

		// Remove invisible models from scene
		std::erase_if(_scene->_models,
			[&](const std::shared_ptr<Anubis::Model3D>& model)
			{
				return std::find_if(visibleObjects.begin(), visibleObjects.end(),
					[&](const std::shared_ptr<PlacedGameObject>& object)
					{
						return object && object->_model == model;
					}) == visibleObjects.end();
			});
	}

	void Editor::createMainMenu() {
		_main_menu = std::make_shared<MainMenu>();
	}

	void Editor::createPalette() {
		_palette = std::make_shared<Palette>();
	}

	void Editor::cursorHover() {

		Main::Editor::cursorHover();

		if (Main::windows_manager->get_back())
			return;

		if (_main_menu->_state != Components::MainMenuStates::Closed) {
			_main_menu->cursorHover();
			return;
		}

		_map->cursorHover();
		_palette->cursorHover();
		_main_menu->cursorHover();
	}

	void Editor::handleEvent(const sf::Event& event) {

		if (Main::windows_manager->get_back())
			return;

		if(const auto& kp = event.getIf<sf::Event::KeyPressed>(); kp && kp->code == sf::Keyboard::Key::Delete) {
			
			if (!_cursor_on_map->_selectedObjects.empty()) {

				for(auto& selectedObject : _cursor_on_map->_selectedObjects) {
					if (auto object = selectedObject->_object) {
						_game_objects->removeGameObject(object);
						for(auto& chunk : _map->_chunks) {
							chunk->removePlacedGameObject(object);
						}
					}
				}

				std::shared_ptr<RemoveGameObjectsOnMap> step = std::make_shared<RemoveGameObjectsOnMap>(_cursor_on_map->_selectedObjects);
				history->addStep(step);
				for (auto& selected : _cursor_on_map->_selectedObjects) {
					selected->_object->_isSelected = false;
				}
				_cursor_on_map->_selectedObjects.clear();
				return;
			}
			
		}

		_main_menu->handleEvent(event);

		if (_main_menu->_state != Components::MainMenuStates::Closed)
			return;

		// Middle-button panning keeps the grabbed terrain point under the cursor (also for y != 0).
		if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Middle) {
			_scene->_panPlaneY = _cursor_on_map->_isOnTerrain ? _cursor_on_map->_worldPosition.y : 0.f;
		}

		_camera->handleEvent(event);
		_scene->handleEvent(event);

		_map->handleEvent(event);
		_cursor_on_map->handleEvent(event);
		_palette->handleEvent(event);


	}

	void Editor::update() {

		_main_menu->update();
		_palette->update();

		_camera->update();

		_cursor_on_map->update();

		_map->update();
		_game_objects->update();

		_scene->update();
	}

	void Editor::draw() {

		_camera->setView();

		Anubis::render_window->resetGLStates();

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glClearDepth(1.0);
		glClear(GL_DEPTH_BUFFER_BIT);

		_scene->setViewport();
		_scene->setProjection();
		_scene->setCamera();

		_scene->renderShadowMap();
		_map->draw();

		// Przywracamy test g??boko?ci po rysowaniu SFML w Map::draw().
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glClearDepth(1.0);
		glClear(GL_DEPTH_BUFFER_BIT);

		_scene->setViewport();
		_scene->setProjection();
		_scene->setCamera();

		_scene->draw();

		Anubis::render_window->resetGLStates();

		_cursor_on_map->draw();

		GUI_manager->setView();
		_main_menu->draw();
		_palette->draw();
	}

	std::shared_ptr<Editor> editor = nullptr;
}