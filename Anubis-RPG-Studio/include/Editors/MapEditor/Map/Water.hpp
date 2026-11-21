#pragma once
#include <SFML/Graphics.hpp>
#include "Objects/PlacedGameObject.hpp"
#include "3D/Mesh3D.hpp"
#include <cstdint>

namespace MapEditor {

	class Chunk;

	// Part of a water in one chunk. Masks have one value per weight point of the chunk (65 x 65, every 4 world units).
	class WaterChunk {
	public:
		sf::Vector2i _coords;					// coords of the chunk
		std::vector<std::uint8_t> _fill;		// from the Fill tool (counted again when the level changes)
		std::vector<std::uint8_t> _added;		// painted with the water brush
		std::vector<std::uint8_t> _removed;		// painted with the eraser
		std::vector<std::uint8_t> _shore;		// distance to the shore / 2 (0 - 255 = 0 - 510 world units), for the foam
		std::shared_ptr<sf::Texture> _maskTexture;
		std::shared_ptr<Anubis::Mesh3D> _mesh;

		WaterChunk(sf::Vector2i coords);
		~WaterChunk();

		bool isEmpty();
		std::uint8_t getMask(int point);
	};

	// Everything needed to save a water or to bring it back (undo).
	class WaterState {
	public:
		int _kind = 0;
		float _level = 0.f;
		std::vector<sf::Vector2f> _seeds;
		std::vector<sf::Vector2i> _coords;
		std::vector<std::vector<std::uint8_t>> _fill;
		std::vector<std::vector<std::uint8_t>> _added;
		std::vector<std::vector<std::uint8_t>> _removed;
	};

	// Flat water surface (ocean, swamp, lava) - separate from the terrain mesh.
	// It is a PlacedGameObject, so it can be selected like the other objects (shared_ptr in _selectedObjects).
	class Water : public PlacedGameObject {
	public:
		int _kind;					// 0 = ocean, 1 = swamp, 2 = lava
		float _level;				// height of the water surface
		std::vector<sf::Vector2f> _seeds;	// points of the Fill tool (world x, z)
		std::vector<std::shared_ptr<WaterChunk>> _chunks;

		Water(int kind, float level);
		~Water();

		virtual sf::Vector2f getPosition();
		virtual void setPosition(sf::Vector2f position);

		std::shared_ptr<WaterChunk> getWaterChunk(sf::Vector2i coords, bool create);
		float getMask(float x, float z);
		bool isWaterAt(float x, float z);
		bool getRayHit(sf::Vector3f origin, sf::Vector3f dir, sf::Vector3f& hit);

		void setMaskPoint(int gx, int gy, int layer, std::uint8_t value);
		void fill();
		void paint(sf::Vector2f center, float radius, bool erase, float amount);
		void removeEmptyChunks();

		void updateShoreDistances();
		void updateChunk(std::shared_ptr<WaterChunk> waterChunk);
		void updateAllChunks();
		void updateChunksOf(std::vector<sf::Vector2i> coords);

		WaterState getState();
		void setState(WaterState state);

		void draw();
	};

}
