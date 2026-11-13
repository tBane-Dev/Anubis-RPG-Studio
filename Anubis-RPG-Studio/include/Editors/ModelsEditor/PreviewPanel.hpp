#pragma once
#include "Components/Panel.hpp"
#include "Controls/NumberInput.hpp"
#include "Controls/RealInput.hpp"
#include "Controls/ButtonWithTextAndSprite.hpp"
#include "3D/Scene.hpp"
#include "3D/Model3D.hpp"

namespace ModelsEditor {
	class PreviewPanel : public Components::Panel {
	public:
		
		std::unique_ptr<sf::Text> _title;

		std::shared_ptr<ButtonWithTextAndSprite> _loadBtn;
		sf::IntRect _sceneRect;
		sf::IntRect _scenePixelRect;
		float _modelRadius;
		std::shared_ptr<Anubis::Scene> _scene;
		std::shared_ptr<Anubis::Model3D> _model;

		PreviewPanel(sf::Vector2i margin);
		~PreviewPanel();

		void loadModel(std::shared_ptr<Anubis::Model3D> model);
		void drawModel();
		void setTextInputsRange();

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};
}
