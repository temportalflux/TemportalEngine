#include "build/asset/BuildShader.hpp"

#include "Editor.hpp"
#include "asset/Asset.hpp"
#include "asset/Shader.hpp"

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.hpp>
#include <regex>

using namespace build;

shaderc_shader_kind TransformShaderFlagsToKind(vk::ShaderStageFlagBits stage)
{
	switch (stage)
	{
	case vk::ShaderStageFlagBits::eVertex: return shaderc_shader_kind::shaderc_vertex_shader;
	case vk::ShaderStageFlagBits::eFragment: return shaderc_shader_kind::shaderc_fragment_shader;
	case vk::ShaderStageFlagBits::eGeometry: return shaderc_shader_kind::shaderc_geometry_shader;
	case vk::ShaderStageFlagBits::eCompute: return shaderc_shader_kind::shaderc_compute_shader;
	case vk::ShaderStageFlagBits::eTessellationControl: return shaderc_shader_kind::shaderc_glsl_tess_control_shader;
	case vk::ShaderStageFlagBits::eTessellationEvaluation: return shaderc_shader_kind::shaderc_glsl_tess_evaluation_shader;
	case vk::ShaderStageFlagBits::eRaygenKHR /*& eRaygenNV*/: return shaderc_shader_kind::shaderc_raygen_shader;
	case vk::ShaderStageFlagBits::eClosestHitKHR /*& eClosestHitNV*/: return shaderc_shader_kind::shaderc_closesthit_shader;
	case vk::ShaderStageFlagBits::eMissKHR /*& eMissNV*/: return shaderc_shader_kind::shaderc_miss_shader;
	case vk::ShaderStageFlagBits::eIntersectionKHR /*& eIntersectionNV*/: return shaderc_shader_kind::shaderc_vertex_shader;
	case vk::ShaderStageFlagBits::eCallableKHR /*& eCallableNV*/: return shaderc_shader_kind::shaderc_callable_shader;
	case vk::ShaderStageFlagBits::eTaskNV: return shaderc_shader_kind::shaderc_task_shader;
	case vk::ShaderStageFlagBits::eMeshNV: return shaderc_shader_kind::shaderc_mesh_shader;
	case vk::ShaderStageFlagBits::eAllGraphics:
	case vk::ShaderStageFlagBits::eAll:
		break;
	}
	return shaderc_shader_kind::shaderc_glsl_infer_from_source;
}

std::shared_ptr<BuildAsset> BuildShader::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildShader>(asset);
}

BuildShader::BuildShader(std::shared_ptr<asset::Asset> asset) : BuildAsset(asset)
{
	auto shader = this->get<asset::Shader>();
	this->setContent(shader->readSource(), shader->getStage());
}

void BuildShader::setContent(std::string content, ui32 stage)
{
	this->mSourceContent = content;
	this->mStage = stage;
}

std::vector<ui32> BuildShader::getBinary() const
{
	return this->mCompiledBinary;
}

std::vector<std::string> BuildShader::compile()
{
	auto compilationResult = shaderc::Compiler().CompileGlslToSpv(
		this->mSourceContent,
		TransformShaderFlagsToKind((vk::ShaderStageFlagBits)this->mStage),
		"BuildShader"
	);

	auto status = compilationResult.GetCompilationStatus();
	auto bWasSuccessful = status == shaderc_compilation_status::shaderc_compilation_status_success;
	if (bWasSuccessful)
	{
		this->mCompiledBinary.assign(compilationResult.cbegin(), compilationResult.cend());
		return {};
	}
	else
	{
		std::istringstream f(compilationResult.GetErrorMessage());
		std::string tmp;
		auto errors = std::vector<std::string>();
		while (getline(f, tmp, '\n'))
		{
			errors.push_back(tmp);
		}
		return errors;
	}
}

#define STRING_MATCH(str, value, result) if (str == value) return result

uSize shaderTypeToByteCount(std::string attributeType)
{
#define MATCH_BYTECOUNT(str, type, mult) STRING_MATCH(attributeType, str, sizeof(type) * mult)
	MATCH_BYTECOUNT("float", f32, 1);
	MATCH_BYTECOUNT("vec2", f32, 2);
	MATCH_BYTECOUNT("vec3", f32, 3);
	MATCH_BYTECOUNT("vec4", f32, 4);
	MATCH_BYTECOUNT("mat2", f32, 2 * 2);
	MATCH_BYTECOUNT("mat3", f32, 3 * 3);
	MATCH_BYTECOUNT("mat4", f32, 4 * 4);
	return 0;
}

ui32 shaderTypeToFormat(std::string attributeType)
{
#define MATCH_FORMAT(str, format) STRING_MATCH(attributeType, str, (ui32)format)
	MATCH_FORMAT("float", vk::Format::eR32Sfloat);
	MATCH_FORMAT("vec2", vk::Format::eR32G32Sfloat);
	MATCH_FORMAT("vec3", vk::Format::eR32G32B32Sfloat);
	MATCH_FORMAT("vec4", vk::Format::eR32G32B32A32Sfloat);
	// TODO: Matrix formats are unknown
	MATCH_FORMAT("mat2", 0);
	MATCH_FORMAT("mat3", 0);
	MATCH_FORMAT("mat4", 0);
	return 0;
}

graphics::ShaderMetadata BuildShader::parseShader() const
{
	static std::regex RegexAttributeInput(R"(layout[\s]*\([\s]*location[\s]*=[\s]*([0-9]+)\)[\s]*in[\s]+(.*?)[\s]+(.*?)[\s]*;)");
	static std::regex RegexAttributeOutput(R"(layout[\s]*\([\s]*location[\s]*=[\s]*([0-9]+)\)[\s]*out[\s]+(.*?)[\s]+(.*?)[\s]*;)");
	static std::regex RegexAttributeUniform(R"(layout[\s]*\([\s]*location[\s]*=[\s]*([0-9]+)\)[\s]*uniform[\s]+(.*?)[\s]+(.*?)[\s]*;)");
	std::smatch regexMatch;
	graphics::ShaderMetadata metadata;
	auto content = this->mSourceContent;
	while (std::regex_search(content, regexMatch, RegexAttributeInput))
	{
		metadata.inputAttributes.push_back(
			{
				/*slot*/ (ui32)std::stoi(regexMatch[1]),
				/*propertyName*/ regexMatch[3],
				/*typeName*/ regexMatch[2],
				/*byteCount*/ shaderTypeToByteCount(regexMatch[2]),
				/*vk::Format*/ shaderTypeToFormat(regexMatch[2])
			}
		);
		content = regexMatch.suffix();
	}
	return metadata;
}

void BuildShader::save()
{
	this->get<asset::Shader>()->setBinary(this->getBinary(), this->parseShader());
	BuildAsset::save();
}
