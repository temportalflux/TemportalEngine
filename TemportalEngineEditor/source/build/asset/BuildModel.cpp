#include "build/asset/BuildModel.hpp"

#include "asset/Asset.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

using namespace build;

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
	auto bSuccess = loadObjModel(
		asset->getAbsoluteSourcePath(),
		[&logger, asset](logging::ECategory const& cate, std::string const& msg)
		{
			logger.log(cate, "[%s] %s", asset->getFileName().c_str(), msg.c_str());
		},
		this->mCompiledVertices, this->mCompiledIndices,
		errors
	);
	return errors;
}

void BuildModel::save()
{
	auto asset = this->get<asset::Model>();
	asset->setSourceBinary(this->mCompiledVertices, this->mCompiledIndices);
	BuildAsset::save();
}
