#include "build/asset/BuildModel.hpp"

#include "asset/Asset.hpp"
#include "math/Matrix.hpp"
#include "utility/StringUtils.hpp"

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
	std::string mPrefix;
public:
	AssimpLogger(logging::Logger &logger, std::string const& prefix)
		: Assimp::Logger(), mLogger(logger), mPrefix(prefix) {}
	void log(logging::ELogLevel cate, char const* content)
	{
		mLogger.log(cate, "%s %s", mPrefix.c_str(), content);
	}
	void OnDebug(const char* message) override { log(LOG_DEBUG, message); }
	void OnInfo(const char* message) override { log(LOG_INFO, message); }
	void OnWarn(const char* message) override { log(LOG_WARN, message); }
	void OnError(const char* message) override { log(LOG_ERR, message); }

	bool attachStream(Assimp::LogStream *pStream, unsigned int severity) override { assert(false); return false; }
	bool detatchStream(Assimp::LogStream *pStream, unsigned int severity) override { assert(false); return false; }
};

std::shared_ptr<BuildAsset> BuildModel::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildModel>(asset);
}

BuildModel::BuildModel(std::shared_ptr<asset::Asset> asset) : BuildAsset(asset)
{
}

ModelVertex convertVertex(aiMesh const* mesh, uIndex iVertex);

std::vector<std::string> BuildModel::compile(logging::Logger &logger)
{
	std::vector<std::string> errors;

	auto modelPath = this->get<asset::Model>()->getAbsoluteSourcePath();
	auto logPrefix = utility::formatStr("[BuildModel][%s]", modelPath.filename().string().c_str());
	Assimp::DefaultLogger::set(new AssimpLogger(logger, logPrefix));

	Assimp::Importer importer;

	aiScene const* scene = importer.ReadFile(
		modelPath.string(),
		aiProcess_CalcTangentSpace
		| aiProcess_Triangulate
		| aiProcess_JoinIdenticalVertices
		| aiProcess_SortByPType
		| aiProcess_FlipUVs // ensures that the upper-left is the origin for UV coords
	);

	if (!scene)
	{
		errors.push_back(std::string(importer.GetErrorString()));
		Assimp::DefaultLogger::kill();
		return errors;
	}

	struct VertexWeight
	{
		std::unordered_map<ui32, f32> vertexIndexToWeight;
		// A matrix which transforms from bone-space to mesh-space based on the pose the mesh was skinned/bound in.
		math::Matrix4x4 inverseBindMatrix;
	};
	// sorted by key, not by the order in which the keys were encountered when populated
	std::unordered_map<std::string, VertexWeight> relevantNodeVertexWeights;

	// https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#data-structures
	ui32 totalVertexCount = 0;
	for (uIndex iMesh = 0; iMesh < scene->mNumMeshes; ++iMesh)
	{
		aiMesh const* mesh = scene->mMeshes[iMesh];
		ui32 preMeshVertexCount = totalVertexCount; // the number of vertices before the mesh was processed
		
		// Gather all unique vertices (duplicates where removed on ReadFile via aiProcess_JoinIdenticalVertices)
		for (uIndex iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex)
		{
			this->mCompiledVertices.push_back(std::move(convertVertex(mesh, iVertex)));
			totalVertexCount++;
		}
		
		// Gather the indices for each face in the mesh
		for (uIndex iFace = 0; iFace < mesh->mNumFaces; ++iFace)
		{
			aiFace const& face = mesh->mFaces[iFace];
			for (uIndex iFaceIndex = 0; iFaceIndex < face.mNumIndices; ++iFaceIndex)
			{
				ui32 const& index = face.mIndices[iFaceIndex] + preMeshVertexCount;
				this->mCompiledIndices.push_back(index);
			}
		}
		
		// Gather the bone-weights for vertices
		for (uIndex iBone = 0; iBone < mesh->mNumBones; ++iBone)
		{
			aiBone const* bone = mesh->mBones[iBone];
			auto vertexWeight = VertexWeight{};
			memcpy_s(vertexWeight.inverseBindMatrix.data(), sizeof(math::Matrix4x4), &bone->mOffsetMatrix, sizeof(math::Matrix4x4));
			for (uIndex iWeight = 0; iWeight < bone->mNumWeights; ++iWeight)
			{
				aiVertexWeight const& weightMapping = bone->mWeights[iWeight];
				ui32 mIdxCompiledVertex = preMeshVertexCount + weightMapping.mVertexId;
				vertexWeight.vertexIndexToWeight.insert(std::make_pair(mIdxCompiledVertex, weightMapping.mWeight));
			}
			relevantNodeVertexWeights.insert(std::make_pair(std::string(bone->mName.C_Str()), std::move(vertexWeight)));
		}

	}

	// Find all relevant nodes (i.e. the actual bones) for skinning and animating
	// https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#the-node-hierarchy
	scene->mRootNode;

	for (uIndex iAnimation = 0; iAnimation < scene->mNumAnimations; ++iAnimation)
	{
		aiAnimation const* anim = scene->mAnimations[iAnimation];
		auto name = std::string(anim->mName.length > 0 ? anim->mName.C_Str() : "default");
		
	}

	logger.log(
		LOG_INFO, "%s Compiled %i vertices and %i indices with %i bones and %i animations", logPrefix.c_str(),
		totalVertexCount, this->mCompiledIndices.size(),
		0, 0
	);

	Assimp::DefaultLogger::kill();
	return errors;
}

math::Vector3 convertVector(aiVector3D const& in)
{
	// re-orient the model with the proper coordinates because the import format uses different axes
	return { -in.y, in.z, -in.x };
}

ModelVertex convertVertex(aiMesh const* mesh, uIndex iVertex)
{
	auto vertex = ModelVertex{};
	vertex.position = convertVector(mesh->mVertices[iVertex]);
	vertex.normal = convertVector(mesh->mNormals[iVertex]);
	vertex.tangent = convertVector(mesh->mTangents[iVertex]);
	vertex.bitangent = convertVector(mesh->mBitangents[iVertex]);

	// To support more than 1 texCoord per vertex, iterate over [0, AI_MAX_NUMBER_OF_TEXTURECOORDS) to get the texCoord index
	ui32 iTexCoord = 0;
	aiVector3D const& texCoord = mesh->mTextureCoords[iTexCoord][iVertex];
	for (ui32 iTexCoordComp = 0; iTexCoordComp < mesh->mNumUVComponents[iTexCoord]; ++iTexCoordComp)
	{
		// vertex texCoord defaults to fill with 0s, so no need to force-write 0 for elements not in the mesh
		vertex.texCoord[iTexCoordComp] = texCoord[iTexCoordComp];
	}

	return vertex;
}

void BuildModel::save()
{
	auto asset = this->get<asset::Model>();
	asset->setSourceBinary(this->mCompiledVertices, this->mCompiledIndices);
	BuildAsset::save();
}
