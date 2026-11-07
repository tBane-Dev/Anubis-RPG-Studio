#include "Editors/BuildingsEditor/BuildingPanel.hpp"
#include "RenderWindow.hpp"
#include "Editors/BuildingsEditor/BuildingsListPanel.hpp"
#include "Editors/BuildingsEditor/Editor.hpp"
#include "Editors/BuildingsEditor/NamePanel.hpp"
#include "ShadersManager.hpp"
#include "EditorsManager.hpp"

namespace BuildingsEditor {

	BuildingPanel::BuildingPanel(sf::Vector2i margin) : Components::Panel(
		sf::Vector2i(BuildingsEditor::editor->_name_panel->getSize().x + 4, 768 + 4),
		sf::Vector2i(BuildingsEditor::editor->_name_panel->getPosition().x, BuildingsEditor::editor->_name_panel->getPosition().y + BuildingsEditor::editor->_name_panel->getSize().y + 4)) {

		_game_objects = std::make_shared<PlacedGameObjects>();

	}

	BuildingPanel::~BuildingPanel() {

	}

	void BuildingPanel::cursorHover() {
		Components::Panel::cursorHover();
	}

	void BuildingPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);
	}

	void BuildingPanel::update() {
		Components::Panel::update();
		
		for (auto& object : _game_objects->_visiblePlacedGameObjects) {
			object->update();
		}
	}

	void BuildingPanel::draw() {
		Components::Panel::draw();
	}
}
