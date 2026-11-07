#pragma once
#include "Components/Palette/Slot.hpp"

class BuildingSlot : public Slot {
public:

	BuildingSlot(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position = sf::Vector2i(0, 0));
	~BuildingSlot();

	void cursorHover();
	void handleEvent(const sf::Event& event);
	void update();
	void draw();
};