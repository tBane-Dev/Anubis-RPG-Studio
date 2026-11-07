#pragma once
#include <SFML/Graphics.hpp>
#include "Objects/PlacedGameObject.hpp"
#include "Loader.hpp"
#include <fstream>

namespace MapEditor {
    class PlacedGameObjects {
    public:

        std::array<float, 16> _billboard;
        const sf::Texture* _boundTexture = nullptr;

        std::vector<std::shared_ptr<PlacedGameObject>> _visiblePlacedGameObjects;
        std::weak_ptr<PlacedGameObject> _hoveredPlacedGameObject;

        PlacedGameObjects();
        ~PlacedGameObjects();

        void addGameObject(std::weak_ptr<PlacedGameObject> placedGameObject);
        void removeGameObject(std::weak_ptr<PlacedGameObject> placedGameObject);
        void updateModelTransform(const std::shared_ptr<PlacedGameObject>& object);

        // Shared by placed objects and the placement preview of CursorOnMap.
        static float getModelScale(const std::shared_ptr<GameObject>& prefab);
        static float getModelWind(const std::shared_ptr<GameObject>& prefab);
        static sf::Vector3f getModelPosition(const std::shared_ptr<GameObject>& prefab, sf::Vector2i position);
        static std::shared_ptr<Anubis::Model3D> createModel(const std::shared_ptr<GameObject>& prefab);

        void removeGameObjectsByPrefab(std::weak_ptr<GameObject> prefab);
        void removeGameObjectsByAnimations(int animationID);
        void replacePrefab(std::shared_ptr<GameObject> oldPrefab, std::shared_ptr<GameObject> newPrefab);

        void saveFromProject(std::ofstream& file);
        void loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader);

        virtual void cursorHover();
        virtual void update();
        virtual void draw();
    };



}