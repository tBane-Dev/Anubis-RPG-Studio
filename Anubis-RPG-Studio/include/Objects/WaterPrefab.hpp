#pragma once
#include <SFML/Graphics.hpp>
#include "Objects/Object.hpp"

// Kind of water in the palette (category Waters): 0 = ocean, 1 = swamp, 2 = lava.
class WaterPrefab : public Object {
public:
	int _id = 0;
	std::wstring _name;
	sf::IntRect _textureRect;	// preview in assets\tex\palette\waters.png

	WaterPrefab(int id, std::wstring name, sf::IntRect textureRect);
	~WaterPrefab();
};
