#include "Editors/MapEditor/Map/CursorOnMap.hpp"
#include "Cursor.hpp"
#include "Editors/MapEditor/Map/CameraOnMap.hpp"
#include "Editors/MapEditor/Map/Map.hpp"
#include "EditorsManager.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "Objects/GameObject.hpp"
#include "Objects/Terrain.hpp"
#include "Objects/Monster.hpp"
#include "Objects/Nature.hpp"
#include "PrefabsManager.hpp"
#include "Editors/MapEditor/Map/PlacedGameObjects.hpp"
#include <typeinfo>
#include "Animator.hpp"
#include "DebugLog.hpp"
#include <set>
#include "WindowsManager.hpp"
#include "History.hpp"
#include <cmath>
#include <algorithm>
#include <SFML/OpenGL.hpp>
#include "3D/OpenGLBuffers.hpp"

namespace MapEditor {

	CursorOnMap::CursorOnMap() : CursorWithObject() {

		_prevSelectedObjects.clear();
		_selectedObjects.clear();

		_prevPosition = sf::Vector2i(-1, -1);

		_isDragging = false;
		_isSelecting = false;
		_selectionRect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(0, 0));

		_selectionStart = sf::Vector3f(0.f, 0.f, 0.f);
		_selectionStartOnTerrain = false;

		_pickClock = sf::Clock();
		_pickInterval = 100;			// 100 ms

