#pragma once
#include "Components/Panel.hpp"
#include "Controls/TextInput.hpp"
#include "Controls/ButtonWithTextAndSprite.hpp"
#include "3D/Model3D.hpp"

namespace ModelsEditor {
	class CollidersPanel : public Components::Panel {
	public:

		std::unique_ptr<sf::Text> _title;
		
		std::shared_ptr<ButtonWithTextAndSprite> _sphere;
		std::shared_ptr<ButtonWithTextAndSprite> _cuboid;

		CollidersPanel(sf::Vector2i margin);
		~CollidersPanel();

		void loadModel(std::shared_ptr<Anubis::Model3D> model);

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};
}
