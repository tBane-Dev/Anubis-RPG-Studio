#include "Editors/MapEditor/Map/PlacedGameObjects.hpp"
#include "DebugLog.hpp"
#include "Objects/Nature.hpp"
#include "Objects/Monster.hpp"
#include "Objects/Building/Building.hpp"
#include "Objects/Building/Roof1.hpp"
#include "Objects/Building/Roof2.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include <typeinfo>
#include "PrefabsManager.hpp"
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"
#include "ShadersManager.hpp"

namespace MapEditor {

	PlacedGameObjects::PlacedGameObjects() {
		_visiblePlacedGameObjects.clear();
		_hoveredPlacedGameObject = std::weak_ptr<PlacedGameObject>();


	}

	PlacedGameObjects::~PlacedGameObjects() {

	}

	void PlacedGameObjects::addGameObject(std::weak_ptr<PlacedGameObject> placedGameObject) {

		if (std::find(_visiblePlacedGameObjects.begin(), _visiblePlacedGameObjects.end(), placedGameObject.lock()) != _visiblePlacedGameObjects.end())
			return;

		_visiblePlacedGameObjects.emplace_back(placedGameObject.lock());

		if (placedGameObject.lock()->_type == ObjectType::Building) {
			std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(placedGameObject.lock());
			building->addDoorsToVisibleGameObjects(MapEditor::editor);
			building->addWindowsToVisibleGameObjects(MapEditor::editor);
			building->addWallMountedToVisibleGameObjects(MapEditor::editor);
			building->addWallsToVisibleGameObjects(MapEditor::editor);
			building->addSkeletsToVisibleGameObjects(MapEditor::editor);
			building->addOutsideToVisibleGameObjects(MapEditor::editor);
		}
	}

	void PlacedGameObjects::removeGameObject(std::weak_ptr<PlacedGameObject> placedGameObject) {
		std::shared_ptr<PlacedGameObject> objectToRemove = placedGameObject.lock();

		if (!objectToRemove)
			return;

		std::erase_if(_visiblePlacedGameObjects,
			[&](const std::shared_ptr<PlacedGameObject>& object)
			{
				if (objectToRemove->_type == ObjectType::Building && object->_type == ObjectType::Wall) {
					std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(objectToRemove);
					std::shared_ptr<Wall> wall = std::dynamic_pointer_cast<Wall>(object);
					return wall->_building.lock() == building;
				}

				if (objectToRemove->_type == ObjectType::Building && object->_type == ObjectType::Skelet) {
					std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(objectToRemove);
					std::shared_ptr<Skelet> skelet = std::dynamic_pointer_cast<Skelet>(object);
					return skelet->_building.lock() == building;
				}

				if (objectToRemove->_type == ObjectType::Building && object->_type == ObjectType::Outside) {
					std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(objectToRemove);
					std::shared_ptr<Outside> outside = std::dynamic_pointer_cast<Outside>(object);
					return outside->_building.lock() == building;
				}

				if (objectToRemove->_type == ObjectType::Building && object->_type == ObjectType::Door) {
					std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(objectToRemove);
					std::shared_ptr<Door> door = std::dynamic_pointer_cast<Door>(object);
					return door->_building.lock() == building;
				}

				return object == objectToRemove;
			});
	}

	void PlacedGameObjects::removeGameObjectsByAnimations(int animationID)
	{
		std::shared_ptr<Animations> animation = animations_manager->getAnimations(animationID).lock();

		if (!animation)
			return;

		std::erase_if(_visiblePlacedGameObjects,
			[&](const std::weak_ptr<PlacedGameObject>& weakObject)
			{
				auto object = weakObject.lock();
				if (!object)
					return true;

				auto prefab = object->_prefab.lock();
				if (!prefab)
					return true;

				auto prefabAnimation = prefab->_animations.lock();
				if (!prefabAnimation)
					return true;

				return prefabAnimation == animation;
			});
	}

	void PlacedGameObjects::removeGameObjectsByPrefab(std::weak_ptr<GameObject> prefab)
	{
		auto prefabPtr = prefab.lock();

		if (!prefabPtr)
			return;

		std::erase_if(_visiblePlacedGameObjects,
			[&](const std::shared_ptr<PlacedGameObject>& object)
			{
				if (!object)
					return true;

				auto objectPrefab = object->_prefab.lock();

				if (!objectPrefab)
					return true;

				return objectPrefab == prefabPtr;
			});
	}


