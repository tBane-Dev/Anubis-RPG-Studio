#include "Editors/MapEditor/Map/GhostChunk.hpp"
#include "Editors/MapEditor/Map/Tile.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Editors/MapEditor/Map/Map.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "Editors/MapEditor/Map/CursorOnMap.hpp"
#include "Objects/Monster.hpp"
#include "DebugLog.hpp"
#include "Cursor.hpp"
#include "RenderWindow.hpp"
#include "ShadersManager.hpp"
#include "Time.hpp"

namespace MapEditor {

	GhostChunkButton::GhostChunkButton(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position) : ButtonWithSprite(texture, hoverTexture, pressTexture, inactiveTexture, position) {
		
	}

	GhostChunkButton::~GhostChunkButton() {

	}

	void GhostChunkButton::cursorHover() {

		if (GUI_manager->Element_pressed != nullptr) {
			if (GUI_manager->Element_pressed.get() != this)
				return;
		}

		std::shared_ptr<CameraOnMap> camera = MapEditor::editor->_camera;

		if (!camera)
			return;

		sf::View view = camera->_view;
		sf::Vector2f size = view.getSize();
		sf::Vector2f center = view.getCenter();
		sf::Vector2u windowSize = Main::render_window->getSize();
		sf::FloatRect viewport = view.getViewport();

		if (windowSize.x == 0 || windowSize.y == 0 ||
			size.x <= 0.f || size.y <= 0.f)
			return;

		float viewportX = viewport.position.x * windowSize.x;
		float viewportY = viewport.position.y * windowSize.y;
		float viewportWidth = viewport.size.x * windowSize.x;
		float viewportHeight = viewport.size.y * windowSize.y;

		if (viewportWidth <= 0.f || viewportHeight <= 0.f)
			return;

		sf::Vector2i mouse = sf::Mouse::getPosition(*Main::render_window);

		float u = (mouse.x - viewportX) / viewportWidth;
		float v = (mouse.y - viewportY) / viewportHeight;

		if (u < 0.f || u > 1.f || v < 0.f || v > 1.f)
			return;

		float aspect = float(windowSize.x) / windowSize.y;
		float distance = (size.y * 0.5f) / std::tan(60.f * 0.5f * 3.14159265f / 180.f);

		float angle = sf::degrees(camera->_angle).asRadians();
		float s = std::sin(angle);
		float c = std::cos(angle);
		float f = 1.7320508f;

		float ndcX = u * 2.f - 1.f;
		float screenY = v * 2.f - 1.f; // Dodatnie w dół.

		float denominator = f * c + screenY * s;

		if (std::abs(denominator) < 0.000001f)
			return;

		float py = screenY * distance / denominator;
		float depth = distance - py * s;

		if (depth <= 0.1f)
			return;

		float px = ndcX * aspect * depth / f;

		sf::Vector2f mapPosition(center.x + px, center.y + py);

		if (!sf::FloatRect(_rect).contains(mapPosition))
			return;

		if (!_isActive)
			return;

		GUI_manager->Element_hovered = this->shared_from_this();
	}


	void GhostChunkButton::draw() {

		std::shared_ptr<Texture> texture;

		if (_isActive) {
			switch (_state)
			{
			case ButtonState::Idle:
				texture = _texture;
				break;
			case ButtonState::Hover:
				texture = _hoverTexture;
				break;
			case ButtonState::Pressed:
				texture = _pressTexture;
				break;
			default:
				texture = _texture;
				break;
			}
		}
		else {
			texture = _inactiveTexture;
		}

		sf::RenderStates states;
		const sf::View& view = Main::render_window->getView();
		sf::Vector2f size = view.getSize();
		sf::Vector2u windowSize = Main::render_window->getSize();
		float aspect = (float)(windowSize.x) / windowSize.y;
		float fov = 60.0f;
		float distance = (size.y * 0.5f) / std::tan(fov * 0.5f * 3.14159265f / 180.0f);
		sf::Vector2f position = sf::Vector2f(_rect.position);
		ghost_chunk_shader->setUniform("cameraCenter", view.getCenter());
		ghost_chunk_shader->setUniform("cameraDistance", distance);
		ghost_chunk_shader->setUniform("cameraAngle", MapEditor::editor->_camera->_angle);
		ghost_chunk_shader->setUniform("aspect", aspect);
		ghost_chunk_shader->setUniform("position", position);
		states.texture = &*texture->_texture;
		states.shader = &*ghost_chunk_shader;

		sf::Sprite sprite(*texture->_texture);
		Main::render_window->draw(sprite, states);

	}

