#pragma once
#include <SFML/Graphics.hpp>
#include "GUIManager.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Editors/MapEditor/Map/GhostChunk.hpp"
#include "Editors/MapEditor/Map/Water.hpp"
#include "Loader.hpp"
#include <fstream>
#include "SoundsManager.hpp"

enum class ToolTerrainType;

namespace MapEditor {
	class Map : public Anubis::Element {
	public:

		std::shared_ptr<Texture> _texture;			// tileset.png - 8 terrains in one column
		std::vector<std::shared_ptr<Chunk>> _chunks;
		std::vector<std::shared_ptr<Chunk>> _visibleChunks;
		std::shared_ptr<GhostChunk> _ghostChunk;
		std::vector<std::shared_ptr<Water>> _waters;

		std::shared_ptr<sf::Sound> _oceanAmbience;					// loop sound of the ocean, stands on the nearest point of the ocean
		int _waveIndex;												// the next wave to play
		int _waveCycle;												// the cycle of the wave in the shader when the last wave was played
		sf::Time _waterSoundsTime;
		sf::Vector2f _oceanPoint;									// the nearest point of the ocean - the wave sound uses its phase

		Map();
		~Map();

		void addChunk(std::shared_ptr<Chunk> chunk, int terrain);
		void addSounds();
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
		void makeSlope(std::vector<std::shared_ptr<Chunk>> chunks, sf::Vector3f start, sf::Vector3f end, float radius);
		void shapeSeabed(std::shared_ptr<Water> water, float width, float depth);

		void removeGameObjectsFromChunksByPrefab(std::shared_ptr<GameObject> object);

		void setVisibleChunks();

		// Point of the terrain surface under a window pixel. Works for any terrain heights:
		// only the terrain is drawn into the scene's picking depth buffer and the depth is unprojected.
		bool pickTerrain(sf::Vector2i pixel, sf::Vector3f& hit);

		// Brush on the terrain surface (soft circle or square). start != end: along the line (slope tool).
		void drawBrushOverlay(sf::Vector2f start, float radius, bool square, sf::Color color, sf::Vector2f end);

		// Terrain height at a map point (0 outside the map).
		float getHeight(float x, float z);

		// WATERS
		void addWater(std::shared_ptr<Water> water);
		void removeWater(std::shared_ptr<Water> water);
		std::shared_ptr<Water> getWaterUnderCursor(sf::Vector2i pixel);
		void drawWaters(sf::Glsl::Mat4 view, sf::Glsl::Mat4 projection);
		float getDistance(sf::Vector2f position);
		float getDistanceToWater(sf::Vector2f position, int kind, sf::Vector2f& nearest);

		void saveFromProject(std::ofstream& file);
		void loadFromProject(std::ifstream& file, std::shared_ptr<Anubis::Loader>& loader);

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();
	};

}
