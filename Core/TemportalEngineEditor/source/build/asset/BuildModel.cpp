#include "build/asset/BuildModel.hpp"

#include "asset/Asset.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Logger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

using namespace build;

class AssimpLogger : public Assimp::Logger
{
	logging::Logger mLogger;
	std::filesystem::path const mModelPath;
public:
	AssimpLogger(logging::Logger &logger, std::filesystem::path const& modelPath)
		: Assimp::Logger(), mLogger(logger), mModelPath(modelPath) {}
	void log(logging::ECategory cate, char const* content)
	{
		mLogger.log(cate, "[BuildModel][%s] %s", mModelPath.filename().string().c_str(), content);
	}
	void OnDebug(const char* message) override { log(LOG_DEBUG, message); }
	void OnInfo(const char* message) override { log(LOG_INFO, message); }
	void OnWarn(const char* message) override { log(LOG_WARN, message); }
	void OnError(const char* message) override { log(LOG_ERR, message); }

	bool attachStream(Assimp::LogStream *pStream, unsigned int severity) override { assert(false); return false; }
	bool detatchStream(Assimp::LogStream *pStream, unsigned int severity) override { assert(false); return false; }
};

bool loadObjModel(
	std::filesystem::path const& objPath,
	std::function<void(logging::ECategory const&, std::string const&)> log,
	std::vector<asset::Model::Vertex> &outVertices, std::vector<ui32> &outIndices,
	std::vector<std::string> &outErrors
)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning, error;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, objPath.string().c_str()))
	{
		if (warning.length() > 0) log(LOG_WARN, warning);
		if (error.length() > 0) log(LOG_ERR, error);
		outErrors.push_back(error);
		return false;
	}

	uSize uniqueVertexCount = 0;
	for (auto const& shape : shapes)
	{
		for (auto const& index : shape.mesh.indices)
		{
			asset::Model::Vertex vertex = {};
			vertex.position = {
				attrib.vertices[index.vertex_index * 3 + 0],
				attrib.vertices[index.vertex_index * 3 + 1],
				attrib.vertices[index.vertex_index * 3 + 2]
			};
			vertex.texCoord = {
				attrib.texcoords[index.texcoord_index * 2 + 0],
				attrib.texcoords[index.texcoord_index * 2 + 1]
			};

			auto idx = ui32(uniqueVertexCount);
			for (uIndex i = 0; i < uniqueVertexCount; ++i)
			{
				if (vertex == outVertices[i])
				{
					idx = ui32(i);
					break;
				}
			}
			if (idx == uniqueVertexCount)
			{
				outVertices.push_back(vertex);
				uniqueVertexCount++;
			}
			outIndices.push_back(idx);
		}
	}

	return true;
}

std::shared_ptr<BuildAsset> BuildModel::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildModel>(asset);
}

BuildModel::BuildModel(std::shared_ptr<asset::Asset> asset) : BuildAsset(asset)
{
}

std::vector<std::string> BuildModel::compile(logging::Logger &logger)
{
	auto asset = this->get<asset::Model>();
	std::vector<std::string> errors;
	/*
	auto bSuccess = loadObjModel(
		asset->getAbsoluteSourcePath(),
		[&logger, asset](logging::ECategory const& cate, std::string const& msg)
		{
			logger.log(cate, "[%s] %s", asset->getFileName().c_str(), msg.c_str());
		},
		this->mCompiledVertices, this->mCompiledIndices,
		errors
	);
	//*/

	auto modelPath = asset->getAbsoluteSourcePath();
	Assimp::DefaultLogger::set(new AssimpLogger(logger, modelPath));

	Assimp::Importer importer;

	aiScene const* scene = importer.ReadFile(
		modelPath.string(),
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType
	);

	if (!scene)
	{
		errors.push_back(std::string(importer.GetErrorString()));
		Assimp::DefaultLogger::kill();
		return errors;
	}



	Assimp::DefaultLogger::kill();
	return errors;
}

void BuildModel::save()
{
	auto asset = this->get<asset::Model>();
	asset->setSourceBinary(this->mCompiledVertices, this->mCompiledIndices);
	BuildAsset::save();
}
