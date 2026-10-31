#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Theme.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "RenderWindow.hpp"
#include "ShadersManager.hpp"
#include "Time.hpp"
#include "Objects/Collider.hpp"
#include "Objects/Monster.hpp"
#include "DebugLog.hpp"

namespace MapEditor {

	Chunk::Chunk(int x, int y) {

		_coords.x = x;
		_coords.y = y;

		_vertexBuffer = sf::VertexBuffer(sf::PrimitiveType::Triangles);

		_isVisible = false;
		_gameObjectsOnMap.clear();
		_gameObjectsOnMapRect = getRect();

		std::wstring text = std::to_wstring(_coords.x) + L"x" + std::to_wstring(_coords.y);
		
		_coordsText = std::make_unique<sf::Text>(basicFont, text, 24);
		_coordsText->setFillColor(basic_text_color);

		
	}

	Chunk::~Chunk() {

	}


	bool Chunk::operator<(const Chunk& other) const {
		if (_coords.x == other._coords.x)
			return _coords.y < other._coords.y;
		return _coords.x < other._coords.x;
	}

	std::shared_ptr<Tile> Chunk::getTileByTileGlobalCoords(int x, int y) {

		int xx = x - _coords.x * tilesCols;
		int yy = y - _coords.y * tilesRows;

		if (xx < 0 || yy < 0 || xx >= tilesCols || yy >= tilesRows)
			return nullptr;

		return _tiles[yy * Chunk::tilesCols + xx];
	}

	std::shared_ptr<Tile> Chunk::getTileByGlobalPosition() {

		sf::IntRect rect;
		rect.position.x = _coords.x * tilesCols * Tile::tileSize;
		rect.position.y = _coords.y * tilesRows * Tile::tileSize;
		rect.size.x = tilesCols * Tile::tileSize;
		rect.size.y = tilesRows * Tile::tileSize;

		if (rect.contains(MapEditor::editor->_cursor_on_map->_position)) {
			int x = MapEditor::editor->_cursor_on_map->_position.x / Tile::tileSize;
			int y = MapEditor::editor->_cursor_on_map->_position.y / Tile::tileSize;
			return getTileByTileGlobalCoords(x, y);
		}

		return nullptr;
	}

	void Chunk::generateVertexBuffer(
		std::shared_ptr<Chunk> leftTopChunk,
		std::shared_ptr<Chunk> topChunk,
		std::shared_ptr<Chunk> rightTopChunk,
		std::shared_ptr<Chunk> leftChunk,
		std::shared_ptr<Chunk> rightChunk,
		std::shared_ptr<Chunk> leftBottomChunk,
		std::shared_ptr<Chunk> bottomChunk,
		std::shared_ptr<Chunk> rightBottomChunk
	) {

		_vertexBuffer.create(16 * 16 * 6);
		_vertexBuffer.setPrimitiveType(sf::PrimitiveType::Triangles);

		if (tileset == nullptr) return;

		std::vector<sf::Vertex> vertices;

		for (int y = 0; y < tilesRows; y++) {
			for (int x = 0; x < tilesCols; x++) {

				int tx = _coords.x * tilesCols + x;
				int ty = _coords.y * tilesRows + y;

				float xx = float(tx * Tile::tileSize);
				float yy = float(ty * Tile::tileSize);

				sf::Color color = sf::Color::White;

				int tileType = 5;
				int type = getTileByTileGlobalCoords(tx, ty)->_type;

				tileType = getTileType(
					type, x, y, shared_from_this(),
					leftTopChunk, topChunk, rightTopChunk,
					leftChunk, rightChunk,
					leftBottomChunk, bottomChunk, rightBottomChunk
				);

				std::shared_ptr<Tile> t = getTileByTileGlobalCoords(tx, ty);
				Tileset::Tile tile = tileset->groups[t->_type]->tiles[tileType];

				int u = (tile.x) * Tile::tileSize + tx * Tile::tileSize % 64;
				int v = (tile.y) * Tile::tileSize + ty * Tile::tileSize % 64;

				sf::Vertex v1 = sf::Vertex({ xx, yy }, color, sf::Vector2f((float)(u + 1), (float)(v + 1)));
				sf::Vertex v2 = sf::Vertex({ xx, yy + Tile::tileSize }, color, sf::Vector2f((float)(u + 1), (float)(v + Tile::tileSize - 1)));
				sf::Vertex v3 = sf::Vertex({ xx + Tile::tileSize, yy + Tile::tileSize }, color, sf::Vector2f((float)(u + Tile::tileSize - 1), (float)(v + Tile::tileSize - 1)));
				sf::Vertex v4 = sf::Vertex({ xx + Tile::tileSize, yy }, color, sf::Vector2f((float)(u + Tile::tileSize - 1), (float)(v + 1)));

				vertices.push_back(v1);
				vertices.push_back(v2);
				vertices.push_back(v3);

				vertices.push_back(v1);
				vertices.push_back(v3);
				vertices.push_back(v4);
			}
		}

		_vertexBuffer.update(vertices.data());
	}

