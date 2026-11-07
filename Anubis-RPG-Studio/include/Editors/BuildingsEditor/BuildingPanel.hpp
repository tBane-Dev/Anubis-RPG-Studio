#pragma once
#include "Components/Panel.hpp"
#include "TexturesManager.hpp"
#include "Editors/BuildingsEditor/PlacedGameObjects.hpp"

namespace BuildingsEditor {

	class BuildingPanel : public Components::Panel {
	public:

		std::shared_ptr<PlacedGameObjects> _game_objects;

		BuildingPanel(sf::Vector2i margin);
		~BuildingPanel();

		virtual void cursorHover();
		virtual void handleEvent(const sf::Event& event);
		virtual void update();
		virtual void draw();
	};

}