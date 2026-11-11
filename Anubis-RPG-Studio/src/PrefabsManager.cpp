#include "PrefabsManager.hpp"
#include "DebugLog.hpp"
#include "Objects/Monster.hpp"
#include "Objects/Nature.hpp"
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"
#include "Objects/Collider.hpp"
#include "AnimationsManager.hpp" // TO-DO - must be a model manager

PrefabsManager::PrefabsManager() {
	_prefabs.clear();
}

PrefabsManager::~PrefabsManager() {

}

void PrefabsManager::addPrefab(std::shared_ptr<GameObject> prefab) {
	_prefabs.push_back(prefab);
}

void PrefabsManager::removePrefab(std::weak_ptr<GameObject> prefab)
{
    auto prefabPtr = prefab.lock();
    if (!prefabPtr)
        return;

    std::erase_if(_prefabs,
        [&](const std::shared_ptr<GameObject>& p)
        {
			if(p == prefabPtr)
                DebugLog(L"usunieto prefab: " + p->_name);
            
            return p == prefabPtr;
        });
}

std::shared_ptr<GameObject> PrefabsManager::getPrefab(std::wstring path) {
	for (auto& prefab : _prefabs) {
		if (prefab->_name == path) {
			return prefab;
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<GameObject>>& PrefabsManager::getAllPrefabs() {
	return _prefabs;
}

std::vector<std::shared_ptr<GameObject>> PrefabsManager::getPrefabs(ObjectType type) {
	std::vector<std::shared_ptr<GameObject>> prefabsOfType;

    for (auto& prefab : _prefabs) {
        if(prefab->_type == type) {
            prefabsOfType.push_back(prefab);
		}
    }

    return prefabsOfType;
}

void PrefabsManager::replacePrefab(
    std::shared_ptr<GameObject> oldPrefab,
    std::shared_ptr<GameObject> newPrefab
) {
    if (!oldPrefab || !newPrefab)
        return;

    if (oldPrefab == newPrefab)
        return;

    auto it = std::find(_prefabs.begin(), _prefabs.end(), oldPrefab);

    if (it == _prefabs.end())
        return;

    *it = newPrefab;
}

void PrefabsManager::saveFromProject(std::ofstream& file) {

	BinaryWriter writer(file);
     
    for (auto& prefab : _prefabs) {
        
        if (prefab->_type == ObjectType::Monster) {
			std::shared_ptr<MonsterPrefab> monsterPrefab = std::dynamic_pointer_cast<MonsterPrefab>(prefab);
			writer.write_int8((int)monsterPrefab->_type);
            writer.write_wstring(monsterPrefab->_name);
			saveCollider(monsterPrefab->getCollider(), file);
			saveMesh(monsterPrefab->getMesh(), file);
            writer.write_Vector2i(monsterPrefab->_origin);
            writer.write_int32(monsterPrefab->_stepSize);
            writer.write_wstring(L"");
        }
        
        if (prefab->_type == ObjectType::Nature) {
			std::shared_ptr<NaturePrefab> naturePrefab = std::dynamic_pointer_cast<NaturePrefab>(prefab);
			writer.write_int8((int)naturePrefab->_type);
            writer.write_wstring(naturePrefab->_name);
            saveCollider(naturePrefab->getCollider(), file);
			saveMesh(naturePrefab->getMesh(), file);
			writer.write_Vector2i(naturePrefab->_origin);
            writer.write_wstring(L"");
        }

        

    }

}

void PrefabsManager::loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader) {
    
    loader->setTitle(L"Loading prefabs ... ");
    BinaryReader reader(file);

    _prefabs.clear();

    // Loaded animations
    DebugLog(L"Loading prefabs:");

    int prefabsCount = loader->counts.prefabsCount;

    for (int i = 0; i < prefabsCount; i++) {
        int type = reader.read_int8();

        if(type == (int)ObjectType::Monster) {

            std::wstring name = reader.read_wstring();
            loader->nextStep(name);
            std::shared_ptr<Collider> collider = loadCollider(file);
            std::shared_ptr<Mesh> mesh = loadMesh(file);
            sf::Vector2i origin = reader.read_Vector2i();
            int stepSize = reader.read_int32();
            std::wstring animationsPath = reader.read_wstring();
            std::shared_ptr<GameObject> prefab = std::make_shared<MonsterPrefab>(name, origin, stepSize, collider, mesh, Anubis::models_manager->getModel(name));
            addPrefab(prefab);
			DebugStat(name);
		}

        if(type == (int)ObjectType::Nature) {
            std::wstring name = reader.read_wstring();
            loader->nextStep(name);
            std::shared_ptr<Collider> collider = loadCollider(file);
			std::shared_ptr<Mesh> mesh = loadMesh(file);
            sf::Vector2i origin = reader.read_Vector2i();
            std::wstring animationsPath = reader.read_wstring();
            std::shared_ptr<GameObject> prefab = std::make_shared<NaturePrefab>(name, origin, collider, mesh, Anubis::models_manager->getModel(name));
            addPrefab(prefab);
            DebugStat(name);
		}

        

        
    }
}

std::shared_ptr<PrefabsManager> prefabs_manager = nullptr;

namespace Anubis {

    Prefab::Prefab(std::wstring name, std::shared_ptr<Model3D> model) {
        _name = name;
        _model = model;
    }

    Prefab::~Prefab() {

    }

    PrefabsManager::PrefabsManager() {

    }

    PrefabsManager::~PrefabsManager() {

    }

    void PrefabsManager::addPrefab(std::shared_ptr<Prefab> prefab) {

        if (!prefab)
            return;

        _prefabs.push_back(prefab);
    }

    void PrefabsManager::addPrefab(std::wstring name, std::shared_ptr<Model3D> model) {

        if (name.empty())
            return;

        if (!model)
            return;

        _prefabs.push_back(std::make_shared<Prefab>(name, model));
    }

    std::shared_ptr<Prefab> PrefabsManager::getPrefab(std::wstring name) {

        for (auto& prefab : _prefabs) {
            if (prefab->_name == name)
                return prefab;
        }

        return nullptr;
    }

    std::shared_ptr<PrefabsManager> prefabs_manager = nullptr;
}