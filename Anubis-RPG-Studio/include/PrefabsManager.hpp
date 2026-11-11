#pragma once
#include <SFML/Graphics.hpp>
#include "Objects/GameObject.hpp"
#include <fstream>
#include "3D/Model3D.hpp"

class PrefabsManager {
public:
    std::vector<std::shared_ptr<GameObject>> _prefabs;
    
    PrefabsManager();
    ~PrefabsManager();
    
	void addPrefab(std::shared_ptr<GameObject> prefab);
    void removePrefab(std::weak_ptr<GameObject> prefab);
    std::shared_ptr<GameObject> getPrefab(std::wstring path);
	std::vector<std::shared_ptr<GameObject>>& getAllPrefabs();
	std::vector<std::shared_ptr<GameObject>> getPrefabs(ObjectType type);
    void replacePrefab(std::shared_ptr<GameObject> oldPrefab, std::shared_ptr<GameObject> newPrefab);

	void saveFromProject(std::ofstream& file);
	void loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader);
};

extern std::shared_ptr<PrefabsManager> prefabs_manager;

namespace Anubis {

    class Prefab {
    public:
        std::wstring _name;
        std::shared_ptr<Model3D> _model;

        Prefab(std::wstring name, std::shared_ptr<Model3D> model);
        ~Prefab();
    };

    class PrefabsManager {
    public:
        std::vector<std::shared_ptr<Prefab>> _prefabs;

        PrefabsManager();
        ~PrefabsManager();

        void addPrefab(std::shared_ptr<Prefab> prefab);
        void addPrefab(std::wstring name, std::shared_ptr<Model3D> model);
        std::shared_ptr<Prefab> getPrefab(std::wstring name);

        void load(std::ifstream& file, std::shared_ptr<Loader> loader);
    };

    extern std::shared_ptr<PrefabsManager> prefabs_manager;
}