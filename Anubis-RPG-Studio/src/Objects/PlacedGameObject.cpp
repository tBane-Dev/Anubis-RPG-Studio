#include "Objects/PlacedGameObject.hpp"
#include "RenderWindow.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "DebugLog.hpp"
#include "Theme.hpp"
#include "ShadersManager.hpp"

PlacedGameObject::PlacedGameObject(std::weak_ptr<GameObject> prefab, bool play) : Object() {
	_prefab = prefab;
	_model = nullptr;


	if(_prefab.expired()) {
		_type = ObjectType::None;
	}
	else {
		_type = _prefab.lock()->_type;
	}

	_position = sf::Vector2f(0.0f, 0.0f);

	_isSelected = false;

	_text = std::make_unique<sf::Text>(basicFont, (_prefab.expired()) ? L"Unknown" : _prefab.lock()->getName(), 14);
}

PlacedGameObject::~PlacedGameObject() {

}

sf::Vector2f PlacedGameObject::getPosition() {
	return _position;
}

void PlacedGameObject::setPosition(sf::Vector2f position) {
	_position = position;
}

void PlacedGameObject::update() {

	if (_model)
		_model->update();
}