	void PlacedGameObjects::replacePrefab(std::shared_ptr<GameObject> oldPrefab, std::shared_ptr<GameObject> newPrefab) {
		if (!oldPrefab || !newPrefab)
			return;

		for (auto& chunk : MapEditor::editor->_map->_chunks) {
			for (auto& objectOnMap : chunk->_gameObjectsOnMap) {

				std::shared_ptr<GameObject> currentPrefab = objectOnMap->_prefab.lock();

				if (currentPrefab != oldPrefab)
					continue;

				// get the position of the current object on the map
				sf::Vector2i position = objectOnMap->_position;

				if (currentPrefab->_type == ObjectType::Monster) {
					auto monster = std::dynamic_pointer_cast<Monster>(objectOnMap);
					position = monster->_basePosition - monster->_prefab.lock()->getOrigin();
				}


				// create a new object on the map with the new prefab
				std::shared_ptr<PlacedGameObject> newObjectOnMap;

				if (newPrefab->_type == ObjectType::Monster) newObjectOnMap = std::make_shared<Monster>(newPrefab);
				else if (newPrefab->_type == ObjectType::Nature) newObjectOnMap = std::make_shared<Nature>(newPrefab);
				else newObjectOnMap = std::make_shared<PlacedGameObject>(newPrefab);

				// set the position
				if (newObjectOnMap->_type == ObjectType::Monster) {
					newObjectOnMap->setPosition(position + newPrefab->getOrigin());
				}
				else
					newObjectOnMap->setPosition(position);

				objectOnMap = newObjectOnMap;
			}
		}

		MapEditor::editor->setVisibleChunks();
		
	}

	void PlacedGameObjects::saveFromProject(std::ofstream& saver) {

		BinaryWriter writer(saver);

		int32_t count = 0;
		for (auto& chunk : MapEditor::editor->_map->_chunks) {
			for (auto& object : chunk->_gameObjectsOnMap) {

				if (!object)
					continue;

				if (object->_type == ObjectType::None ||
					object->_type == ObjectType::Nature ||
					object->_type == ObjectType::Monster ||
					object->_type == ObjectType::Building) {
					count++;
				}
			}
		}

		writer.write_int32(count);

		for (auto& chunk : MapEditor::editor->_map->_chunks) {
			for (auto& object : chunk->_gameObjectsOnMap) {

				if (!object) continue;

				if (object->_type == ObjectType::None) {
					writer.write_int8((int8_t)object->_type);
					writer.write_Vector2i(object->_position);
					writer.write_int8(object->_animator->_animation);
					writer.write_int8(object->_animator->_frame);
					writer.write_float(object->_animator->_timer);
				}

				else if (object->_type == ObjectType::Nature) {
					std::shared_ptr<Nature> nature = std::dynamic_pointer_cast<Nature>(object);
					if (nature) {
						writer.write_int8((int8_t)nature->_type);
						writer.write_wstring((!nature->_prefab.expired()) ? nature->_prefab.lock()->_name : L"");
						writer.write_Vector2i(nature->_position);
						writer.write_int8(nature->_animator->_animation);
						writer.write_int8(nature->_animator->_frame);
						writer.write_float(nature->_animator->_timer);
					}
				}

				else if (object->_type == ObjectType::Monster) {
					std::shared_ptr<Monster> monster = std::dynamic_pointer_cast<Monster>(object);
					if (monster) {
						writer.write_int8((int8_t)monster->_type);
						writer.write_wstring((!monster->_prefab.expired()) ? monster->_prefab.lock()->_name : L"");
						writer.write_Vector2i(monster->_basePosition);
						writer.write_Vector2i(monster->_position);
						writer.write_int8((int)monster->_direction);
						writer.write_int8((int)monster->_state);
						writer.write_int8(monster->_animator->_animation);
						writer.write_int8(monster->_animator->_frame);
						writer.write_float(monster->_animator->_timer);
					}
				}

				else if (object->_type == ObjectType::Building) {
					std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(object);
					std::shared_ptr<BuildingPrefab> bp = std::dynamic_pointer_cast<BuildingPrefab>(building->_prefab.lock());

					writer.write_int8((int8_t)bp->_type);
					writer.write_wstring(bp->_name);
					writer.write_int8(bp->_wallHeight);
					writer.write_int8(bp->_skeletType);
					int roof = 0;
					if (std::dynamic_pointer_cast<Roof1>(bp->_roof)) roof = 1;
					if (std::dynamic_pointer_cast<Roof2>(bp->_roof)) roof = 2;
					writer.write_int8(roof);
					writer.write_int8((int8_t)bp->_roof->_type);
					writer.write_int8((int8_t)bp->_roof->_wallsRoof);

					writer.write_Vector2i(sf::Vector2i(bp->_floor[0].size(), bp->_floor.size()));
					writer.write_Vector2i(sf::Vector2i(bp->_walls[0].size(), bp->_walls.size()));

					// save floor
					for (int y = 0; y < bp->_floor.size(); y += 1) {
						for (int x = 0; x < bp->_floor[y].size(); x += 1) {
							writer.write_int8(bp->_floor[y][x]);
						}
					}

					// save walls
					for (int y = 0; y < bp->_walls.size(); y += 1) {
						for (int x = 0; x < bp->_walls[y].size(); x += 1) {
							writer.write_int8(bp->_walls[y][x]);
						}
					}

					// save doors
					writer.write_int32(bp->_doors.size());
					for (auto& door : bp->_doors) {
						writer.write_wstring(door->_prefab.lock()->_name);
						writer.write_Vector2i(door->_position);
					}

					// save windows
					writer.write_int32(bp->_windows.size());
					for (auto& window : bp->_windows) {
						writer.write_wstring(window->_prefab.lock()->_name);
						writer.write_Vector2i(window->_position);
						writer.write_int8(window->_level);
					}

					// save wall mounter
					writer.write_int32(bp->_wallMounted.size());
					for (auto& wallMounted : bp->_wallMounted) {
						writer.write_wstring(wallMounted->_prefab.lock()->_name);
						writer.write_Vector2i(wallMounted->_position);
						writer.write_int8(wallMounted->_level);
					}

					// save position
					writer.write_Vector2i(building->_position);
				}
			}
		}

	}

