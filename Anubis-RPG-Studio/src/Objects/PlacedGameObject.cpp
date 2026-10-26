#include "Objects/PlacedGameObject.hpp"
#include "RenderWindow.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "DebugLog.hpp"
#include "Animator.hpp"
#include "Theme.hpp"
#include "ShadersManager.hpp"

PlacedGameObject::PlacedGameObject(std::weak_ptr<GameObject> prefab, bool play) : Object() {
	_prefab = prefab;

	if(_prefab.expired()) {
		_type = ObjectType::None;
	}
	else {
		_type = _prefab.lock()->_type;
	}

	_animator = std::make_shared<Animator>((!_prefab.expired())?prefab.lock()->getAnimations() : std::weak_ptr<Animations>());

	_animator->setRandFrame();
	_animator->setRandTime();
	if(true) {
		_animator->play();
	}
	_position = sf::Vector2i(0, 0);

	_isSelected = false;

	if (_animator && !_animator->getAnimations().expired())
		_sprite = std::make_shared<sf::Sprite>(*_animator->getAnimations().lock()->getTexture()->_texture);

	_text = std::make_unique<sf::Text>(basicFont, (_prefab.expired()) ? L"Unknown" : _prefab.lock()->getName(), 14);
}

PlacedGameObject::~PlacedGameObject() {

}

void PlacedGameObject::drawFrame(sf::Color color) {

	auto prefab = _prefab.lock();
	if (!prefab)
		return;

	int x, y, w, h;

	float thickness = 2.f;
	float paddingLeft = 14.f;
	float textPadding = 6.f;

	sf::Vector2i objectPosition = _position;

	if (prefab->_type == ObjectType::Monster) {
		objectPosition.x -= prefab->getOrigin().x;
		objectPosition.y -= prefab->getOrigin().y;
	}

	if (prefab->_type == ObjectType::Door) {
		objectPosition.y -= 48;
	}

	auto animations = prefab->getAnimations().lock();

	if (animations) {
		auto frameRect = animations->getFrameRect(0, 0);

		x = objectPosition.x;
		y = objectPosition.y;
		w = frameRect.size.x;
		h = frameRect.size.y;
	}
	else {
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

			if (_type == ObjectType::Building) {
				y -= 32;
				h += 32;
			}
		}
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
	Main::render_window->draw(topLeft);

	if (gapEnd < w) {
		sf::RectangleShape topRight(sf::Vector2f(w - gapEnd, thickness));
		topRight.setPosition(sf::Vector2f(x + gapEnd, y));
		topRight.setFillColor(color);
		Main::render_window->draw(topRight);
	}

	sf::RectangleShape left(sf::Vector2f(thickness, h));
	left.setPosition(sf::Vector2f(x, y));
	left.setFillColor(color);
	Main::render_window->draw(left);

	sf::RectangleShape right(sf::Vector2f(thickness, h));
	right.setPosition(sf::Vector2f(x + w - thickness, y));
	right.setFillColor(color);
	Main::render_window->draw(right);

	sf::RectangleShape bottom(sf::Vector2f(w, thickness));
	bottom.setPosition(sf::Vector2f(x, y + h - thickness));
	bottom.setFillColor(color);
	Main::render_window->draw(bottom);

	Main::render_window->draw(*_text);
	
	
	
	
	
}

sf::Vector2i PlacedGameObject::getPosition() {
	return _position;
}

void PlacedGameObject::setPosition(sf::Vector2i position) {
	_position = position;
}

sf::Vector2i PlacedGameObject::getSortPosition() {

	sf::Vector2i position;
	if (_prefab.lock()->getCollider()->_type == ColliderType::Rectangular) {
		std::shared_ptr<RectangularCollider> collider = std::dynamic_pointer_cast<RectangularCollider>(_prefab.lock()->getCollider());
		position = _position + collider->_rect.position + collider->_rect.size / 2;
	}
	else {
		position = _position + _prefab.lock()->getOrigin();
	}

	return position;
}

void PlacedGameObject::cursorHover() {

	if (!_animator)
		return;

	if (_animator->_animations.expired())
		return;

	sf::IntRect rect = _animator->_animations.lock()->getFrameRect(0,0);
	rect.position.x += _position.x;
	rect.position.y += _position.y;

	if (rect.contains(MapEditor::editor->_cursor_on_map->_globalPosition)) {
		if (!_prefab.expired()) {
			std::shared_ptr<Mesh> mesh = _prefab.lock()->getMesh();
			if (mesh && mesh->isPointInside(MapEditor::editor->_cursor_on_map->_globalPosition, _position)) {
				MapEditor::editor->_game_objects->_hoveredPlacedGameObject = shared_from_this();
			}
		}
			
	}
}

void PlacedGameObject::update() {
	_animator->update();
}

void PlacedGameObject::draw() {

	if (_prefab.expired()) return;
	
	bool renderAllColliders = MapEditor::editor->_main_menu->_render_colliders->_checkbox->_value == 1;

	std::shared_ptr<Collider> collider = _prefab.lock()->getCollider();

	if (renderAllColliders) {
		if (collider->_type == ColliderType::Rectangular) {
			collider->draw(_position);
		}
		else if (collider->_type == ColliderType::Circular) {
			collider->draw(_position + _prefab.lock()->getOrigin());
		}
	}

	if(_isSelected == true) {
		drawFrame(sf::Color(255, 30, 45));
	}
	else if (MapEditor::editor->_main_menu->_render_sprites_outline->_checkbox->_value == 1) {
		drawFrame();
	}

	if(_animator->_animations.expired())
		return;

	std::shared_ptr<Animations> animations = _animator->getAnimations().lock();

	if (animations) {

		if (_sprite) {
			_sprite->setTextureRect(animations->getFrameRect(_animator->_animation, _animator->_frame));

			if (MapEditor::editor->_game_objects->_hoveredPlacedGameObject.lock().get() == this)
				_sprite->setColor(sf::Color(255, 30 + 64, 45 + 64)); // TO-DO - must be a shader highlight
			else if (_isSelected == true)
				_sprite->setColor(sf::Color(255, 30 + 64, 45 + 64));
			else
				_sprite->setColor(sf::Color::White);

			sf::Vector2f origin(_prefab.lock()->getOrigin());
			if (collider->_type == ColliderType::Rectangular)
				origin += sf::Vector2f(collider->getRect().size / 2);
			_sprite->setOrigin(origin);

			sprite25D_shader->setUniform("spritePosition", sf::Vector2f(_position) + origin);
			
			sf::RenderStates states;
			states.shader = &*sprite25D_shader;

			Main::render_window->draw(*_sprite, states);
		}

		
	}

	if(MapEditor::editor->_main_menu->_render_meshes->_checkbox->_value == 1) {
		if (_prefab.lock()->getMesh()) {
			_prefab.lock()->getMesh()->draw(_position, sf::Color::Red);
		}
	}

}
