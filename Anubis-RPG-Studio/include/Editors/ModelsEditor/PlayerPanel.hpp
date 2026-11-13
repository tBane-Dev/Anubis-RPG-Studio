#pragma once
#include "Components/Panel.hpp"
#include "Controls/TextInput.hpp"
#include "Controls/ButtonWithTextAndSprite.hpp"

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

		sf::IntRect _statsRect;
		std::unique_ptr<sf::Text> _animations_label;
		std::unique_ptr<sf::Text> _triangles_label;
		std::unique_ptr<sf::Text> _meshes_label;
		std::unique_ptr<sf::Text> _nodes_label;
		std::unique_ptr<sf::Text> _skins_label;
		std::unique_ptr<sf::Text> _joints_label;
		std::unique_ptr<sf::Text> _textures_label;
		std::unique_ptr<sf::Text> _size_label;

		std::unique_ptr<sf::Text> _animations;
		std::unique_ptr<sf::Text> _triangles;
		std::unique_ptr<sf::Text> _meshes;
		std::unique_ptr<sf::Text> _nodes;
		std::unique_ptr<sf::Text> _skins;
		std::unique_ptr<sf::Text> _joints;
		std::unique_ptr<sf::Text> _textures;
		std::unique_ptr<sf::Text> _size;

		PlayerPanel(sf::Vector2i margin);
		~PlayerPanel();

		void loadModel();
		void setButtonsActivity();
		void setTooltips();

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};
}
