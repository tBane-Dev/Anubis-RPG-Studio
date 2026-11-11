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

// TO-DO - to delete
void PlacedGameObject::drawFrame(sf::Color color) {


	auto prefab = _prefab.lock();
	if (!prefab)
		return;

	int x, y, w, h;

	float thickness = 2.f;
	float paddingLeft = 14.f;
	float textPadding = 6.f;

	sf::Vector2f objectPosition = _position;

	if (prefab->_type == ObjectType::Monster) {
		objectPosition.x -= prefab->getOrigin().x;
		objectPosition.y -= prefab->getOrigin().y;
	}

	x = objectPosition.x;
	y = objectPosition.y;
	w = 96;
	h = 96;

	std::shared_ptr<Collider> collider = prefab->getCollider();

	if (collider) {
		sf::IntRect rect = collider->getRect();

		x = _position.x + rect.position.x - static_cast<int>(thickness);
		y = _position.y + rect.position.y - static_cast<int>(thickness);
		w = rect.size.x + static_cast<int>(2.f * thickness);
		h = rect.size.y + static_cast<int>(2.f * thickness);
	}
		
	_text->setString((_prefab.expired()) ? L"Unknown" : _prefab.lock()->getName());
	_text->setFillColor(color);
	_text->setStyle(sf::Text::Bold);
	_text->setPosition(sf::Vector2f(x + paddingLeft, y - 10));

	auto bounds = _text->getLocalBounds();

	float gapStart = paddingLeft - textPadding;
	float gapEnd = paddingLeft + bounds.size.x + textPadding;

	sf::RectangleShape topLeft(sf::Vector2f(gapStart, thickness));
	topLeft.setPosition(sf::Vector2f(x, y));
	topLeft.setFillColor(color);
	Anubis::render_window->draw(topLeft);

	if (gapEnd < w) {
		sf::RectangleShape topRight(sf::Vector2f(w - gapEnd, thickness));
		topRight.setPosition(sf::Vector2f(x + gapEnd, y));
		topRight.setFillColor(color);
		Anubis::render_window->draw(topRight);
	}

	sf::RectangleShape left(sf::Vector2f(thickness, h));
	left.setPosition(sf::Vector2f(x, y));
	left.setFillColor(color);
	Anubis::render_window->draw(left);

	sf::RectangleShape right(sf::Vector2f(thickness, h));
	right.setPosition(sf::Vector2f(x + w - thickness, y));
	right.setFillColor(color);
	Anubis::render_window->draw(right);

	sf::RectangleShape bottom(sf::Vector2f(w, thickness));
	bottom.setPosition(sf::Vector2f(x, y + h - thickness));
	bottom.setFillColor(color);
	Anubis::render_window->draw(bottom);

	Anubis::render_window->draw(*_text);
	
	
	
	
	
}

sf::Vector2f PlacedGameObject::getPosition() {
	return _position;
}

void PlacedGameObject::setPosition(sf::Vector2f position) {
	_position = position;
}

// TO-DO - to remove
void PlacedGameObject::cursorHover() {

}

void PlacedGameObject::update() {

	if (_model)
		_model->update();
}

// TO-DO - to remove
void PlacedGameObject::draw() {

	
}