	GhostChunk::GhostChunk() {

		_isVisible = false;

		_rect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(Chunk::tilesCols * Tile::tileSize, Chunk::tilesRows * Tile::tileSize));

		_button = std::make_shared<GhostChunkButton>(
			textures_manager->getTexture(L"assets\\tex\\ghost_chunk_plus.png"),
			textures_manager->getTexture(L"assets\\tex\\ghost_chunk_plus_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\ghost_chunk_plus_press.png"),
			textures_manager->getTexture(L"assets\\tex\\ghost_chunk_plus_press.png")
		);
	}

	GhostChunk::~GhostChunk() {

	}

	void GhostChunk::cursorHover() {

		std::shared_ptr<CameraOnMap> camera = MapEditor::editor->_camera;
		std::shared_ptr<CursorOnMap> cursor = MapEditor::editor->_cursor_on_map;

		if (!camera)
			return;

		if (cursor->_selectionRect.size.x != 0 ||
			cursor->_selectionRect.size.y != 0)
			return;

		if (cursor->_isSelecting || cursor->_isDragging)
			return;

		sf::View view = camera->_view;
		sf::Vector2f size = view.getSize();
		sf::Vector2f center = view.getCenter();
		sf::Vector2u windowSize = Main::render_window->getSize();
		sf::FloatRect viewport = view.getViewport();

		if (windowSize.x == 0 || windowSize.y == 0 || size.x <= 0.f || size.y <= 0.f)
			return;

		sf::Vector2i mouse = sf::Mouse::getPosition(*Main::render_window);

		sf::IntRect windowRect(sf::Vector2i(0, 0), sf::Vector2i(windowSize));

		if (!windowRect.contains(mouse))
			return;

		float viewportX = viewport.position.x * windowSize.x;
		float viewportY = viewport.position.y * windowSize.y;
		float viewportWidth = viewport.size.x * windowSize.x;
		float viewportHeight = viewport.size.y * windowSize.y;

		if (viewportWidth <= 0.f || viewportHeight <= 0.f)
			return;

		float u = (mouse.x - viewportX) / viewportWidth;
		float v = (mouse.y - viewportY) / viewportHeight;

		if (u < 0.f || u >= 1.f || v < 0.f || v >= 1.f)
			return;

		float aspect = float(windowSize.x) / windowSize.y;
		float distance = (size.y * 0.5f) / std::tan(60.f * 0.5f * 3.14159265f / 180.f);

		float angle = sf::degrees(camera->_angle).asRadians();
		float s = std::sin(angle);
		float c = std::cos(angle);
		float f = 1.7320508f;

		float ndcX = u * 2.f - 1.f;
		float screenY = v * 2.f - 1.f;

		 float denominator = f * c + screenY * s;

		if (std::abs(denominator) < 0.000001f)
			return;

		float py = screenY * distance / denominator;
		float depth = distance - py * s;

		if (depth <= 0.1f)
			return;

		float px = ndcX * aspect * depth / f;

		// Pozycja kursora na mapie przed projekcją.
		sf::Vector2f mapPosition(center.x + px, center.y + py);

		int chunkWidth = Chunk::tilesCols * Tile::tileSize;
		int chunkHeight = Chunk::tilesRows * Tile::tileSize;

		sf::Vector2i coords;
		coords.x = int(std::floor(mapPosition.x / chunkWidth));
		coords.y = int(std::floor(mapPosition.y / chunkHeight));
		if (
			MapEditor::editor->_map->getChunkByCoords(coords.x, coords.y) == nullptr && (
				MapEditor::editor->_map->getChunkByCoords(coords.x - 1, coords.y) ||
				MapEditor::editor->_map->getChunkByCoords(coords.x + 1, coords.y) ||
				MapEditor::editor->_map->getChunkByCoords(coords.x, coords.y - 1) ||
				MapEditor::editor->_map->getChunkByCoords(coords.x, coords.y + 1))
			)
		{

			_rect.position = sf::Vector2i(
				coords.x * chunkWidth,
				coords.y * chunkHeight
			);
			_button->setPosition(_rect.position + _rect.size / 2 - sf::Vector2i(24, 24));
			_button->_onclick_func = [this, coords]() {

				int offsetX = 0;
				int offsetY = 0;

				if (coords.x < 0) offsetX = -coords.x;
				if (coords.y < 0) offsetY = -coords.y;

				int pixelOffsetX = offsetX * Chunk::tilesCols * Tile::tileSize;
				int pixelOffsetY = offsetY * Chunk::tilesRows * Tile::tileSize;

				// move the camera
				if (offsetX != 0) MapEditor::editor->_camera->_position.x += (float)pixelOffsetX;
				if (offsetY != 0) MapEditor::editor->_camera->_position.y += (float)pixelOffsetY;

				
				MapEditor::editor->_camera->update();
				MapEditor::editor->_camera->setView();

				// move the gameobjects
				for (auto& chunk : MapEditor::editor->_map->_chunks) {
					for (auto& go : chunk->_gameObjectsOnMap) {

						if (go->_type == ObjectType::Wall) // because walls are moved by Building::setPosition()
							continue;

						sf::Vector2i pos = go->getPosition();
						if (offsetX != 0) pos.x += pixelOffsetX;
						if (offsetY != 0) pos.y += pixelOffsetY;
						go->setPosition(pos);

						if (go->_type == ObjectType::Monster) {
							std::shared_ptr<Monster> monster = std::dynamic_pointer_cast<Monster>(go);
							monster->_position.x += pixelOffsetX;
							monster->_position.y += pixelOffsetY;
						}

					}
				}


				std::shared_ptr<Map> map = MapEditor::editor->_map;

				// move existing chunks if new chunk would have negative coords
				if (offsetX != 0 || offsetY != 0) {
					for (auto& c : map->_chunks) {
						c->_coords.x += offsetX;
						c->_coords.y += offsetY;

						for (auto& tile : c->_tiles) {
							tile->_coords.x += offsetX * Chunk::tilesCols;
							tile->_coords.y += offsetY * Chunk::tilesRows;

							tile->_position.x = (float)(tile->_coords.x * Tile::tileSize);
							tile->_position.y = (float)(tile->_coords.y * Tile::tileSize);
						}
					}
				}

				std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>(
					coords.x + offsetX,
					coords.y + offsetY
				);

				map->addChunk(newChunk, 2);

				// refresh all chunks, because existing chunks may have changed position
				for (auto& c : map->_chunks) {
					int x = c->_coords.x;
					int y = c->_coords.y;

					c->generateVertexBuffer(
						map->getChunkByCoords(x - 1, y - 1), map->getChunkByCoords(x, y - 1), map->getChunkByCoords(x + 1, y - 1),
						map->getChunkByCoords(x - 1, y), map->getChunkByCoords(x + 1, y),
						map->getChunkByCoords(x - 1, y + 1), map->getChunkByCoords(x, y + 1), map->getChunkByCoords(x + 1, y + 1)
					);

				}


				
				MapEditor::editor->setVisibleChunks();
			};

			if (sf::FloatRect(_rect).contains(mapPosition)) {
				GUI_manager->Element_hovered = shared_from_this();

				// Manual cursor hover for button.
				if (sf::FloatRect(_button->_rect).contains(mapPosition)) {
					GUI_manager->Element_hovered = _button;
				}
			}
		}

	}

	void GhostChunk::handleEvent(const sf::Event& event) {

		_button->handleEvent(event);
	}

	void GhostChunk::update() {

		if (GUI_manager->Element_pressed != nullptr &&
			GUI_manager->Element_pressed != _button &&
			GUI_manager->Element_pressed != MapEditor::editor->_map
			) {
			_isVisible = false;
			return;
		}

		if (MapEditor::editor->_cursor_on_map->_state == Cursors::CursorWithObjectState::Drawing) {
			_isVisible = false;
			return;
		}

		_isVisible = (GUI_manager->Element_hovered.get() == this || GUI_manager->Element_hovered == _button);

		if (_isVisible)
			_button->update();
	}

	void GhostChunk::draw() {
		if (!_isVisible)
			return;

		

		int borderWidth = 4;
		sf::Vector2f position = sf::Vector2f(_rect.position.x + borderWidth, _rect.position.y + borderWidth);
		ghost_chunk_shader->setUniform("position", position);
		sf::RenderStates states;
		states.shader = &*ghost_chunk_shader;

		sf::RectangleShape rect(sf::Vector2f(Chunk::tilesCols * Tile::tileSize - 2 * borderWidth, Chunk::tilesRows * Tile::tileSize - 2 * borderWidth));
		rect.setPosition(sf::Vector2f(0,0));
		rect.setFillColor(sf::Color(63, 63, 63, 127));
		rect.setOutlineThickness(borderWidth);
		rect.setOutlineColor(sf::Color(31, 31, 31, 255));
		Main::render_window->draw(rect, states);

		_button->draw();
	}
}