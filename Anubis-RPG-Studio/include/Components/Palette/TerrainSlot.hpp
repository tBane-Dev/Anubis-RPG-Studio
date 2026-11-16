#pragma once
#include "Components/Palette/Slot.hpp"

class TerrainSlot : public Slot {
public:


	TerrainSlot(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position = sf::Vector2i(0, 0));
	~TerrainSlot();

	void cursorHover();
	void handleEvent(const sf::Event& event);
	void update();
	void draw();
};