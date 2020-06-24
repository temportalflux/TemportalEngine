#pragma once

#include "TemportalEnginePCH.hpp"

#include "thread/Thread.hpp"
#include "graphics/ShaderMetadata.hpp"

#include <string>
#include <vector>

NS_TASK

class TaskCompileShader
{
public:
	typedef std::function<void(std::vector<ui32> binary, std::vector<std::string> errors)> OnComplete;

	TaskCompileShader(std::string context);
	~TaskCompileShader();

	void compile(std::string source, ui32 stage);
	bool queryForCompletion();

	bool wasCompilationSuccessful() const;
	std::vector<ui32> getBinary() const;
	graphics::ShaderMetadata getBinaryMetadata() const;
	std::vector<std::string> getErrors() const;

private:
	ui32 mStage;
	std::string mSourceContent;
	std::string mContext;

	Thread mThread;

	bool bIsComplete;
	std::vector<ui32> mCompiledBinary;
	std::vector<std::string> mCompilationErrors;
	graphics::ShaderMetadata mBinaryMetadata;

	bool doCompile();
	void onComplete();
	void parseShader();

};

NS_END
