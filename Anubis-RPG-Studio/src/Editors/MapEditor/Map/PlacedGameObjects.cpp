#include "Editors/MapEditor/Map/PlacedGameObjects.hpp"
#include "DebugLog.hpp"
#include "Objects/Nature.hpp"
#include "Objects/Monster.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include <typeinfo>
#include "PrefabsManager.hpp"
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"
#include "ShadersManager.hpp"
#include "3D/OpenGLBuffers.hpp"
#include <SFML/OpenGL.hpp>

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
		if (placedGameObject.lock()->_model != nullptr) {

			updateModelTransform(placedGameObject.lock());
			MapEditor::editor->_scene->addModel(placedGameObject.lock()->_model);
			
		}

	}

	float PlacedGameObjects::getModelScale(const std::shared_ptr<GameObject>& prefab) {

		// TO-DO - scale from prefab data
		float tileSize = 16;
		float scale = 8;

		if (!prefab)
			return scale * tileSize;

		if (prefab->_type == ObjectType::Monster) scale = 4;
		if (prefab->_name == L"tree_1") scale = 8;
		if (prefab->_name == L"tree_2") scale = 8;
		if (prefab->_name == L"tree_3") scale = 8;
		if (prefab->_name == L"mushroom") scale = 1;
		if (prefab->_name == L"bush") scale = 8;
		if (prefab->_name == L"hunter_tent") scale = 10;
		if (prefab->_name == L"path_1") scale = 2;
		if (prefab->_name == L"path_2") scale = 2;
		if (prefab->_name == L"palisade_1") scale = 4;
		if (prefab->_name == L"palisade_2") scale = 4;
		if (prefab->_name == L"wooden_gate") scale = 4;
		if (prefab->_name == L"well") scale = 4;
		if (prefab->_name == L"pile_of_wood_1") scale = 4;
		if (prefab->_name == L"log_with_saw") scale = 4;
		if (prefab->_name == L"stone_platform") scale = 4;
		if (prefab->_name == L"tile_grass") scale = 2;
		if (prefab->_name == L"reed_1") scale = 2;
		if (prefab->_name == L"reed_2") scale = 2;
		if (prefab->_name == L"driftwood") scale = 4;
		if (prefab->_name == L"boulder_1") scale = 4;
		if (prefab->_name == L"boulder_2") scale = 4;
		if (prefab->_name == L"boulder_3") scale = 4;
		if (prefab->_name == L"column_1") scale = 4;
		if (prefab->_name == L"column_2") scale = 4;
		if (prefab->_name == L"small_grass_1") scale = 1;
		if (prefab->_name == L"small_grass_2") scale = 1;
		if (prefab->_name == L"small_grass_3") scale = 1;

		return scale * tileSize;
	}

	float PlacedGameObjects::getModelWind(const std::shared_ptr<GameObject>& prefab) {

		float windStrength = 0.0f;

		if (!prefab)
			return windStrength;

		if (prefab->_type == ObjectType::Monster) windStrength = 0;
		if (prefab->_name == L"tree_1") windStrength = 0.005f;
		if (prefab->_name == L"tree_2") windStrength = 0.005f;
		if (prefab->_name == L"tree_3") windStrength = 0.005f;
		if (prefab->_name == L"mushroom") windStrength = 0;
		if (prefab->_name == L"bush") windStrength = 0.2f;
		if (prefab->_name == L"hunter_tent") windStrength = 0;
		if (prefab->_name == L"path_1") windStrength = 0;
		if (prefab->_name == L"path_2") windStrength = 0;
		if (prefab->_name == L"palisade_1") windStrength = 0;
		if (prefab->_name == L"palisade_2") windStrength = 0;
		if (prefab->_name == L"wooden_gate") windStrength = 0;
		if (prefab->_name == L"well") windStrength = 0;
		if (prefab->_name == L"pile_of_wood_1") windStrength = 0;
		if (prefab->_name == L"log_with_saw") windStrength = 0;
		if (prefab->_name == L"stone_platform") windStrength = 0;
		if (prefab->_name == L"tile_grass") windStrength = 0.5;
		if (prefab->_name == L"reed_1") windStrength = 0.025f;
		if (prefab->_name == L"reed_2") windStrength = 0.025f;
		if (prefab->_name == L"driftwood") windStrength = 0;
		if (prefab->_name == L"boulder_1") windStrength = 0;
		if (prefab->_name == L"boulder_2") windStrength = 0;
		if (prefab->_name == L"boulder_3") windStrength = 0;
		if (prefab->_name == L"column_1") windStrength = 0;
		if (prefab->_name == L"column_2") windStrength = 0;
		if (prefab->_name == L"small_grass_1") windStrength = 0.15f;
		if (prefab->_name == L"small_grass_2") windStrength = 0.15f;
		if (prefab->_name == L"small_grass_3") windStrength = 0.15f;

		return windStrength * 0.5f;
	}


	sf::Vector3f PlacedGameObjects::getModelPosition(const std::shared_ptr<GameObject>& prefab, sf::Vector2i position) {

		float x = float(position.x);
		float z = float(position.y);

		if (prefab) {
			auto collider = prefab->getCollider();

			if (prefab->_name == L"hunter_tent" && collider) { x += collider->getRect().position.x / 10 + collider->getRect().size.x / 20; }

			if (prefab->_type != ObjectType::Monster && collider) {
				if (collider->_type == ColliderType::Circular) {
					x += prefab->getOrigin().x;
					z += prefab->getOrigin().y;
				}
				else {
					// TO-DO - to rewrite
					x += collider->getRect().size.x / 2;
					z += collider->getRect().size.y / 2;
				}
			}
		}

		// The model stands on the terrain surface.
		return sf::Vector3f(x, MapEditor::editor->_map->getHeight(x, z), z);
	}

	std::shared_ptr<Anubis::Model3D> PlacedGameObjects::createModel(const std::shared_ptr<GameObject>& prefab) {

		if (!prefab || !Anubis::prefabs_manager)
			return nullptr;

		std::shared_ptr<Anubis::Prefab> modelPrefab = Anubis::prefabs_manager->getPrefab(prefab->_name);

		if (!modelPrefab || !modelPrefab->_model)
			return nullptr;

		std::shared_ptr<Anubis::Model3D> model = std::make_shared<Anubis::Model3D>(*modelPrefab->_model);
		model->_scale = getModelScale(prefab);
		model->_windStrength = getModelWind(prefab);
		return model;
	}

	void PlacedGameObjects::updateModelTransform(const std::shared_ptr<PlacedGameObject>& object) {

		if (!object || !object->_model || object->_prefab.expired())
			return;

		std::shared_ptr<GameObject> prefab = object->_prefab.lock();

		object->_model->_scale = getModelScale(prefab);
		object->_model->_windStrength = getModelWind(prefab);
		object->_model->_position = getModelPosition(prefab, sf::Vector2i(object->getPosition()));

		// Models of placed objects take part in 3D picking (click and rect selection).
		object->_model->_isSelectable = true;
	}

	void PlacedGameObjects::removeGameObject(std::weak_ptr<PlacedGameObject> placedGameObject) {
		std::shared_ptr<PlacedGameObject> objectToRemove = placedGameObject.lock();

		if (!objectToRemove)
			return;

		// The object keeps its model, so undo (delete -> restore) brings it back in 3D.
		if (objectToRemove->_model != nullptr) {
			MapEditor::editor->_scene->removeModel(objectToRemove->_model);
		}

		std::erase_if(_visiblePlacedGameObjects,
			[&](const std::shared_ptr<PlacedGameObject>& object)
			{
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
				sf::Vector2i position = sf::Vector2i(objectOnMap->getPosition());

				if (currentPrefab->_type == ObjectType::Monster) {
					auto monster = std::dynamic_pointer_cast<Monster>(objectOnMap);
					position = sf::Vector2i(monster->_basePosition) - monster->_prefab.lock()->getOrigin();
				}


				// create a new object on the map with the new prefab
				std::shared_ptr<PlacedGameObject> newObjectOnMap;

				if (newPrefab->_type == ObjectType::Monster) newObjectOnMap = std::make_shared<Monster>(newPrefab);
				else if (newPrefab->_type == ObjectType::Nature) newObjectOnMap = std::make_shared<Nature>(newPrefab);
				else newObjectOnMap = std::make_shared<PlacedGameObject>(newPrefab);

				// set the position
				if (newObjectOnMap->_type == ObjectType::Monster) {
					newObjectOnMap->setPosition(sf::Vector2f(position + newPrefab->getOrigin()));
				}
				else
					newObjectOnMap->setPosition(sf::Vector2f(position));

				objectOnMap = newObjectOnMap;
			}
		}

		MapEditor::editor->setVisibleChunks();
		
	}

	void PlacedGameObjects::saveFromProject(std::ofstream& file) {

		BinaryWriter writer(file);

		for (auto& chunk : MapEditor::editor->_map->_chunks) {
			for (auto& object : chunk->_gameObjectsOnMap) {

				if (!object) continue;

				if (object->_type == ObjectType::None) {
					writer.write_int8((int8_t)object->_type);
					writer.write_Vector2i(sf::Vector2i(object->getPosition()));
					writer.write_int8(object->_animator->_animation);
					writer.write_int8(object->_animator->_frame);
					writer.write_float(object->_animator->_timer);
				}

				else if (object->_type == ObjectType::Nature) {
					std::shared_ptr<Nature> nature = std::dynamic_pointer_cast<Nature>(object);
					if (nature) {
						writer.write_int8((int8_t)nature->_type);
						writer.write_wstring((!nature->_prefab.expired()) ? nature->_prefab.lock()->_name : L"");
						writer.write_Vector2i(sf::Vector2i(nature->getPosition()));
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
						writer.write_Vector2i(sf::Vector2i(monster->_basePosition));
						writer.write_Vector2i(sf::Vector2i(monster->getPosition()));
						writer.write_int8((int)monster->_direction);
						writer.write_int8((int)monster->_state);
						writer.write_int8(monster->_animator->_animation);
						writer.write_int8(monster->_animator->_frame);
						writer.write_float(monster->_animator->_timer);
					}
				}
			}
		}

	}

	void PlacedGameObjects::loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader) {

		loader->setTitle(L"Loading GameObjects ...");

		BinaryReader reader(file);

		_visiblePlacedGameObjects.clear();

		int objectsCount = loader->counts.gameObjectsCount;

		for (int i = 0; i < objectsCount; i++) {
			ObjectType type = (ObjectType)reader.read_int8();

			if (type == ObjectType::None) {
				std::shared_ptr<PlacedGameObject> object = std::make_shared<PlacedGameObject>(std::weak_ptr<GameObject>());
				loader->nextStep(L"empty object");
				object->setPosition(sf::Vector2f(reader.read_Vector2i()));
				object->_animator->_animation = reader.read_int8();
				object->_animator->_frame = reader.read_int8();
				object->_animator->_timer = reader.read_float();
				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(object->getPosition()));
				if (chunk) chunk->addPlacedGameObject(object);
			}

			if (type == ObjectType::Nature) {
				std::wstring prefabName = reader.read_wstring();
				loader->nextStep(prefabName);
				std::shared_ptr<GameObject> prefab = prefabs_manager->getPrefab(prefabName);
				std::shared_ptr<Nature> nature = std::make_shared<Nature>(prefab);
				nature->setPosition(sf::Vector2f(reader.read_Vector2i()));
				
				if (prefab) {
					nature->_model = std::make_shared<Anubis::Model3D>(*prefab->_model.lock());
					nature->_model->_position.x = nature->getPosition().x;
					nature->_model->_position.y = 0;
					nature->_model->_position.z = nature->getPosition().y;
				}

				
				nature->_animator->_animation = reader.read_int8();
				nature->_animator->_frame = reader.read_int8();
				nature->_animator->_timer = reader.read_float();
				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(nature->getPosition()));
				if (chunk) chunk->addPlacedGameObject(nature);
			}

			if (type == ObjectType::Monster) {
				std::wstring prefabName = reader.read_wstring();
				loader->nextStep(prefabName);
				std::shared_ptr<GameObject> prefab = prefabs_manager->getPrefab(prefabName);
				std::shared_ptr<Monster> monster = std::make_shared<Monster>(prefab);
				monster->_basePosition = sf::Vector2f(reader.read_Vector2i());
				monster->setPosition(sf::Vector2f(reader.read_Vector2i()));

				if (prefab) {
					monster->_model = std::make_shared<Anubis::Model3D>(*prefab->_model.lock());
					monster->_model->_position.x = monster->getPosition().x;
					monster->_model->_position.y = 0;
					monster->_model->_position.z = monster->getPosition().y;
				}

				monster->_direction = (Direction)reader.read_int8();
				monster->_state = (MonsterState)reader.read_int8();
				monster->_animator->_animation = reader.read_int8();
				monster->_animator->_frame = reader.read_int8();
				monster->_animator->_timer = reader.read_float();
				std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(monster->_basePosition));
				if (chunk) chunk->addPlacedGameObject(monster);
			}
		}

		MapEditor::editor->setVisibleChunks();

	}

	void PlacedGameObjects::cursorHover() {
		for (auto& object : _visiblePlacedGameObjects) {
			object->cursorHover();
		}

	}

	void PlacedGameObjects::update() {

		for (auto& object : _visiblePlacedGameObjects) {
			object->update();
		}
	}

	void PlacedGameObjects::draw() {

		MapEditor::editor->_scene->setViewport();
		MapEditor::editor->_scene->setProjection();
		MapEditor::editor->_scene->setCamera();

		sf::Shader::bind(nullptr);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);

		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		glDisable(GL_CULL_FACE);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.01f);

		Anubis::GL::activeTexture(0x84C0); // GL_TEXTURE0

		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		GLfloat view[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);

		_billboard = {
			view[0], view[4], view[8],  0.f,
			view[1], view[5], view[9],  0.f,
			view[2], view[6], view[10], 0.f,
			0.f,     0.f,     0.f,      1.f
		};

		_boundTexture = nullptr;

		for (auto& object : _visiblePlacedGameObjects) {
			if (object) {
				object->draw();
			}
		}
	}
}