#pragma once
#include "Components/Palette/Palette.hpp"

namespace MapEditor {
	class Palette : public Components::Palette {
	public:

		ObjectType _toolsCategory = ObjectType::None;	// category of the current tools

		Palette();
		~Palette();

		virtual void addTools();
		void selectFirstSlot();
		virtual void loadAll(ObjectType type);

		virtual void cursorHover();
		virtual void handleEvent(const sf::Event& event);
		virtual void draw();
	};

}