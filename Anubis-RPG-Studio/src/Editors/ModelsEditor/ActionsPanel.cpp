#include "Editors/ModelsEditor/ActionsPanel.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"
#include "Objects/Monster.hpp"
#include "PrefabsManager.hpp"
#include "Editors/MapEditor/Editor.hpp"

namespace ModelsEditor {
	
	ActionsPanel::ActionsPanel(sf::Vector2i margin) : Panel(
		sf::Vector2i(420, 300), 
		sf::Vector2i(ModelsEditor::editor->_list_panel->getPosition().x, ModelsEditor::editor->_list_panel->getPosition().y + ModelsEditor::editor->_list_panel->getSize().y + 4)) {

		// title
		_title = std::make_unique<sf::Text>(basicFont, L"Actions", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));
		
		sf::Vector2i startPosition;
		startPosition.x = getPosition().x;
		startPosition.y = getPosition().y + margin.y;

		int marginBetweenButtons = 8;

		_saveBtn = std::make_shared<ButtonWithTextAndSprite>(
			L"Save Model",
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_inactive.png"),
			sf::Vector2i(startPosition.x+16, startPosition.y)
		);

		_addBtn = std::make_shared<ButtonWithTextAndSprite>(
			L"Add Model",
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_inactive.png"),
			sf::Vector2i(startPosition.x +16, _saveBtn->getPosition().y + _saveBtn->getSize().y + marginBetweenButtons)
		);

		_removeBtn = std::make_shared<ButtonWithTextAndSprite>(
			L"Remove Model",
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\largeButton_inactive.png"), 
			sf::Vector2i(startPosition.x + 16, _addBtn->getPosition().y + _addBtn->getSize().y + marginBetweenButtons)
		);

		_saveBtn->_onclick_func = [this]() {
			int index = ModelsEditor::editor->_list_panel->_selectedItemIndex;
			if (index < 0) return;

			std::shared_ptr<Anubis::Model3D> model = ModelsEditor::editor->_model_panel->_model;
			model->_name = ModelsEditor::editor->_name_panel->_name->getText();
			Anubis::models_manager->_models[index] = model;

			ModelsEditor::editor->_model_panel->_model = std::make_shared<Anubis::Model3D>(*model);
			ModelsEditor::editor->_list_panel->loadList();
			ModelsEditor::editor->_list_panel->selectItem(index);

		};

		_addBtn->_onclick_func = []() {
			if (!ModelsEditor::editor->_model_panel->_model)
				return;

			std::shared_ptr<Anubis::Model3D> model = std::make_shared<Anubis::Model3D>(*ModelsEditor::editor->_model_panel->_model);
			model->_name = ModelsEditor::editor->_name_panel->_name->getText();
			Anubis::models_manager->addModel(model);
			ModelsEditor::editor->_list_panel->loadList();
			ModelsEditor::editor->_list_panel->selectItem(Anubis::models_manager->getModelsCount() - 1);
			};

		_removeBtn->_onclick_func = []() {
			int index = ModelsEditor::editor->_list_panel->_selectedItemIndex;
			if (index < 0 || index >= Anubis::models_manager->getModelsCount())
				return;

			std::shared_ptr<Anubis::Model3D> oldModel = Anubis::models_manager->getModel(index).lock();

			for (auto& prefab : Anubis::prefabs_manager->getAllPrefabs()) {
				if (prefab->_model.lock() != oldModel)
					continue;

				if (MapEditor::editor->_cursor_on_map->_object.lock() == prefab)
					MapEditor::editor->_cursor_on_map->_object = std::weak_ptr<GameObject>();

				MapEditor::editor->_cursor_on_map->removeFromSelected(prefab);
				MapEditor::editor->_map->removeGameObjectsFromChunksByPrefab(prefab);
				MapEditor::editor->_game_objects->removeGameObjectsByPrefab(prefab);
				prefab->_model.reset();
			}

			Anubis::models_manager->_models.erase(Anubis::models_manager->_models.begin() + index);
			MapEditor::editor->setVisibleChunks();

			int count = Anubis::models_manager->getModelsCount();
			if (index >= count) index = count - 1;

			std::shared_ptr<Anubis::Model3D> model = Anubis::models_manager->getModel(index).lock();
			ModelsEditor::editor->_name_panel->_name->setText((model) ? model->_name : L"");
			ModelsEditor::editor->_model_panel->loadModel(model);
			ModelsEditor::editor->_list_panel->loadList();
			ModelsEditor::editor->_list_panel->selectItem(index);
			ModelsEditor::editor->_actions_panel->setButtonsActivity();
			ModelsEditor::editor->_actions_panel->setTooltips();
			ModelsEditor::editor->_preview_panel->loadModel(model);
			ModelsEditor::editor->_player_panel->setButtonsActivity();
			ModelsEditor::editor->_player_panel->setTooltips();
			ModelsEditor::editor->_statistics_panel->loadModel(model);
		};

