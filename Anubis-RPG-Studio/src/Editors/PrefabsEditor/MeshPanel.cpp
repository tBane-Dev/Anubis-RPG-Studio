#include "Editors/PrefabsEditor/MeshPanel.hpp"
#include "Editors/PrefabsEditor/Editor.hpp"
#include "ShadersManager.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include <iostream>
#include "WindowsManager.hpp"

namespace PrefabsEditor {
	MeshPanel::MeshPanel(sf::Vector2i margin) : Panel(sf::Vector2i(420, 375), sf::Vector2i(840 + margin.x, PrefabsEditor::editor->_main_menu->getSize().y + margin.y + PrefabsEditor::editor->_collider_panel->getSize().y + 4)) {
		_title = std::make_unique<sf::Text>(basicFont, L"Mesh", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));

		_set_mesh = std::make_shared<ButtonWithTextAndSprite>(
			L"Set Mesh",
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_inactive.png"),
			sf::Vector2i(_rect.position.x + _rect.size.x / 2 - 192 / 2, _rect.position.y + margin.y + 192 + 16)
		);

		_set_mesh->_onclick_func = [this]() {

			};

		setButtonsActivity();

		_mesh = std::make_shared<Mesh>(2.0f, 1.0f);
		_mesh->addShape(std::make_shared<Shape>());

		if (!PrefabsEditor::editor->_object.expired()) {
			_mesh = std::make_shared<Mesh>(*PrefabsEditor::editor->_object.lock()->getMesh());

			if (_mesh == nullptr) {
				_mesh = std::make_shared<Mesh>(2.0f, 1.0f);
				_mesh->addShape(std::make_shared<Shape>());
			}
		}
	}

	MeshPanel::~MeshPanel() {

	}

	void MeshPanel::setButtonsActivity() {

		if (!editor)
			return;

		_set_mesh->setActive(true);
	}

	void MeshPanel::cursorHover() {
		Components::Panel::cursorHover();

		_set_mesh->cursorHover();
	}

	void MeshPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		_set_mesh->handleEvent(event);
	}

	void MeshPanel::update() {
		Components::Panel::update();

		_set_mesh->update();
	}

	void MeshPanel::draw() {
		Components::Panel::draw();
		Anubis::render_window->draw(*_title);

		// canvas background
		sf::IntRect canvasRect = sf::IntRect(sf::Vector2i(getPosition().x + (getSize().x - 192) / 2, getPosition().y + _title->getFont().getLineSpacing(20) + 24 + 16), sf::Vector2i(192, 192));
		sf::RectangleShape canvasRectShape(sf::Vector2f(canvasRect.size));
		canvasRectShape.setFillColor(sf::Color(47, 47, 47));
		canvasRectShape.setPosition(sf::Vector2f(canvasRect.position));
		Anubis::render_window->draw(canvasRectShape);

		
		// draw checkerboard
		sf::IntRect spriteRect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(192, 192));
		spriteRect.position = sf::Vector2i(canvasRect.position.x + (canvasRect.size.x - spriteRect.size.x) / 2, canvasRect.position.y + (canvasRect.size.y - spriteRect.size.y) / 2);
		sf::RectangleShape rect(sf::Vector2f(spriteRect.size));
		rect.setFillColor(sf::Color::Red);
		rect.setPosition(sf::Vector2f(spriteRect.position));

		sf::RenderStates states;
		states.shader = &*checkerboard_shader;
		checkerboard_shader->setUniform("rectPos", sf::Vector2f(spriteRect.position));
		Anubis::render_window->draw(rect, states);


		_set_mesh->draw();
	}
}   