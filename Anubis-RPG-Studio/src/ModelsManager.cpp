#include "ModelsManager.hpp"
#include "DebugLog.hpp"
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"

namespace Anubis {
	ModelsManager::ModelsManager() {
		_models.clear();
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

		for (auto& model : _models) {
			if (!model)
				continue;

			writer.write_wstring(model->_name);

			if (model->_glbData)
				writer.write_data(*model->_glbData);
			else
				writer.write_int32(0);
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
			std::shared_ptr<std::vector<uint8_t>> data = reader.read_data();

			loader->nextStep(name);

			std::shared_ptr<Model3D> model = openModel3DFromMemory(data);
			if (!model)
				model = std::make_shared<Model3D>();

			model->_name = name;
			addModel(model);

			DebugStat(name);
		}
	}

	std::shared_ptr<ModelsManager> models_manager = nullptr;
}