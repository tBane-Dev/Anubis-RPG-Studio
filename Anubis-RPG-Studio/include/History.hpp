#pragma once
#include "SFML/Graphics.hpp"
#include "Objects/PlacedGameObject.hpp"
#include "Objects/SelectedPlacedGameObject.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "Editors/MapEditor/Map/CursorOnMap.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Editors/MapEditor/Map/Water.hpp"

class Step : public std::enable_shared_from_this<Step> {
public:
	Step();
	virtual ~Step();
	virtual void undo();
	virtual void redo();
};

class EditTerrain : public Step {
public:
	std::vector<std::shared_ptr<MapEditor::TerrainSnapshot>> _snapshots;

	EditTerrain(std::vector<std::shared_ptr<MapEditor::TerrainSnapshot>> snapshots);
	~EditTerrain();
	virtual void undo();
	virtual void redo();
};

// Any change of a water: Fill, brush, eraser, level, add, remove (exists = false when the water is not on the map).
class EditWater : public Step {
public:
	std::shared_ptr<MapEditor::Water> _water;
	MapEditor::WaterState _oldState;
	MapEditor::WaterState _newState;
	bool _oldExists;
	bool _newExists;

	EditWater(std::shared_ptr<MapEditor::Water> water, MapEditor::WaterState oldState, bool oldExists, MapEditor::WaterState newState, bool newExists);
	~EditWater();
	void apply(MapEditor::WaterState state, bool exists);
	virtual void undo();
	virtual void redo();
};

class AddGameObjectOnMap : public Step {
public:

	std::shared_ptr<PlacedGameObject> _object;
	sf::Vector2i _position;

	AddGameObjectOnMap(std::shared_ptr<PlacedGameObject> object, sf::Vector2i position);
	~AddGameObjectOnMap();
	virtual void undo();
	virtual void redo();
};

class RemoveGameObjectsOnMap : public Step {
public:

	std::vector<std::shared_ptr<SelectedPlacedGameObject>> _objects;

	RemoveGameObjectsOnMap(std::vector<std::shared_ptr<SelectedPlacedGameObject>> objects);
	~RemoveGameObjectsOnMap();
	virtual void undo();
	virtual void redo();
};

class MoveGameObjectsOnMap : public Step {
public:

	std::vector<std::shared_ptr<SelectedPlacedGameObject>> _objects;
	std::vector<sf::Vector2i> _oldPositions;
	std::vector<sf::Vector2i> _newPositions;

	MoveGameObjectsOnMap(std::vector<std::shared_ptr<SelectedPlacedGameObject>> objects);
	~MoveGameObjectsOnMap();
	virtual void undo();
	virtual void redo();
};

class History {
public:
	std::vector<std::shared_ptr<Step>> _steps;
	int _current = -1;

	History();
	~History();
	bool canUndo();
	bool canRedo();
	void undo();
	void redo();
	void addStep(std::shared_ptr<Step> step);
};

extern std::shared_ptr<History> history;