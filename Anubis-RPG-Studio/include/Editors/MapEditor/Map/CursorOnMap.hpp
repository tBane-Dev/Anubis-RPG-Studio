#pragma once
#include <SFML/Graphics.hpp>
#include "Objects/Object.hpp"
#include "Cursor.hpp"
#include "Objects/SelectedPlacedGameObject.hpp"
#include <map>
#include <set>

namespace MapEditor {
	class Chunk;

	class CursorOnMap : public Cursors::CursorWithObject {
	public:
		std::vector<std::shared_ptr<SelectedPlacedGameObject>> _prevSelectedObjects;
		std::vector<std::shared_ptr<SelectedPlacedGameObject>> _selectedObjects;
		sf::Vector2i _prevPosition = sf::Vector2i(-1, -1);
		bool _isDragging = false;
		bool _isSelecting = false;
		sf::IntRect _selectionRect;			// in window pixels (screen space)
		sf::Clock _pickClock;				// limits how often hard functions are updated
		float _pickInterval = 100;			// 100 ms
		sf::Vector3f _worldPosition;		// point of the terrain under the cursor (any height)
		bool _isOnTerrain = false;			// false when the cursor does not hit the terrain


		struct TileValues { int oldValue; int newValue; };
		struct Vector2iCompare {
			bool operator()(const sf::Vector2i& a, const sf::Vector2i& b) const {
				return a.x < b.x || (a.x == b.x && a.y < b.y);
			}
		};
		std::map<sf::Vector2i, MapEditor::CursorOnMap::TileValues, Vector2iCompare> _editedTiles;
		std::set<std::shared_ptr<Chunk>> _chunksToRebuild;	// terrain brush: rebuilt once per frame in update()

		CursorOnMap();
		~CursorOnMap();

		void addToSelected(std::shared_ptr<PlacedGameObject> object, sf::Vector2i offset);
		void removeFromSelected(std::shared_ptr<GameObject> object);

		// Placement preview (translucent model of the prefab under the cursor).
		std::shared_ptr<Anubis::Model3D> _previewModel;
		std::weak_ptr<GameObject> _previewPrefab;

		sf::Vector2i getCursorTile();
		sf::Vector2i getPlacementPosition(std::shared_ptr<GameObject> prefab);
		void drawTerrainBrush();
		void drawPlacementPreview(std::shared_ptr<GameObject> prefab);
		void drawPreviewSprite(std::shared_ptr<GameObject> prefab, sf::Vector2i position);

		std::shared_ptr<PlacedGameObject> getObjectUnderCursor();
		std::vector<std::shared_ptr<PlacedGameObject>> getObjectsInSelectionRect();
		void syncSceneSelection();

		virtual void update();
		virtual void handleEvent(const sf::Event& event);
		virtual void draw();
	};

}