#include "Objects/WaterPrefab.hpp"

WaterPrefab::WaterPrefab(int id, std::wstring name, sf::IntRect textureRect) : Object() {
	_id = id;
	_name = name;
	_type = ObjectType::Water;
	_textureRect = textureRect;
}

WaterPrefab::~WaterPrefab() {

}
