#include "TaskCompileShader.hpp"

#include "Engine.hpp"

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.hpp>
#include <regex>

using namespace task;

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

TaskCompileShader::TaskCompileShader(std::string context)
{
	this->mContext = context;
	this->mThread = Thread("Compile Shader " + context, &engine::Engine::LOG_SYSTEM);
	this->mThread.setFunctor(std::bind(&TaskCompileShader::doCompile, this), false);
	this->mThread.setOnComplete(std::bind(&TaskCompileShader::onComplete, this));
	this->bIsComplete = false;
}

TaskCompileShader::~TaskCompileShader()
{
	if (this->mThread.isActive())
		this->mThread.join();
}

void TaskCompileShader::compile(std::string source, ui32 stage)
{
	this->mSourceContent = source;
	this->mStage = stage;
	this->mThread.start();
}

// Runs on the thread
bool TaskCompileShader::doCompile()
{
	auto compilationResult = shaderc::Compiler().CompileGlslToSpv(
		this->mSourceContent,
		TransformShaderFlagsToKind((vk::ShaderStageFlagBits)this->mStage),
		this->mContext.c_str()
	);

	this->mThread.log(LOG_INFO, "Compiling shader");

	auto status = compilationResult.GetCompilationStatus();
	auto bWasSuccessful = status == shaderc_compilation_status::shaderc_compilation_status_success;
	this->mThread.log(LOG_INFO, "Compilation was %ssuccessful", bWasSuccessful ? "" : "un");
	if (bWasSuccessful)
	{
		this->mCompiledBinary.assign(compilationResult.cbegin(), compilationResult.cend());
		this->parseShader();
	}
	else
	{
		std::istringstream f(compilationResult.GetErrorMessage());
		std::string tmp;
		while (getline(f, tmp, '\n'))
		{
			this->mThread.log(LOG_INFO, tmp.c_str());
			this->mCompilationErrors.push_back(tmp);
		}
	}

	return false; // not iterative, we don't care about return value
}

#define STRING_MATCH(str, value, result) if (str == value) return result

uSize shaderTypeToByteCount(std::string attributeType)
{
#define MATCH_BYTECOUNT(str, type, mult) STRING_MATCH(attributeType, str, sizeof(type) * mult)
	MATCH_BYTECOUNT("float", f32, 1);
	MATCH_BYTECOUNT("vec2",  f32, 2);
	MATCH_BYTECOUNT("vec3",  f32, 3);
	MATCH_BYTECOUNT("vec4",  f32, 4);
	MATCH_BYTECOUNT("mat2",  f32, 2 * 2);
	MATCH_BYTECOUNT("mat3",  f32, 3 * 3);
	MATCH_BYTECOUNT("mat4",  f32, 4 * 4);
	return 0;
}

ui32 shaderTypeToFormat(std::string attributeType)
{
#define MATCH_FORMAT(str, format) STRING_MATCH(attributeType, str, (ui32)format)
	MATCH_FORMAT("float", vk::Format::eR32Sfloat);
	MATCH_FORMAT("vec2", 	vk::Format::eR32G32Sfloat);
	MATCH_FORMAT("vec3", 	vk::Format::eR32G32B32Sfloat);
	MATCH_FORMAT("vec4", 	vk::Format::eR32G32B32A32Sfloat);
	// TODO: Matrix formats are unknown
	MATCH_FORMAT("mat2", 	0);
	MATCH_FORMAT("mat3", 	0);
	MATCH_FORMAT("mat4", 	0);
	return 0;
}

void TaskCompileShader::parseShader()
{
	static std::regex RegexAttributeInput(R"(layout[\s]*\([\s]*location[\s]*=[\s]*([0-9]+)\)[\s]*in[\s]+(.*?)[\s]+(.*?)[\s]*;)");
	static std::regex RegexAttributeOutput(R"(layout[\s]*\([\s]*location[\s]*=[\s]*([0-9]+)\)[\s]*out[\s]+(.*?)[\s]+(.*?)[\s]*;)");
	static std::regex RegexAttributeUniform(R"(layout[\s]*\([\s]*location[\s]*=[\s]*([0-9]+)\)[\s]*uniform[\s]+(.*?)[\s]+(.*?)[\s]*;)");
	std::smatch regexMatch;
	std::string content = this->mSourceContent;
	while (std::regex_search(content, regexMatch, RegexAttributeInput))
	{
		this->mBinaryMetadata.inputAttributes.push_back(
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
}

// Runs on the thread
void TaskCompileShader::onComplete()
{
	this->bIsComplete = true;
}

// Main thread for checking on the task thread
bool TaskCompileShader::queryForCompletion()
{
	if (!this->mThread.isValid()) return false;
	auto isActive = this->mThread.isActive();
	if (!isActive)
	{
		this->mThread.join();
	}
	return !isActive;
}

bool TaskCompileShader::wasCompilationSuccessful() const
{
	return this->getBinary().size() > 0;
}

std::vector<ui32> TaskCompileShader::getBinary() const
{
	return this->mCompiledBinary;
}

graphics::ShaderMetadata TaskCompileShader::getBinaryMetadata() const
{
	return this->mBinaryMetadata;
}

std::vector<std::string> TaskCompileShader::getErrors() const
{
	return this->mCompilationErrors;
}
