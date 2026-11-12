#include "Editors/PrefabsEditor/ColliderPanel.hpp"
#include "Editors/PrefabsEditor/Editor.hpp"
#include "ShadersManager.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "Cursor.hpp"

namespace PrefabsEditor {
	ColliderPanel::ColliderPanel(sf::Vector2i margin) : Panel(sf::Vector2i(420, 475), sf::Vector2i(840 + margin.x, PrefabsEditor::editor->_main_menu->getSize().y + margin.y)) {
		_title = std::make_unique<sf::Text>(basicFont, L"Collider", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));

		// text inputs
		_type = std::make_shared<TextInput>(sf::Vector2i(256, 30), L"type", 24, 18);
		_type->setPosition(sf::Vector2i(_rect.position.x + 84, _rect.position.y + margin.y + 192 + 16));
		_type->setText(L"Circular");

		_x = std::make_shared<NumberInput>(sf::Vector2i(256, 30), L"x", 24, 18);
		_x->setPosition(sf::Vector2i(_rect.position.x + 84, _type->getPosition().y + _type->getSize().y + 8));
		_x->setText(L"0");
		
		_y = std::make_shared<NumberInput>(sf::Vector2i(256, 30), L"y", 24, 18);
		_y->setPosition(sf::Vector2i(_rect.position.x + 84, _x->getPosition().y + _x->getSize().y + 8));
		_y->setText(L"0");

		_w = std::make_shared<NumberInput>(sf::Vector2i(256, 30), L"width", 24, 18);
		_w->setPosition(sf::Vector2i(_rect.position.x + 84, _y->getPosition().y + _y->getSize().y + 8));
		_w->setText(L"0");

		_h = std::make_shared<NumberInput>(sf::Vector2i(256, 30), L"height", 24, 18);
		_h->setPosition(sf::Vector2i(_rect.position.x + 84, _w->getPosition().y + _w->getSize().y + 8));
		_h->setText(L"0");

		// functions
		_x->_onEditedFunction = [this]() {
			editor->_main_panel->setButtonsActivity();
			editor->_main_panel->setTooltips();
		};

		_y->_onEditedFunction = [this]() {
			editor->_main_panel->setButtonsActivity();
			editor->_main_panel->setTooltips();
			};


		_w->_onEditedFunction = [this]() {
			editor->_main_panel->setButtonsActivity();
			editor->_main_panel->setTooltips();
			};


		_h->_onEditedFunction = [this]() {
			editor->_main_panel->setButtonsActivity();
			editor->_main_panel->setTooltips();
			};

		// texts labels
		int x = _x->getPosition().x - 32;

		_typeLabel = std::make_unique<sf::Text>(basicFont, L"t", 18);
		_typeLabel->setFillColor(basic_text_color);
		_typeLabel->setPosition(sf::Vector2f(x - _typeLabel->getGlobalBounds().size.x / 2, _type->getPosition().y));

		_xLabel = std::make_unique<sf::Text>(basicFont, L"x", 18);
		_xLabel->setFillColor(basic_text_color);
		_xLabel->setPosition(sf::Vector2f(x - _xLabel->getGlobalBounds().size.x / 2, _x->getPosition().y));

		_yLabel = std::make_unique<sf::Text>(basicFont, L"y", 18);
		_yLabel->setFillColor(basic_text_color);
		_yLabel->setPosition(sf::Vector2f(x - _yLabel->getGlobalBounds().size.x / 2, _y->getPosition().y));

		_wLabel = std::make_unique<sf::Text>(basicFont, L"w", 18);
		_wLabel->setFillColor(basic_text_color);
		_wLabel->setPosition(sf::Vector2f(x - _wLabel->getGlobalBounds().size.x / 2, _w->getPosition().y));

		_hLabel = std::make_unique<sf::Text>(basicFont, L"h", 18);
		_hLabel->setFillColor(basic_text_color);
		_hLabel->setPosition(sf::Vector2f(x - _hLabel->getGlobalBounds().size.x / 2, _h->getPosition().y));
	}

	ColliderPanel::~ColliderPanel() {

	}

	void ColliderPanel::cursorHover() {
		Components::Panel::cursorHover();

		_type->cursorHover();
		_x->cursorHover();
		_y->cursorHover();
		_w->cursorHover();
		_h->cursorHover();
	}

	void ColliderPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		_type->handleEvent(event);
		_x->handleEvent(event);
		_y->handleEvent(event);
		_w->handleEvent(event);
		_h->handleEvent(event);
	}

	void ColliderPanel::update() {
		Components::Panel::update();

		_type->update();
		_x->update();
		_y->update();
		_w->update();
		_h->update();
	}

	void ColliderPanel::draw() {
		Components::Panel::draw();
		Anubis::render_window->draw(*_title);

		// canvas background
		sf::IntRect canvasRect = sf::IntRect(sf::Vector2i(getPosition().x + (getSize().x - 192) / 2, getPosition().y + _title->getFont().getLineSpacing(20) + 24 + 16), sf::Vector2i(192, 192));
		sf::RectangleShape canvasRectShape(sf::Vector2f(canvasRect.size));
		canvasRectShape.setFillColor(sf::Color(47, 47, 47));
		canvasRectShape.setPosition(sf::Vector2f(canvasRect.position));
		Anubis::render_window->draw(canvasRectShape);
			
		// draw checkerboard
		sf::IntRect spriteRect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(192, 192));
		spriteRect.position = sf::Vector2i(canvasRect.position.x + (canvasRect.size.x - spriteRect.size.x) / 2, canvasRect.position.y + (canvasRect.size.y - spriteRect.size.y) / 2);
		sf::RectangleShape rect(sf::Vector2f(spriteRect.size));
		rect.setFillColor(sf::Color::Red);
		rect.setPosition(sf::Vector2f(spriteRect.position));

		sf::RenderStates states;
		states.shader = &*Anubis::shaders_manager->checkerboard_shader;
		Anubis::shaders_manager->checkerboard_shader->setUniform("rectPos", sf::Vector2f(spriteRect.position));
		Anubis::render_window->draw(rect, states);

		// texts labels
		Anubis::render_window->draw(*_typeLabel);
		Anubis::render_window->draw(*_xLabel);
		Anubis::render_window->draw(*_yLabel);
		Anubis::render_window->draw(*_wLabel);
		Anubis::render_window->draw(*_hLabel);

		// text inputs
		_type->draw();
		_x->draw();
		_y->draw();
		_w->draw();
		_h->draw();

	}
}