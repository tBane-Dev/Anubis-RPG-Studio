#include "DebugLog.hpp"
#include "Editors/BuildingsEditor/Editor.hpp"
#include "Editors/BuildingsEditor/PlacedGameObjects.hpp"
#include <typeinfo>
#include "PrefabsManager.hpp"

namespace BuildingsEditor {

	PlacedGameObjects::PlacedGameObjects() {
		_visiblePlacedGameObjects.clear();
	}

	PlacedGameObjects::~PlacedGameObjects() {

	}

	void PlacedGameObjects::addGameObject(std::weak_ptr<PlacedGameObject> placedGameObject) {

		_visiblePlacedGameObjects.push_back(placedGameObject.lock());
	}

	void PlacedGameObjects::removeGameObject(std::weak_ptr<PlacedGameObject> placedGameObject) {
		std::shared_ptr<PlacedGameObject> objectToRemove = placedGameObject.lock();

		if (!objectToRemove)
			return;

		std::erase_if(_visiblePlacedGameObjects,
			[&](const std::shared_ptr<PlacedGameObject>& object) {
				return object == objectToRemove;
			});
	}


	void PlacedGameObjects::update() {

		for (auto& object : _visiblePlacedGameObjects) {
			object->update();
		}
	}
}