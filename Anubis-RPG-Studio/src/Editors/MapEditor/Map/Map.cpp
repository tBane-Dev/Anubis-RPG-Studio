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

namespace MapEditor {
	Map::Map() {
		_ghostChunk = std::make_shared<GhostChunk>();
		_texture = textures_manager->getTexture(L"assets\\tex\\tileset.png");
		

	}

	Map::~Map() {

	}

	void Map::addChunk(std::shared_ptr<Chunk> chunk, int tileType) {

		int x = chunk->_coords.x;
		int y = chunk->_coords.y;

		for (int ty = 0; ty < Chunk::tilesRows; ty++) {
			for (int tx = 0; tx < Chunk::tilesCols; tx++) {
				std::shared_ptr<Tile> tile = std::make_shared<Tile>();

				tile->_type = tileType;

				tile->_coords.x = x * Chunk::tilesCols + tx;
				tile->_coords.y = y * Chunk::tilesRows + ty;

				tile->_position.x = (float)(tile->_coords.x * Tile::tileSize);
				tile->_position.y = (float)(tile->_coords.y * Tile::tileSize);

				chunk->_tiles.push_back(tile);
			}
		}

		_chunks.push_back(chunk);

		for (int yy = -1; yy <= 1; yy++) {
			for (int xx = -1; xx <= 1; xx++) {

				int cx = x + xx;
				int cy = y + yy;

				std::shared_ptr<Chunk> currentChunk = getChunkByCoords(cx, cy);

				if (!currentChunk)
					continue;

				currentChunk->generateVertexBuffer(
					getChunkByCoords(cx - 1, cy - 1), getChunkByCoords(cx, cy - 1), getChunkByCoords(cx + 1, cy - 1),
					getChunkByCoords(cx - 1, cy), getChunkByCoords(cx + 1, cy),
					getChunkByCoords(cx - 1, cy + 1), getChunkByCoords(cx, cy + 1), getChunkByCoords(cx + 1, cy + 1)
				);
			}
		}
	}

