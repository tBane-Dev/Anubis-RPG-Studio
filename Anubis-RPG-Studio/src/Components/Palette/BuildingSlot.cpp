#include "Components/Palette/BuildingSlot.hpp"
#include "RenderWindow.hpp"


BuildingSlot::BuildingSlot(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position) : Slot(texture, hoverTexture, pressTexture, inactiveTexture, position) {
	
	_object = std::weak_ptr<Object>();
	_animator = nullptr;
}

BuildingSlot::~BuildingSlot() {

}

void BuildingSlot::cursorHover() {
	ButtonWithSprite::cursorHover();
}

void BuildingSlot::handleEvent(const sf::Event& event) {
	ButtonWithSprite::handleEvent(event);
}

void BuildingSlot::update() {
	ButtonWithSprite::update();
}

void BuildingSlot::draw() {
	Slot::draw();
}
