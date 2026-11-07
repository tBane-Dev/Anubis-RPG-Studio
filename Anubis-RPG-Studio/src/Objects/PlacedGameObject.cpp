#include "Objects/PlacedGameObject.hpp"
#include "RenderWindow.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "DebugLog.hpp"
#include "Animator.hpp"
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

	_animator = std::make_shared<Animator>((!_prefab.expired())?prefab.lock()->getAnimations() : std::weak_ptr<Animations>());

	_animator->setRandFrame();
	_animator->setRandTime();
	if(true) {
		_animator->play();
	}

	_position = sf::Vector2f(0.0f, 0.0f);

	_isSelected = false;

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

	sf::Vector2f objectPosition = _position;

	if (prefab->_type == ObjectType::Monster) {
		objectPosition.x -= prefab->getOrigin().x;
		objectPosition.y -= prefab->getOrigin().y;
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
			if (mesh && mesh->isPointInside(MapEditor::editor->_cursor_on_map->_globalPosition, sf::Vector2i(_position))) {
				MapEditor::editor->_game_objects->_hoveredPlacedGameObject = shared_from_this();
			}
		}
			
	}
}

void PlacedGameObject::update() {
	_animator->update();

	if (_model)
		_model->update();
}

void PlacedGameObject::draw() {

	if (_model != nullptr)
		return;

	std::shared_ptr<GameObject> prefab = _prefab.lock();

	if (!prefab || !_animator)
		return;

	std::shared_ptr<Animations> animations = _animator->getAnimations().lock();

	if (!animations)
		return;

	auto texture = animations->getTexture();

	if (!texture || !texture->_texture)
		return;

	std::shared_ptr<Collider> collider = prefab->getCollider();

	auto& gameObjects = *MapEditor::editor->_game_objects;

	sf::IntRect frame = animations->getFrameRect(
		_animator->_animation,
		_animator->_frame
	);

	sf::Color color = sf::Color::White;

	if (gameObjects._hoveredPlacedGameObject.lock().get() == this)
		color = sf::Color(255, 30 + 64, 45 + 64);
	else if (_isSelected == true)
		color = sf::Color(255, 30 + 64, 45 + 64);

	sf::Vector2f origin(prefab->getOrigin());

	if (collider && collider->_type == ColliderType::Rectangular)
		origin += sf::Vector2f(collider->getRect().size / 2);

	sf::Vector2f position = sf::Vector2f(_position) + origin;

	float width = float(std::abs(frame.size.x));
	float height = float(std::abs(frame.size.y));

	float left = float(frame.position.x);
	float top = float(frame.position.y);
	float right = float(frame.position.x + frame.size.x);
	float bottom = float(frame.position.y + frame.size.y);

	const sf::Texture* currentTexture = texture->_texture.get();

	if (gameObjects._boundTexture != currentTexture) {
		sf::Texture::bind(currentTexture, sf::CoordinateType::Pixels);
		gameObjects._boundTexture = currentTexture;
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glTranslatef(position.x, 0.f, position.y);
	glMultMatrixf(gameObjects._billboard.data());

	glScalef(1.f, -1.f, 1.f);

	glTranslatef(-origin.x, -origin.y, 0.f);

	glColor4ub(color.r, color.g, color.b, color.a);

	glBegin(GL_TRIANGLES);

	glTexCoord2f(left, top);
	glVertex2f(0.f, 0.f);

	glTexCoord2f(left, bottom);
	glVertex2f(0.f, height);

	glTexCoord2f(right, bottom);
	glVertex2f(width, height);

	glTexCoord2f(left, top);
	glVertex2f(0.f, 0.f);

	glTexCoord2f(right, bottom);
	glVertex2f(width, height);

	glTexCoord2f(right, top);
	glVertex2f(width, 0.f);

	glEnd();

	glPopMatrix();
}