	sf::IntRect Chunk::getRect() {

		sf::IntRect rect;

		// TO-DO - to delete
		if (MapEditor::editor->_camera) {
			sf::View view = MapEditor::editor->_camera->_view;
			sf::Vector2f cameraCenter = view.getCenter();
			sf::Vector2u windowSize = Anubis::render_window->getSize();

			float zoom = MapEditor::editor->_camera->_zoom;

			sf::Vector2f size(float(windowSize.x) / zoom, float(windowSize.y) / zoom);
			float aspect = float(windowSize.x) / windowSize.y;
			float fov = 60.f;
			float distance = (size.y * 0.5f) / std::tan(fov * 0.5f * 3.14159265f / 180.f);
			float angle = sf::degrees(MapEditor::editor->_camera->_angle).asRadians();

			float s = std::sin(angle);
			float c = std::cos(angle);
			float f = 1.7320508f;

			float width = float(tilesCols * Tile::tileSize);
			float height = float(tilesRows * Tile::tileSize);

			float x = float(_coords.x) * width;
			float y = float(_coords.y) * height;

			const sf::Vector2f points[] = {
				{x,         y},
				{x + width, y},
				{x + width, y + height},
				{x,         y + height}
			};

			bool allBehind = true;

			for (const auto& point : points) {
				float py = point.y - cameraCenter.y;
				float depth = distance - py * s;

				if (depth > 0.1f) {
					allBehind = false;
					break;
				}
			}

			if (allBehind)
				return sf::IntRect();

			std::vector<sf::Vector2f> polygon(points, points + 4);

			auto clip = [&](auto plane) {

				if (polygon.empty())
					return;

				std::vector<sf::Vector2f> result;
				result.reserve(polygon.size() + 1);

				sf::Vector2f previous = polygon.back();
				float previousDistance = plane(previous);

				for (const auto& current : polygon) {
					float currentDistance = plane(current);

					bool previousInside = previousDistance >= 0.0f;
					bool currentInside = currentDistance >= 0.0f;

					if (previousInside != currentInside) {
						float t = previousDistance /
							(previousDistance - currentDistance);

						result.push_back(previous + (current - previous) * t);
					}

					if (currentInside)
						result.push_back(current);

					previous = current;
					previousDistance = currentDistance;
				}

				polygon = std::move(result);
				};

			auto getDepth = [&](sf::Vector2f point) {
				return distance - (point.y - cameraCenter.y) * s;
				};

			// Płaszczyzna bliska.
			clip([&](sf::Vector2f point) {
				return getDepth(point) - 0.1f;
				});

			// Lewa granica: clipX >= -depth.
			clip([&](sf::Vector2f point) {
				float clipX = (point.x - cameraCenter.x) * f / aspect;
				return getDepth(point) + clipX;
				});

			// Prawa granica: clipX <= depth.
			clip([&](sf::Vector2f point) {
				float clipX = (point.x - cameraCenter.x) * f / aspect;
				return getDepth(point) - clipX;
				});

			// Dolna granica: clipY >= -depth.
			clip([&](sf::Vector2f point) {
				float clipY = -(point.y - cameraCenter.y) * c * f;
				return getDepth(point) + clipY;
				});

			// Górna granica: clipY <= depth.
			clip([&](sf::Vector2f point) {
				float clipY = -(point.y - cameraCenter.y) * c * f;
				return getDepth(point) - clipY;
				});

			if (polygon.size() < 3)
				return sf::IntRect();

			float minX = std::numeric_limits<float>::max();
			float minY = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::lowest();
			float maxY = std::numeric_limits<float>::lowest();

			for (const auto& point : polygon) {
				float depth = getDepth(point);

				float ndcX = ((point.x - cameraCenter.x) * f / aspect) / depth;
				float ndcY = (-(point.y - cameraCenter.y) * c * f) / depth;

				float projectedX = cameraCenter.x + ndcX * size.x * 0.5f;
				float projectedY = cameraCenter.y - ndcY * size.y * 0.5f;

				minX = std::min(minX, projectedX);
				minY = std::min(minY, projectedY);
				maxX = std::max(maxX, projectedX);
				maxY = std::max(maxY, projectedY);
			}

			if (maxX <= minX || maxY <= minY)
				return sf::IntRect();

			int left = int(std::floor(minX));
			int top = int(std::floor(minY));
			int right = int(std::ceil(maxX));
			int bottom = int(std::ceil(maxY));

			return sf::IntRect(sf::Vector2i(left, top), sf::Vector2i(right - left, bottom - top));
		}

		return rect;

	}