	void PlacedGameObjects::loadFromProject(std::ifstream& loader) {

		_visiblePlacedGameObjects.clear();

		BinaryReader reader(loader);

		int objectsCount = reader.read_int32();

		for (int i = 0; i < objectsCount; i++) {
			ObjectType type = (ObjectType)reader.read_int8();

			if (type == ObjectType::None) {
				std::shared_ptr<PlacedGameObject> object = std::make_shared<PlacedGameObject>(std::weak_ptr<GameObject>());
				object->setPosition(reader.read_Vector2i());
				object->_animator->_animation = reader.read_int8();
				object->_animator->_frame = reader.read_int8();
				object->_animator->_timer = reader.read_float();
				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(object->_position);
				if (chunk) chunk->addPlacedGameObject(object);
			}

			if (type == ObjectType::Nature) {
				std::wstring prefabName = reader.read_wstring();
				std::shared_ptr<GameObject> prefab = prefabs_manager->getPrefab(prefabName);
				std::shared_ptr<Nature> nature = std::make_shared<Nature>(prefab);
				nature->setPosition(reader.read_Vector2i());
				nature->_animator->_animation = reader.read_int8();
				nature->_animator->_frame = reader.read_int8();
				nature->_animator->_timer = reader.read_float();
				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(nature->_position);
				if (chunk) chunk->addPlacedGameObject(nature);
			}

			if (type == ObjectType::Monster) {
				std::wstring prefabName = reader.read_wstring();
				std::shared_ptr<GameObject> prefab = prefabs_manager->getPrefab(prefabName);
				std::shared_ptr<Monster> monster = std::make_shared<Monster>(prefab);
				monster->_basePosition = reader.read_Vector2i();
				monster->setPosition(reader.read_Vector2i());
				monster->_direction = (Direction)reader.read_int8();
				monster->_state = (MonsterState)reader.read_int8();
				monster->_animator->_animation = reader.read_int8();
				monster->_animator->_frame = reader.read_int8();
				monster->_animator->_timer = reader.read_float();
				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(monster->_basePosition);
				if (chunk) chunk->addPlacedGameObject(monster);
			}

			if (type == ObjectType::Building) {
				std::wstring name = reader.read_wstring();
				int wallHeight = reader.read_int8();
				int skeletType = reader.read_int8();
				int roof = reader.read_int8();
				int roofType = reader.read_int8();
				int wallsRoof = reader.read_int8();

				sf::Vector2i floorSize = reader.read_Vector2i();
				sf::Vector2i wallsSize = reader.read_Vector2i();
				std::shared_ptr<BuildingPrefab> bp = std::make_shared<BuildingPrefab>(name);
				prefabs_manager->addPrefab(bp);



				// load floor
				bp->_floor.clear();
				for (int y = 0; y < floorSize.y; y += 1) {
					bp->_floor.push_back(std::vector<int>(floorSize.x, 0));
					for (int x = 0; x < floorSize.x; x += 1) {
						bp->_floor[y][x] = reader.read_int8();
					}
				}

				// load walls
				bp->_walls.clear();
				for (int y = 0; y < wallsSize.y; y += 1) {
					bp->_walls.push_back(std::vector<int>(wallsSize.x, -1));
					for (int x = 0; x < wallsSize.x; x += 1) {
						bp->_walls[y][x] = reader.read_int8();
					}
				}

				// parameters
				bp->_wallHeight = wallHeight;
				bp->_skeletType = skeletType;

				std::shared_ptr<Building> building = std::make_shared<Building>(bp);

				// load doors
				int doorsCount = reader.read_int32();
				for (int i = 0; i < doorsCount; i += 1) {
					std::wstring name = reader.read_wstring();
					std::shared_ptr<Door> door = std::make_shared<Door>(buildings_parts_prefabs_manager->getPrefab(name), building);
					sf::Vector2i pos = reader.read_Vector2i();
					door->setPosition(pos);
					bp->_doors.push_back(door);
				}

				// load windows
				int windowsCount = reader.read_int32();
				for (int i = 0; i < windowsCount; i += 1) {
					std::wstring name = reader.read_wstring();
					sf::Vector2i pos = reader.read_Vector2i();
					int level = reader.read_int8();
					std::shared_ptr<Window> window = std::make_shared<Window>(buildings_parts_prefabs_manager->getPrefab(name), building, level);
					window->setPosition(pos);
					bp->_windows.push_back(window);
				}

				// load wall mounted
				int wallMountedCount = reader.read_int32();
				for (int i = 0; i < wallMountedCount; i += 1) {
					std::wstring name = reader.read_wstring();
					sf::Vector2i pos = reader.read_Vector2i();
					int level = reader.read_int8();
					std::shared_ptr<WallMounted> wallMounted = std::make_shared<WallMounted>(buildings_parts_prefabs_manager->getPrefab(name), building, level);
					wallMounted->setPosition(pos);
					bp->_wallMounted.push_back(wallMounted);
				}

				// position
				sf::Vector2i pos = reader.read_Vector2i();

				if (roof == 1) {
					bp->_roof = std::make_shared<Roof1>(roofType, bp->_wallHeight, wallsRoof);
				}
				else if (roof == 2) {
					bp->_roof = std::make_shared<Roof2>(roofType, bp->_wallHeight, wallsRoof);
				}
				else {
					bp->_roof = std::make_shared<Roof1>(roofType, bp->_wallHeight, wallsRoof);
				}

				bp->_roof->generate(bp->_walls, sf::Vector2i(0, 0), 1.0f);

				building->setPosition(pos);

				bp->generate(pos, 1.0f, building);
				building->generate();

				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(building->getPosition());
				if (chunk) chunk->addPlacedGameObject(building);
			}
		}

		MapEditor::editor->setVisibleChunks();

	}

