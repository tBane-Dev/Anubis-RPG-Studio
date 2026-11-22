#pragma once
#include <SFML/Graphics.hpp>
#include "Objects/Object.hpp"
#include "Cursor.hpp"
#include "Objects/SelectedPlacedGameObject.hpp"
#include "Editors/MapEditor/Map/Water.hpp"
#include <map>
#include <set>

namespace MapEditor {
	class Chunk;
	class TerrainSnapshot;

	class CursorOnMap : public Cursors::CursorWithObject {
	public:
		std::vector<std::shared_ptr<SelectedPlacedGameObject>> _prevSelectedObjects;
		std::vector<std::shared_ptr<SelectedPlacedGameObject>> _selectedObjects;
		sf::Vector2i _prevPosition = sf::Vector2i(-1, -1);
		bool _isDragging = false;
		bool _isSelecting = false;
		sf::IntRect _selectionRect;			// in window pixels (screen space)
		sf::Vector3f _selectionStart;		// start of the selection on the terrain
		bool _selectionStartOnTerrain = false;
		sf::Clock _pickClock;				// limits how often hard functions are updated
		float _pickInterval = 100;			// 100 ms
		sf::Vector3f _worldPosition;		// point of the terrain under the cursor (any height)
		bool _isOnTerrain = false;			// false when the cursor does not hit the terrain


		// terrain brush (smooth terrain)
		bool _isPainting = false;			// left button is held with a terrain tool
		bool _heightsChanged = false;		// meshes of _chunksToRebuild need a rebuild (not only splat textures)
		float _flattenHeight = 0.f;			// Flatten tool: height under the cursor at the start of the stroke
		std::vector<std::shared_ptr<TerrainSnapshot>> _strokeSnapshots;	// chunks changed by the stroke (undo)
		std::set<std::shared_ptr<Chunk>> _chunksToRebuild;	// terrain brush: refreshed once per frame in update()

		// slope tool: LMB pressed at the top of the slope, released at the bottom
		bool _isSlopeDragging = false;
		sf::Vector3f _slopeStart;			// terrain point where the slope starts

		// waters
		float _newWaterDepth = 4.f;				// a new water starts this much above the clicked terrain
		std::shared_ptr<Water> _paintWater;		// water changed by the water brush / eraser
		WaterState _paintWaterOldState;
		bool _paintWaterExisted = false;
		bool _isDraggingWater = false;			// LMB on a selected water - its level follows the cursor
		sf::Vector3f _waterGrab;				// grabbed point of the water surface
		sf::Vector3f _waterDragNormal;
		int _dragStartMouseY = 0;
		std::vector<std::shared_ptr<Water>> _dragWaters;
		std::vector<float> _dragWaterLevels;
		std::vector<WaterState> _dragWaterStates;

		CursorOnMap();
		~CursorOnMap();

		void addToSelected(std::shared_ptr<PlacedGameObject> object, sf::Vector2i offset);
		void removeFromSelected(std::shared_ptr<GameObject> object);

		// Placement preview (translucent model of the prefab under the cursor).
		std::shared_ptr<Anubis::Model3D> _previewModel;
		std::weak_ptr<GameObject> _previewPrefab;

		sf::Vector2i getCursorTile();
		sf::Vector2i getPlacementPosition(std::shared_ptr<GameObject> prefab);
		bool isHeightTool();
		void handleTerrainEvent(const sf::Event& event);
		float getBrushRadius();
		void addToStroke(std::shared_ptr<Chunk> chunk);
		void paintTerrain();
		void endTerrainStroke();
		void useSlope();
		void drawTerrainBrush();

		bool isWaterTool();
		std::shared_ptr<Water> getWaterToEdit(int kind);
		void useWaterFill();
		void useWaterSeabed();
		void startWaterStroke();
		void endWaterStroke();
		void startWaterDrag(std::shared_ptr<Water> clickedWater);
		void dragWaters();
		void endWaterDrag();
		void drawPlacementPreview(std::shared_ptr<GameObject> prefab);

		std::shared_ptr<PlacedGameObject> getObjectUnderCursor();
		std::vector<std::shared_ptr<PlacedGameObject>> getObjectsInSelectionRect();
		void syncSceneSelection();

		virtual void update();
		virtual void handleEvent(const sf::Event& event);
		virtual void draw();
	};

}