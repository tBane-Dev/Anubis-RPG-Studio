#include "History.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "DebugLog.hpp"
#include "Editors/MapEditor/Map/Tile.hpp"
#include "Editors/MapEditor/Map/Chunk.hpp"
#include "Editors/MapEditor/Map/Map.hpp"
#include <set>

Step::Step() {

}

Step::~Step() {

}

void Step::undo() {

}

void Step::redo() {

}

EditTerrain::EditTerrain(std::vector<std::shared_ptr<MapEditor::TerrainSnapshot>> snapshots) {
    _snapshots = snapshots;
}

EditTerrain::~EditTerrain() {

}

void EditTerrain::undo() {

    std::vector<std::shared_ptr<MapEditor::Chunk>> chunks;

    for (auto& snapshot : _snapshots) {
        snapshot->_chunk->_heights = snapshot->_oldHeights;
        snapshot->_chunk->_weights = snapshot->_oldWeights;
        chunks.push_back(snapshot->_chunk);
    }

    MapEditor::editor->_map->refreshChunks(chunks, true);
}

void EditTerrain::redo() {

    std::vector<std::shared_ptr<MapEditor::Chunk>> chunks;

    for (auto& snapshot : _snapshots) {
        snapshot->_chunk->_heights = snapshot->_newHeights;
        snapshot->_chunk->_weights = snapshot->_newWeights;
        chunks.push_back(snapshot->_chunk);
    }

    MapEditor::editor->_map->refreshChunks(chunks, true);
}


AddGameObjectOnMap::AddGameObjectOnMap(std::shared_ptr<PlacedGameObject> object, sf::Vector2i position) : Step() {
	_object = object;
	_position = position;
}

AddGameObjectOnMap::~AddGameObjectOnMap() {

}

void AddGameObjectOnMap::undo() {
	MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(_object->getPosition()))->removePlacedGameObject(_object);
	MapEditor::editor->setVisibleChunks();
}

void AddGameObjectOnMap::redo() {
	_object->setPosition(sf::Vector2f(_position));
	MapEditor::editor->_map->getChunkByGlobalPosition(_position)->addPlacedGameObject(_object);
	MapEditor::editor->setVisibleChunks();
}

RemoveGameObjectsOnMap::RemoveGameObjectsOnMap(std::vector<std::shared_ptr<SelectedPlacedGameObject>> objects) : Step() {
    _objects = objects;
}

RemoveGameObjectsOnMap::~RemoveGameObjectsOnMap() {

}

void RemoveGameObjectsOnMap::undo() {
    for (auto& object : _objects) {
        if (auto obj = object) {
            MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(obj->_object->getPosition()))->addPlacedGameObject(obj->_object);
        }
    }
    MapEditor::editor->setVisibleChunks();
}

void RemoveGameObjectsOnMap::redo() {
    for (auto& object : _objects) {
        if (auto obj = object) {
            MapEditor::editor->_game_objects->removeGameObject(obj->_object);
            for (auto& chunk : MapEditor::editor->_map->_chunks) {
                chunk->removePlacedGameObject(obj->_object);
            }
        }
    }
}

EditWater::EditWater(std::shared_ptr<MapEditor::Water> water, MapEditor::WaterState oldState, bool oldExists, MapEditor::WaterState newState, bool newExists) {
    _water = water;
    _oldState = oldState;
    _oldExists = oldExists;
    _newState = newState;
    _newExists = newExists;
}

EditWater::~EditWater() {

}

void EditWater::apply(MapEditor::WaterState state, bool exists) {

    if (!exists) {
        MapEditor::editor->_map->removeWater(_water);
        return;
    }

    MapEditor::editor->_map->addWater(_water);
    _water->setState(state);
}

void EditWater::undo() {
    apply(_oldState, _oldExists);
}

void EditWater::redo() {
    apply(_newState, _newExists);
}

MoveGameObjectsOnMap::MoveGameObjectsOnMap(std::vector<std::shared_ptr<SelectedPlacedGameObject>> objects) : Step() {
    _objects = objects;

    for (auto& obj : _objects) {
        _oldPositions.push_back(obj->_prevPosition);
        _newPositions.push_back(sf::Vector2i(obj->_object->getPosition()));
    }
}

MoveGameObjectsOnMap::~MoveGameObjectsOnMap() {

}

void MoveGameObjectsOnMap::undo() {

    for (int i = 0; i < _objects.size(); i++) {
        MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(_objects[i]->_object->getPosition()))->removePlacedGameObject(_objects[i]->_object);
        _objects[i]->_object->setPosition(sf::Vector2f(_oldPositions[i]));
        MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(_objects[i]->_object->getPosition()))->addPlacedGameObject(_objects[i]->_object);
    }

    MapEditor::editor->setVisibleChunks();
}

void MoveGameObjectsOnMap::redo() {

    for (int i = 0; i < _objects.size(); i++) {
        MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(_objects[i]->_object->getPosition()))->removePlacedGameObject(_objects[i]->_object);
        _objects[i]->_object->setPosition(sf::Vector2f(_newPositions[i]));
        MapEditor::editor->_map->getChunkByGlobalPosition(sf::Vector2i(_objects[i]->_object->getPosition()))->addPlacedGameObject(_objects[i]->_object);
    }

    MapEditor::editor->setVisibleChunks();
}

History::History() {

}

History::~History() {

}

bool History::canUndo() {
    return _current >= 0 || !MapEditor::editor->_cursor_on_map->_selectedObjects.empty();
}

bool History::canRedo() {
    return _current < (int)(_steps.size()) - 1;
}

void History::undo() {

    if (!canUndo())
        return;

    if (!MapEditor::editor->_cursor_on_map->_selectedObjects.empty()) {
        for (auto& selected : MapEditor::editor->_cursor_on_map->_selectedObjects) {
            selected->_object->_isSelected = false;
        }

        MapEditor::editor->_cursor_on_map->_selectedObjects.clear();
        return;
    }

    _steps[_current]->undo();
    _current -= 1;
}

void History::redo() {
    if (!canRedo())
        return;

    _current += 1;
    _steps[_current]->redo();
}

void History::addStep(std::shared_ptr<Step> step) {

    std::wstring type = L"none";
    if(std::dynamic_pointer_cast<AddGameObjectOnMap>(step)) type = L"Add";
    if(std::dynamic_pointer_cast<RemoveGameObjectsOnMap>(step)) type = L"Remove";
    if(std::dynamic_pointer_cast<MoveGameObjectsOnMap>(step)) type = L"Move";
    if(std::dynamic_pointer_cast<EditTerrain>(step)) type = L"Edit Terrain";
    DebugLog(L"Add Step: " + type);


    if (_current + 1 < (int)_steps.size()) {
        // Parentheses matter: begin() + _current would be begin() - 1 when _current == -1.
        _steps.erase(
            _steps.begin() + (_current + 1),
            _steps.end()
        );
    }

    _steps.push_back(std::move(step));
    _current += 1;
}

std::shared_ptr<History> history = nullptr;