	void PlacedGameObjects::cursorHover() {
		for (auto& object : _visiblePlacedGameObjects) {
			if (object->_type == ObjectType::Building) {
				object->cursorHover();
			}
		}

		for (auto& object : _visiblePlacedGameObjects) {
			if (object->_type != ObjectType::Building) {
				object->cursorHover();
			}
		}

	}

	void PlacedGameObjects::update() {

		for (auto& object : _visiblePlacedGameObjects) {
			object->update();
		}
	}

	void PlacedGameObjects::draw() {

		MapEditor::editor->_camera->setView();

		const sf::View& view = Anubis::render_window->getView();
		sf::Vector2f size = view.getSize();
		sf::IntRect viewport = Anubis::render_window->getViewport(view);
		const float aspect = (float)(viewport.size.x) / (float)(viewport.size.y);
		float pi = 3.14159265f;
		float fov = 60.0f;
		const float distance = (size.y * 0.5f) / std::tan(fov * 0.5f * pi / 180.0f);

		sprite25D_shader->setUniform("cameraCenter", view.getCenter());
		sprite25D_shader->setUniform("cameraDistance", distance);
		sprite25D_shader->setUniform("cameraAngle", MapEditor::editor->_camera->_angle);
		sprite25D_shader->setUniform("aspect", aspect);
		sprite25D_shader->setUniform("spriteScale", sf::Vector2f(1.0f, 1.0f));

		for (auto& object : _visiblePlacedGameObjects) {
			if (object->_type == ObjectType::Building) {
				std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(object);
				if (building) {
					std::shared_ptr<BuildingPrefab> buildingPrefab = std::dynamic_pointer_cast<BuildingPrefab>(building->_prefab.lock());
					if (buildingPrefab)
						buildingPrefab->drawOnlyCollider(*Anubis::render_window, building->getPosition());

					building->draw(); // draw selected frame
				}
			}
		}

		for (auto& object : _visiblePlacedGameObjects) {
			if (object->_type == ObjectType::Building) {
				std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(object);
				if (building) {
					std::shared_ptr<BuildingPrefab> buildingPrefab = std::dynamic_pointer_cast<BuildingPrefab>(building->_prefab.lock());
					if (buildingPrefab)
						buildingPrefab->drawOnlyFloor(*Anubis::render_window, building->getPosition());
				}
			}
		}

		for (auto& object : _visiblePlacedGameObjects) {
			if (object->_type != ObjectType::Building) {
				object->draw();
			}
		}

	}
}