		setButtonsActivity();
		setTooltips();
	}

	ActionsPanel::~ActionsPanel() {

	}

	void ActionsPanel::setButtonsActivity() {

		_saveBtn->setActive(false);
		_addBtn->setActive(false);
		_removeBtn->setActive(false);;

		if (!ModelsEditor::editor || !ModelsEditor::editor->_model_panel)
			return;

		if (ModelsEditor::editor->_model_panel->_model && !ModelsEditor::editor->_name_panel->_name->getText().empty() && ModelsEditor::editor->_list_panel->_selectedItemIndex >= 0) {
			_saveBtn->setActive(true);
		}

		if (ModelsEditor::editor->_model_panel->_model && !ModelsEditor::editor->_name_panel->_name->getText().empty()) {
			_addBtn->setActive(true);
		}

		if (ModelsEditor::editor->_list_panel->_selectedItemIndex >= 0) {
			_removeBtn->setActive(true);
		}
		
		

	}

	void ActionsPanel::setTooltips() {

		_saveBtn->setTooltip(256, L"Save Model", L"Saves the current model to the selected model");
		_addBtn->setTooltip(256, L"Add Model", L"Adds a new model to the list");
		_removeBtn->setTooltip(256, L"Remove Model", L"Removes the selected item from the list");

		if (!ModelsEditor::editor || !ModelsEditor::editor->_model_panel)
			return;

		// btn to save
		if (ModelsEditor::editor->_model_panel->_model && !ModelsEditor::editor->_name_panel->_name->getText().empty() && ModelsEditor::editor->_list_panel->_selectedItemIndex >= 0) {
			// no edit
		}
		else if(!ModelsEditor::editor->_model_panel->_model){
			_saveBtn->setTooltip(256, L"Save Model", L"Model has not loaded");
		}
		else if (ModelsEditor::editor->_name_panel->_name->getText().empty()) {
			_saveBtn->setTooltip(256, L"Save Model", L"Not setted model name");
		}
		else {
			_saveBtn->setTooltip(256, L"Save Model", L"Not selected item from list");
		}

		// btn to add
		if (ModelsEditor::editor->_model_panel->_model && !ModelsEditor::editor->_name_panel->_name->getText().empty()) {
			// no edit
		}
		else if (!ModelsEditor::editor->_model_panel->_model) {
			_addBtn->setTooltip(256, L"Add Model", L"Model has not loaded");
		}
		else if (ModelsEditor::editor->_name_panel->_name->getText().empty()) {
			_addBtn->setTooltip(256, L"Add Model", L"Not setted model name");
		}

		// btn to remove
		if (ModelsEditor::editor->_list_panel->_selectedItemIndex >= 0) {
			// no edit
		} else {
			_removeBtn->setTooltip(256, L"Remove Model", L"Not selected item from list to remove");
		}
	}

	void ActionsPanel::cursorHover() {
		Components::Panel::cursorHover();
	
		_saveBtn->cursorHover();
		_addBtn->cursorHover();
		_removeBtn->cursorHover();	
	}

	void ActionsPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		_saveBtn->handleEvent(event);
		_addBtn->handleEvent(event);
		_removeBtn->handleEvent(event);
	}

	void ActionsPanel::update() {
		Components::Panel::update();
		
		_saveBtn->update();
		_addBtn->update();
		_removeBtn->update();
	}

	void ActionsPanel::draw() {
		Components::Panel::draw();

		Anubis::render_window->draw(*_title);

		_saveBtn->draw();
		_addBtn->draw();
		_removeBtn->draw();
	}
}