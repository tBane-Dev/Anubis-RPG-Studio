#pragma once
#include <SFML/Graphics.hpp>
#include "GUIManager.hpp"
#include "Controls/ButtonWithSprite.hpp"

namespace MapEditor {

	class GhostChunkButton : public ButtonWithSprite {
		public:
			GhostChunkButton(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position = sf::Vector2i(0,0));
			~GhostChunkButton();

			virtual void cursorHover();
			virtual void draw();
	};

	class GhostChunk : public Element {
	public:

		bool _isVisible = false;
		sf::IntRect _rect;
		std::shared_ptr<GhostChunkButton> _button;

		GhostChunk();
		~GhostChunk();

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};

}