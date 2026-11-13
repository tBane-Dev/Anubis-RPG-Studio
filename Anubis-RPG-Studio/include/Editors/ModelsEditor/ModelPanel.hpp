#pragma once
#include "Components/Panel.hpp"
#include "Controls/TextInput.hpp"
#include "Controls/ButtonWithTextAndSprite.hpp"
#include "3D/Model3D.hpp"

namespace ModelsEditor {
	class ModelPanel : public Components::Panel {
	public:

		std::unique_ptr<sf::Text> _title;
		std::shared_ptr<ButtonWithTextAndSprite> _btn;
		std::shared_ptr<Anubis::Model3D> _model;

		ModelPanel(sf::Vector2i margin);
		~ModelPanel();

		void loadModel(std::shared_ptr<Anubis::Model3D> model);

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};
}
