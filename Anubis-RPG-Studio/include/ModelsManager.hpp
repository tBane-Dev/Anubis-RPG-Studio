#pragma once
#include <SFML/Graphics.hpp>
#include "3D/Model3D.hpp"
#include "Loader.hpp"
#include <fstream>

namespace Anubis {

    class ModelsManager {
    public:
        std::vector<std::shared_ptr<Model3D>> _models;

        ModelsManager();
        ~ModelsManager();

        int addModel(std::wstring name, std::filesystem::path path);
        int addModel(std::wstring name, std::shared_ptr<Model3D> model);
        int addModel(std::shared_ptr<Model3D> model);
        std::weak_ptr<Model3D> getModel(std::wstring name);
        std::weak_ptr<Model3D> getModel(int index);
        int getModelID(std::wstring name);
        void removeModel(int index);
        int getModelsCount();

        void saveFromProject(std::ofstream& file);
        void loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader);

        void load();
    };

    extern std::shared_ptr<ModelsManager> models_manager;
}