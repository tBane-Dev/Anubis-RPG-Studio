#pragma once
#include "Components/Panel.hpp"
#include "Controls/TextInput.hpp"
#include "Controls/ButtonWithTextAndSprite.hpp"
#include "3D/Model3D.hpp"

namespace ModelsEditor {
	class PlayerPanel : public Components::Panel {
	public:

		std::unique_ptr<sf::Text> _title;

		std::shared_ptr<ButtonWithSprite> _anim_prev;
		std::shared_ptr<ButtonWithSprite> _prev;
		std::shared_ptr<ButtonWithSprite> _play;
		std::shared_ptr<ButtonWithSprite> _pause;
		std::shared_ptr<ButtonWithSprite> _next;
		std::shared_ptr<ButtonWithSprite> _anim_next;

		PlayerPanel(sf::Vector2i margin);
		~PlayerPanel();

		void setButtonsActivity();
		void setTooltips();

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};
}
