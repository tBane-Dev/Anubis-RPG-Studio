#pragma once
#include <SFML/Graphics.hpp>
#include "Objects/PlacedGameObject.hpp"

namespace BuildingsEditor {
    class PlacedGameObjects {
    public:
        std::vector<std::shared_ptr<PlacedGameObject>> _visiblePlacedGameObjects;

        PlacedGameObjects();
        ~PlacedGameObjects();

        void addGameObject(std::weak_ptr<PlacedGameObject> placedGameObjectOn);
        void removeGameObject(std::weak_ptr<PlacedGameObject> placedGameObject);

        virtual void update();
    };

}


