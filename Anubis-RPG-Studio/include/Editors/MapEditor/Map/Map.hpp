#pragma once
#include <SFML/Graphics.hpp>
#include "GUIManager.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Editors/MapEditor/Map/GhostChunk.hpp"
#include "Loader.hpp"
#include <fstream>

enum class ToolTerrainType;

namespace MapEditor {
	class Map : public Anubis::Element {
	public:

		std::shared_ptr<Texture> _texture;			// tileset.png - 8 terrains in one column
		std::vector<std::shared_ptr<Chunk>> _chunks;
		std::vector<std::shared_ptr<Chunk>> _visibleChunks;
		std::shared_ptr<GhostChunk> _ghostChunk;

		Map();
		~Map();

		void addChunk(std::shared_ptr<Chunk> chunk, int terrain);
		void create(int width, int height);

		void drawCircle(sf::Vector2f center, float radius, int terrain);

		std::shared_ptr<Chunk> getChunkByCoords(int x, int y);
		std::shared_ptr<Chunk> getChunkByTileGlobalCoords(int x, int y);
		std::shared_ptr<Chunk> getChunkByGlobalPosition();
		std::shared_ptr<Chunk> getChunkByGlobalPosition(sf::Vector2i position);
		sf::IntRect getRect();
		std::vector<std::shared_ptr<Chunk>> getChunksInRect(sf::Vector2f center, float radius);

		// SMOOTH TERRAIN
		void stitchChunk(std::shared_ptr<Chunk> chunk);
		void refreshChunks(std::vector<std::shared_ptr<Chunk>> chunks, bool heightsChanged);
		float getBrushFalloff(sf::Vector2f delta, float radius, bool square);
		void paintTerrain(std::vector<std::shared_ptr<Chunk>> chunks, sf::Vector2f center, float radius, bool square, int terrain, float amount);
		void changeHeights(std::vector<std::shared_ptr<Chunk>> chunks, sf::Vector2f center, float radius, bool square, ToolTerrainType tool, float amount, float flattenHeight);
		void convertOldTiles();

		void removeGameObjectsFromChunksByPrefab(std::shared_ptr<GameObject> object);

		void setVisibleChunks();

		// Point of the terrain surface under a window pixel. Works for any terrain heights:
		// only the terrain is drawn into the scene's picking depth buffer and the depth is unprojected.
		bool pickTerrain(sf::Vector2i pixel, sf::Vector3f& hit);

		// Brush on the terrain surface (soft circle or square).
		void drawBrushOverlay(sf::Vector2f center, float radius, bool square, sf::Color color);

		// Terrain height at a map point (0 outside the map).
		float getHeight(float x, float z);

		void saveFromProject(std::ofstream& file);
		void loadFromProject(std::ifstream& file, std::shared_ptr<Anubis::Loader>& loader);

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();
	};

}