	void Map::create(int width, int height) {

		int tileType = 2;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(x, y);
				addChunk(chunk, tileType);
			}
		}
	}

	void Map::drawCircle(sf::Vector2i center, int radius, int type) {

		std::set<std::shared_ptr<Chunk>> chunksToEdit;
		for (int yy = center.y - radius; yy <= center.y + radius; yy++) {
			for (int xx = center.x - radius; xx <= center.x + radius; xx++) {

				if ((yy - center.y) * (yy - center.y) + (xx - center.x) * (xx - center.x) >= radius * radius) continue;

				std::shared_ptr<Chunk> c = getChunkByTileGlobalCoords(xx, yy);
				if (!c) continue;

				std::shared_ptr<Tile> t = c->getTileByTileGlobalCoords(xx, yy);
				if (!t) continue;

				t->_type = type;
				chunksToEdit.insert(c);

				if (getChunkByCoords(c->_coords.x - 1, c->_coords.y - 1)) chunksToEdit.insert(getChunkByCoords(c->_coords.x - 1, c->_coords.y - 1));
				if (getChunkByCoords(c->_coords.x, c->_coords.y - 1)) chunksToEdit.insert(getChunkByCoords(c->_coords.x, c->_coords.y - 1));
				if (getChunkByCoords(c->_coords.x + 1, c->_coords.y - 1)) chunksToEdit.insert(getChunkByCoords(c->_coords.x + 1, c->_coords.y - 1));

				if (getChunkByCoords(c->_coords.x - 1, c->_coords.y)) chunksToEdit.insert(getChunkByCoords(c->_coords.x - 1, c->_coords.y));
				if (getChunkByCoords(c->_coords.x + 1, c->_coords.y)) chunksToEdit.insert(getChunkByCoords(c->_coords.x + 1, c->_coords.y));

				if (getChunkByCoords(c->_coords.x - 1, c->_coords.y + 1)) chunksToEdit.insert(getChunkByCoords(c->_coords.x - 1, c->_coords.y + 1));
				if (getChunkByCoords(c->_coords.x, c->_coords.y + 1)) chunksToEdit.insert(getChunkByCoords(c->_coords.x, c->_coords.y + 1));
				if (getChunkByCoords(c->_coords.x + 1, c->_coords.y + 1)) chunksToEdit.insert(getChunkByCoords(c->_coords.x + 1, c->_coords.y + 1));
			}
		}

		for (auto& c : chunksToEdit)
			c->generateVertexBuffer(
				getChunkByCoords(c->_coords.x - 1, c->_coords.y - 1), getChunkByCoords(c->_coords.x, c->_coords.y - 1), getChunkByCoords(c->_coords.x + 1, c->_coords.y - 1),
				getChunkByCoords(c->_coords.x - 1, c->_coords.y), getChunkByCoords(c->_coords.x + 1, c->_coords.y),
				getChunkByCoords(c->_coords.x - 1, c->_coords.y + 1), getChunkByCoords(c->_coords.x, c->_coords.y + 1), getChunkByCoords(c->_coords.x + 1, c->_coords.y + 1)
			);
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

	std::shared_ptr<Tile> Map::getTileByTileGlobalCoords(int x, int y) {

		std::shared_ptr<Chunk> chunk = getChunkByTileGlobalCoords(x, y);

		if (chunk == nullptr) {
			return nullptr;
		}

		return chunk->getTileByTileGlobalCoords(x, y);
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

	void Map::saveFromProject(std::ofstream& file) {
		BinaryWriter writer(file);

		for (auto& chunk : _chunks) {
			writer.write_Vector2i(chunk->_coords);

			for (int ty = 0; ty < Chunk::tilesRows; ty++) {
				for (int tx = 0; tx < Chunk::tilesCols; tx++) {
					int index = ty * Chunk::tilesCols + tx;
					writer.write_int8(chunk->_tiles[index]->_type);
				}
			}
		}
	}

	void Map::loadFromProject(std::ifstream& file, std::shared_ptr<Anubis::Loader>& loader) {

		loader->setTitle(L"Loading map ...");
		
		BinaryReader reader(file);

		// clear all
		_visibleChunks.clear();
		_chunks.clear();
		MapEditor::editor->_game_objects->_visiblePlacedGameObjects.clear();

		int chunksCount = loader->counts.chunksCount;

		for (int i = 0; i < chunksCount; i++) {

			sf::Vector2i chunkCoords = reader.read_Vector2i();
			std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(chunkCoords.x, chunkCoords.y);
			loader->nextStep(L"chunks");
			for (int ty = 0; ty < Chunk::tilesRows; ty++) {
				for (int tx = 0; tx < Chunk::tilesCols; tx++) {
					std::shared_ptr<Tile> tile = std::make_shared<Tile>();
					tile->_type = reader.read_int8();
					tile->_coords.x = chunkCoords.x * Chunk::tilesCols + tx;
					tile->_coords.y = chunkCoords.y * Chunk::tilesRows + ty;
					tile->_position.x = (float)(tile->_coords.x * Tile::tileSize);
					tile->_position.y = (float)(tile->_coords.y * Tile::tileSize);
					chunk->_tiles.push_back(tile);
				}
			}

			_chunks.push_back(chunk);
		}

		for (auto& chunk : _chunks) {
			chunk->generateVertexBuffer(
				getChunkByCoords(chunk->_coords.x - 1, chunk->_coords.y - 1), getChunkByCoords(chunk->_coords.x, chunk->_coords.y - 1), getChunkByCoords(chunk->_coords.x + 1, chunk->_coords.y - 1),
				getChunkByCoords(chunk->_coords.x - 1, chunk->_coords.y), getChunkByCoords(chunk->_coords.x + 1, chunk->_coords.y),
				getChunkByCoords(chunk->_coords.x - 1, chunk->_coords.y + 1), getChunkByCoords(chunk->_coords.x, chunk->_coords.y + 1), getChunkByCoords(chunk->_coords.x + 1, chunk->_coords.y + 1)
			);
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

	void Map::drawTileOverlay(sf::Vector2i firstTile, sf::Vector2i size, const std::vector<float>& mask, sf::Color color) {

		constexpr int maxTiles = 441; // must match uMask[] in tileOverlay3D.frag

		auto scene = MapEditor::editor ? MapEditor::editor->_scene : nullptr;

		if (!scene || !Anubis::shaders_manager->tileOverlay3D_shader || Anubis::shaders_manager->tileOverlay3D_shader->getNativeHandle() == 0)
			return;

		if (!Anubis::GL::bindVertexArray)
			return;

		if (size.x <= 0 || size.y <= 0 || size.x * size.y > maxTiles || int(mask.size()) < size.x * size.y)
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

		std::shared_ptr<sf::Shader> shader = Anubis::shaders_manager->tileOverlay3D_shader;
		shader->setUniform("uView", sf::Glsl::Mat4(viewMatrix));
		shader->setUniform("uProjection", sf::Glsl::Mat4(projectionMatrix));
		shader->setUniform("uTileSize", float(Tile::tileSize));
		shader->setUniform("uFirstTile", sf::Glsl::Vec2((float)firstTile.x, (float)firstTile.y));
		shader->setUniform("uWidth", size.x);
		shader->setUniform("uHeight", size.y);
		shader->setUniformArray("uMask", mask.data(), std::size_t(size.x * size.y));
		shader->setUniform("uColor", sf::Glsl::Vec4(color));
		shader->setUniform("uGridWidth", 1.f);

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

		sf::Shader::bind(Anubis::shaders_manager->tileOverlay3D_shader.get());

		for (auto& chunk : _visibleChunks)
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
		// TO-DO - terrain heights: interpolate tile corner heights here.
		return 0.f;
	}

	void Map::cursorHover() {

		if (MapEditor::editor->_cursor_on_map != nullptr && MapEditor::editor->_cursor_on_map->_isOnTerrain && getRect().contains(MapEditor::editor->_cursor_on_map->_globalPosition)) {
			GUI_manager->Element_hovered = shared_from_this();
		}

		//_ghostChunk->cursorHover(); // TO-DO
	}

	void Map::handleEvent(const sf::Event& event) {

		if (GUI_manager->Element_hovered.get() == this) {
			if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left) {
				GUI_manager->Element_pressed = shared_from_this();
			}

		}

		else if (GUI_manager->Element_pressed.get() == this) {
			if (!(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)))
				GUI_manager->Element_pressed = nullptr;
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
		shader->setUniform("uTexture", 0);
		shader->setUniform("resolution", sf::Vector2f(windowSize));
		shader->setUniform("time", Anubis::currentTime.asSeconds());
		shader->setUniform("startPos", sf::Vector2f(0.f, 0.f));
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
		glBindTexture(GL_TEXTURE_2D, 0);

		Anubis::GL::activeTexture(0x84C1); // GL_TEXTURE1
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

	int getNeighbourTypeLocal(
		int nx, int ny,
		const std::shared_ptr<Chunk>& chunk,
		const std::shared_ptr<Chunk>& leftTopChunk,
		const std::shared_ptr<Chunk>& topChunk,
		const std::shared_ptr<Chunk>& rightTopChunk,
		const std::shared_ptr<Chunk>& leftChunk,
		const std::shared_ptr<Chunk>& rightChunk,
		const std::shared_ptr<Chunk>& leftBottomChunk,
		const std::shared_ptr<Chunk>& bottomChunk,
		const std::shared_ptr<Chunk>& rightBottomChunk
	)
	{
		constexpr int EMPTY_TYPE = -1;

		if (nx >= 0 && nx < chunk->tilesCols && ny >= 0 && ny < chunk->tilesRows) {
			return chunk->_tiles[ny * chunk->tilesCols + nx]->_type;
		}

		const int dx = (nx < 0) ? -1 : (nx >= chunk->tilesCols ? 1 : 0);
		const int dy = (ny < 0) ? -1 : (ny >= chunk->tilesRows ? 1 : 0);

		std::shared_ptr<Chunk> c = nullptr;

		if (dx == -1 && dy == -1) c = leftTopChunk;
		else if (dx == 0 && dy == -1) c = topChunk;
		else if (dx == 1 && dy == -1) c = rightTopChunk;

		else if (dx == -1 && dy == 0) c = leftChunk;
		else if (dx == 1 && dy == 0) c = rightChunk;

		else if (dx == -1 && dy == 1) c = leftBottomChunk;
		else if (dx == 0 && dy == 1) c = bottomChunk;
		else if (dx == 1 && dy == 1) c = rightBottomChunk;

		if (!c) return EMPTY_TYPE;

		int lx = nx;
		int ly = ny;

		if (lx < 0) lx += chunk->tilesCols;
		else if (lx >= chunk->tilesCols) lx -= chunk->tilesCols;

		if (ly < 0) ly += chunk->tilesRows;
		else if (ly >= chunk->tilesRows) ly -= chunk->tilesRows;
		return c->_tiles[ly * chunk->tilesCols + lx]->_type;
	}

	int getTileType(
		int type,
		int x, int y,
		std::shared_ptr<Chunk> chunk,
		std::shared_ptr<Chunk> leftTopChunk,
		std::shared_ptr<Chunk> topChunk,
		std::shared_ptr<Chunk> rightTopChunk,
		std::shared_ptr<Chunk> leftChunk,
		std::shared_ptr<Chunk> rightChunk,
		std::shared_ptr<Chunk> leftBottomChunk,
		std::shared_ptr<Chunk> bottomChunk,
		std::shared_ptr<Chunk> rightBottomChunk) {

		int tileType = 5; // domyślnie środek

		// Sprawdzanie sąsiadów z uwzględnieniem krawędzi
		bool hasLeft = areFriends(type, getNeighbourTypeLocal(
			x - 1, y, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		bool hasRight = areFriends(type, getNeighbourTypeLocal(
			x + 1, y, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		bool hasTop = areFriends(type, getNeighbourTypeLocal(
			x, y - 1, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		bool hasBottom = areFriends(type, getNeighbourTypeLocal(
			x, y + 1, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		// Narożniki:
		bool hasTopLeft = areFriends(type, getNeighbourTypeLocal(
			x - 1, y - 1, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		bool hasTopRight = areFriends(type, getNeighbourTypeLocal(
			x + 1, y - 1, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		bool hasBottomLeft = areFriends(type, getNeighbourTypeLocal(
			x - 1, y + 1, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		bool hasBottomRight = areFriends(type, getNeighbourTypeLocal(
			x + 1, y + 1, chunk,
			leftTopChunk, topChunk, rightTopChunk,
			leftChunk, rightChunk,
			leftBottomChunk, bottomChunk, rightBottomChunk
		));

		// Logika dobierania odpowiedniego fragmentu
		if (!hasTop && !hasLeft) {
			if (hasRight && hasBottom && !hasBottomRight) {
				tileType = 13; // wewnętrzny lewo-góra
			}
			else {
				tileType = 1; // standardowy lewo-góra
			}
		}
		else if (!hasTop && !hasRight) {
			tileType = 3; // prawo-góra
		}
		else if (!hasBottom && !hasLeft) {
			tileType = 7; // lewo-dół
		}
		else if (!hasBottom && !hasRight) {
			tileType = 9; // prawo-dół
		}
		else if (!hasTop) {
			tileType = 2; // góra
		}
		else if (!hasLeft) {
			tileType = 4; // lewo
		}
		else if (!hasRight) {
			tileType = 6; // prawo
		}
		else if (!hasBottom) {
			tileType = 8; // dół
		}
		else {
			// Dla pełnego otoczenia - sprawdź wewnętrzne narożniki
			if (!hasTopLeft && hasTop && hasLeft) {
				tileType = 11; // wewnętrzny lewo-góra dla środka
			}
			else if (!hasTopRight && hasTop && hasRight) {
				tileType = 12; // wewnętrzny prawo-góra dla środka
			}
			else if (!hasBottomLeft && hasBottom && hasLeft) {
				tileType = 13; // wewnętrzny lewo-dół dla środka
			}
			else if (!hasBottomRight && hasBottom && hasRight) {
				tileType = 14; // wewnętrzny prawo-dół dla środka
			}
			else {
				tileType = 5; // środek
			}
		}

		return tileType;
	}
}