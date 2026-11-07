#include "Loader.hpp"
#include <SFML/Graphics.hpp>
#include "RenderWindow.hpp"
#include "Theme.hpp"
#include "BinaryReader.hpp"
#include "GUIManager.hpp"

Loader::Loader(std::ifstream& file) {
	_total = 4;
	_titleText = L"Loading ...";
	_current = -1;
	_currentText = L"";

	file.clear();
	file.seekg(0, std::ios::beg);

	BinaryReader reader(file);
	
	counts.animationsCount = reader.read_int32();
	counts.prefabsCount = reader.read_int32();
	counts.modelsCount = 33;
	counts.chunksCount = reader.read_int32();
	counts.gameObjectsCount = reader.read_int32();

	_total = counts.animationsCount + counts.prefabsCount + counts.modelsCount + counts.chunksCount + counts.gameObjectsCount; // TO-DO: + models

	draw();

	sf::sleep(sf::seconds(1.0f));
}

Loader::~Loader() {
	
}

void Loader::setTitle(std::wstring title) {
	_titleText = title;
	sf::sleep(sf::seconds(0.25f));
}

void Loader::nextStep(std::wstring stepName) {
	_currentText = stepName;
	_current += 1;

	while (const std::optional event = Anubis::render_window->pollEvent()) {

		if (event->is<sf::Event::Closed>()) {
			Anubis::render_window->close();
			exit(0);
		}
	}

	draw();
}

void Loader::end() {
	_current = _total;
	_currentText = L"finishing";

	while (const std::optional event = Anubis::render_window->pollEvent()) {

		if (event->is<sf::Event::Closed>()) {
			Anubis::render_window->close();
			exit(0);
		}
	}

	draw();

}

void Loader::draw() {

	

	sf::IntRect bar;
	bar.size = sf::Vector2i(Anubis::render_window->getSize().x * 8 / 10, 36);
	bar.position = sf::Vector2i((Anubis::render_window->getSize().x - bar.size.x) / 2, Anubis::render_window->getSize().y * 4 / 5);

	int border = 4;
	sf::IntRect inside;
	float progress = (float)_current / (float)_total;
	if (progress < 0.01f) 
		progress = 0.01f;

	inside.size = sf::Vector2i(int(float(bar.size.x - 2 * border) * progress), bar.size.y - 2 * border);
	inside.position = sf::Vector2i(bar.position.x + border, bar.position.y + border);

	sf::RectangleShape rect(sf::Vector2f(bar.size));
	rect.setPosition(sf::Vector2f(bar.position));
	rect.setFillColor(sf::Color(47, 47, 47, 255));

	sf::RectangleShape insideRect(sf::Vector2f(inside.size));
	insideRect.setPosition(sf::Vector2f(inside.position));
	insideRect.setFillColor(sf::Color(159, 47, 47, 255));

	sf::Text title(basicFont, _titleText, 32);
	title.setPosition(sf::Vector2f(bar.position.x, bar.position.y - basicFont.getLineSpacing(32) - 16));

	sf::Text step(basicFont, _currentText, 16);
	step.setPosition(sf::Vector2f(bar.position.x, bar.position.y + bar.size.y + 16));

	Anubis::render_window->resetGLStates();
	GUI_manager->setView();

	Anubis::render_window->clear();
	Anubis::render_window->draw(rect);
	Anubis::render_window->draw(insideRect);
	Anubis::render_window->draw(title);
	Anubis::render_window->draw(step);
	Anubis::render_window->display();

}