		_worldPosition = sf::Vector3f(0.0f, 0.0f, 0.0f);
		_isOnTerrain = false;
	}

	CursorOnMap::~CursorOnMap() {

	}

	void CursorOnMap::addToSelected(std::shared_ptr<PlacedGameObject> object, sf::Vector2i offset) {

		if (std::find_if(_selectedObjects.begin(), _selectedObjects.end(),
			[&](std::shared_ptr<SelectedPlacedGameObject>& selectedObject) {
				return selectedObject->_object == object;
			}
		) == _selectedObjects.end()) {
			std::shared_ptr<SelectedPlacedGameObject> selectedObject = std::make_shared<SelectedPlacedGameObject>(object, sf::Vector2i(object->getPosition()), offset);
			selectedObject->_object->_isSelected = true;
			_selectedObjects.push_back(selectedObject);
		}
	}

	void CursorOnMap::removeFromSelected(std::shared_ptr<GameObject> object) {
		std::erase_if(_selectedObjects, [&](std::shared_ptr<SelectedPlacedGameObject>& obj) {
			bool condition = obj->_object->_prefab.lock() == object;
			if (condition)
				obj->_object->_isSelected = false;
			return condition;
			});
	}

	std::shared_ptr<PlacedGameObject> CursorOnMap::getObjectUnderCursor() {

		std::vector<std::shared_ptr<PlacedGameObject>>& objects = MapEditor::editor->_game_objects->_visiblePlacedGameObjects;

		// Only 3D models are selectable. Picking returns the model nearest to the camera.
		int id = MapEditor::editor->_scene->getHoveredModelId(_position);

		if (id >= 0) {
			for (auto& object : objects) {
				if (object && object->_model && object->_model->_id == id)
					return object;
			}
		}

		return nullptr;
	}

	std::vector<std::shared_ptr<PlacedGameObject>> CursorOnMap::getObjectsInSelectionRect() {

		std::vector<std::shared_ptr<PlacedGameObject>> result;

		if (_selectionRect.size.x == 0 && _selectionRect.size.y == 0)
			return result;

		// 3D models touching the screen rect (also ones hidden behind other models).
		std::vector<std::shared_ptr<Anubis::Model3D>> models = MapEditor::editor->_scene->getModelsInRect(_selectionRect);

		for (auto& object : MapEditor::editor->_game_objects->_visiblePlacedGameObjects) {

			if (!object || !object->_model) continue;

			if (std::find(models.begin(), models.end(), object->_model) != models.end())
				result.push_back(object);
		}

		return result;
	}

	void CursorOnMap::syncSceneSelection() {

		std::shared_ptr<Anubis::Scene> scene = MapEditor::editor->_scene;

		// Selected objects are highlighted by Model3D::draw() through Scene::_selected.
		scene->_selected.clear();

		for (auto& selected : _selectedObjects) {
			if (selected && selected->_object && selected->_object->_model)
				scene->_selected.push_back(selected->_object->_model);
		}

		scene->_hovered.clear();

		if (_isDragging || _isSelecting || !_object.expired())
			return;

		if (GUI_manager->Element_hovered != MapEditor::editor->_map)
			return;

		int id = scene->getHoveredModelId(_position);

		if (id < 0)
			return;

		for (auto& model : scene->_models) {
			if (model && model->_id == id) {
				scene->_hovered.push_back(model);
				break;
			}
		}
	}

	void CursorOnMap::update() {
		Cursor::update();

		// Terrain point under the cursor (works for any terrain heights).
		// _globalPosition keeps the map coordinates (world x, world z) for the existing 2D map logic.
		// Rebuild chunks changed by the terrain brush (once per frame).
		if (!_chunksToRebuild.empty()) {
			std::shared_ptr<Map> mapa = MapEditor::editor->_map;

			for (auto& c : _chunksToRebuild)
				c->generateVertexBuffer(
					mapa->getChunkByCoords(c->_coords.x - 1, c->_coords.y - 1), mapa->getChunkByCoords(c->_coords.x, c->_coords.y - 1), mapa->getChunkByCoords(c->_coords.x + 1, c->_coords.y - 1),
					mapa->getChunkByCoords(c->_coords.x - 1, c->_coords.y), mapa->getChunkByCoords(c->_coords.x + 1, c->_coords.y),
					mapa->getChunkByCoords(c->_coords.x - 1, c->_coords.y + 1), mapa->getChunkByCoords(c->_coords.x, c->_coords.y + 1), mapa->getChunkByCoords(c->_coords.x + 1, c->_coords.y + 1)
				);

			_chunksToRebuild.clear();
		}

		if (_pickClock.getElapsedTime() > sf::milliseconds(_pickInterval)) {
			_isOnTerrain = MapEditor::editor->_map->pickTerrain(_position, _worldPosition);

		}

		if (_isOnTerrain) {
			_globalPosition = sf::Vector2i(
				(int)std::floor(_worldPosition.x),
				(int)std::floor(_worldPosition.z)
			);
		}

		if (_pickClock.getElapsedTime() > sf::milliseconds(_pickInterval)) {
			syncSceneSelection();
			_pickClock.restart();
		}

		if (_isDragging || _isSelecting) {
			if (_isDragging) {

				_prevPosition = _position;

				for (auto& object : _selectedObjects) {

					if (!object) continue;
					if (!object->_object) continue;

					std::shared_ptr<PlacedGameObject> gameObject = object->_object;

					sf::Vector2i oldPos = sf::Vector2i(object->_object->getPosition());

					std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(oldPos);

					if (chunk)
						chunk->removePlacedGameObject(gameObject);

					sf::Vector2i newPos = MapEditor::editor->_cursor_on_map->_globalPosition - object->_offset;

					newPos.x = (newPos.x / Tile::tileSize) * Tile::tileSize;
					newPos.y = (newPos.y / Tile::tileSize) * Tile::tileSize;

					gameObject->setPosition(sf::Vector2f(newPos));
					MapEditor::editor->_game_objects->updateModelTransform(gameObject);


					chunk = MapEditor::editor->_map->getChunkByGlobalPosition(newPos);

					if (chunk)
						chunk->addPlacedGameObject(gameObject);
				}

				return;
			}

			if (!_isSelecting)
				return;

			if (_selectionStartOnTerrain) {
				sf::Vector2f startPixel;
				if (MapEditor::editor->_scene->worldToScreen(_selectionStart, startPixel))
					_selectionRect.position = sf::Vector2i(startPixel);
			}

			if (GUI_manager->Element_pressed == MapEditor::editor->_map)
				_selectionRect.size = _position - _selectionRect.position;

			if (!(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))) {
				for (auto& object : _selectedObjects) {
					if (object->_object)
						object->_object->_isSelected = false;
				}
				_selectedObjects.clear();
			}

			if (_selectionRect.size.x != 0 || _selectionRect.size.y != 0) {
				std::vector<std::shared_ptr<PlacedGameObject>> selectedGameObjects = getObjectsInSelectionRect();

				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
					for (auto& selected : _selectedObjects) {
						if (selected->_object)
							selected->_object->_isSelected = false;
					}

					_selectedObjects.clear();

					for (auto& prev : _prevSelectedObjects) {
						if (prev->_object) {
							prev->_object->_isSelected = true;
							_selectedObjects.push_back(prev);
						}
					}

					for (auto& object : selectedGameObjects) {

						std::vector<std::shared_ptr<SelectedPlacedGameObject>>::iterator it = std::find_if(
							_selectedObjects.begin(),
							_selectedObjects.end(),
							[&](std::shared_ptr<SelectedPlacedGameObject>& selected) {
								return selected->_object && selected->_object.get() == object.get();
							}
						);

						if (it == _selectedObjects.end()) {
							object->_isSelected = true;
							_selectedObjects.push_back(std::make_shared<SelectedPlacedGameObject>(object, sf::Vector2i(object->getPosition()), MapEditor::editor->_cursor_on_map->_globalPosition - sf::Vector2i(object->getPosition())));
						}
					}
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {

					for (auto& selected : _selectedObjects) {
						if (selected->_object)
							selected->_object->_isSelected = false;
					}

					_selectedObjects.clear();


					for (auto& prev : _prevSelectedObjects) {
						if (!prev->_object)
							continue;

						prev->_object->_isSelected = true;
						_selectedObjects.push_back(prev);
					}

					for (auto& object : selectedGameObjects) {
						std::vector<std::shared_ptr<SelectedPlacedGameObject>>::iterator prevIt = std::find_if(
							_prevSelectedObjects.begin(),
							_prevSelectedObjects.end(),
							[&](std::shared_ptr<SelectedPlacedGameObject>& selected) {
								return selected->_object && selected->_object.get() == object.get();
							}
						);

						if (prevIt != _prevSelectedObjects.end()) {

							object->_isSelected = false;
							std::vector<std::shared_ptr<SelectedPlacedGameObject>>::iterator it = std::find_if(
								_selectedObjects.begin(),
								_selectedObjects.end(),
								[&](std::shared_ptr<SelectedPlacedGameObject>& selected) {
									return selected->_object && selected->_object.get() == object.get();
								}
							);

							if (it != _selectedObjects.end())
								_selectedObjects.erase(it);
						}
						else {
							object->_isSelected = true;
							_selectedObjects.push_back(std::make_shared<SelectedPlacedGameObject>(object, sf::Vector2i(object->getPosition()), MapEditor::editor->_cursor_on_map->_globalPosition - sf::Vector2i(object->getPosition())));
						}
					}
				}
				else {
					for (auto& object : selectedGameObjects) {
						object->_isSelected = true;
						_selectedObjects.push_back(std::make_shared<SelectedPlacedGameObject>(object, sf::Vector2i(object->getPosition()), MapEditor::editor->_cursor_on_map->_globalPosition - sf::Vector2i(object->getPosition())));
					}
				}


			}
		}
	}

	void CursorOnMap::handleEvent(const sf::Event& event) {

		if (_object.expired()) {

			if (const auto* mm = event.getIf<sf::Event::MouseMoved>(); mm && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				if (!_isDragging) {
					if (!_isSelecting) {
						_selectionRect.position = _position;
						_isSelecting = true;
						_selectionStartOnTerrain = MapEditor::editor->_map->pickTerrain(_position, _selectionStart);
					}
					_selectionRect.size = _position - _selectionRect.position;
					return;
				}
			}

			// A click on the empty background (outside the terrain, not on GUI) also deselects.
			if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left && GUI_manager->Element_hovered == nullptr) {

				bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);
				bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);

				if (!ctrl && !shift) {
					for (auto& object : _selectedObjects) {
						if (object->_object)
							object->_object->_isSelected = false;
					}
					_selectedObjects.clear();
					_prevSelectedObjects.clear();
				}
			}

			if (GUI_manager->Element_pressed == MapEditor::editor->_map) {

				if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left) {

					_isDragging = false;
					_isSelecting = false;

					bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);
					bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);

					std::shared_ptr<PlacedGameObject> selectedGameObject = getObjectUnderCursor();

					if (selectedGameObject) {
						std::vector<std::shared_ptr<SelectedPlacedGameObject>>::iterator selectedIt =
							std::find_if(
								_selectedObjects.begin(),
								_selectedObjects.end(),
								[&](std::shared_ptr<SelectedPlacedGameObject>& selectedObject) {
									return selectedObject->_object && selectedObject->_object == selectedGameObject;
								}
							);

						bool alreadySelected = selectedIt != _selectedObjects.end();

						if (!ctrl && !shift) {
							if (alreadySelected) {

								_isDragging = true;
								_prevPosition = _position;

								for (auto& object : _selectedObjects) {
									if (!object->_object)
										continue;

									object->_prevPosition = sf::Vector2i(object->_object->_position);
									object->_offset = _globalPosition - sf::Vector2i(object->_object->_position);
								}
								return;
							}

							for (auto& object : _selectedObjects) {

								if (object->_object)
									object->_object->_isSelected = false;
							}

							_selectedObjects.clear();
							selectedGameObject->_isSelected = true;
							_selectedObjects.push_back(std::make_shared<SelectedPlacedGameObject>(selectedGameObject, sf::Vector2i(selectedGameObject->getPosition()), MapEditor::editor->_cursor_on_map->_globalPosition - sf::Vector2i(selectedGameObject->getPosition())));
							return;
						}

						if (shift) {
							if (!alreadySelected) {
								selectedGameObject->_isSelected = true;
								_selectedObjects.push_back(std::make_shared<SelectedPlacedGameObject>(selectedGameObject, sf::Vector2i(selectedGameObject->getPosition()), MapEditor::editor->_cursor_on_map->_globalPosition - sf::Vector2i(selectedGameObject->getPosition())));
							}
							return;
						}

						if (ctrl) {
							if (alreadySelected) {
								selectedGameObject->_isSelected = false;
								_selectedObjects.erase(selectedIt);
							}
							else {
								selectedGameObject->_isSelected = true;
								_selectedObjects.push_back(std::make_shared<SelectedPlacedGameObject>(selectedGameObject, sf::Vector2i(selectedGameObject->getPosition()), MapEditor::editor->_cursor_on_map->_globalPosition - sf::Vector2i(selectedGameObject->getPosition())));
							}
							return;
						}
					}

					if (!ctrl && !shift) {
						for (auto& object : _selectedObjects) {
							if (object->_object)
								object->_object->_isSelected = false;
						}
						_selectedObjects.clear();
					}

					_isSelecting = true;
					_selectionRect.position = _position;
					_selectionRect.size = sf::Vector2i(0, 0);
					_selectionStartOnTerrain = MapEditor::editor->_map->pickTerrain(_position, _selectionStart);
				}
			}

			if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Right) {
				if (!_selectedObjects.empty()) {
					for (auto& object : _selectedObjects) {
						if (object->_object)
							object->_object->_isSelected = false;
					}
					_selectedObjects.clear();
					return;
				}
			}

			if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Left) {
				_prevSelectedObjects.clear();

				for (auto& object : _selectedObjects) {
					if (!object->_object)
						continue;

					if (object->_object->_isSelected)
						_prevSelectedObjects.push_back(object);
				}

				if (_isDragging) {
					std::shared_ptr<MoveGameObjectsOnMap> step = std::make_shared<MoveGameObjectsOnMap>(_prevSelectedObjects);
					history->addStep(step);
				}


				_isDragging = false;
				_isSelecting = false;
				_selectionRect.size = sf::Vector2i(0, 0);
			}
		}

		if (_object.expired())
			return;

		if (Main::windows_manager->get_back())
			return;

		if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Right) {
			if (std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools); tools && tools->_selectedTool != nullptr)
				tools->setTool(tools->_tools[0], ToolTerrainType::None);

			if (MapEditor::editor->_palette->_slots->_selectedSlot != nullptr)
				MapEditor::editor->_palette->_slots->selectSlot(-1);

			if (_state == Cursors::CursorWithObjectState::Drawing)
				_state = Cursors::CursorWithObjectState::Idle;

			_object = std::weak_ptr<Object>();
			return;
		}

		if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Left) {

			if (_state == Cursors::CursorWithObjectState::Drawing)
				_state = Cursors::CursorWithObjectState::Idle;

		}

		if (!(GUI_manager->Element_pressed == nullptr || GUI_manager->Element_pressed == MapEditor::editor->_map))
			return;

		if (_object.lock()->_type == ObjectType::Terrain) {

			std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);

			bool conditionToDraw =
				GUI_manager->Element_hovered == MapEditor::editor->_map &&
				_isOnTerrain &&
				sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) &&
				MapEditor::editor->_map->getChunkByGlobalPosition() != nullptr &&
				(tools && tools->_toolType == ToolTerrainType::Circle || tools->_toolType == ToolTerrainType::Rect);

			if (conditionToDraw) {

				std::shared_ptr<Map> mapa = std::dynamic_pointer_cast<Map>(MapEditor::editor->_map);

				// Same tile as the brush preview drawn on the terrain.
				sf::Vector2i tileCoords = getCursorTile();

				int type = std::dynamic_pointer_cast<Terrain>(MapEditor::editor->_cursor_on_map->_object.lock())->_id;

				std::set<std::shared_ptr<Chunk>> chunksToEdit;

				int brushSize = MapEditor::editor->_palette->_brushSize;
				std::vector<std::vector<bool>> brush;

				if (tools->_toolType == ToolTerrainType::Rect)
					brush = Cursors::square_brushes[brushSize];

				if (tools->_toolType == ToolTerrainType::Circle)
					brush = Cursors::circle_brushes[brushSize];


				for (int yy = 0; yy < brush.size(); yy++) {
					for (int xx = 0; xx < brush[yy].size(); xx++) {
						if (brush[yy][xx]) {

							int tileX = tileCoords.x + (xx - brush[yy].size() / 2);
							int tileY = tileCoords.y + (yy - brush.size() / 2);



							std::shared_ptr<Chunk> c = mapa->getChunkByTileGlobalCoords(tileX, tileY);
							if (!c) continue;

							std::shared_ptr<Tile> t = c->getTileByTileGlobalCoords(tileX, tileY);
							if (!t) continue;

							sf::Vector2i pos(tileX, tileY);

							if (_editedTiles.find(pos) == _editedTiles.end())
								_editedTiles[pos] = TileValues{ t->_type, type };
							else
								_editedTiles[pos].newValue = type;

							chunksToEdit.insert(c);

							// Rebuild only when the tile really changes.
							if (t->_type == type)
								continue;

							t->_type = type;

							_chunksToRebuild.insert(c);

							// Tile shapes depend on neighbours 1 tile away, so a neighbour chunk
							// needs a rebuild only when the tile lies on the edge of its chunk.
							int localX = tileX - c->_coords.x * Chunk::tilesCols;
							int localY = tileY - c->_coords.y * Chunk::tilesRows;

							int dxMin = (localX == 0) ? -1 : 0;
							int dxMax = (localX == Chunk::tilesCols - 1) ? 1 : 0;
							int dyMin = (localY == 0) ? -1 : 0;
							int dyMax = (localY == Chunk::tilesRows - 1) ? 1 : 0;

							for (int dy = dyMin; dy <= dyMax; dy++) {
								for (int dx = dxMin; dx <= dxMax; dx++) {
									if (dx == 0 && dy == 0) continue;
									if (std::shared_ptr<Chunk> neighbour = mapa->getChunkByCoords(c->_coords.x + dx, c->_coords.y + dy))
										_chunksToRebuild.insert(neighbour);
								}
							}
						}
					}
				}

				// Chunks are rebuilt once per frame in update(), not on every mouse event.

				if (!chunksToEdit.empty())
					_state = Cursors::CursorWithObjectState::Drawing;
			}

			if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Left) {
				std::shared_ptr<EditTerrain> step = std::make_shared<EditTerrain>(_editedTiles);
				history->addStep(step);
				_editedTiles.clear();
			}

			return;
		}



		if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Left) {

			if (GUI_manager->Element_pressed == MapEditor::editor->_map) {
				std::shared_ptr<GameObject> prefab = std::dynamic_pointer_cast<GameObject>(_object.lock());
				if (!prefab || !_isOnTerrain)
					return;

				// position of object on the map, aligned to the terrain tile grid (same as the preview)
				sf::Vector2i position = getPlacementPosition(prefab);

				// create object on map by type
				std::shared_ptr<PlacedGameObject> objectOnMap;

				
				if (prefab->_type == ObjectType::Monster) objectOnMap = std::make_shared<Monster>(prefab);
				else if (prefab->_type == ObjectType::Nature) objectOnMap = std::make_shared<Nature>(prefab);
				else objectOnMap = std::make_shared<PlacedGameObject>(prefab);

				// positioning and adding object to map
				objectOnMap->setPosition(sf::Vector2f(position));

				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(position);
				if (!chunk)
					return;

				// 3D model of the prefab (positioned by PlacedGameObjects::updateModelTransform in setVisibleChunks)
				objectOnMap->_model = PlacedGameObjects::createModel(prefab);

				chunk->addPlacedGameObject(objectOnMap);

				std::shared_ptr<AddGameObjectOnMap> step = std::make_shared<AddGameObjectOnMap>(objectOnMap, position);
				history->addStep(step);

				MapEditor::editor->setVisibleChunks();
				return;
			}

		}

	}


	sf::Vector2i CursorOnMap::getCursorTile() {
		return sf::Vector2i(
			(int)std::floor(_worldPosition.x / (float)Tile::tileSize),
			(int)std::floor(_worldPosition.z / (float)Tile::tileSize)
		);
	}

	sf::Vector2i CursorOnMap::getPlacementPosition(std::shared_ptr<GameObject> prefab) {

		// Where the model / sprite stands, relative to the object's position.
		sf::Vector3f anchor = PlacedGameObjects::getModelPosition(prefab, sf::Vector2i(0, 0));

		// The anchor follows the cursor; the object's position snaps to the terrain tile grid.
		float x = _worldPosition.x - anchor.x;
		float z = _worldPosition.z - anchor.z;

		return sf::Vector2i(
			(int)(std::round(x / Tile::tileSize) * Tile::tileSize),
			(int)(std::round(z / Tile::tileSize) * Tile::tileSize)
		);
	}

	void CursorOnMap::drawTerrainBrush() {

		std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);

		if (!tools)
			return;

		int brushSize = MapEditor::editor->_palette->_brushSize;
		std::vector<std::vector<bool>> brush;

		if (tools->_toolType == ToolTerrainType::Rect)
			brush = Cursors::square_brushes[brushSize];

		if (tools->_toolType == ToolTerrainType::Circle)
			brush = Cursors::circle_brushes[brushSize];

		if (brush.empty() || brush[0].empty())
			return;

		int height = (int)brush.size();
		int width = (int)brush[0].size();

		std::vector<float> mask(width * height, 0.f);

		for (int yy = 0; yy < height; yy++)
			for (int xx = 0; xx < width && xx < (int)brush[yy].size(); xx++)
				mask[yy * width + xx] = brush[yy][xx] ? 1.f : 0.f;

		// Same tiles as the painting code in handleEvent().
		sf::Vector2i firstTile = getCursorTile() - sf::Vector2i(width / 2, height / 2);

		MapEditor::editor->_map->drawTileOverlay(firstTile, sf::Vector2i(width, height), mask, sf::Color(255, 47, 47, 127));
	}

	void CursorOnMap::drawPlacementPreview(std::shared_ptr<GameObject> prefab) {

		std::shared_ptr<Anubis::Scene> scene = MapEditor::editor->_scene;
		sf::Vector2i position = getPlacementPosition(prefab);
		sf::Vector3f anchor = PlacedGameObjects::getModelPosition(prefab, position);

		// Recreate the preview model when the prefab changes.
		if (_previewPrefab.lock() != prefab) {
			_previewPrefab = prefab;
			_previewModel = PlacedGameObjects::createModel(prefab);

			if (_previewModel) {
				_previewModel->setScene(scene);
				_previewModel->_isSelectable = false; // never picked, it is not in Scene::_models
			}
		}

		if (!_previewModel) {
			drawPreviewSprite(prefab, position);
			return;
		}

		_previewModel->_scale = PlacedGameObjects::getModelScale(prefab);
		_previewModel->_position = anchor;

		GLint previousMatrixMode = 0;
		glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		scene->setViewport();
		scene->setProjection();
		scene->setCamera();

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		GLfloat view[16];
		GLfloat projection[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);

		if (scene->beginModelPass(view, projection, false, false)) {
			GLfloat tint[4] = { 1.f, 1.f, 1.f, 1.f };
			scene->drawModelFast(*_previewModel, tint, nullptr);
			scene->endModelPass();
		}

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(previousMatrixMode);
		glPopAttrib();
	}

	void CursorOnMap::drawPreviewSprite(std::shared_ptr<GameObject> prefab, sf::Vector2i position) {

		// 2D prefab without a 3D model: billboard like PlacedGameObject::draw(), but translucent.
		std::shared_ptr<Animations> animations = prefab->getAnimations().lock();

		if (!animations)
			return;

		std::shared_ptr<Texture> texture = animations->getTexture();

		if (!texture || !texture->_texture)
			return;

		std::shared_ptr<Anubis::Scene> scene = MapEditor::editor->_scene;
		sf::IntRect frame = animations->getFrameRect(0, 0);

		sf::Vector2f origin(prefab->getOrigin());
		std::shared_ptr<Collider> collider = prefab->getCollider();

		if (collider && collider->_type == ColliderType::Rectangular)
			origin += sf::Vector2f(collider->getRect().size / 2);

		sf::Vector2f anchor = sf::Vector2f(position) + origin;
		float y = MapEditor::editor->_map->getHeight(anchor.x, anchor.y);

		float width = (float)std::abs(frame.size.x);
		float height = (float)std::abs(frame.size.y);
		float left = (float)frame.position.x;
		float top = (float)frame.position.y;
		float right = float(frame.position.x + frame.size.x);
		float bottom = float(frame.position.y + frame.size.y);

		GLint previousMatrixMode = 0;
		glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		scene->setViewport();
		scene->setProjection();
		scene->setCamera();

		GLfloat view[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);

		// Inverse of the camera rotation: the sprite always faces the camera.
		GLfloat billboard[16] = {
			view[0], view[4], view[8],  0.f,
			view[1], view[5], view[9],  0.f,
			view[2], view[6], view[10], 0.f,
			0.f,     0.f,     0.f,      1.f
		};

		sf::Shader::bind(nullptr);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_FALSE);
		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.01f);

		Anubis::GL::activeTexture(0x84C0); // GL_TEXTURE0
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		sf::Texture::bind(texture->_texture.get(), sf::CoordinateType::Pixels);

		glTranslatef(anchor.x, y, anchor.y);
		glMultMatrixf(billboard);
		glScalef(1.f, -1.f, 1.f);
		glTranslatef(-origin.x, -origin.y, 0.f);

		glColor4ub(255, 255, 255, 140);

		glBegin(GL_TRIANGLES);
		glTexCoord2f(left, top);      glVertex2f(0.f, 0.f);
		glTexCoord2f(left, bottom);   glVertex2f(0.f, height);
		glTexCoord2f(right, bottom);  glVertex2f(width, height);
		glTexCoord2f(left, top);      glVertex2f(0.f, 0.f);
		glTexCoord2f(right, bottom);  glVertex2f(width, height);
		glTexCoord2f(right, top);     glVertex2f(width, 0.f);
		glEnd();

		sf::Texture::bind(nullptr);

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(previousMatrixMode);
		glPopAttrib();
	}

	void CursorOnMap::draw() {
		if (!(GUI_manager->Element_pressed == MapEditor::editor->_map || GUI_manager->Element_hovered == MapEditor::editor->_map || GUI_manager->Element_hovered == nullptr))
			return;

		if (MapEditor::editor->_main_menu->_state != Components::MainMenuStates::Closed)
			return;

		if (_selectionRect.size.x != 0 || _selectionRect.size.y != 0) {
			// The selection rect is in window pixels.
			Anubis::render_window->setView(Anubis::render_window->getDefaultView());

			sf::RectangleShape selectionRectShape(sf::Vector2f(_selectionRect.size));
			selectionRectShape.setPosition(sf::Vector2f(_selectionRect.position));
			selectionRectShape.setFillColor(sf::Color(0, 0, 255, 50));
			selectionRectShape.setOutlineThickness(1);
			selectionRectShape.setOutlineColor(sf::Color(0, 0, 255, 200));
			Anubis::render_window->draw(selectionRectShape);

			return;
		}

		if (_object.expired() || !_isOnTerrain)
			return;

		if (_object.lock()->_type == ObjectType::Terrain) {
			drawTerrainBrush();
			Anubis::render_window->resetGLStates();
			return;
		}

		if (std::shared_ptr<GameObject> prefab = std::dynamic_pointer_cast<GameObject>(_object.lock())) {
			drawPlacementPreview(prefab);
			Anubis::render_window->resetGLStates();
		}
	}
}
