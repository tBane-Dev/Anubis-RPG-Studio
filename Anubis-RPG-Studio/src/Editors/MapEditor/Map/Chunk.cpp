#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Theme.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "RenderWindow.hpp"
#include "ShadersManager.hpp"
#include "Time.hpp"
#include "Objects/Collider.hpp"
#include "Objects/Monster.hpp"
#include "DebugLog.hpp"
#include "3D/OpenGLBuffers.hpp"
#include <cstdint>

namespace MapEditor {

	Chunk::Chunk(int x, int y) {

		_coords.x = x;
		_coords.y = y;

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

		if (!tileset || !MapEditor::editor || !MapEditor::editor->_map)
			return;

		auto texture = MapEditor::editor->_map->_texture;

		if (!texture || !texture->_texture)
			return;

		sf::Vector2u textureSize = texture->_texture->getSize();

		if (textureSize.x == 0 || textureSize.y == 0)
			return;

		if (_tiles.size() != tilesCols * tilesRows) {
			DebugError(L"Cannot generate an incomplete chunk");
			return;
		}

		std::shared_ptr<Anubis::Mesh3D> mesh = std::make_shared<Anubis::Mesh3D>();
		mesh->setTexture(texture);

		std::vector<Anubis::Vertex3D> vertices;
		std::vector<std::uint32_t> indices;
		vertices.reserve(tilesCols * tilesRows * 4);
		indices.reserve(tilesCols * tilesRows * 6);

		std::shared_ptr<Chunk> chunk = shared_from_this();

		for (int y = 0; y < tilesRows; y++) {
			for (int x = 0; x < tilesCols; x++) {

				int tx = _coords.x * tilesCols + x;
				int ty = _coords.y * tilesRows + y;

				float xx = float(tx * Tile::tileSize);
				float zz = float(ty * Tile::tileSize);

				std::shared_ptr<Tile> t = getTileByTileGlobalCoords(tx, ty);

				if (!t || t->_type < 0 || t->_type >= int(tileset->groups.size()) || !tileset->groups[t->_type]) {
					DebugError(L"Invalid tile in chunk");
					return;
				}

				int tileType = getTileType(
					t->_type, x, y, chunk,
					leftTopChunk, topChunk, rightTopChunk,
					leftChunk, rightChunk,
					leftBottomChunk, bottomChunk, rightBottomChunk
				);

				Tileset::Tile tile = tileset->groups[t->_type]->tiles[tileType];

				// Positive remainder also keeps the pattern correct in negative chunks.
				int u = tile.x * Tile::tileSize + (tx * Tile::tileSize % 64 + 64) % 64;
				int v = tile.y * Tile::tileSize + (ty * Tile::tileSize % 64 + 64) % 64;

				float left = float(u + 1) / float(textureSize.x);
				float top = float(v + 1) / float(textureSize.y);
				float right = float(u + Tile::tileSize - 1) / float(textureSize.x);
				float bottom = float(v + Tile::tileSize - 1) / float(textureSize.y);

				std::uint32_t index = (std::uint32_t)vertices.size();

				vertices.emplace_back(xx,                  0.f, zz,                  0.f, 1.f, 0.f, left,  top);
				vertices.emplace_back(xx,                  0.f, zz + Tile::tileSize, 0.f, 1.f, 0.f, left,  bottom);
				vertices.emplace_back(xx + Tile::tileSize, 0.f, zz + Tile::tileSize, 0.f, 1.f, 0.f, right, bottom);
				vertices.emplace_back(xx + Tile::tileSize, 0.f, zz,                  0.f, 1.f, 0.f, right, top);

				indices.push_back(index);
				indices.push_back(index + 1);
				indices.push_back(index + 2);

				indices.push_back(index);
				indices.push_back(index + 2);
				indices.push_back(index + 3);
			}
		}

		mesh->uploadToGPU(vertices, indices);

		if (!mesh->_geometry || mesh->_geometry->_indexCount == 0)
			return;

		_mesh = std::move(mesh);
	}

	sf::IntRect Chunk::getRect() {

		sf::IntRect rect;
		rect.position.x = _coords.x * tilesCols * Tile::tileSize;
		rect.position.y = _coords.y * tilesRows * Tile::tileSize;
		rect.size.x = tilesCols * Tile::tileSize;
		rect.size.y = tilesRows * Tile::tileSize;

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

		_coordsText->setPosition(sf::Vector2f(0.f, 0.f));
		Anubis::shaders_manager->ghost_chunk_shader->setUniform("position", position);

		sf::RenderStates states;
		states.shader = &*Anubis::shaders_manager->ghost_chunk_shader;

		Anubis::render_window->draw(*_coordsText, states);
	}

	void Chunk::draw() {

		if (!_mesh || !_mesh->_geometry || !Anubis::GL::bindVertexArray)
			return;

		const auto& geometry = *_mesh->_geometry;

		if (geometry._vertexArray == 0 || geometry._indexCount == 0)
			return;

		// Map::draw() binds the terrain shader and tileset once for all chunks.
		Anubis::GL::bindVertexArray(geometry._vertexArray);
		glDrawElements(GL_TRIANGLES, geometry._indexCount, GL_UNSIGNED_INT, nullptr);
	}

}
