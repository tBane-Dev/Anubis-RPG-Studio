#include "Editors/ModelsEditor/PlayerPanel.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"
#include "Objects/Monster.hpp"
#include "PrefabsManager.hpp"
#include "ShadersManager.hpp"

namespace ModelsEditor {

	PlayerPanel::PlayerPanel(sf::Vector2i margin) : Panel(sf::Vector2i(520, 192), sf::Vector2i(1332 + margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y + 192)) {

		_title = std::make_unique<sf::Text>(basicFont, L"Player", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));
	
		// buttons

		_anim_prev = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev_inactive.png"));

		_prev = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev_inactive.png"));

		_play = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play_inactive.png"));

		_pause = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause_inactive.png"));

		_next = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next_inactive.png"));

		_anim_next = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next_inactive.png"));

		int padding = 10;
		int total_width = _rect.size.x - padding * 2;
		int btn_w = 64;
		int big_btn_w = 80;
		int visible_count = 5;
		int buttons_width = (visible_count - 2) * btn_w + 2 * big_btn_w;
		int margin_between_buttons = 8;
		int all_width = buttons_width + margin_between_buttons * (visible_count - 1);

		sf::Vector2f startPosition;
		startPosition.x = _rect.position.x + padding + (total_width - all_width) / 2;
		startPosition.y = _rect.position.y + _title->getFont().getLineSpacing(20) + 24 + 16 + 16;

		_anim_prev->setPosition(sf::Vector2i(startPosition.x, startPosition.y));
		_prev->setPosition(sf::Vector2i(_anim_prev->getPosition().x + 80 + margin_between_buttons, _anim_prev->getPosition().y));
		_play->setPosition(sf::Vector2i(_prev->getPosition().x + btn_w + margin_between_buttons, _prev->getPosition().y));
		_pause->setPosition(sf::Vector2i(_play->getPosition().x, _play->getPosition().y));
		_next->setPosition(sf::Vector2i(_pause->getPosition().x + btn_w + margin_between_buttons, _pause->getPosition().y));
		_anim_next->setPosition(sf::Vector2i(_next->getPosition().x + btn_w + margin_between_buttons, _next->getPosition().y));

		_prev->_onclick_func = [this]() {
			ModelsEditor::editor->_preview_panel->_model->_player._time -= 0.1f;
			ModelsEditor::editor->_preview_panel->_model->updateAnimation(-0.1f);
		};

		_play->_onclick_func = [this]() {
			ModelsEditor::editor->_preview_panel->_model->_isAnimating = true;
		};

		_pause->_onclick_func = [this]() {
			ModelsEditor::editor->_preview_panel->_model->_isAnimating = false;
		};

		_next->_onclick_func = [this]() {
			ModelsEditor::editor->_preview_panel->_model->_player._time += 0.1f;
			ModelsEditor::editor->_preview_panel->_model->updateAnimation(0.1f);
		};

		_anim_prev->_onclick_func = [this]() {
			if (!ModelsEditor::editor->_preview_panel->_model->_animations.empty()) {
				ModelsEditor::editor->_preview_panel->_model->_player._index -= 1;
				if (ModelsEditor::editor->_preview_panel->_model->_player._index < 0)
					ModelsEditor::editor->_preview_panel->_model->_player._index = (int)ModelsEditor::editor->_preview_panel->_model->_animations.size() - 1;
			}
		};

		_anim_next->_onclick_func = [this]() {
			if (!ModelsEditor::editor->_preview_panel->_model->_animations.empty()) {
				ModelsEditor::editor->_preview_panel->_model->_player._index += 1;
				if (ModelsEditor::editor->_preview_panel->_model->_player._index >= (int)ModelsEditor::editor->_preview_panel->_model->_animations.size())
					ModelsEditor::editor->_preview_panel->_model->_player._index = 0;
			}

		};

