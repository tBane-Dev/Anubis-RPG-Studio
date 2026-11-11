#pragma once
#include <SFML/Graphics.hpp>
#include "GUIManager.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Editors/MapEditor/Map/GhostChunk.hpp"
#include "Loader.hpp"
#include <fstream>

namespace MapEditor {
	class Map : public Element {
	public:

		std::shared_ptr<Texture> _texture;
		std::vector<std::shared_ptr<Chunk>> _chunks;
		std::vector<std::shared_ptr<Chunk>> _visibleChunks;
		std::shared_ptr<GhostChunk> _ghostChunk;

		Map();
		~Map();

		void addChunk(std::shared_ptr<Chunk> chunk, int tileType);
		void create(int width, int height);

		void drawCircle(sf::Vector2i center, int radius, int type);

		std::shared_ptr<Chunk> getChunkByCoords(int x, int y);
		std::shared_ptr<Chunk> getChunkByTileGlobalCoords(int x, int y);
		std::shared_ptr<Chunk> getChunkByGlobalPosition();
		std::shared_ptr<Chunk> getChunkByGlobalPosition(sf::Vector2i position);
		std::shared_ptr<Tile> getTileByTileGlobalCoords(int x, int y);
		sf::IntRect getRect();

		void removeGameObjectsFromChunksByPrefab(std::shared_ptr<GameObject> object);

		void setVisibleChunks();

		// Point of the terrain surface under a window pixel. Works for any terrain heights:
		// only the terrain is drawn into the scene's picking depth buffer and the depth is unprojected.
		bool pickTerrain(sf::Vector2i pixel, sf::Vector3f& hit);

		// Marks tiles on the terrain surface (terrain brush, tile under a placed prefab).
		// mask has size.x * size.y values (row by row), 1 = marked. firstTile is in global tile coords.
		void drawTileOverlay(sf::Vector2i firstTile, sf::Vector2i size, const std::vector<float>& mask, sf::Color color);

		// Terrain height at a map point. TO-DO - read it from tile heights once the map stores them.
		float getHeight(float x, float z);

		void saveFromProject(std::ofstream& file);
		void loadFromProject(std::ifstream& file, std::shared_ptr<Anubis::Loader>& loader);

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();
	};

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
	);

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
		std::shared_ptr<Chunk> rightBottomChunk
	);

}
