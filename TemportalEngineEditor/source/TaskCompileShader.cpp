#include "TaskCompileShader.hpp"

#include "Engine.hpp"

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.hpp>
#include <regex>

using namespace task;

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
	this->mBuilder.setContent(source, stage);
	this->mThread.start();
}

// Runs on the thread
bool TaskCompileShader::doCompile()
{
	this->mThread.log(LOG_INFO, "Compiling shader");
	this->mCompilationErrors = this->mBuilder.compile();
	this->mThread.log(LOG_INFO, "Compilation was %ssuccessful", this->mCompilationErrors.empty() ? "" : "un");
	return false; // not iterative, we don't care about return value
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
	return this->mBuilder.getBinary();
}

graphics::ShaderMetadata TaskCompileShader::getBinaryMetadata() const
{
	return this->mBuilder.parseShader();
}

std::vector<std::string> TaskCompileShader::getErrors() const
{
	return this->mCompilationErrors;
}