		loadModel();
		setButtonsActivity();
		setTooltips();
	}

	PlayerPanel::~PlayerPanel() {

	}

	void PlayerPanel::loadModel() {

	}

	void PlayerPanel::setButtonsActivity() {

		if (ModelsEditor::editor->_preview_panel->_model && ModelsEditor::editor->_preview_panel->_model->_animations.size() > 1) {
			_anim_prev->setActive(true);
			_anim_next->setActive(true);
		}
		else {
			_anim_prev->setActive(false);
			_anim_next->setActive(false);
		}
	
		if (ModelsEditor::editor->_preview_panel->_model && !ModelsEditor::editor->_preview_panel->_model->_animations.empty()) {
			_prev->setActive(true);
			_play->setActive(true);
			_pause->setActive(true);
			_next->setActive(true);
		}
		else {
			_prev->setActive(false);
			_play->setActive(false);
			_pause->setActive(false);
			_next->setActive(false);
		}

	}

	void PlayerPanel::setTooltips() {

		if (!ModelsEditor::editor->_preview_panel->_model) {
			_anim_prev->setTooltip(256, L"Previous Animation", L"Model is not loaded");
			_anim_next->setTooltip(256, L"Next Animation", L"Model is not loaded");
		}
		else if (ModelsEditor::editor->_preview_panel->_model->_animations.empty()) {
			_anim_prev->setTooltip(256, L"Previous Animation", L"Model has no animations");
			_anim_next->setTooltip(256, L"Next Animation", L"Model has no animations");
		}
		else if(ModelsEditor::editor->_preview_panel->_model->_animations.size() == 1){
			_anim_prev->setTooltip(256, L"Previous Animation", L"Model has only one animation");
			_anim_next->setTooltip(256, L"Next Animation", L"Model has only one animation");
		}
		else {
			_anim_prev->setTooltip(256, L"Previous Animation", L"Set the next Animation");
			_anim_next->setTooltip(256, L"Next Animation", L"Set the next Animation");
		}

		if (!ModelsEditor::editor->_preview_panel->_model) {
			_prev->setTooltip(256, L"- 0.1s", L"Model is not loaded");
			_play->setTooltip(256, L"Play Animation", L"Model is not loaded");
			_pause->setTooltip(256, L"Pause Animation", L"Model is not loaded");
			_next->setTooltip(256, L"+ 0.1s", L"Model is not loaded");
		}
		else if (ModelsEditor::editor->_preview_panel->_model->_animations.empty()) {
			_prev->setTooltip(256, L"- 0.1s", L"Model has no animations");
			_play->setTooltip(256, L"Play Animation", L"Model has no animations");
			_pause->setTooltip(256, L"Pause Animation", L"Model has no animations");
			_next->setTooltip(256, L"+ 0.1s", L"Model has no animations");
		}  
		else {
			_prev->setTooltip(256, L"- 0.1s", L"");
			_play->setTooltip(256, L"Play Animation", L"");
			_pause->setTooltip(256, L"Pause Animation", L"");
			_next->setTooltip(256, L"+ 0.1s", L"");
		}
		
		
	}

	void PlayerPanel::cursorHover() {
		Components::Panel::cursorHover();

		_anim_prev->cursorHover();
		_prev->cursorHover();
		
		bool previewExists = ModelsEditor::editor->_preview_panel != nullptr;
		bool modelExists = previewExists && ModelsEditor::editor->_preview_panel->_model != nullptr;
		bool hasAnimations = modelExists && !ModelsEditor::editor->_preview_panel->_model->_animations.empty();
		bool activeAnimation = hasAnimations && ModelsEditor::editor->_preview_panel->_model->_player._index != -1;
		bool isStoped = activeAnimation && ModelsEditor::editor->_preview_panel->_model->_isAnimating == false;
		(!modelExists || !hasAnimations || isStoped) ? _play->cursorHover() : _pause->cursorHover();
		
		_next->cursorHover();
		_anim_next->cursorHover();

	}

	void PlayerPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		_anim_prev->handleEvent(event);
		_prev->handleEvent(event);

		bool previewExists = ModelsEditor::editor->_preview_panel != nullptr;
		bool modelExists = previewExists && ModelsEditor::editor->_preview_panel->_model != nullptr;
		bool hasAnimations = modelExists && !ModelsEditor::editor->_preview_panel->_model->_animations.empty();
		bool activeAnimation = hasAnimations && ModelsEditor::editor->_preview_panel->_model->_player._index != -1;
		bool isStoped = activeAnimation && ModelsEditor::editor->_preview_panel->_model->_isAnimating == false;

		(!modelExists || !hasAnimations || isStoped) ? _play->handleEvent(event) : _pause->handleEvent(event);

		_next->handleEvent(event);
		_anim_next->handleEvent(event);
	}

	void PlayerPanel::update() {

		if (!editor)
			return;

		Components::Panel::update();

		_anim_prev->update();

		_prev->update();
		_play->update();
		_pause->update();
		_next->update();
		_anim_next->update();
	}

	void PlayerPanel::draw() {
		Components::Panel::draw();

		Anubis::render_window->draw(*_title);
		
		_anim_prev->draw();
		_prev->draw();

		bool previewExists = ModelsEditor::editor->_preview_panel != nullptr;
		bool modelExists = previewExists && ModelsEditor::editor->_preview_panel->_model != nullptr;
		bool hasAnimations = modelExists && !ModelsEditor::editor->_preview_panel->_model->_animations.empty();
		bool activeAnimation = hasAnimations && ModelsEditor::editor->_preview_panel->_model->_player._index != -1;
		bool isStoped = activeAnimation && ModelsEditor::editor->_preview_panel->_model->_isAnimating == false;
		
		(!modelExists || !hasAnimations || isStoped) ? _play->draw() : _pause->draw();

		_next->draw();
		_anim_next->draw();

	}
}