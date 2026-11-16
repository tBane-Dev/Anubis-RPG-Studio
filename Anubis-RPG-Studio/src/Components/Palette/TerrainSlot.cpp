#include "Components/Palette/TerrainSlot.hpp"
#include "Objects/Terrain.hpp"
#include "ShadersManager.hpp"
#include "Time.hpp"
#include "RenderWindow.hpp"

TerrainSlot::TerrainSlot(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position) : Slot(texture, hoverTexture, pressTexture, inactiveTexture, position) {
}

TerrainSlot::~TerrainSlot() {

}

void TerrainSlot::cursorHover() {
	ButtonWithSprite::cursorHover();
}

void TerrainSlot::handleEvent(const sf::Event& event) {
	ButtonWithSprite::handleEvent(event);
}

void TerrainSlot::update() {
	ButtonWithSprite::update();
}

void TerrainSlot::draw() {
	Slot::draw();

	if (_object.expired())
		return;

	std::shared_ptr<Terrain> terrain = std::dynamic_pointer_cast<Terrain>(_object.lock());

	if (!terrain)
		return;

	sf::RenderStates states;
	states.shader = &*Anubis::shaders_manager->terrain2D_shader;
	Anubis::shaders_manager->terrain2D_shader->setUniform("startPos", sf::Vector2f(_rect.position));
	std::shared_ptr<Texture> texture = textures_manager->getTexture(L"assets\\tex\\tileset.png");

	sf::Vector2f size(132.f, 132.f);
	sf::Vector2f offset((160.f - size.x) / 2.f, (160.f - size.y) / 2.f);

	sf::Sprite sprite(*texture->_texture);
	sprite.setTextureRect(terrain->_textureRect);
	sprite.setScale(sf::Vector2f(size.x / (float)terrain->_textureRect.size.x, size.y / (float)terrain->_textureRect.size.y));
	sprite.setPosition(sf::Vector2f(_rect.position) + offset);

	Anubis::render_window->draw(sprite, states);
}
