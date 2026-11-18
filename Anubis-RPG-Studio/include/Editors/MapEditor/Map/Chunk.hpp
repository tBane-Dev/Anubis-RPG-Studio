#pragma once
#include <SFML/Graphics.hpp>
#include "Editors/MapEditor/Map/Tile.hpp"
#include "Objects/PlacedGameObject.hpp"
#include "3D/Mesh3D.hpp"
#include <array>
#include <cstdint>

namespace MapEditor {

	// Smooth terrain (no tiles):
	// - heights: grid of points every 8 world units (33 x 33 per chunk),
	// - terrains: 8 weights in a grid of points every 4 world units (65 x 65 per chunk).
	// Points on the edge are shared - both chunks keep the same value, so there are no cracks.
	class Chunk : public std::enable_shared_from_this<Chunk> {
	public:
		const static int tilesCols = 16;
		const static int tilesRows = 16;
		const static int heightsCols = 33;
		const static int weightsCols = 65;
		const static int terrainsCount = 8;

		sf::Vector2i _coords;
		std::vector<float> _heights;								// heightsCols * heightsCols
		std::vector<std::uint8_t> _weights;							// weightsCols * weightsCols * terrainsCount, sum of one point = 255
		std::shared_ptr<Anubis::Mesh3D> _mesh;
		std::vector<std::shared_ptr<sf::Texture>> _splatTextures;	// [0] = weights of terrains 0-3, [1] = terrains 4-7

		bool _isVisible;
		std::unique_ptr<sf::Text> _coordsText;

		std::vector<std::shared_ptr<PlacedGameObject>> _gameObjectsOnMap;
		sf::IntRect _gameObjectsOnMapRect;

		Chunk(int x, int y);
		~Chunk();

		bool operator<(const Chunk& other) const;

		float getSize();
		float getHeightStep();
		float getWeightStep();
		sf::Vector2f getWorldPosition();

		void fill(int terrain, float height);
		void setWeights(int point, std::array<float, 8> weights);
		float getHeightPoint(int x, int y);
		float getHeight(float x, float z);

		void updateSplatTextures();
		void generateVertexBuffer();

		sf::IntRect getRect();
		void calculatePlacedGameObjectRect();
		sf::IntRect getPlacedGameObjectsRect();

		void addPlacedGameObject(std::shared_ptr<PlacedGameObject> placedGameObject);
		void removePlacedGameObject(std::shared_ptr<PlacedGameObject> placedGameObject);

		void setVisible();

		void drawCoords();
		void draw();
	};

	// Terrain of one chunk before and after a brush stroke (undo / redo).
	class TerrainSnapshot {
	public:
		std::shared_ptr<Chunk> _chunk;
		std::vector<float> _oldHeights;
		std::vector<float> _newHeights;
		std::vector<std::uint8_t> _oldWeights;
		std::vector<std::uint8_t> _newWeights;
	};

}
