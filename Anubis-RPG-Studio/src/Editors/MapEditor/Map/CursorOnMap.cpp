#include "Editors/MapEditor/Map/CursorOnMap.hpp"
#include "Cursor.hpp"
#include "Editors/MapEditor/Map/CameraOnMap.hpp"
#include "Editors/MapEditor/Map/Map.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "EditorsManager.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "Objects/GameObject.hpp"
#include "Objects/Terrain.hpp"
#include "Objects/WaterPrefab.hpp"
#include "Objects/Monster.hpp"
#include "Objects/Nature.hpp"
#include "PrefabsManager.hpp"
#include "Editors/MapEditor/Map/PlacedGameObjects.hpp"
#include <typeinfo>
#include "DebugLog.hpp"
#include <set>
#include "WindowsManager.hpp"
#include "History.hpp"
#include "Time.hpp"
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

		// no model - maybe a water surface
		return MapEditor::editor->_map->getWaterUnderCursor(_position);
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

		if (Anubis::GUI_manager->Element_hovered != MapEditor::editor->_map)
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
		// Terrain brush: paints every frame while the left button is held.
		if (_isPainting) {
			if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
				endTerrainStroke();
			else {
				// the terrain under the cursor changes while painting heights - pick it every frame
				_isOnTerrain = MapEditor::editor->_map->pickTerrain(_position, _worldPosition);

				if (_isOnTerrain)
					paintTerrain();
			}
		}

		// Slope: the end of the slope follows the cursor, the slope is made when the left button is released
		if (_isSlopeDragging) {
			_isOnTerrain = MapEditor::editor->_map->pickTerrain(_position, _worldPosition);

			if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				useSlope();
				endTerrainStroke();
			}
		}

		// Refresh chunks changed by the terrain brush (once per frame).
		if (!_chunksToRebuild.empty()) {
			std::vector<std::shared_ptr<Chunk>> chunks(_chunksToRebuild.begin(), _chunksToRebuild.end());
			MapEditor::editor->_map->refreshChunks(chunks, _heightsChanged);

			_chunksToRebuild.clear();
			_heightsChanged = false;
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
			if (_isDragging && _isDraggingWater) {
				dragWaters();
				return;
			}

			if (_isDragging) {

				_prevPosition = _position;

				for (auto& object : _selectedObjects) {

					if (!object) continue;
					if (!object->_object) continue;

					// waters do not move - only their level (dragWaters)
					if (object->_object->_type == ObjectType::Water) continue;

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

			if (Anubis::GUI_manager->Element_pressed == MapEditor::editor->_map)
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

		// height tools work without an object (no terrain has to be selected)
		if (isHeightTool()) {
			handleTerrainEvent(event);
			return;
		}

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
			if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left && Anubis::GUI_manager->Element_hovered == nullptr) {

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

			if (Anubis::GUI_manager->Element_pressed == MapEditor::editor->_map) {

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

								if (selectedGameObject->_type == ObjectType::Water)
									startWaterDrag(std::dynamic_pointer_cast<Water>(selectedGameObject));

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

				if (_isDragging && _isDraggingWater)
					endWaterDrag();
				else if (_isDragging) {
					// waters are not moved (and must not be added to chunks by undo)
					std::vector<std::shared_ptr<SelectedPlacedGameObject>> movedObjects;
					for (auto& object : _prevSelectedObjects)
						if (object->_object && object->_object->_type != ObjectType::Water) movedObjects.push_back(object);

					if (!movedObjects.empty()) {
						std::shared_ptr<MoveGameObjectsOnMap> step = std::make_shared<MoveGameObjectsOnMap>(movedObjects);
						history->addStep(step);
					}
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

		if (!(Anubis::GUI_manager->Element_pressed == nullptr || Anubis::GUI_manager->Element_pressed == MapEditor::editor->_map))
			return;

		if (_object.lock()->_type == ObjectType::Terrain || _object.lock()->_type == ObjectType::Water) {
			handleTerrainEvent(event);
			return;
		}



		if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Left) {

			if (Anubis::GUI_manager->Element_pressed == MapEditor::editor->_map) {
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

	bool CursorOnMap::isHeightTool() {

		std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);

		if (!tools)
			return false;

		return tools->_toolType == ToolTerrainType::Raise || tools->_toolType == ToolTerrainType::Lower || tools->_toolType == ToolTerrainType::Smooth || tools->_toolType == ToolTerrainType::Flatten || tools->_toolType == ToolTerrainType::Slope || tools->_toolType == ToolTerrainType::WaterSeabed;
	}

	// terrain and height tools: start and end of a brush stroke - painting itself is in update() (every frame while the button is held)
	void CursorOnMap::handleTerrainEvent(const sf::Event& event) {

		if (Main::windows_manager->get_back())
			return;

		std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);

		if (!tools)
			return;

		// right button - no tool
		if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Right) {
			endTerrainStroke();
			tools->setTool(tools->_tools[0], ToolTerrainType::None);
			return;
		}

		if (!(Anubis::GUI_manager->Element_pressed == nullptr || Anubis::GUI_manager->Element_pressed == MapEditor::editor->_map))
			return;

		if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left) {
			if (tools->_toolType != ToolTerrainType::None && Anubis::GUI_manager->Element_hovered == MapEditor::editor->_map && _isOnTerrain) {

				// Fill is one click, no stroke
				if (tools->_toolType == ToolTerrainType::WaterFill) {
					useWaterFill();
					return;
				}

				// Seabed is one click on a water
				if (tools->_toolType == ToolTerrainType::WaterSeabed) {
					useWaterSeabed();
					return;
				}

				// Slope is made on release
				if (tools->_toolType == ToolTerrainType::Slope) {
					MapEditor::editor->_map->pickTerrain(_position, _worldPosition);
					_isSlopeDragging = true;
					_slopeStart = _worldPosition;
					_strokeSnapshots.clear();
					_state = Cursors::CursorWithObjectState::Drawing;
					return;
				}

				if (isWaterTool())
					startWaterStroke();

				_isPainting = true;
				_strokeSnapshots.clear();
				_flattenHeight = _worldPosition.y;
				_state = Cursors::CursorWithObjectState::Drawing;
			}
		}

		if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Left) {
			if (_isSlopeDragging)
				useSlope();

			endTerrainStroke();
		}
	}

	// brush sizes 0 - 5: 8, 16, 32, 64, 128, 256 world units
	float CursorOnMap::getBrushRadius() {
		return float(Tile::tileSize) * 0.5f * float(1 << MapEditor::editor->_palette->_brushSize);
	}

	// keeps the terrain of the chunk from before the stroke (undo)
	void CursorOnMap::addToStroke(std::shared_ptr<Chunk> chunk) {

		for (auto& snapshot : _strokeSnapshots) {
			if (snapshot->_chunk == chunk)
				return;
		}

		std::shared_ptr<TerrainSnapshot> snapshot = std::make_shared<TerrainSnapshot>();
		snapshot->_chunk = chunk;
		snapshot->_oldHeights = chunk->_heights;
		snapshot->_oldWeights = chunk->_weights;
		_strokeSnapshots.push_back(snapshot);
	}

	void CursorOnMap::paintTerrain() {

		std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);

		if (!tools || tools->_toolType == ToolTerrainType::None)
			return;

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;

		float radius = getBrushRadius();
		bool square = tools->_toolType == ToolTerrainType::Rect;
		sf::Vector2f center(_worldPosition.x, _worldPosition.z);
		float dt = Anubis::deltaTime.asSeconds();

		if (isWaterTool()) {
			if (_paintWater)
				_paintWater->paint(center, radius, tools->_toolType == ToolTerrainType::WaterEraser, dt * 4.f);
			return;
		}

		std::vector<std::shared_ptr<Chunk>> chunks = mapa->getChunksInRect(center, radius);

		if (chunks.empty())
			return;

		for (auto& chunk : chunks)
			addToStroke(chunk);

		if (tools->_toolType == ToolTerrainType::Circle || tools->_toolType == ToolTerrainType::Rect) {
			std::shared_ptr<Terrain> terrain = std::dynamic_pointer_cast<Terrain>(_object.lock());

			if (!terrain)
				return;

			mapa->paintTerrain(chunks, center, radius, square, terrain->_id, dt * 4.f);	// full terrain in the middle after 0.25 s
		}
		else {
			float amount = dt * 6.f;									// Smooth, Flatten: speed

			if (tools->_toolType == ToolTerrainType::Raise || tools->_toolType == ToolTerrainType::Lower)
				amount = dt * 48.f;									// world units per second

			mapa->changeHeights(chunks, center, radius, square, tools->_toolType, amount, _flattenHeight);
			_heightsChanged = true;
		}

		for (auto& chunk : chunks)
			_chunksToRebuild.insert(chunk);
	}

	void CursorOnMap::endTerrainStroke() {

		endWaterStroke();

		if (!_strokeSnapshots.empty()) {
			for (auto& snapshot : _strokeSnapshots) {
				snapshot->_newHeights = snapshot->_chunk->_heights;
				snapshot->_newWeights = snapshot->_chunk->_weights;
			}

			std::shared_ptr<EditTerrain> step = std::make_shared<EditTerrain>(_strokeSnapshots);
			history->addStep(step);

			// new terrain heights - the shore of the waters (foam) is counted again
			for (auto& water : MapEditor::editor->_map->_waters)
				water->updateAllChunks();
		}

		_strokeSnapshots.clear();
		_isPainting = false;
		_isSlopeDragging = false;

		if (_state == Cursors::CursorWithObjectState::Drawing)
			_state = Cursors::CursorWithObjectState::Idle;
	}

	// Slope: from the point where LMB was pressed to the point under the cursor
	void CursorOnMap::useSlope() {

		_isSlopeDragging = false;

		if (!_isOnTerrain)
			return;

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;

		sf::Vector2f start(_slopeStart.x, _slopeStart.z);
		sf::Vector2f end(_worldPosition.x, _worldPosition.z);
		sf::Vector2f center = (start + end) * 0.5f;
		float radius = getBrushRadius();
		float halfLength = std::sqrt((end.x - start.x) * (end.x - start.x) + (end.y - start.y) * (end.y - start.y)) * 0.5f;

		std::vector<std::shared_ptr<Chunk>> chunks = mapa->getChunksInRect(center, halfLength + radius);

		if (chunks.empty())
			return;

		// old heights for undo - the history step is added by endTerrainStroke()
		for (auto& chunk : chunks)
			addToStroke(chunk);

		mapa->makeSlope(chunks, _slopeStart, _worldPosition, radius);
		mapa->refreshChunks(chunks, true);
	}

	void CursorOnMap::drawTerrainBrush() {

		std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);

		if (!tools || tools->_toolType == ToolTerrainType::None)
			return;

		// Fill is a click - no brush
		if (tools->_toolType == ToolTerrainType::WaterFill)
			return;

		bool square = tools->_toolType == ToolTerrainType::Rect;

		// red = terrains, blue = heights, cyan = water brush, white = water eraser
		sf::Color color(255, 47, 47, 95);

		if (!(tools->_toolType == ToolTerrainType::Circle || tools->_toolType == ToolTerrainType::Rect))
			color = sf::Color(47, 127, 255, 95);

		if (tools->_toolType == ToolTerrainType::WaterBrush) color = sf::Color(47, 207, 255, 95);
		if (tools->_toolType == ToolTerrainType::WaterEraser) color = sf::Color(255, 255, 255, 95);
		if (tools->_toolType == ToolTerrainType::WaterSeabed) color = sf::Color(255, 207, 47, 95);

		sf::Vector2f position(_worldPosition.x, _worldPosition.z);
		sf::Vector2f start = position;

		// slope: the line from the start of the slope to the cursor
		if (_isSlopeDragging)
			start = sf::Vector2f(_slopeStart.x, _slopeStart.z);

		MapEditor::editor->_map->drawBrushOverlay(start, getBrushRadius(), square, color, position);
	}

	// WATERS ///////////////////////////////////////////////////////////////////////////////////////////////////

	bool CursorOnMap::isWaterTool() {

		std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);

		if (!tools)
			return false;

		return tools->_toolType == ToolTerrainType::WaterFill || tools->_toolType == ToolTerrainType::WaterBrush || tools->_toolType == ToolTerrainType::WaterEraser;
	}

	// the water under the cursor, or the only selected water - of the kind chosen in the palette
	std::shared_ptr<Water> CursorOnMap::getWaterToEdit(int kind) {

		std::shared_ptr<Water> water = MapEditor::editor->_map->getWaterUnderCursor(_position);

		if (water && water->_kind == kind)
			return water;

		std::shared_ptr<Water> selectedWater = nullptr;
		int selectedWaters = 0;

		for (auto& selected : _selectedObjects) {
			if (std::shared_ptr<Water> w = std::dynamic_pointer_cast<Water>(selected->_object)) {
				selectedWater = w;
				selectedWaters += 1;
			}
		}

		if (selectedWaters == 1 && selectedWater->_kind == kind)
			return selectedWater;

		return nullptr;
	}

	// Fill: click in a hollow - the water spreads to the shore. A click on an existing water adds a new start point to it.
	void CursorOnMap::useWaterFill() {

		std::shared_ptr<WaterPrefab> prefab = std::dynamic_pointer_cast<WaterPrefab>(_object.lock());

		if (!prefab)
			return;

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;
		std::shared_ptr<Water> water = getWaterToEdit(prefab->_id);
		bool existed = water != nullptr;
		WaterState oldState;

		if (water)
			oldState = water->getState();
		else {
			water = std::make_shared<Water>(prefab->_id, _worldPosition.y + _newWaterDepth);
			mapa->addWater(water);
		}

		water->_seeds.push_back(sf::Vector2f(_worldPosition.x, _worldPosition.z));
		water->fill();

		std::shared_ptr<EditWater> step = std::make_shared<EditWater>(water, oldState, existed, water->getState(), true);
		history->addStep(step);
	}

	// Seabed: click on a water - the terrain under it goes down evenly from the beach.
	// The size of the brush says how far from the beach the slope goes (width = 2 x radius, depth = width / 8).
	void CursorOnMap::useWaterSeabed() {

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;
		std::shared_ptr<Water> water = mapa->getWaterUnderCursor(_position);

		if (!water)
			return;

		float width = getBrushRadius() * 2.f;
		float depth = width / 8.f;

		// the chunks of the water and the chunks around (shared edges)
		std::vector<std::shared_ptr<Chunk>> chunks;

		for (auto& waterChunk : water->_chunks) {
			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					std::shared_ptr<Chunk> chunk = mapa->getChunkByCoords(waterChunk->_coords.x + dx, waterChunk->_coords.y + dy);

					if (chunk && std::find(chunks.begin(), chunks.end(), chunk) == chunks.end())
						chunks.push_back(chunk);
				}
			}
		}

		// old heights for undo - the history step is added by endTerrainStroke()
		_strokeSnapshots.clear();

		for (auto& chunk : chunks)
			addToStroke(chunk);

		mapa->shapeSeabed(water, width, depth);
		mapa->refreshChunks(chunks, true);
		endTerrainStroke();
	}

	// Brush / Eraser: start of a stroke
	void CursorOnMap::startWaterStroke() {

		_paintWater = nullptr;

		std::shared_ptr<ToolsTerrain> tools = std::dynamic_pointer_cast<ToolsTerrain>(MapEditor::editor->_palette->_tools);
		std::shared_ptr<WaterPrefab> prefab = std::dynamic_pointer_cast<WaterPrefab>(_object.lock());

		if (!tools || !prefab)
			return;

		_paintWater = getWaterToEdit(prefab->_id);
		_paintWaterExisted = _paintWater != nullptr;

		if (_paintWater) {
			_paintWaterOldState = _paintWater->getState();
			return;
		}

		// the eraser needs a water
		if (tools->_toolType == ToolTerrainType::WaterEraser)
			return;

		_paintWaterOldState = WaterState();
		_paintWater = std::make_shared<Water>(prefab->_id, _worldPosition.y + _newWaterDepth);
		MapEditor::editor->_map->addWater(_paintWater);
	}

	void CursorOnMap::endWaterStroke() {

		if (!_paintWater)
			return;

		_paintWater->removeEmptyChunks();
		_paintWater->updateAllChunks();		// the shore (foam) is counted again after the stroke
		bool exists = !_paintWater->_chunks.empty();

		if (!exists)
			MapEditor::editor->_map->removeWater(_paintWater);

		if (exists || _paintWaterExisted) {
			std::shared_ptr<EditWater> step = std::make_shared<EditWater>(_paintWater, _paintWaterOldState, _paintWaterExisted, _paintWater->getState(), exists);
			history->addStep(step);
		}

		_paintWater = nullptr;
	}

	// LMB pressed on a selected water: the grabbed point of the surface goes up and down with the cursor
	void CursorOnMap::startWaterDrag(std::shared_ptr<Water> clickedWater) {

		_isDraggingWater = true;
		_dragWaters.clear();
		_dragWaterLevels.clear();
		_dragWaterStates.clear();

		for (auto& selected : _selectedObjects) {
			if (std::shared_ptr<Water> water = std::dynamic_pointer_cast<Water>(selected->_object)) {
				_dragWaters.push_back(water);
				_dragWaterLevels.push_back(water->_level);
				_dragWaterStates.push_back(water->getState());
			}
		}

		_dragStartMouseY = _position.y;
		_waterGrab = _worldPosition;
		_waterDragNormal = sf::Vector3f(0.f, 0.f, 0.f);

		sf::Vector3f origin, dir;

		if (MapEditor::editor->_scene->screenToRay(_position, origin, dir)) {

			sf::Vector3f hit;
			if (clickedWater && clickedWater->getRayHit(origin, dir, hit))
				_waterGrab = hit;

			// vertical plane through the grabbed point, turned to the camera
			float length = std::sqrt(dir.x * dir.x + dir.z * dir.z);
			if (length > 0.2f)
				_waterDragNormal = sf::Vector3f(dir.x / length, 0.f, dir.z / length);
		}
	}

	void CursorOnMap::dragWaters() {

		float newY = _waterGrab.y + float(_dragStartMouseY - _position.y) * 0.5f;	// camera looks straight down
		sf::Vector3f origin, dir;

		if (_waterDragNormal != sf::Vector3f(0.f, 0.f, 0.f) && MapEditor::editor->_scene->screenToRay(_position, origin, dir)) {
			float along = dir.x * _waterDragNormal.x + dir.z * _waterDragNormal.z;

			if (std::abs(along) > 0.0001f) {
				sf::Vector3f toGrab = _waterGrab - origin;
				float t = (toGrab.x * _waterDragNormal.x + toGrab.z * _waterDragNormal.z) / along;
				newY = origin.y + dir.y * t;
			}
		}

		float delta = newY - _waterGrab.y;

		for (int i = 0; i < (int)_dragWaters.size(); i++)
			_dragWaters[i]->_level = _dragWaterLevels[i] + delta;
	}

	// the new level changes the shape - the water is filled again from its start points
	void CursorOnMap::endWaterDrag() {

		for (int i = 0; i < (int)_dragWaters.size(); i++) {

			if (_dragWaters[i]->_level == _dragWaterLevels[i])
				continue;

			_dragWaters[i]->fill();

			std::shared_ptr<EditWater> step = std::make_shared<EditWater>(_dragWaters[i], _dragWaterStates[i], true, _dragWaters[i]->getState(), true);
			history->addStep(step);
		}

		_dragWaters.clear();
		_dragWaterLevels.clear();
		_dragWaterStates.clear();
		_isDraggingWater = false;
	}

	void CursorOnMap::drawPlacementPreview(std::shared_ptr<GameObject> prefab) {

		std::shared_ptr<Anubis::Scene> scene = MapEditor::editor->_scene;
		sf::Vector2i position = getPlacementPosition(prefab);
		sf::Vector3f anchor = PlacedGameObjects::getModelPosition(prefab, position);

		// Recreate the preview model when the prefab changes.
		if (_previewPrefab.lock() != prefab) {
			_previewPrefab = prefab;
			_previewModel = PlacedGameObjects::createModel(prefab);
		}

		if (prefab->_model.expired()) 
			_previewModel = nullptr;

		if (!_previewModel) return;

		_previewModel->setScene(scene);
		_previewModel->_isSelectable = false; // never picked, it is not in Scene::_models
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
	void CursorOnMap::draw() {
		if (!(Anubis::GUI_manager->Element_pressed == MapEditor::editor->_map || Anubis::GUI_manager->Element_hovered == MapEditor::editor->_map || Anubis::GUI_manager->Element_hovered == nullptr))
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

		if (isHeightTool()) {
			if (_isOnTerrain) {
				drawTerrainBrush();
				Anubis::render_window->resetGLStates();
			}
			return;
		}

		if (_object.expired() || !_isOnTerrain)
			return;

		if (_object.lock()->_type == ObjectType::Terrain || _object.lock()->_type == ObjectType::Water) {
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
