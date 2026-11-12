#include "Editors/ModelsEditor/ModelsListPanel.hpp"
#include "Theme.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include <iostream>

namespace ModelsEditor {

	ModelsListPanel::ModelsListPanel(sf::Vector2i margin) : Components::ListPanel(margin, sf::Vector2i(420, 600), sf::Vector2i(margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y), 10) {
		_title->setString(L"Models List");
		loadAll(10);
	}

	ModelsListPanel::~ModelsListPanel() {

	}

	void ModelsListPanel::loadList() {
		resetItems();
	}
}
