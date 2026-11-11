#pragma once
#include "Controls/ButtonWithSprite.hpp"
#include "Objects/Object.hpp"

class Slot : public ButtonWithSprite {
public:
	std::weak_ptr<Object> _object;

	Slot(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position = sf::Vector2i(0, 0));
	~Slot();

	virtual void setObject(std::shared_ptr<Object> object);
};
