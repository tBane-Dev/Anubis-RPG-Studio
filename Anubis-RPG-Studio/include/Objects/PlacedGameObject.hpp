#pragma once
#include "Objects/Object.hpp"
#include "Objects/GameObject.hpp"
#include "3D/Model3D.hpp"


class PlacedGameObject : public Object, public std::enable_shared_from_this<PlacedGameObject> {
public:

	std::weak_ptr<GameObject> _prefab;
	std::shared_ptr<Anubis::Model3D> _model;
    sf::Vector2f _position;
	
	bool _isSelected;
	std::unique_ptr<sf::Text> _text;

	PlacedGameObject(std::weak_ptr<GameObject> prefab, bool play = true);
	~PlacedGameObject();

	void drawFrame(sf::Color color = sf::Color(127+64, 30, 45));

	virtual sf::Vector2f getPosition();
	virtual void setPosition(sf::Vector2f position);

	virtual void cursorHover();
	virtual void update();
	virtual void draw();
};