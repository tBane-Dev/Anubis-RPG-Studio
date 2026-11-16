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
#include <algorithm>
#include <cmath>

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

		fill(2, 0.f);
	}

	Chunk::~Chunk() {

	}


	bool Chunk::operator<(const Chunk& other) const {
		if (_coords.x == other._coords.x)
			return _coords.y < other._coords.y;
		return _coords.x < other._coords.x;
	}

	float Chunk::getSize() {
		return float(tilesCols * Tile::tileSize);
	}

	float Chunk::getHeightStep() {
		return getSize() / float(heightsCols - 1);
	}

	float Chunk::getWeightStep() {
		return getSize() / float(weightsCols - 1);
	}

	sf::Vector2f Chunk::getWorldPosition() {
		return sf::Vector2f(float(_coords.x) * getSize(), float(_coords.y) * getSize());
	}

	void Chunk::fill(int terrain, float height) {

		_heights.assign(heightsCols * heightsCols, height);
		_weights.assign(weightsCols * weightsCols * terrainsCount, 0);

		terrain = std::clamp(terrain, 0, terrainsCount - 1);

		for (int i = 0; i < weightsCols * weightsCols; i++)
			_weights[i * terrainsCount + terrain] = 255;
	}

	// weights in any scale - they are normalized to the sum 255
	void Chunk::setWeights(int point, std::array<float, 8> weights) {

		float sum = 0.f;
		int biggest = 0;

		for (int i = 0; i < terrainsCount; i++) {
			weights[i] = std::max(weights[i], 0.f);
			sum += weights[i];

			if (weights[i] > weights[biggest])
				biggest = i;
		}

		if (sum <= 0.f) {
			weights.fill(0.f);
			weights[0] = 1.f;
			sum = 1.f;
		}

		int total = 0;

		for (int i = 0; i < terrainsCount; i++) {
			int value = (int)(weights[i] / sum * 255.f + 0.5f);
			_weights[point * terrainsCount + i] = (std::uint8_t)value;
			total += value;
		}

		// the rounding rest goes to the biggest terrain
		_weights[point * terrainsCount + biggest] = (std::uint8_t)(int(_weights[point * terrainsCount + biggest]) + 255 - total);
	}

	// height of a point of the grid, points outside this chunk are taken from the neighbours
	float Chunk::getHeightPoint(int x, int y) {

		if (x >= 0 && y >= 0 && x < heightsCols && y < heightsCols)
			return _heights[y * heightsCols + x];

		sf::Vector2f position = getWorldPosition();
		float step = getHeightStep();

		return MapEditor::editor->_map->getHeight(position.x + float(x) * step, position.y + float(y) * step);
	}

	// height on the surface, the same triangles as in generateVertexBuffer()
	float Chunk::getHeight(float x, float z) {

		sf::Vector2f position = getWorldPosition();
		float step = getHeightStep();

		float fx = std::clamp((x - position.x) / step, 0.f, float(heightsCols - 1));
		float fz = std::clamp((z - position.y) / step, 0.f, float(heightsCols - 1));

		int x0 = std::min((int)fx, heightsCols - 2);
		int z0 = std::min((int)fz, heightsCols - 2);

		float tx = fx - float(x0);
		float tz = fz - float(z0);

		float leftTop = _heights[z0 * heightsCols + x0];
		float rightTop = _heights[z0 * heightsCols + x0 + 1];
		float leftBottom = _heights[(z0 + 1) * heightsCols + x0];
		float rightBottom = _heights[(z0 + 1) * heightsCols + x0 + 1];

		if (tz > tx)
			return leftTop + (rightBottom - leftBottom) * tx + (leftBottom - leftTop) * tz;

		return leftTop + (rightTop - leftTop) * tx + (rightBottom - rightTop) * tz;
	}

	void Chunk::updateSplatTextures() {

		if (_weights.size() != weightsCols * weightsCols * terrainsCount)
			return;

		std::vector<std::vector<std::uint8_t>> pixels(2, std::vector<std::uint8_t>(weightsCols * weightsCols * 4, 0));

		for (int i = 0; i < weightsCols * weightsCols; i++) {
			for (int t = 0; t < terrainsCount; t++)
				pixels[t / 4][i * 4 + t % 4] = _weights[i * terrainsCount + t];
		}

		if (_splatTextures.size() != 2) {
			_splatTextures.clear();

			for (int i = 0; i < 2; i++) {
				std::shared_ptr<sf::Texture> splatTexture = std::make_shared<sf::Texture>();

				if (!splatTexture->resize(sf::Vector2u((unsigned int)weightsCols, (unsigned int)weightsCols))) {
					DebugError(L"Cannot create a splat texture");
					_splatTextures.clear();
					return;
				}

				splatTexture->setSmooth(true);	// linear filtering = smooth weights between the points
				_splatTextures.push_back(splatTexture);
			}
		}

		for (int i = 0; i < 2; i++)
			_splatTextures[i]->update(pixels[i].data());
	}

	void Chunk::generateVertexBuffer() {

		if (!MapEditor::editor || !MapEditor::editor->_map)
			return;

		if (_heights.size() != heightsCols * heightsCols) {
			DebugError(L"Cannot generate a chunk without heights");
			return;
		}

		std::shared_ptr<Anubis::Mesh3D> mesh = std::make_shared<Anubis::Mesh3D>();

		std::vector<Anubis::Vertex3D> vertices;
		std::vector<std::uint32_t> indices;
		vertices.reserve(heightsCols * heightsCols);
		indices.reserve((heightsCols - 1) * (heightsCols - 1) * 6);

		sf::Vector2f position = getWorldPosition();
		float step = getHeightStep();
		float weightStep = getWeightStep();

		for (int y = 0; y < heightsCols; y++) {
			for (int x = 0; x < heightsCols; x++) {

				float xx = position.x + float(x) * step;
				float zz = position.y + float(y) * step;
				float yy = _heights[y * heightsCols + x];

				// normal from the neighbour heights (also from the neighbour chunks)
				float nx = getHeightPoint(x - 1, y) - getHeightPoint(x + 1, y);
				float nz = getHeightPoint(x, y - 1) - getHeightPoint(x, y + 1);
				float ny = 2.f * step;
				float length = std::sqrt(nx * nx + ny * ny + nz * nz);

				// texcoord = position in the splat texture (texel centers are the weight points)
				float u = (float(x) * step / weightStep + 0.5f) / float(weightsCols);
				float v = (float(y) * step / weightStep + 0.5f) / float(weightsCols);

				vertices.emplace_back(xx, yy, zz, nx / length, ny / length, nz / length, u, v);
			}
		}

		for (int y = 0; y < heightsCols - 1; y++) {
			for (int x = 0; x < heightsCols - 1; x++) {

				std::uint32_t index = (std::uint32_t)(y * heightsCols + x);
				std::uint32_t below = index + (std::uint32_t)heightsCols;

				indices.push_back(index);
				indices.push_back(below);
				indices.push_back(below + 1);

				indices.push_back(index);
				indices.push_back(below + 1);
				indices.push_back(index + 1);
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

		// Map::draw() binds the terrain shader and the terrain atlas once for all chunks.
		// Splat textures are different for every chunk: units 2 and 3.
		if (_splatTextures.size() == 2) {
			Anubis::GL::activeTexture(0x84C2); // GL_TEXTURE2
			glBindTexture(GL_TEXTURE_2D, _splatTextures[0]->getNativeHandle());
			Anubis::GL::activeTexture(0x84C3); // GL_TEXTURE3
			glBindTexture(GL_TEXTURE_2D, _splatTextures[1]->getNativeHandle());
			Anubis::GL::activeTexture(0x84C0); // GL_TEXTURE0
		}

		Anubis::GL::bindVertexArray(geometry._vertexArray);
		glDrawElements(GL_TRIANGLES, geometry._indexCount, GL_UNSIGNED_INT, nullptr);
	}

}
