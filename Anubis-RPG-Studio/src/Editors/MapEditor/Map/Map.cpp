#include "Editors/MapEditor/Map/Map.hpp"
#include <set>
#include "Editors/MapEditor/Editor.hpp"
#include "RenderWindow.hpp"
#include "ShadersManager.hpp"
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"
#include "Editors/MapEditor/Map/PlacedGameObjects.hpp"
#include "DebugLog.hpp"
#include "Time.hpp"
#include <SFML/OpenGL.hpp>
#include "3D/OpenGLBuffers.hpp"
#include "Components/Palette/Tools-Terrain.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace MapEditor {
	Map::Map() {
		_ghostChunk = std::make_shared<GhostChunk>();
		_texture = textures_manager->getTexture(L"assets\\tex\\tileset.png");
	}

	Map::~Map() {

	}

	void Map::addChunk(std::shared_ptr<Chunk> chunk, int terrain) {

		chunk->fill(terrain, 0.f);
		_chunks.push_back(chunk);

		// shared edge points get the values of the existing neighbours
		stitchChunk(chunk);
		chunk->updateSplatTextures();

		// neighbours too - their normals on the edge use the heights of the new chunk
		for (int yy = -1; yy <= 1; yy++) {
			for (int xx = -1; xx <= 1; xx++) {

				std::shared_ptr<Chunk> currentChunk = getChunkByCoords(chunk->_coords.x + xx, chunk->_coords.y + yy);

				if (currentChunk)
					currentChunk->generateVertexBuffer();
			}
		}
	}

	void Map::create(int width, int height) {

		int terrain = 2;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(x, y);
				addChunk(chunk, terrain);
			}
		}
	}

	void Map::drawCircle(sf::Vector2f center, float radius, int terrain) {
		std::vector<std::shared_ptr<Chunk>> chunks = getChunksInRect(center, radius);
		paintTerrain(chunks, center, radius, false, terrain, 1.f);
		refreshChunks(chunks, false);
	}

	std::shared_ptr<Chunk> Map::getChunkByCoords(int x, int y) {
		for (auto& chunk : _chunks) {
			if (chunk->_coords.x == x && chunk->_coords.y == y) {
				return chunk;
			}
		}

		return nullptr;
	}

	std::shared_ptr<Chunk> Map::getChunkByTileGlobalCoords(int x, int y) {
		x = std::floor((float)x / (float)Chunk::tilesCols);
		y = std::floor((float)y / (float)Chunk::tilesRows);

		for (auto& chunk : _chunks) {
			if (chunk->_coords.x == x && chunk->_coords.y == y) {
				return chunk;
			}
		}

		return nullptr;
	}

	std::shared_ptr<Chunk> Map::getChunkByGlobalPosition() {

		int x = std::floor((float)MapEditor::editor->_cursor_on_map->_globalPosition.x / (float)Tile::tileSize);
		int y = std::floor((float)MapEditor::editor->_cursor_on_map->_globalPosition.y / (float)Tile::tileSize);

		return getChunkByTileGlobalCoords(x, y);
	}

	std::shared_ptr<Chunk> Map::getChunkByGlobalPosition(sf::Vector2i position) {

		int x = std::floor((float)position.x / (float)Tile::tileSize);
		int y = std::floor((float)position.y / (float)Tile::tileSize);

		return getChunkByTileGlobalCoords(x, y);
	}

	sf::IntRect Map::getRect() {
		sf::Vector2i minCoords = sf::Vector2i(INT_MAX, INT_MAX);
		sf::Vector2i maxCoords = sf::Vector2i(0, 0);

		for (auto& chunk : _chunks) {
			if (chunk->_coords.x < minCoords.x) minCoords.x = chunk->_coords.x;
			if (chunk->_coords.y < minCoords.y) minCoords.y = chunk->_coords.y;
			if (chunk->_coords.x > maxCoords.x) maxCoords.x = chunk->_coords.x;
			if (chunk->_coords.y > maxCoords.y) maxCoords.y = chunk->_coords.y;
		}


		sf::IntRect rect;
		rect.position = sf::Vector2i(
			minCoords.x * Chunk::tilesCols * Tile::tileSize,
			minCoords.y * Chunk::tilesRows * Tile::tileSize
		);

		rect.size = sf::Vector2i(
			(maxCoords.x - minCoords.x + 1) * Chunk::tilesCols * Tile::tileSize,
			(maxCoords.y - minCoords.y + 1) * Chunk::tilesRows * Tile::tileSize
		);

		return rect;
	}

	std::vector<std::shared_ptr<Chunk>> Map::getChunksInRect(sf::Vector2f center, float radius) {

		std::vector<std::shared_ptr<Chunk>> result;

		for (auto& chunk : _chunks) {
			sf::Vector2f position = chunk->getWorldPosition();
			float size = chunk->getSize();

			if (center.x + radius < position.x || center.x - radius > position.x + size) continue;
			if (center.y + radius < position.y || center.y - radius > position.y + size) continue;

			result.push_back(chunk);
		}

		return result;
	}

	// Copies the shared edge points (heights and weights) from the existing neighbours.
	void Map::stitchChunk(std::shared_ptr<Chunk> chunk) {

		int hc = Chunk::heightsCols;
		int wc = Chunk::weightsCols;
		int tc = Chunk::terrainsCount;

		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {

				if (dx == 0 && dy == 0)
					continue;

				std::shared_ptr<Chunk> neighbour = getChunkByCoords(chunk->_coords.x + dx, chunk->_coords.y + dy);

				if (!neighbour || neighbour == chunk)
					continue;

				// only the points that lie in both chunks (the shared edge)
				for (int y = 0; y < hc; y++) {
					for (int x = 0; x < hc; x++) {
						int nx = x - dx * (hc - 1);
						int ny = y - dy * (hc - 1);

						if (nx < 0 || ny < 0 || nx >= hc || ny >= hc)
							continue;

						chunk->_heights[y * hc + x] = neighbour->_heights[ny * hc + nx];
					}
				}

				for (int y = 0; y < wc; y++) {
					for (int x = 0; x < wc; x++) {
						int nx = x - dx * (wc - 1);
						int ny = y - dy * (wc - 1);

						if (nx < 0 || ny < 0 || nx >= wc || ny >= wc)
							continue;

						for (int t = 0; t < tc; t++)
							chunk->_weights[(y * wc + x) * tc + t] = neighbour->_weights[(ny * wc + nx) * tc + t];
					}
				}
			}
		}
	}

	// After a change of the terrain data: splat textures, and when heights changed also meshes (+ neighbours) and models on them.
	void Map::refreshChunks(std::vector<std::shared_ptr<Chunk>> chunks, bool heightsChanged) {

		for (auto& chunk : chunks)
			chunk->updateSplatTextures();

		if (!heightsChanged)
			return;

		std::set<std::shared_ptr<Chunk>> toRebuild;

		for (auto& chunk : chunks) {
			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					std::shared_ptr<Chunk> c = getChunkByCoords(chunk->_coords.x + dx, chunk->_coords.y + dy);
					if (c) toRebuild.insert(c);
				}
			}
		}

		for (auto& chunk : toRebuild)
			chunk->generateVertexBuffer();

		// models stand on the new heights
		for (auto& chunk : chunks) {
			for (auto& object : chunk->_gameObjectsOnMap)
				MapEditor::editor->_game_objects->updateModelTransform(object);
		}
	}

	// 1 in the middle of the brush, 0 on the edge. Full strength up to the half of the radius.
	float Map::getBrushFalloff(sf::Vector2f delta, float radius, bool square) {

		float distance = square ? std::max(std::abs(delta.x), std::abs(delta.y)) : std::sqrt(delta.x * delta.x + delta.y * delta.y);

		if (distance >= radius)
			return 0.f;

		float f = std::clamp((radius - distance) / (radius * 0.5f), 0.f, 1.f);
		return f * f * (3.f - 2.f * f);
	}

	// amount = how much of the terrain is added in the middle of the brush (0 - 1)
	void Map::paintTerrain(std::vector<std::shared_ptr<Chunk>> chunks, sf::Vector2f center, float radius, bool square, int terrain, float amount) {

		int wc = Chunk::weightsCols;
		int tc = Chunk::terrainsCount;

		terrain = std::clamp(terrain, 0, tc - 1);

		for (auto& chunk : chunks) {

			sf::Vector2f position = chunk->getWorldPosition();
			float step = chunk->getWeightStep();

			for (int y = 0; y < wc; y++) {
				for (int x = 0; x < wc; x++) {

					sf::Vector2f point(position.x + float(x) * step, position.y + float(y) * step);
					float a = std::clamp(getBrushFalloff(point - center, radius, square) * amount, 0.f, 1.f);

					if (a <= 0.f)
						continue;

					// other terrains get smaller, the painted terrain gets the rest to 255
					int index = (y * wc + x) * tc;
					int total = 0;

					for (int t = 0; t < tc; t++) {
						if (t == terrain)
							continue;

						int value = (int)(float(chunk->_weights[index + t]) * (1.f - a));
						chunk->_weights[index + t] = (std::uint8_t)value;
						total += value;
					}

					chunk->_weights[index + terrain] = (std::uint8_t)(255 - total);
				}
			}
		}
	}

	// Raise / Lower: amount = world units in the middle of the brush. Smooth / Flatten: amount = speed (0 - 1).
	void Map::changeHeights(std::vector<std::shared_ptr<Chunk>> chunks, sf::Vector2f center, float radius, bool square, ToolTerrainType tool, float amount, float flattenHeight) {

		int hc = Chunk::heightsCols;

		// 1. new heights of all chunks first - Smooth reads the neighbours,
		//    so the shared edge points get the same values in both chunks
		std::vector<std::vector<float>> newHeights;

		for (auto& chunk : chunks) {

			std::vector<float> heights = chunk->_heights;
			sf::Vector2f position = chunk->getWorldPosition();
			float step = chunk->getHeightStep();

			for (int y = 0; y < hc; y++) {
				for (int x = 0; x < hc; x++) {

					sf::Vector2f point(position.x + float(x) * step, position.y + float(y) * step);
					float f = getBrushFalloff(point - center, radius, square);

					if (f <= 0.f)
						continue;

					float h = chunk->_heights[y * hc + x];

					if (tool == ToolTerrainType::Raise)
						h += amount * f;
					else if (tool == ToolTerrainType::Lower)
						h -= amount * f;
					else if (tool == ToolTerrainType::Smooth) {
						float average = (
							chunk->getHeightPoint(x - 1, y) + chunk->getHeightPoint(x + 1, y) +
							chunk->getHeightPoint(x, y - 1) + chunk->getHeightPoint(x, y + 1)
						) * 0.25f;
						h += (average - h) * std::min(amount * f, 1.f);
					}
					else if (tool == ToolTerrainType::Flatten)
						h += (flattenHeight - h) * std::min(amount * f, 1.f);

					heights[y * hc + x] = h;
				}
			}

			newHeights.push_back(heights);
		}

		// 2. apply
		for (int i = 0; i < (int)chunks.size(); i++)
			chunks[i]->_heights = newHeights[i];
	}

	// Old maps keep one terrain per tile - weights are interpolated between the tile centers.
	void Map::convertOldTiles() {

		std::vector<int> oldTileTerrains = { 0, 1, 2, 3, 3, 4, 5, 6, 7 };	// tileset group -> terrain

		int wc = Chunk::weightsCols;

		for (auto& chunk : _chunks) {

			if (chunk->_oldTiles.size() != Chunk::tilesCols * Chunk::tilesRows)
				continue;

			sf::Vector2f position = chunk->getWorldPosition();
			float step = chunk->getWeightStep();

			for (int y = 0; y < wc; y++) {
				for (int x = 0; x < wc; x++) {

					// position in the "tile centers" space
					float sx = (position.x + float(x) * step) / float(Tile::tileSize) - 0.5f;
					float sy = (position.y + float(y) * step) / float(Tile::tileSize) - 0.5f;

					int tx = (int)std::floor(sx);
					int ty = (int)std::floor(sy);
					float fx = sx - float(tx);
					float fy = sy - float(ty);

					std::array<float, 8> weights = {};

					for (int i = 0; i < 4; i++) {
						int gx = tx + i % 2;
						int gy = ty + i / 2;
						float w = (i % 2 == 0 ? 1.f - fx : fx) * (i / 2 == 0 ? 1.f - fy : fy);

						std::shared_ptr<Chunk> c = getChunkByTileGlobalCoords(gx, gy);

						// no chunk there (edge of the map) - the nearest tile of this chunk
						if (!c || c->_oldTiles.size() != Chunk::tilesCols * Chunk::tilesRows) {
							c = chunk;
							gx = std::clamp(gx, chunk->_coords.x * Chunk::tilesCols, chunk->_coords.x * Chunk::tilesCols + Chunk::tilesCols - 1);
							gy = std::clamp(gy, chunk->_coords.y * Chunk::tilesRows, chunk->_coords.y * Chunk::tilesRows + Chunk::tilesRows - 1);
						}

						int lx = gx - c->_coords.x * Chunk::tilesCols;
						int ly = gy - c->_coords.y * Chunk::tilesRows;
						int type = c->_oldTiles[ly * Chunk::tilesCols + lx];

						if (type < 0 || type >= (int)oldTileTerrains.size())
							type = 0;

						weights[oldTileTerrains[type]] += w;
					}

					chunk->setWeights(y * wc + x, weights);
				}
			}
		}

		for (auto& chunk : _chunks)
			chunk->_oldTiles.clear();
	}

	void Map::removeGameObjectsFromChunksByPrefab(std::shared_ptr<GameObject> object) {

		for (auto& chunk : _chunks) {
			std::erase_if(chunk->_gameObjectsOnMap, [&](const std::shared_ptr<PlacedGameObject>& objectOnMap) {
				return objectOnMap->_prefab.lock().get() == object.get();
				});
		}

	}

	void Map::setVisibleChunks() {

		for (auto& chunk : _visibleChunks)
			chunk->_isVisible = false;

		_visibleChunks.clear();
		MapEditor::editor->_game_objects->_visiblePlacedGameObjects.clear();

		MapEditor::editor->_scene->setProjection();
		MapEditor::editor->_scene->setCamera();

		GLfloat view[16];
		GLfloat projection[16];

		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);

		// Macierz projection * view, układ kolumnowy.
		GLfloat matrix[16] = {};

		for (int column = 0; column < 4; column++) {
			for (int row = 0; row < 4; row++) {
				for (int k = 0; k < 4; k++) {
					matrix[column * 4 + row] += projection[k * 4 + row] * view[column * 4 + k];
				}
			}
		}

		float chunkWidth = float(Chunk::tilesCols * Tile::tileSize);
		float chunkDepth = float(Chunk::tilesRows * Tile::tileSize);

		for (auto& chunk : _chunks) {

			
			float left = float(chunk->_coords.x) * chunkWidth ;
			float top = float(chunk->_coords.y) * chunkDepth;

			// Narożniki terenu na płaszczyźnie XZ.
			float margin = 256.0f;
			const sf::Vector2f corners[] = {
				{ left - margin,              top - margin},
				{ left + chunkWidth + margin, top - margin},
				{ left + chunkWidth + margin, top + chunkDepth + margin },
				{ left - margin,              top + chunkDepth + margin }
			};

			bool outsideLeft = true;
			bool outsideRight = true;
			bool outsideBottom = true;
			bool outsideTop = true;
			bool outsideNear = true;
			bool outsideFar = true;

			for (const auto& corner : corners) {

				float x = corner.x;
				float z = corner.y;

				// Współrzędne clip-space, bez dzielenia przez W.
				float clipX = matrix[0] * x + matrix[8] * z + matrix[12];
				float clipY = matrix[1] * x + matrix[9] * z + matrix[13];
				float clipZ = matrix[2] * x + matrix[10] * z + matrix[14];
				float clipW = matrix[3] * x + matrix[11] * z + matrix[15];

				outsideLeft = outsideLeft && (clipX < -clipW);
				outsideRight = outsideRight && (clipX > clipW);
				outsideBottom = outsideBottom && (clipY < -clipW);
				outsideTop = outsideTop && (clipY > clipW);
				outsideNear = outsideNear && (clipZ < -clipW);
				outsideFar = outsideFar && (clipZ > clipW);
			}

			bool visible = !(outsideLeft || outsideRight || outsideBottom || outsideTop || outsideNear || outsideFar);

			if (visible) {
				chunk->setVisible();
			}
			else if (!MapEditor::editor->_cursor_on_map->_selectedObjects.empty()) {
				for (auto& object : chunk->_gameObjectsOnMap) {
					if (object && object->_isSelected) {
						chunk->setVisible();
						break;
					}
				}
			}
		}
	}

	// WATERS ///////////////////////////////////////////////////////////////////////////////////////////////////

	void Map::addWater(std::shared_ptr<Water> water) {
		if (std::find(_waters.begin(), _waters.end(), water) == _waters.end())
			_waters.push_back(water);
	}

	void Map::removeWater(std::shared_ptr<Water> water) {

		std::erase(_waters, water);

		// a removed water can not stay selected
		std::shared_ptr<CursorOnMap> cursor = MapEditor::editor->_cursor_on_map;
		water->_isSelected = false;
		std::erase_if(cursor->_selectedObjects, [&](std::shared_ptr<SelectedPlacedGameObject>& selected) { return selected->_object == water; });
		std::erase_if(cursor->_prevSelectedObjects, [&](std::shared_ptr<SelectedPlacedGameObject>& selected) { return selected->_object == water; });
	}

	// the nearest water under the window pixel, not hidden by the terrain
	std::shared_ptr<Water> Map::getWaterUnderCursor(sf::Vector2i pixel) {

		std::shared_ptr<Anubis::Scene> scene = MapEditor::editor->_scene;
		sf::Vector3f origin, dir;

		if (!scene->screenToRay(pixel, origin, dir))
			return nullptr;

		// terrain in front of the water hides it
		float terrainDistance = 1000000.f;
		sf::Vector3f terrainHit;

		if (pickTerrain(pixel, terrainHit)) {
			sf::Vector3f d = terrainHit - origin;
			terrainDistance = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
		}

		std::shared_ptr<Water> result = nullptr;
		float bestDistance = terrainDistance + 1.f;	// +1: the water may lie on the terrain surface

		for (auto& water : _waters) {
			sf::Vector3f hit;

			if (!water->getRayHit(origin, dir, hit))
				continue;

			sf::Vector3f d = hit - origin;
			float distance = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);

			if (distance < bestDistance) {
				bestDistance = distance;
				result = water;
			}
		}

		return result;
	}

	void Map::drawWaters(sf::Glsl::Mat4 view, sf::Glsl::Mat4 projection) {

		std::shared_ptr<MapEditor::Editor> map_editor = MapEditor::editor;
		std::shared_ptr<sf::Shader> shader = Anubis::shaders_manager->water3D_shader;

		if (_waters.empty() || !shader || shader->getNativeHandle() == 0)
			return;

		std::shared_ptr<Texture> noise = textures_manager->getTexture(L"assets\\tex\\noise.png");

		if (!noise || !noise->_texture)
			return;

		noise->_texture->setRepeated(true);
		noise->_texture->setSmooth(true);

		sf::Vector2u windowSize = Anubis::render_window->getSize();

		shader->setUniform("uView", view);
		shader->setUniform("uProjection", projection);
		shader->setUniform("uNoise", 0);
		shader->setUniform("uShadowMap", 1);
		shader->setUniform("uMask", 2);
		shader->setUniform("uLightViewProjection", sf::Glsl::Mat4(map_editor->_scene->_lightViewProjection.data()));
		shader->setUniform("uShadowStrength", 0.45f);
		shader->setUniform("uShadowSamples", map_editor->_scene->_shadowSamples);
		shader->setUniform("uShoreWidth", 12.f);
		shader->setUniform("time", Anubis::currentTime.asSeconds());
		shader->setUniform("resolution", sf::Vector2f(windowSize));
		shader->setUniform("startPos", sf::Vector2f(0.f, 0.f));

		// transparent: tested with the terrain depth, but does not write it
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		Anubis::GL::activeTexture(0x84C1); // GL_TEXTURE1
		glBindTexture(GL_TEXTURE_2D, map_editor->_scene->_shadowDepth);
		Anubis::GL::activeTexture(0x84C0); // GL_TEXTURE0
		glBindTexture(GL_TEXTURE_2D, noise->_texture->getNativeHandle());

		for (auto& water : _waters)
			water->draw();

		Anubis::GL::bindVertexArray(0);
		sf::Shader::bind(nullptr);

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	void Map::saveFromProject(std::ofstream& file) {
		BinaryWriter writer(file);

		for (auto& chunk : _chunks) {
			writer.write_Vector2i(chunk->_coords);

			// -2 = smooth terrain (old maps have here the type of the first tile: 0 or more)
			writer.write_int8(-2);

			for (auto& height : chunk->_heights)
				writer.write_float(height);

			writer.write_data(chunk->_weights);
		}
	}

	void Map::loadFromProject(std::ifstream& file, std::shared_ptr<Anubis::Loader>& loader) {

		loader->setTitle(L"Loading map ...");
		
		BinaryReader reader(file);

		// clear all
		_visibleChunks.clear();
		_chunks.clear();
		_waters.clear();
		MapEditor::editor->_game_objects->_visiblePlacedGameObjects.clear();

		int chunksCount = loader->counts.chunksCount;

		for (int i = 0; i < chunksCount; i++) {

			sf::Vector2i chunkCoords = reader.read_Vector2i();
			std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(chunkCoords.x, chunkCoords.y);
			loader->nextStep(L"chunks");

			int format = reader.read_int8();

			if (format == -2) {
				for (auto& height : chunk->_heights)
					height = reader.read_float();

				std::shared_ptr<std::vector<uint8_t>> weights = reader.read_data();

				if (weights->size() == chunk->_weights.size())
					chunk->_weights = *weights;
				else
					DebugError(L"Wrong size of the terrain weights in chunk " + std::to_wstring(chunkCoords.x) + L"x" + std::to_wstring(chunkCoords.y));
			}
			else {
				// old map - one terrain per tile, converted in convertOldTiles()
				chunk->_oldTiles.push_back(format);

				for (int t = 1; t < Chunk::tilesCols * Chunk::tilesRows; t++)
					chunk->_oldTiles.push_back(reader.read_int8());
			}

			_chunks.push_back(chunk);
		}

		convertOldTiles();

		for (auto& chunk : _chunks) {
			chunk->updateSplatTextures();
			chunk->generateVertexBuffer();
		}

		editor->setVisibleChunks();

	}

	bool Map::pickTerrain(sf::Vector2i pixel, sf::Vector3f& hit) {

		auto scene = MapEditor::editor ? MapEditor::editor->_scene : nullptr;

		if (!scene || !Anubis::shaders_manager->terrain3D_shader || Anubis::shaders_manager->terrain3D_shader->getNativeHandle() == 0)
			return false;

		if (!Anubis::GL::bindVertexArray || !Anubis::GL::bindFramebuffer)
			return false;

		const sf::Vector2u size = Anubis::render_window->getSize();

		if (pixel.x < 0 || pixel.y < 0 || pixel.x >= int(size.x) || pixel.y >= int(size.y))
			return false;

		if (!scene->createPickingBuffer(size.x, size.y))
			return false;

		const int pixelX = pixel.x;
		const int pixelY = int(size.y) - pixel.y - 1;

		GLint previousMatrixMode = 0;
		glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		Anubis::GL::bindFramebuffer(Anubis::GL::Framebuffer, scene->_pickingFBO);

		scene->setViewport();
		scene->setProjection();
		scene->setCamera();

		GLfloat viewMatrix[16];
		GLfloat projectionMatrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, viewMatrix);
		glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);

		Anubis::shaders_manager->terrain3D_shader->setUniform("uView", sf::Glsl::Mat4(viewMatrix));
		Anubis::shaders_manager->terrain3D_shader->setUniform("uProjection", sf::Glsl::Mat4(projectionMatrix));

		// Only the depth of the pixel under the cursor is needed.
		glEnable(GL_SCISSOR_TEST);
		glScissor(pixelX, pixelY, 1, 1);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glClearDepth(1.0);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Terrain only - models are not drawn, so the cursor reaches the ground behind them.
		sf::Shader::bind(Anubis::shaders_manager->terrain3D_shader.get());

		for (auto& chunk : _visibleChunks)
			chunk->draw();

		Anubis::GL::bindVertexArray(0);
		sf::Shader::bind(nullptr);

		GLfloat depth = 1.f;
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(pixelX, pixelY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

		Anubis::GL::bindFramebuffer(Anubis::GL::Framebuffer, 0);

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(previousMatrixMode);

		glPopClientAttrib();
		glPopAttrib();

		return scene->unprojectDepth(pixel, depth, hit);
	}

	void Map::drawBrushOverlay(sf::Vector2f center, float radius, bool square, sf::Color color) {

		auto scene = MapEditor::editor ? MapEditor::editor->_scene : nullptr;

		if (!scene || !Anubis::shaders_manager->brush3D_shader || Anubis::shaders_manager->brush3D_shader->getNativeHandle() == 0)
			return;

		if (!Anubis::GL::bindVertexArray)
			return;

		GLint previousMatrixMode = 0;
		glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		scene->setViewport();
		scene->setProjection();
		scene->setCamera();

		GLfloat viewMatrix[16];
		GLfloat projectionMatrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, viewMatrix);
		glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);

		std::shared_ptr<sf::Shader> shader = Anubis::shaders_manager->brush3D_shader;
		shader->setUniform("uView", sf::Glsl::Mat4(viewMatrix));
		shader->setUniform("uProjection", sf::Glsl::Mat4(projectionMatrix));
		shader->setUniform("uCenter", center);
		shader->setUniform("uRadius", radius);
		shader->setUniform("uSquare", square);
		shader->setUniform("uColor", sf::Glsl::Vec4(color));
		shader->setUniform("uLineWidth", 1.5f);

		// Same geometry as the terrain, drawn slightly in front of it; models still cover it.
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.f, -1.f);
		glDisable(GL_CULL_FACE);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		sf::Shader::bind(shader.get());

		for (auto& chunk : getChunksInRect(center, radius))
			chunk->draw();

		Anubis::GL::bindVertexArray(0);
		sf::Shader::bind(nullptr);

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(previousMatrixMode);

		glPopAttrib();
	}

	float Map::getHeight(float x, float z) {

		if (_chunks.empty())
			return 0.f;

		float size = _chunks[0]->getSize();
		std::shared_ptr<Chunk> chunk = getChunkByCoords((int)std::floor(x / size), (int)std::floor(z / size));

		if (!chunk)
			return 0.f;

		return chunk->getHeight(x, z);
	}

	void Map::cursorHover() {

		if (MapEditor::editor->_cursor_on_map != nullptr && MapEditor::editor->_cursor_on_map->_isOnTerrain && getRect().contains(MapEditor::editor->_cursor_on_map->_globalPosition)) {
			Anubis::GUI_manager->Element_hovered = shared_from_this();
		}

		//_ghostChunk->cursorHover(); // TO-DO
	}

	void Map::handleEvent(const sf::Event& event) {

		if (Anubis::GUI_manager->Element_hovered.get() == this) {
			if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left) {
				Anubis::GUI_manager->Element_pressed = shared_from_this();
			}

		}

		else if (Anubis::GUI_manager->Element_pressed.get() == this) {
			if (!(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)))
				Anubis::GUI_manager->Element_pressed = nullptr;
		}


		//_ghostChunk->handleEvent(event); // TO-DO

	}

	void Map::update() {
		_ghostChunk->update();

		if (MapEditor::editor->_camera->_isMoving || MapEditor::editor->_scene->_cameraIsMoving)
			editor->setVisibleChunks();
	}

	void Map::draw() {

		std::shared_ptr<MapEditor::Editor> map_editor = std::dynamic_pointer_cast<MapEditor::Editor>(Main::editor_manager->get_back());
		if (!map_editor || !map_editor->_scene || !map_editor->_camera)
			return;

		if (!_texture || !_texture->_texture || !Anubis::shaders_manager->terrain3D_shader || Anubis::shaders_manager->terrain3D_shader->getNativeHandle() == 0)
			return;

		if (!Anubis::GL::bindVertexArray || !Anubis::GL::activeTexture)
			return;

		sf::Vector2u windowSize = Anubis::render_window->getSize();

		if (windowSize.x == 0 || windowSize.y == 0)
			return;

		Anubis::render_window->setView(map_editor->_camera->_view);

		map_editor->_scene->setViewport();
		map_editor->_scene->setProjection();
		map_editor->_scene->setCamera();

		GLfloat viewMatrix[16];
		GLfloat projectionMatrix[16];

		glGetFloatv(GL_MODELVIEW_MATRIX, viewMatrix);
		glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);

		std::shared_ptr<sf::Shader> shader = Anubis::shaders_manager->terrain3D_shader;
		shader->setUniform("uView", sf::Glsl::Mat4(viewMatrix));
		shader->setUniform("uProjection", sf::Glsl::Mat4(projectionMatrix));
		shader->setUniform("uTerrainTexture", 0);
		shader->setUniform("uSplat0", 2);
		shader->setUniform("uSplat1", 3);
		// tileset.png = terrains in one column, every terrain is a square (64 x 64, 256 x 256 ...)
		sf::Vector2u tilesetSize = _texture->_texture->getSize();
		shader->setUniform("uTerrainCount", (int)(tilesetSize.y / tilesetSize.x));
		shader->setUniform("uTextureWorldSize", (float)tilesetSize.x);	// 1 texel = 1 world unit
		shader->setUniform("uSplatWorldSize", float(Tile::tileSize * Chunk::tilesCols) / float(Chunk::weightsCols - 1) * float(Chunk::weightsCols));
		shader->setUniform("uTileSize", float(Tile::tileSize));
		shader->setUniform("uSunDirection", map_editor->_scene->_sunDirection);
		shader->setUniform("uLightViewProjection", sf::Glsl::Mat4(map_editor->_scene->_lightViewProjection.data()));
		shader->setUniform("uShadowMap", 1);
		shader->setUniform("uShadowStrength", 0.45f);
		shader->setUniform("uShadowSamples", map_editor->_scene->_shadowSamples);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Anubis::GL::activeTexture(0x84C1); // GL_TEXTURE1
		glBindTexture(GL_TEXTURE_2D, map_editor->_scene->_shadowDepth);

		Anubis::GL::activeTexture(0x84C0); // GL_TEXTURE0
		glBindTexture(GL_TEXTURE_2D, _texture->_texture->getNativeHandle());
		sf::Shader::bind(shader.get());

		for (auto& chunk : _visibleChunks) {
			chunk->draw();
		}

		Anubis::GL::bindVertexArray(0);
		sf::Shader::bind(nullptr);

		// waters after the terrain - they use its depth (shore line)
		drawWaters(sf::Glsl::Mat4(viewMatrix), sf::Glsl::Mat4(projectionMatrix));

		glBindTexture(GL_TEXTURE_2D, 0);

		Anubis::GL::activeTexture(0x84C1); // GL_TEXTURE1
		glBindTexture(GL_TEXTURE_2D, 0);
		Anubis::GL::activeTexture(0x84C2); // GL_TEXTURE2
		glBindTexture(GL_TEXTURE_2D, 0);
		Anubis::GL::activeTexture(0x84C3); // GL_TEXTURE3
		glBindTexture(GL_TEXTURE_2D, 0);
		Anubis::GL::activeTexture(0x84C0); // GL_TEXTURE0

		// SFML draws editor overlays using the same captured 3D matrices.
		Anubis::render_window->resetGLStates();

		Anubis::shaders_manager->ghost_chunk_shader->setUniform("uView", sf::Glsl::Mat4(viewMatrix));
		Anubis::shaders_manager->ghost_chunk_shader->setUniform("uProjection", sf::Glsl::Mat4(projectionMatrix));

		if (map_editor->_main_menu->_render_chunks_info->_checkbox->_value == 1) {

			const sf::IntRect mapRect = getRect();
			sf::Vector2f size_rect(mapRect.size);
			sf::Vector2f position_rect(mapRect.position);

			std::shared_ptr<sf::Shader> shader = Anubis::shaders_manager->grid_shader;
			shader->setUniform("uView", sf::Glsl::Mat4(viewMatrix));
			shader->setUniform("uProjection", sf::Glsl::Mat4(projectionMatrix));
			shader->setUniform("rectPosition", position_rect);
			shader->setUniform("gridSize", float(Tile::tileSize));
			shader->setUniform("chunkSize", float(Chunk::tilesCols));
			shader->setUniform("gridWidth", 2.f);
			shader->setUniform("gridMainColor", sf::Glsl::Vec4(sf::Color(7, 7, 7, 255)));
			shader->setUniform("gridSecondColor", sf::Glsl::Vec4(sf::Color(47, 47, 47, 255)));
			shader->setUniform("position", position_rect);

			sf::RenderStates states;
			states.shader = shader.get();

			sf::RectangleShape rect(size_rect);
			rect.setFillColor(sf::Color(127, 47, 47, 127));
			Anubis::render_window->draw(rect, states);

			for (auto& chunk : _visibleChunks) {
				chunk->drawCoords();
			}
		}

		_ghostChunk->draw();
	}

}
