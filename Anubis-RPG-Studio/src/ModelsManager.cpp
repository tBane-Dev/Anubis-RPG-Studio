#include "ModelsManager.hpp"
#include "DebugLog.hpp"
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"

namespace Anubis {
	ModelsManager::ModelsManager() {
		_models.clear();

		load();
	}

	ModelsManager::~ModelsManager() {

	}

	int ModelsManager::addModel(std::wstring name, std::filesystem::path path) {
		std::shared_ptr<Model3D> model = openModel3DFromFile(path);
		model->_name = name;
		_models.push_back(model);
		return getModelsCount() - 1;
	}

	int ModelsManager::addModel(std::wstring name, std::shared_ptr<Model3D> model) {
		if (!model)
			return -1;

		std::shared_ptr<Model3D> m = std::make_shared<Model3D>(*model);
		m->_name = name;

		_models.push_back(m);
		return getModelsCount() - 1;
	}

	int ModelsManager::addModel(std::shared_ptr<Model3D> model) {
		_models.push_back(model);
		return getModelsCount() - 1;
	}

	std::weak_ptr<Model3D> ModelsManager::getModel(std::wstring name) {
		for (auto& model : _models) {
			if (model && model->_name == name) {
				return model;
			}
		}

		return std::weak_ptr<Model3D>();
	}

	std::weak_ptr<Model3D> ModelsManager::getModel(int index) {
		if (index >= 0 && index < (int)_models.size()) {
			return _models[index];
		}

		return std::weak_ptr<Model3D>();
	}

	int ModelsManager::getModelID(std::wstring name) {
		for (int i = 0; i < (int)_models.size(); i++) {
			if (_models[i] && _models[i]->_name == name) {
				return i;
			}
		}

		return -1;
	}

	void ModelsManager::removeModel(int index) {
		if (index >= 0 && index < (int)_models.size()) {
			_models.erase(_models.begin() + index);
		}
	}

	int ModelsManager::getModelsCount() {
		return (int)_models.size();
	}



	void ModelsManager::saveFromProject(std::ofstream& file) {

		BinaryWriter writer(file);

		for (auto& m : _models) {
			writer.write_wstring(m->_name);
			//writer.write_modelGLB(m);   //TO-DO - save meshes etc ... 
			DebugLog(m->_name);
		}
	}

	void ModelsManager::loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader) {

		loader->setTitle(L"Loading Models ...");
		BinaryReader reader(file);

		_models.clear();
		int modelsCount = loader->counts.modelsCount;

		DebugLog(L"Loading Models:");

		for (int i = 0; i < modelsCount; i++) {
			std::wstring name = reader.read_wstring();

			loader->nextStep(name);

			std::shared_ptr<Model3D> model = std::make_shared<Model3D>(name);
			// reader.read_model_GLB(); //TO-DO - load meshes etc from file
			addModel(model);

			DebugStat(name);
		}
	}

	void ModelsManager::load() {

		_models.clear();

		struct p {
			std::wstring name;
			std::wstring path;
		};

		std::vector<p> pp;

		pp.emplace_back(L"dziobak", L"assets\\mdl\\dziobak.glb");
		pp.emplace_back(L"golem", L"assets\\mdl\\golem.glb");
		pp.emplace_back(L"troll", L"assets\\mdl\\troll.glb");
		pp.emplace_back(L"bies", L"assets\\mdl\\bies.glb");
		pp.emplace_back(L"deer", L"assets\\mdl\\deer.glb");
		pp.emplace_back(L"goblin", L"assets\\mdl\\goblin.glb");
		pp.emplace_back(L"bzyczeń", L"assets\\mdl\\bzyczeń.glb");
		pp.emplace_back(L"bear", L"assets\\mdl\\bear.glb");


		pp.emplace_back(L"tree_1", L"assets\\mdl\\tree_1.glb");
		pp.emplace_back(L"tree_2", L"assets\\mdl\\tree_2.glb");
		pp.emplace_back(L"tree_3", L"assets\\mdl\\tree_3.glb");
		pp.emplace_back(L"mushroom", L"assets\\mdl\\mushroom.glb");
		pp.emplace_back(L"bush", L"assets\\mdl\\bush.glb");
		pp.emplace_back(L"hunter_tent", L"assets\\mdl\\hunter_tent.glb");
		pp.emplace_back(L"path_1", L"assets\\mdl\\path_1.glb");
		pp.emplace_back(L"path_2", L"assets\\mdl\\path_2.glb");
		pp.emplace_back(L"palisade_1", L"assets\\mdl\\palisade_1.glb");
		pp.emplace_back(L"palisade_2", L"assets\\mdl\\palisade_2.glb");
		pp.emplace_back(L"wooden_gate", L"assets\\mdl\\wooden_gate.glb");
		pp.emplace_back(L"well", L"assets\\mdl\\well.glb");
		pp.emplace_back(L"pile_of_wood_1", L"assets\\mdl\\pile_of_wood_1.glb");
		pp.emplace_back(L"log_with_saw", L"assets\\mdl\\log_with_saw.glb");
		pp.emplace_back(L"stone_platform", L"assets\\mdl\\stone_platform.glb");
		pp.emplace_back(L"tile_grass", L"assets\\mdl\\tile_grass.glb");
		pp.emplace_back(L"reed_1", L"assets\\mdl\\reed_1.glb");
		pp.emplace_back(L"reed_2", L"assets\\mdl\\reed_2.glb");
		pp.emplace_back(L"driftwood", L"assets\\mdl\\driftwood.glb");
		pp.emplace_back(L"boulder_1", L"assets\\mdl\\boulder_1.glb");
		pp.emplace_back(L"boulder_2", L"assets\\mdl\\boulder_2.glb");
		pp.emplace_back(L"boulder_3", L"assets\\mdl\\boulder_3.glb");
		pp.emplace_back(L"column_1", L"assets\\mdl\\column_1.glb");
		pp.emplace_back(L"column_2", L"assets\\mdl\\column_2.glb");
		pp.emplace_back(L"pile_of_wood_1", L"assets\\mdl\\pile_of_wood_1.glb");
		pp.emplace_back(L"small_grass_1", L"assets\\mdl\\small_grass_1.glb");
		pp.emplace_back(L"small_grass_2", L"assets\\mdl\\small_grass_2.glb");
		pp.emplace_back(L"small_grass_3", L"assets\\mdl\\small_grass_3.glb");


		for (auto& p : pp) {
			addModel(p.name, openModel3DFromFile(p.path));
		}

	}

	std::shared_ptr<ModelsManager> models_manager = nullptr;
}