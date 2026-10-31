#include "Cursor.hpp"
#include "3D/Scene.hpp"

namespace Cursors {
	Cursor::Cursor() {
		_position = sf::Mouse::getPosition(*Anubis::render_window);

	}

	Cursor::~Cursor() {

	}

	void Cursor::update() {
		_position = sf::Mouse::getPosition(*Anubis::render_window);
	}

	void Cursor::handleEvent(const sf::Event& event) {

	}

	void Cursor::draw()
	{

	}

	std::shared_ptr<Cursor> cursor;

    std::vector<std::vector<std::vector<bool>>> circle_brushes = {

        // 0 - 1x1
        {
            {1}
        },

        // 1 - 3x3
        {
            {0, 1, 0},
            {1, 1, 1},
            {0, 1, 0}
        },

        // 2 - 5x5
        {
            {0, 1, 1, 1, 0},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {0, 1, 1, 1, 0}
        },

        // 3 - 7x7
        {
            {0, 0, 1, 1, 1, 0, 0},
            {0, 1, 1, 1, 1, 1, 0},
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1},
            {0, 1, 1, 1, 1, 1, 0},
            {0, 0, 1, 1, 1, 0, 0}
        },

        // 4 - 9x9
        {
            {0, 0, 0, 1, 1, 1, 0, 0, 0},
            {0, 0, 1, 1, 1, 1, 1, 0, 0},
            {0, 1, 1, 1, 1, 1, 1, 1, 0},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {0, 1, 1, 1, 1, 1, 1, 1, 0},
            {0, 0, 1, 1, 1, 1, 1, 0, 0},
            {0, 0, 0, 1, 1, 1, 0, 0, 0}
        },

        // 5 - 11x11
        {
            {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
            {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
            {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
            {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
            {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},

        }
    };

    std::vector<std::vector<std::vector<bool>>> square_brushes = {

        // 0 - 1x1
        {
            {1}
        },

        // 1 - 3x3
        {
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1}
        },

        // 2 - 5x5
        {
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1}
        },

        // 3 - 7x7
        {
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1}
        },

        // 4 - 9x9
        {
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1}
        },

        // 5 - 11x11
        {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

        }
    };

    CursorWithObject::CursorWithObject() {
        _object = std::weak_ptr<Object>();
        _state = CursorWithObjectState::Idle;
	}

    CursorWithObject::~CursorWithObject() {
	    
    }

    void CursorWithObject::update() {
        Cursor::update();
		_globalPosition = sf::Vector2i(Anubis::render_window->mapPixelToCoords(_position));
    }

    void CursorWithObject::handleEvent(const sf::Event& event) {
		Cursor::handleEvent(event);
    }

    void CursorWithObject::draw() {
        Cursor::draw();
    }
}

namespace Anubis {
    Cursor::Cursor() {

        _lastPosition = sf::Mouse::getPosition(*render_window);
        _position = sf::Mouse::getPosition(*render_window);

        _state = CursorState::Idle;
        _rect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(0, 0));

        _isDragging = false;
        _isSelecting = false;
    }

    Cursor::~Cursor() {

    }

    void Cursor::update() {
        _lastPosition = _position;
        _position = sf::Mouse::getPosition(*Anubis::render_window);

        if (_isDragging) {

        }
        else if (_isSelecting) {
            scene->_pressed = scene->getModelsInRect(_rect);
        }
    }

    void Cursor::handleEvent(const sf::Event& event) {

        if (const auto* mp = event.getIf<sf::Event::MouseButtonPressed>(); mp && mp->button == sf::Mouse::Button::Left) {

            _position = mp->position;
            _rect.position = _position;
            _rect.size = { 0, 0 };

            int id = scene->getHoveredModelId();

            _isDragging = id >= 0;
            _isSelecting = !_isDragging;

            _state = CursorState::Selecting;

            if (_isSelecting) {
                scene->_pressed.clear();
            }
            else {
                auto model = std::find_if(
                    scene->_models.begin(), scene->_models.end(),
                    [id](const auto& model) {
                        return model->_id == id;
                    }
                );

                if (model != scene->_models.end() && std::find(scene->_pressed.begin(), scene->_pressed.end(), *model) == scene->_pressed.end()) {

                    scene->_pressed.clear();
                    scene->_pressed.push_back(*model);
                }
            }
        }

        if (const auto* mr = event.getIf<sf::Event::MouseButtonReleased>(); mr && mr->button == sf::Mouse::Button::Left) {
            _state = CursorState::Idle;
            _rect.size = sf::Vector2i(0, 0);
            _isDragging = false;
            _isSelecting = false;
        }

        if (const auto* mm = event.getIf<sf::Event::MouseMoved>(); mm && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

            if (_isDragging) {

            }
            else if (_isSelecting) {
                _rect.size = _position - _rect.position;
            }
        }
    }

    void Cursor::draw() {
        if (_rect.size.x != 0 || _rect.size.y != 0) {
            float border = -2.f;
            sf::RectangleShape rect(sf::Vector2f(_rect.size));
            rect.setPosition(sf::Vector2f(_rect.position));
            rect.setFillColor(sf::Color(15.f, 15.f, 127.f, 95.f));
            rect.setOutlineThickness(border);
            rect.setOutlineColor(sf::Color(15.f, 15.f, 127.f, 159.f));
            render_window->draw(rect);
        }
    }

    std::shared_ptr<Cursor> cursor;
}