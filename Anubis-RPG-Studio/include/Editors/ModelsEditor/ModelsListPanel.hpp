#pragma once
#include "Components/ListPanel.hpp"

namespace ModelsEditor {
	class ModelsListPanel : public Components::ListPanel {
	public:

		ModelsListPanel(sf::Vector2i margin);
		~ModelsListPanel();
		
		virtual void loadList();

	};

}