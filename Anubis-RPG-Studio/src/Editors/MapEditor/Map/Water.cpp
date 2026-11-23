#include "Editors/MapEditor/Map/Water.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "DebugLog.hpp"
#include "ShadersManager.hpp"
#include <SFML/OpenGL.hpp>
#include "3D/OpenGLBuffers.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace MapEditor {

	int floorDiv(int a, int b) {
		int d = a / b;
		if ((a % b != 0) && ((a < 0) != (b < 0))) d -= 1;
		return d;
	}

	// WATER CHUNK //////////////////////////////////////////////////////////////////////////////////////////////

	WaterChunk::WaterChunk(sf::Vector2i coords) {
		_coords = coords;
		_fill.assign(Chunk::weightsCols * Chunk::weightsCols, 0);
		_added.assign(Chunk::weightsCols * Chunk::weightsCols, 0);
		_removed.assign(Chunk::weightsCols * Chunk::weightsCols, 0);
		_shore.assign(Chunk::weightsCols * Chunk::weightsCols, 0);
		_maskTexture = nullptr;
		_mesh = nullptr;
	}

	WaterChunk::~WaterChunk() {

	}

	bool WaterChunk::isEmpty() {
		for (int i = 0; i < (int)_fill.size(); i++)
			if (getMask(i) > 0) return false;

		return true;
	}

	// water = (fill or brush) without the eraser
	std::uint8_t WaterChunk::getMask(int point) {
		int value = std::max(_fill[point], _added[point]);
		return (std::uint8_t)(value * (255 - _removed[point]) / 255);
	}

	// WATER ////////////////////////////////////////////////////////////////////////////////////////////////////

	Water::Water(int kind, float level) : PlacedGameObject(std::weak_ptr<GameObject>(), false) {
		_type = ObjectType::Water;
		_kind = kind;
		_level = level;
		_text->setString(L"Water");
	}

	Water::~Water() {

	}

	sf::Vector2f Water::getPosition() {
		if (!_seeds.empty())
			return _seeds[0];

		if (!_chunks.empty()) {
			float size = float(Chunk::tilesCols * Tile::tileSize);
			return sf::Vector2f((float(_chunks[0]->_coords.x) + 0.5f) * size, (float(_chunks[0]->_coords.y) + 0.5f) * size);
		}

		return _position;
	}

	// water does not move on the map - only its level changes
	void Water::setPosition(sf::Vector2f position) {

	}

	std::shared_ptr<WaterChunk> Water::getWaterChunk(sf::Vector2i coords, bool create) {

		for (auto& waterChunk : _chunks) {
			if (waterChunk->_coords == coords)
				return waterChunk;
		}

		if (!create)
			return nullptr;

		if (!MapEditor::editor->_map->getChunkByCoords(coords.x, coords.y))
			return nullptr;

		std::shared_ptr<WaterChunk> waterChunk = std::make_shared<WaterChunk>(coords);
		_chunks.push_back(waterChunk);
		return waterChunk;
	}

	// 0 - 1, smooth between the mask points
	float Water::getMask(float x, float z) {

		float size = float(Chunk::tilesCols * Tile::tileSize);
		float step = size / float(Chunk::weightsCols - 1);

		sf::Vector2i coords((int)std::floor(x / size), (int)std::floor(z / size));
		std::shared_ptr<WaterChunk> waterChunk = getWaterChunk(coords, false);

		if (!waterChunk)
			return 0.f;

		float fx = std::clamp((x - float(coords.x) * size) / step, 0.f, float(Chunk::weightsCols - 1));
		float fz = std::clamp((z - float(coords.y) * size) / step, 0.f, float(Chunk::weightsCols - 1));

		int x0 = std::min((int)fx, Chunk::weightsCols - 2);
		int z0 = std::min((int)fz, Chunk::weightsCols - 2);
		float tx = fx - float(x0);
		float tz = fz - float(z0);

		int wc = Chunk::weightsCols;
		float a = float(waterChunk->getMask(z0 * wc + x0));
		float b = float(waterChunk->getMask(z0 * wc + x0 + 1));
		float c = float(waterChunk->getMask((z0 + 1) * wc + x0));
		float d = float(waterChunk->getMask((z0 + 1) * wc + x0 + 1));

		float top = a + (b - a) * tx;
		float bottom = c + (d - c) * tx;
		return (top + (bottom - top) * tz) / 255.f;
	}

	bool Water::isWaterAt(float x, float z) {
		return getMask(x, z) >= 0.5f && MapEditor::editor->_map->getHeight(x, z) < _level;
	}

	// point on the water surface under a ray (from the camera)
	bool Water::getRayHit(sf::Vector3f origin, sf::Vector3f dir, sf::Vector3f& hit) {

		if (std::abs(dir.y) < 0.00001f)
			return false;

		float t = (_level - origin.y) / dir.y;

		if (t <= 0.f)
			return false;

		hit = origin + dir * t;
		return isWaterAt(hit.x, hit.z);
	}

	// One mask point (global point coords, every 4 world units). Points on the edge of a chunk are written in all chunks that have them.
	// layer: 0 = fill, 1 = added (brush), 2 = removed (eraser)
	void Water::setMaskPoint(int gx, int gy, int layer, std::uint8_t value) {

		int cells = Chunk::weightsCols - 1;
		int wc = Chunk::weightsCols;

		int cx = floorDiv(gx, cells);
		int cy = floorDiv(gy, cells);

		for (int dy = 0; dy <= 1; dy++) {
			for (int dx = 0; dx <= 1; dx++) {

				int x = cx - dx;
				int y = cy - dy;
				int px = gx - x * cells;
				int py = gy - y * cells;

				if (px < 0 || py < 0 || px >= wc || py >= wc)
					continue;

				// the eraser does not create new parts of the water
				std::shared_ptr<WaterChunk> waterChunk = getWaterChunk(sf::Vector2i(x, y), layer != 2);

				if (!waterChunk)
					continue;

				int index = py * wc + px;

				if (layer == 0) waterChunk->_fill[index] = value;
				if (layer == 1) waterChunk->_added[index] = value;
				if (layer == 2) waterChunk->_removed[index] = value;
			}
		}
	}

	// Water spreads from the seeds to all points lower than the level. The first point above the level (the shore) is also marked,
	// so the water goes a bit under the terrain and the shore line comes from the depth test.
	void Water::fill() {

		for (auto& waterChunk : _chunks)
			std::fill(waterChunk->_fill.begin(), waterChunk->_fill.end(), 0);

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;
		float step = float(Chunk::tilesCols * Tile::tileSize) / float(Chunk::weightsCols - 1);
		int cells = Chunk::weightsCols - 1;

		std::unordered_set<long long> visited;
		std::vector<sf::Vector2i> stack;

		for (auto& seed : _seeds) {
			if (mapa->getHeight(seed.x, seed.y) < _level)
				stack.push_back(sf::Vector2i((int)std::round(seed.x / step), (int)std::round(seed.y / step)));
		}

		while (!stack.empty()) {

			sf::Vector2i p = stack.back();
			stack.pop_back();

			long long key = ((long long)p.x << 32) ^ (long long)(unsigned int)p.y;

			if (visited.count(key) > 0)
				continue;

			visited.insert(key);

			// point outside the map
			bool onMap = false;
			for (int dy = 0; dy <= 1 && !onMap; dy++)
				for (int dx = 0; dx <= 1 && !onMap; dx++)
					if (mapa->getChunkByCoords(floorDiv(p.x, cells) - dx, floorDiv(p.y, cells) - dy)) {
						int px = p.x - (floorDiv(p.x, cells) - dx) * cells;
						int py = p.y - (floorDiv(p.y, cells) - dy) * cells;
						onMap = px >= 0 && py >= 0 && px <= cells && py <= cells;
					}

			if (!onMap)
				continue;

			setMaskPoint(p.x, p.y, 0, 255);

			// shore - marked, but the water does not go further
			if (mapa->getHeight(float(p.x) * step, float(p.y) * step) >= _level)
				continue;

			stack.push_back(sf::Vector2i(p.x - 1, p.y));
			stack.push_back(sf::Vector2i(p.x + 1, p.y));
			stack.push_back(sf::Vector2i(p.x, p.y - 1));
			stack.push_back(sf::Vector2i(p.x, p.y + 1));
		}

		removeEmptyChunks();
		updateAllChunks();
	}

	// water brush / eraser. amount = how much in the middle of the brush (0 - 1)
	void Water::paint(sf::Vector2f center, float radius, bool erase, float amount) {

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;
		float size = float(Chunk::tilesCols * Tile::tileSize);
		float step = size / float(Chunk::weightsCols - 1);
		int cells = Chunk::weightsCols - 1;
		int wc = Chunk::weightsCols;

		int minX = (int)std::floor((center.x - radius) / step);
		int maxX = (int)std::ceil((center.x + radius) / step);
		int minY = (int)std::floor((center.y - radius) / step);
		int maxY = (int)std::ceil((center.y + radius) / step);

		std::vector<sf::Vector2i> changed;

		for (int gy = minY; gy <= maxY; gy++) {
			for (int gx = minX; gx <= maxX; gx++) {

				sf::Vector2f point(float(gx) * step, float(gy) * step);
				float a = std::clamp(mapa->getBrushFalloff(point - center, radius, false) * amount, 0.f, 1.f);

				if (a <= 0.f)
					continue;

				// current values of the point (from a chunk where it lies)
				sf::Vector2i coords(floorDiv(gx, cells), floorDiv(gy, cells));
				std::shared_ptr<WaterChunk> waterChunk = nullptr;

				for (int dy = 0; dy <= 1 && !waterChunk; dy++) {
					for (int dx = 0; dx <= 1 && !waterChunk; dx++) {
						sf::Vector2i c(floorDiv(gx, cells) - dx, floorDiv(gy, cells) - dy);

						if (gx - c.x * cells > cells || gy - c.y * cells > cells)
							continue;

						waterChunk = getWaterChunk(c, !erase);
						coords = c;
					}
				}

				if (!waterChunk)
					continue;

				int index = (gy - coords.y * cells) * wc + (gx - coords.x * cells);
				float added = float(waterChunk->_added[index]);
				float removed = float(waterChunk->_removed[index]);

				// every frame adds a part of the rest (a = only a few % per frame)
				if (!erase) {
					added = added + (255.f - added) * a;
					removed = removed * (1.f - a);
				}
				else {
					removed = removed + (255.f - removed) * a;
					added = added * (1.f - a);
				}

				setMaskPoint(gx, gy, 1, (std::uint8_t)added);
				setMaskPoint(gx, gy, 2, (std::uint8_t)removed);

				if (std::find(changed.begin(), changed.end(), coords) == changed.end())
					changed.push_back(coords);

				// points on the edge are also in the neighbour chunks
				for (int dy = 0; dy <= 1; dy++)
					for (int dx = 0; dx <= 1; dx++) {
						sf::Vector2i c(coords.x - dx, coords.y - dy);
						if (gx - c.x * cells <= cells && gy - c.y * cells <= cells && std::find(changed.begin(), changed.end(), c) == changed.end())
							changed.push_back(c);
					}
			}
		}

		updateChunksOf(changed);
	}

	void Water::removeEmptyChunks() {
		std::erase_if(_chunks, [](std::shared_ptr<WaterChunk>& waterChunk) { return waterChunk->isEmpty(); });
	}

	// Distance from every water point to the nearest land (terrain above the level or no water), counted over all chunks of the water.
	// Two passes over one big grid (chamfer distance): first from the top left, then from the bottom right.
	void Water::updateShoreDistances() {

		if (_chunks.empty())
			return;

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;
		int cells = Chunk::weightsCols - 1;
		int wc = Chunk::weightsCols;
		float step = float(Chunk::tilesCols * Tile::tileSize) / float(cells);
		float diagonal = step * 1.41421356f;
		float maxDistance = 510.f;

		// chunks of the water + one chunk around (land next to the water)
		sf::Vector2i minCoords = _chunks[0]->_coords;
		sf::Vector2i maxCoords = _chunks[0]->_coords;

		for (auto& waterChunk : _chunks) {
			minCoords.x = std::min(minCoords.x, waterChunk->_coords.x);
			minCoords.y = std::min(minCoords.y, waterChunk->_coords.y);
			maxCoords.x = std::max(maxCoords.x, waterChunk->_coords.x);
			maxCoords.y = std::max(maxCoords.y, waterChunk->_coords.y);
		}

		minCoords -= sf::Vector2i(1, 1);
		maxCoords += sf::Vector2i(1, 1);

		int cols = (maxCoords.x - minCoords.x + 1) * cells + 1;
		int rows = (maxCoords.y - minCoords.y + 1) * cells + 1;

		// 0 = land, maxDistance = water (outside the map too - no foam on the edge of the map)
		std::vector<float> distances(cols * rows, maxDistance);

		for (int cy = minCoords.y; cy <= maxCoords.y; cy++) {
			for (int cx = minCoords.x; cx <= maxCoords.x; cx++) {

				std::shared_ptr<Chunk> chunk = mapa->getChunkByCoords(cx, cy);

				if (!chunk)
					continue;

				std::shared_ptr<WaterChunk> waterChunk = getWaterChunk(sf::Vector2i(cx, cy), false);
				sf::Vector2f position = chunk->getWorldPosition();

				for (int y = 0; y < wc; y++) {
					for (int x = 0; x < wc; x++) {

						int gx = (cx - minCoords.x) * cells + x;
						int gy = (cy - minCoords.y) * cells + y;

						bool water = waterChunk && waterChunk->getMask(y * wc + x) >= 128 &&
							chunk->getHeight(position.x + float(x) * step, position.y + float(y) * step) < _level;

						if (!water)
							distances[gy * cols + gx] = 0.f;
					}
				}
			}
		}

		for (int y = 0; y < rows; y++) {
			for (int x = 0; x < cols; x++) {
				float d = distances[y * cols + x];

				if (x > 0) d = std::min(d, distances[y * cols + x - 1] + step);
				if (y > 0) d = std::min(d, distances[(y - 1) * cols + x] + step);
				if (x > 0 && y > 0) d = std::min(d, distances[(y - 1) * cols + x - 1] + diagonal);
				if (x < cols - 1 && y > 0) d = std::min(d, distances[(y - 1) * cols + x + 1] + diagonal);

				distances[y * cols + x] = d;
			}
		}

		for (int y = rows - 1; y >= 0; y--) {
			for (int x = cols - 1; x >= 0; x--) {
				float d = distances[y * cols + x];

				if (x < cols - 1) d = std::min(d, distances[y * cols + x + 1] + step);
				if (y < rows - 1) d = std::min(d, distances[(y + 1) * cols + x] + step);
				if (x < cols - 1 && y < rows - 1) d = std::min(d, distances[(y + 1) * cols + x + 1] + diagonal);
				if (x > 0 && y < rows - 1) d = std::min(d, distances[(y + 1) * cols + x - 1] + diagonal);

				distances[y * cols + x] = d;
			}
		}

		for (auto& waterChunk : _chunks) {
			for (int y = 0; y < wc; y++) {
				for (int x = 0; x < wc; x++) {
					int gx = (waterChunk->_coords.x - minCoords.x) * cells + x;
					int gy = (waterChunk->_coords.y - minCoords.y) * cells + y;
					float d = std::min(distances[gy * cols + gx], maxDistance);
					waterChunk->_shore[y * wc + x] = (std::uint8_t)(d * 0.5f);
				}
			}
		}
	}

	// mask texture + flat mesh of one chunk. Vertex y = terrain height (the shader puts the vertex on the water level
	// and uses the difference as the depth). Only quads with water get triangles.
	void Water::updateChunk(std::shared_ptr<WaterChunk> waterChunk) {

		std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByCoords(waterChunk->_coords.x, waterChunk->_coords.y);

		if (!chunk)
			return;

		int wc = Chunk::weightsCols;
		int hc = Chunk::heightsCols;

		// MASK TEXTURE
		std::vector<std::uint8_t> pixels(wc * wc * 4, 0);

		for (int i = 0; i < wc * wc; i++) {
			std::uint8_t value = waterChunk->getMask(i);
			pixels[i * 4 + 0] = value;
			pixels[i * 4 + 1] = waterChunk->_shore[i];	// distance to the shore (foam)
			pixels[i * 4 + 2] = value;
			pixels[i * 4 + 3] = 255;
		}

		if (!waterChunk->_maskTexture) {
			std::shared_ptr<sf::Texture> texture = std::make_shared<sf::Texture>();

			if (!texture->resize(sf::Vector2u((unsigned int)wc, (unsigned int)wc))) {
				DebugError(L"Cannot create a water mask texture");
				return;
			}

			texture->setSmooth(true);
			waterChunk->_maskTexture = texture;
		}

		waterChunk->_maskTexture->update(pixels.data());

		// MESH
		std::vector<Anubis::Vertex3D> vertices;
		std::vector<std::uint32_t> indices;
		vertices.reserve(hc * hc);

		sf::Vector2f position = chunk->getWorldPosition();
		float step = chunk->getHeightStep();
		float weightStep = chunk->getWeightStep();
		int ratio = (wc - 1) / (hc - 1);	// mask points per height point (2)

		for (int y = 0; y < hc; y++) {
			for (int x = 0; x < hc; x++) {
				float xx = position.x + float(x) * step;
				float zz = position.y + float(y) * step;
				float u = (float(x) * step / weightStep + 0.5f) / float(wc);
				float v = (float(y) * step / weightStep + 0.5f) / float(wc);
				vertices.emplace_back(xx, chunk->_heights[y * hc + x], zz, 0.f, 1.f, 0.f, u, v);
			}
		}

		for (int y = 0; y < hc - 1; y++) {
			for (int x = 0; x < hc - 1; x++) {

				bool hasWater = false;

				for (int my = y * ratio; my <= (y + 1) * ratio && !hasWater; my++)
					for (int mx = x * ratio; mx <= (x + 1) * ratio && !hasWater; mx++)
						if (waterChunk->getMask(my * wc + mx) > 0) hasWater = true;

				if (!hasWater)
					continue;

				std::uint32_t index = (std::uint32_t)(y * hc + x);
				std::uint32_t below = index + (std::uint32_t)hc;

				indices.push_back(index);
				indices.push_back(below);
				indices.push_back(below + 1);

				indices.push_back(index);
				indices.push_back(below + 1);
				indices.push_back(index + 1);
			}
		}

		if (indices.empty()) {
			waterChunk->_mesh = nullptr;
			return;
		}

		std::shared_ptr<Anubis::Mesh3D> mesh = std::make_shared<Anubis::Mesh3D>();
		mesh->uploadToGPU(vertices, indices);
		waterChunk->_mesh = mesh;
	}

	void Water::updateAllChunks() {
		updateShoreDistances();

		for (auto& waterChunk : _chunks)
			updateChunk(waterChunk);
	}

	void Water::updateChunksOf(std::vector<sf::Vector2i> coords) {
		for (auto& waterChunk : _chunks)
			if (std::find(coords.begin(), coords.end(), waterChunk->_coords) != coords.end())
				updateChunk(waterChunk);
	}

	WaterState Water::getState() {

		WaterState state;
		state._kind = _kind;
		state._level = _level;
		state._seeds = _seeds;

		for (auto& waterChunk : _chunks) {
			state._coords.push_back(waterChunk->_coords);
			state._fill.push_back(waterChunk->_fill);
			state._added.push_back(waterChunk->_added);
			state._removed.push_back(waterChunk->_removed);
		}

		return state;
	}

	void Water::setState(WaterState state) {

		_kind = state._kind;
		_level = state._level;
		_seeds = state._seeds;
		_chunks.clear();

		for (int i = 0; i < (int)state._coords.size(); i++) {
			std::shared_ptr<WaterChunk> waterChunk = std::make_shared<WaterChunk>(state._coords[i]);
			waterChunk->_fill = state._fill[i];
			waterChunk->_added = state._added[i];
			waterChunk->_removed = state._removed[i];
			_chunks.push_back(waterChunk);
		}

		updateAllChunks();
	}

	float Water::getDistance(sf::Vector2f position, sf::Vector2f& nearest) {

		std::shared_ptr<Map> mapa = MapEditor::editor->_map;
		float size = float(Chunk::tilesCols * Tile::tileSize);
		int wc = Chunk::weightsCols;
		float step = size / float(wc - 1);
		float best = 1e30f;	// squared - no sqrt in the loop

		for (auto& waterChunk : _chunks) {

			// the whole chunk is further than the best distance - skip it
			sf::Vector2f chunkPosition(float(waterChunk->_coords.x) * size, float(waterChunk->_coords.y) * size);
			float dx = std::max({ chunkPosition.x - position.x, 0.f, position.x - (chunkPosition.x + size) });
			float dz = std::max({ chunkPosition.y - position.y, 0.f, position.y - (chunkPosition.y + size) });

			if (dx * dx + dz * dz >= best)
				continue;

			std::shared_ptr<Chunk> chunk = mapa->getChunkByCoords(waterChunk->_coords.x, waterChunk->_coords.y);

			if (!chunk)
				continue;

			for (int y = 0; y < wc; y++) {
				for (int x = 0; x < wc; x++) {

					if (waterChunk->getMask(y * wc + x) < 128)
						continue;

					float px = chunkPosition.x + float(x) * step;
					float pz = chunkPosition.y + float(y) * step;
					float d = (px - position.x) * (px - position.x) + (pz - position.y) * (pz - position.y);

					if (d >= best)
						continue;

					// the shore point is in the mask, but above the level - not water
					if (chunk->getHeight(px, pz) >= _level)
						continue;

					best = d;
					nearest = sf::Vector2f(px, pz);
				}
			}
		}

		return std::sqrt(best);
	}

	// Map::drawWaters() binds the shader and sets the common uniforms.
	void Water::draw() {

		std::shared_ptr<sf::Shader> shader = Anubis::shaders_manager->water3D_shader;
		shader->setUniform("uLevel", _level);
		shader->setUniform("uWaterType", _kind);
		shader->setUniform("uSelected", _isSelected);
		sf::Shader::bind(shader.get());

		for (auto& waterChunk : _chunks) {

			if (!waterChunk->_mesh || !waterChunk->_mesh->_geometry || !waterChunk->_maskTexture)
				continue;

			std::shared_ptr<Chunk> chunk = MapEditor::editor->_map->getChunkByCoords(waterChunk->_coords.x, waterChunk->_coords.y);

			if (!chunk || !chunk->_isVisible)
				continue;

			const auto& geometry = *waterChunk->_mesh->_geometry;

			if (geometry._vertexArray == 0 || geometry._indexCount == 0)
				continue;

			Anubis::GL::activeTexture(0x84C2); // GL_TEXTURE2
			glBindTexture(GL_TEXTURE_2D, waterChunk->_maskTexture->getNativeHandle());
			Anubis::GL::activeTexture(0x84C0); // GL_TEXTURE0

			Anubis::GL::bindVertexArray(geometry._vertexArray);
			glDrawElements(GL_TRIANGLES, geometry._indexCount, GL_UNSIGNED_INT, nullptr);
		}
	}

}
