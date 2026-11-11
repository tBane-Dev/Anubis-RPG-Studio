#include "Objects/Monster.hpp"
#include "Time.hpp"
#include "RenderWindow.hpp"
#include <iostream>
#include "Editors/MapEditor/Editor.hpp"
#include "DebugLog.hpp"
#include "ShadersManager.hpp"


MonsterPrefab::MonsterPrefab(std::wstring name, std::weak_ptr<Animations> animations, sf::Vector2i origin, int stepSize, std::shared_ptr<Collider> collider, std::shared_ptr<Mesh> mesh, std::weak_ptr<Anubis::Model3D> model) : GameObject(name, animations, origin, collider, mesh, model) {
	_type = ObjectType::Monster;
	_stepSize = stepSize;

	
}

MonsterPrefab::~MonsterPrefab() {

}

Monster::Monster(std::weak_ptr<GameObject> prefab) : PlacedGameObject(prefab) {

	_type = ObjectType::Monster;

	_state = MonsterState::Moving;

	_basePosition = sf::Vector2f(0.0f, 0.0f);
	_direction = Direction::Down;

	_animator->setRandFrame();

	_path = std::make_shared<MapEditor::Path>();
	_path->setStartPoint(sf::Vector2i(_position));
	_path->setEndPoint(sf::Vector2i(_position));
	_path->generatePath();
}

Monster::~Monster() {

}

sf::Vector2f Monster::getPosition() {
	return _position;
}

void Monster::setPosition(sf::Vector2f position) {
	PlacedGameObject::setPosition(position);

	_basePosition = position;

	_path->setStartPoint(sf::Vector2i(_position));
	_path->setEndPoint(sf::Vector2i(_position));
	_path->generatePath();
}

void Monster::update() {
	
	// if is selected then no update
	bool isSelected = false;
	for (auto& selected : MapEditor::editor->_cursor_on_map->_selectedObjects)
		if (selected->_object.get() == this) isSelected = true;

	if (MapEditor::editor->_cursor_on_map->_isDragging && isSelected)
		return;

	// update 
	_model->update();

	if (_state == MonsterState::Moving) {

		float interval = 0.125f;
		if (!_animator->_animations.expired())
			interval = _animator->_animations.lock()->_interval;

		_animator->_timer += Anubis::deltaTime.asSeconds();
		if (_animator->_timer >= interval)
			_animator->_timer = 0.0f;


		float moveSpeed = 32.0f;

		if (!_model->_animations.empty()) {
			float stepLength = (float)std::dynamic_pointer_cast<MonsterPrefab>(_prefab.lock())->_stepSize / 4.0f;
			float moveStep = 32.f / 2.0f / stepLength;
			moveSpeed = moveStep / interval * 0.5f;
			float duration = _model->_animations[0]._duration;

			_model->_player._index = 0;
			_model->_player._loop = true;
			_model->_player._speed = duration / (4.0f * interval) * 0.5f;
		}

		if (_path->isEmpty() || _position == sf::Vector2f(_path->getEndPoint())) {
			_path->setStartPoint(sf::Vector2i(_position));
			sf::Vector2i endPoint = sf::Vector2i(_basePosition);
			int r = 16;
			endPoint.x += (rand() % (2 * r + 1) - r) * MapEditor::Tile::tileSize;
			endPoint.y += (rand() % (2 * r + 1) - r) * MapEditor::Tile::tileSize;
			_path->setEndPoint(endPoint);
			_path->generatePath();
		}

		float distance = moveSpeed * Anubis::deltaTime.asSeconds();
		int guard = 0;

		while (distance > 0.0f && guard < 16 && !_path->isEmpty() && _position != sf::Vector2f(_path->getEndPoint())) {
			guard++;

			sf::Vector2f target = sf::Vector2f(_path->getStartPoint());
			sf::Vector2f diff = target - _position;
			float step = 0.0f;

			if (diff.y > 0.0f) {
				step = std::min(diff.y, distance);
				_position.y += step;
				_direction = Direction::Down;
			}
			else if (diff.y < 0.0f) {
				step = std::min(-diff.y, distance);
				_position.y -= step;
				_direction = Direction::Up;
			}
			else if (diff.x > 0.0f) {
				step = std::min(diff.x, distance);
				_position.x += step;
				_direction = Direction::Right;
			}
			else if (diff.x < 0.0f) {
				step = std::min(-diff.x, distance);
				_position.x -= step;
				_direction = Direction::Left;
			}

			distance -= step;

			if (_position == target)
				_path->nextPoint();
		}

		float angle = 0.0f;
		if (_direction == Direction::Down)  angle = 0.0f;
		if (_direction == Direction::Up)    angle = 180.0f;
		if (_direction == Direction::Left)  angle = -90.0f;
		if (_direction == Direction::Right) angle = 90.0f;
		float rad = angle * 3.14159265f / 180.0f;

		_model->_rotation = Anubis::Quaternion{
			std::cos(rad * 0.5f),
			0.0f,
			std::sin(rad * 0.5f),
			0.0f
		};

		_model->_position.x = _position.x;
		_model->_position.z = _position.y;
	}

	sf::IntRect colliderRect;
	if (_prefab.lock()->getCollider()->_type == ColliderType::Rectangular) {
		std::shared_ptr<RectangularCollider> c = std::dynamic_pointer_cast<RectangularCollider>(_prefab.lock()->getCollider());
		colliderRect = c->_rect;
		colliderRect.position += sf::Vector2i(_position);
	}
	else {
		std::shared_ptr<CircularCollider> c = std::dynamic_pointer_cast<CircularCollider>(_prefab.lock()->getCollider());
		colliderRect = sf::IntRect(sf::Vector2i(_position.x - c->_radiusX, _position.y - c->_radiusY), sf::Vector2i(2 * c->_radiusX, 2 * c->_radiusY));
	}
}

