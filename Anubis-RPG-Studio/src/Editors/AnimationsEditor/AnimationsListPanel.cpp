#include "Editors/AnimationsEditor/AnimationsListPanel.hpp"
#include "Theme.hpp"
#include "AnimationsManager.hpp"
#include "Editors/AnimationsEditor/Editor.hpp"
#include <iostream>

namespace AnimationsEditor {

	AnimationsListPanel::AnimationsListPanel(sf::Vector2i margin) : Components::ListPanel(margin, sf::Vector2i(420, 600), sf::Vector2i(margin.x, AnimationsEditor::editor->_main_menu->getSize().y + margin.y), 10) {
		_title->setString(L"Animations List");
		loadAll(10);
	}

	AnimationsListPanel::~AnimationsListPanel() {

	}

	void AnimationsListPanel::loadList() {
		resetItems();
	}
}
