#include "Editors/ModelsEditor/ModelPanel.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"
#include "Objects/Monster.hpp"
#include "PrefabsManager.hpp"
#include "ShadersManager.hpp"
#include "Windows/FileDialog/FileDialog.hpp"

namespace ModelsEditor {

	ModelPanel::ModelPanel(sf::Vector2i margin) : Panel(sf::Vector2i(520, 192), sf::Vector2i(1332 + margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y)) {

		_title = std::make_unique<sf::Text>(basicFont, L"Model", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));

		_btn = std::make_shared<ButtonWithTextAndSprite>(
			L"Load Model (*.glb)",
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\mediumButton.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\mediumButton_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\mediumButton_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\mediumButton_inactive.png"),
			sf::Vector2i(getPosition().x + (getSize().x - 300) / 2, getPosition().y + (getSize().y - 64) / 2 + 16)
		);

		_btn->_onclick_func = [this]() {

			std::shared_ptr<FileDialog> fileDialog = std::make_shared<FileDialog>(L"Load Spritesheet", L"Load");
			fileDialog->setPosition(sf::Vector2i(256, 192));

			std::function<void()> function = [this, fileDialog]() {

				std::shared_ptr<sf::Image> image = std::make_shared<sf::Image>();

				if (!image->loadFromFile(fileDialog->getPathFile())) {
					DebugError(L"Nie udało się wczytać obrazka: " + fileDialog->getPathFile());
					return;
				}

				// TO-DO
				//ModelsEditor::editor->_sprite_sheet_panel->setTextInputsRange();
				//ModelsEditor::editor->_sprite_sheet_panel->loadModel();
				//ModelsEditor::editor->_actions_panel->setButtonsActivity();
				//ModelsEditor::editor->_actions_panel->setTooltips();
				//ModelsEditor::editor->_preview_panel->loadAnimations();
				//ModelsEditor::editor->_preview_panel->setButtonsActivity();
				//ModelsEditor::editor->_preview_panel->setTooltips();
			};

			fileDialog->setFunction(function);
			Main::windows_manager->push_back(fileDialog);
			};
	}

	ModelPanel::~ModelPanel() {

	}

	void ModelPanel::loadModel() {

	}

	void ModelPanel::cursorHover() {
		Components::Panel::cursorHover();

		_btn->cursorHover();
	}

	void ModelPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		_btn->handleEvent(event);
	}

	void ModelPanel::update() {

		if (!editor)
			return;

		Components::Panel::update();

		_btn->update();
	}

	void ModelPanel::draw() {
		Components::Panel::draw();

		Anubis::render_window->draw(*_title);

		_btn->draw();
	}
}