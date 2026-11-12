#pragma once
#include "Components/Panel.hpp"
#include "Controls/TextInput.hpp"
#include "Controls/ButtonWithTextAndSprite.hpp"

namespace ModelsEditor {
	class ModelPanel : public Components::Panel {
	public:

		std::unique_ptr<sf::Text> _title;
		std::shared_ptr<ButtonWithTextAndSprite> _btn;

		ModelPanel(sf::Vector2i margin);
		~ModelPanel();

		void loadModel();

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};
}