	void Chunk::calculatePlacedGameObjectRect() {

		_gameObjectsOnMapRect = getRect();

		for (auto& object : _gameObjectsOnMap) {


			if (object->_type == ObjectType::Monster)
				continue;

			if (object->_prefab.expired())
				continue;

			std::shared_ptr<Collider> collider = object->_prefab.lock()->getCollider();
			int minX, minY, maxX, maxY;

			if (collider->_type == ColliderType::Rectangular) {
				std::shared_ptr<RectangularCollider> rectangularCollider = std::dynamic_pointer_cast<RectangularCollider>(collider);
				minX = object->_position.x + rectangularCollider->_rect.position.x;
				minY = object->_position.y + rectangularCollider->_rect.position.y;
				maxX = minX + rectangularCollider->_rect.size.x;
				maxY = minY + rectangularCollider->_rect.size.y;
			}

			if (collider->_type == ColliderType::Circular) {
				std::shared_ptr<CircularCollider> circularCollider = std::dynamic_pointer_cast<CircularCollider>(collider);
				minX = object->_position.x - circularCollider->_radiusX;
				minY = object->_position.y - circularCollider->_radiusY;
				maxX = object->_position.x + circularCollider->_radiusX;
				maxY = object->_position.y + circularCollider->_radiusY;
			}

			if (minX < _gameObjectsOnMapRect.position.x) { _gameObjectsOnMapRect.size.x += _gameObjectsOnMapRect.position.x - minX; _gameObjectsOnMapRect.position.x = minX; }
			if (minY < _gameObjectsOnMapRect.position.y) { _gameObjectsOnMapRect.size.y += _gameObjectsOnMapRect.position.y - minY; _gameObjectsOnMapRect.position.y = minY; }
			if (maxX > _gameObjectsOnMapRect.position.x + _gameObjectsOnMapRect.size.x) _gameObjectsOnMapRect.size.x = maxX - _gameObjectsOnMapRect.position.x;
			if (maxY > _gameObjectsOnMapRect.position.y + _gameObjectsOnMapRect.size.y) _gameObjectsOnMapRect.size.y = maxY - _gameObjectsOnMapRect.position.y;
		}
	}

	sf::IntRect Chunk::getPlacedGameObjectsRect() {
		return _gameObjectsOnMapRect;
	}

	void Chunk::addPlacedGameObject(std::shared_ptr<PlacedGameObject> object) {
		_gameObjectsOnMap.push_back(object);
		calculatePlacedGameObjectRect();
	}

	void Chunk::removePlacedGameObject(std::shared_ptr<PlacedGameObject> object) {
		std::erase_if(_gameObjectsOnMap, [&](const std::shared_ptr<PlacedGameObject>& obj) {

			if (object->_type == ObjectType::Building && (obj->_type == ObjectType::Wall)) {
				std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(object);
				std::shared_ptr<Wall> wall = std::dynamic_pointer_cast<Wall>(obj);
				return wall->_building.lock() == building;
			}

			if (object->_type == ObjectType::Building && (obj->_type == ObjectType::Skelet)) {
				std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(object);
				std::shared_ptr<Skelet> skelet = std::dynamic_pointer_cast<Skelet>(obj);
				return skelet->_building.lock() == building;
			}

			if (object->_type == ObjectType::Building && (obj->_type == ObjectType::Outside)) {
				std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(object);
				std::shared_ptr<Outside> outside = std::dynamic_pointer_cast<Outside>(obj);
				return outside->_building.lock() == building;
			}

			if (object->_type == ObjectType::Building && (obj->_type == ObjectType::Door)) {
				std::shared_ptr<Building> building = std::dynamic_pointer_cast<Building>(object);
				std::shared_ptr<Door> door = std::dynamic_pointer_cast<Door>(obj);
				return door->_building.lock() == building;
			}

			return obj == object;
			});

		calculatePlacedGameObjectRect();
	}

	void Chunk::setVisible() {

		if (_isVisible) 
			return;

		_isVisible = true;
		MapEditor::editor->_map->_visibleChunks.push_back(shared_from_this());

		for (auto& placedGameObject : _gameObjectsOnMap) {
			if (placedGameObject) {
				MapEditor::editor->_game_objects->addGameObject(placedGameObject);
			};
		}
	}

	void Chunk::drawCoords() {
		float padding = 8.f;

		sf::Vector2f position(_coords.x * tilesCols * Tile::tileSize + padding, _coords.y * tilesRows * Tile::tileSize + padding);
		position += sf::Vector2f(MapEditor::editor->_map->getRect().position);

		_coordsText->setPosition(sf::Vector2f(0.f, 0.f));
		ghost_chunk_shader->setUniform("position", position);

		sf::RenderStates states;
		states.shader = &*ghost_chunk_shader;

		Anubis::render_window->draw(*_coordsText, states);
	}

	void Chunk::draw() {
		
		if (!MapEditor::editor)
			return;

		if (!MapEditor::editor->_map)
			return;

		Anubis::render_window->draw(_vertexBuffer, MapEditor::editor->_map->_states);

	}

}