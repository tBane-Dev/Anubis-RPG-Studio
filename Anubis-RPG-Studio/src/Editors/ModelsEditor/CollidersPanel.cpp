#include "Editors/ModelsEditor/CollidersPanel.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"

namespace ModelsEditor {

	CollidersPanel::CollidersPanel(sf::Vector2i margin) : Panel(sf::Vector2i(520, 192), sf::Vector2i(1332 + margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y + 192 + 160 + 356)) {

		_title = std::make_unique<sf::Text>(basicFont, L"Colliders", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));

		loadModel(nullptr);

		_sphere = std::make_shared<ButtonWithTextAndSprite>(
			L"Sphere",
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_inactive.png"),
			sf::Vector2i(getPosition().x + (getSize().x - 192) / 2 - 104, getPosition().y + (getSize().y - 64) / 2 + 16)
		);

		_cuboid = std::make_shared<ButtonWithTextAndSprite>(
			L"Cuboid",
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\bottomButton_inactive.png"),
			sf::Vector2i(getPosition().x + (getSize().x - 192) / 2 + 104, getPosition().y + (getSize().y - 64) / 2 + 16)
		);

	}

	CollidersPanel::~CollidersPanel() {

	}

	void CollidersPanel::loadModel(std::shared_ptr<Anubis::Model3D> model) {

	}

	void CollidersPanel::cursorHover() {
		Components::Panel::cursorHover();

		_sphere->cursorHover();
		_cuboid->cursorHover();
	}

	void CollidersPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		_sphere->handleEvent(event);
		_cuboid->handleEvent(event);
	}

	void CollidersPanel::update() {

		if (!editor)
			return;

		Components::Panel::update();

		_sphere->update();
		_cuboid->update();

	}

	void CollidersPanel::draw() {
		Components::Panel::draw();

		Anubis::render_window->draw(*_title);

		_sphere->draw();
		_cuboid->draw();

	}
}