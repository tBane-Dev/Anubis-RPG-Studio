#include "Editors/BuildingsEditor/BuildingsListPanel.hpp"
#include "Editors/BuildingsEditor/Editor.hpp"
#include "Editors/BuildingsEditor/MainMenu.hpp"
#include "PrefabsManager.hpp"
#include "Theme.hpp"

namespace BuildingsEditor {

	BuildingsListPanel::BuildingsListPanel(sf::Vector2i margin) : Components::ListPanel(margin, sf::Vector2i(420, 600), sf::Vector2i(margin.x, BuildingsEditor::editor->_main_menu->getSize().y + margin.y), 10) {
		_title->setString(L"Buildings List");
	}

	BuildingsListPanel::~BuildingsListPanel() {

	}

	void BuildingsListPanel::loadList() {
		resetItems();

		for (int i = 0; i < _items.size(); i++) {
			std::shared_ptr<Components::ListPanelItem> item = _items[i];

			item->setName(L"empty building");
			item->_onclick_func = { };
		}
	}
}
