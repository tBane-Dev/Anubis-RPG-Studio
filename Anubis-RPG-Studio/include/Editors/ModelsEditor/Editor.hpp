#pragma once
#include "EditorsManager.hpp"
#include "Editors/ModelsEditor/MainMenu.hpp"
#include "Components/Panel.hpp"
#include "Editors/ModelsEditor/NamePanel.hpp"
#include "Editors/ModelsEditor/ModelsListPanel.hpp"
#include "Editors/ModelsEditor/ActionsPanel.hpp"
#include "Editors/ModelsEditor/PreviewPanel.hpp"
#include "Editors/ModelsEditor/ModelPanel.hpp"
#include "Editors/ModelsEditor/PlayerPanel.hpp"

namespace ModelsEditor {
	class Editor : public Main::Editor {
	public:

		std::unique_ptr<sf::Text> _title;
		sf::Vector2i _margin;

		std::shared_ptr<MainMenu> _main_menu;

		std::shared_ptr<NamePanel> _name_panel;
		std::shared_ptr<ModelsListPanel> _list_panel;
		std::shared_ptr<ActionsPanel> _actions_panel;
		std::shared_ptr<PreviewPanel> _preview_panel;
		std::shared_ptr<ModelPanel> _model_panel;
		std::shared_ptr<PlayerPanel> _player_panel;

		Editor();
		~Editor();

		void create();

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();

	};

	extern std::shared_ptr<Editor> editor;
}