void Monster::draw() {

	// TO-DO - to rewrite

	//if (_prefab.expired())
	//	return;
	//
	//if (MapEditor::editor->_main_menu->_render_paths->_checkbox->_value == 1) {
	//	_path->draw();
	//}
	//
	//bool renderAllColliders = MapEditor::editor->_main_menu->_render_colliders->_checkbox->_value == 1;
	//
	//std::shared_ptr<Collider> collider = _prefab.lock()->getCollider();
	//
	//if (collider != nullptr && (renderAllColliders || collider->cursorHover(MapEditor::editor->_cursor_on_map->_globalPosition + _prefab.lock()->getOrigin(), _position))) {
	//	_prefab.lock()->getCollider()->draw(_position);
	//}
	//
	//
	//std::shared_ptr<Animations> anim = _animator->getAnimations().lock();
	//
	//if (!anim) {
	//	drawFrame();
	//	return;
	//}
	//
	//sf::IntRect frameRect = anim->getFrameRect(_animator->_animation, _animator->_frame);
	//
	//if (_isSelected == true) {
	//	drawFrame(sf::Color(255, 30, 45));
	//}
	//else if (MapEditor::editor->_main_menu->_render_sprites_outline->_checkbox->_value == 1) {
	//	drawFrame();
	//}
	//
	//sf::Sprite sprite(*anim->getTexture()->_texture);
	//sprite.setOrigin(sf::Vector2f(_prefab.lock()->getOrigin()));
	//sprite.setTextureRect(frameRect);
	//
	//sf::Vector2f origin(_prefab.lock()->getOrigin());
	//sf::Vector2f anchorPosition = sf::Vector2f(_position) + origin;
	//sprite25D_shader->setUniform("spritePosition", anchorPosition);
	//
	//sf::RenderStates states;
	//states.shader = &*sprite25D_shader;
	//
	//Anubis::render_window->draw(sprite, states);
	//
	//
	//if (MapEditor::editor->_main_menu->_render_meshes->_checkbox->_value == 1) {
	//	if (_prefab.lock()->getMesh()) {
	//		_prefab.lock()->getMesh()->draw(_position - _prefab.lock()->getOrigin(), sf::Color::Red);
	//	}
	//}
}