#include "Editors/ModelsEditor/ModelsListPanel.hpp"
#include "Theme.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include <iostream>
#include "ModelsManager.hpp"

namespace ModelsEditor {

	ModelsListPanel::ModelsListPanel(sf::Vector2i margin) : Components::ListPanel(margin, sf::Vector2i(420, 600), sf::Vector2i(margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y), 10) {
		_title->setString(L"Models List");
		loadAll(10);
	}

	ModelsListPanel::~ModelsListPanel() {

	}

	void ModelsListPanel::loadList() {
		resetItems();
	
		int models_count = Anubis::models_manager->getModelsCount();
		int max_scroll = models_count * basic_text_rect_height - _list_rect.size.y;
		if (max_scroll < 0) max_scroll = 0;
		_scrollbar->setMax(max_scroll);

		int first = _scrollbar->getValue() / basic_text_rect_height;

		for (int i = 0; i < (int)_items.size(); i += 1) {
			int index = first + i;
			if (index < models_count) {
				_items[i]->setName(Anubis::models_manager->_models[index]->_name);
				_items[i]->_onclick_func = [this, index, i]() { 
					selectItem(index); 
					std::shared_ptr<Anubis::Model3D> model = Anubis::models_manager->_models[index];
					std::shared_ptr<Anubis::Model3D> tempModel = std::make_shared<Anubis::Model3D>(*model);
					ModelsEditor::editor->_name_panel->_name->setText(model->_name);
					ModelsEditor::editor->_model_panel->loadModel(tempModel);
					ModelsEditor::editor->_actions_panel->setButtonsActivity();
					ModelsEditor::editor->_actions_panel->setTooltips();
					ModelsEditor::editor->_preview_panel->loadModel(tempModel);
					ModelsEditor::editor->_player_panel->setButtonsActivity();
					ModelsEditor::editor->_player_panel->setTooltips();
					ModelsEditor::editor->_statistics_panel->loadModel(tempModel);
					};
			}
			else {
				_items[i]->setName(L"");
				_items[i]->_onclick_func = { };
			}
				
		}

	}
}
