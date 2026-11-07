#pragma once
#include "Components/Palette/Slot.hpp"
#include "Objects/Object.hpp"
#include "3D/Scene.hpp"

class GameObjectSlot : public Slot {
public:
	
	// TO-DO - to remove
	std::shared_ptr<Texture> _objectTexture;
	std::shared_ptr<sf::Sprite> _objectSprite;
	//

	std::shared_ptr<Anubis::Scene> _scene;
	std::shared_ptr<Anubis::Model3D> _model;
	float _modelRadius;

	GameObjectSlot(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position = sf::Vector2i(0, 0));
	~GameObjectSlot();

	virtual void setObject(std::shared_ptr<Object> object);\

	void drawModel();

	void cursorHover();
	void handleEvent(const sf::Event& event);
	void update();
	void draw();
};
