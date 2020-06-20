#pragma once

#include "Model.hpp"

class ModelCube : public Model
{

public:
	ModelCube() : Model()
	{
		// <x, y, z> = <Right/Left, Forward/Backward, Up/Down>
		auto idxFLU = this->pushVertex({ {-0.5f, +0.5f, 1.0f}, {1.0f, 0.0f, 1.0f} });
		auto idxFRU = this->pushVertex({ {+0.5f, +0.5f, 1.0f}, {1.0f, 1.0f, 1.0f} });
		auto idxBRU = this->pushVertex({ {+0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 1.0f} });
		auto idxBLU = this->pushVertex({ {-0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f} });
		auto idxFLD = this->pushVertex({ {-0.5f, +0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} });
		auto idxFRD = this->pushVertex({ {+0.5f, +0.5f, 0.0f}, {1.0f, 1.0f, 0.0f} });
		auto idxBRD = this->pushVertex({ {+0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} });
		auto idxBLD = this->pushVertex({ {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f} });
		// +X Right
		this->pushIndex(idxFRD);
		this->pushIndex(idxFRU);
		this->pushIndex(idxBRD);
		this->pushIndex(idxBRD);
		this->pushIndex(idxFRU);
		this->pushIndex(idxBRU);
		// -X Left
		this->pushIndex(idxFLU);
		this->pushIndex(idxFRD);
		this->pushIndex(idxBLU);
		this->pushIndex(idxBLU);
		this->pushIndex(idxFRD);
		this->pushIndex(idxBRD);
		// +Y Forward
		this->pushIndex(idxFRD);
		this->pushIndex(idxFLD);
		this->pushIndex(idxFRU);
		this->pushIndex(idxFRU);
		this->pushIndex(idxFLD);
		this->pushIndex(idxFLU);
		// -Y Backward
		this->pushIndex(idxBRU);
		this->pushIndex(idxBLU);
		this->pushIndex(idxBRD);
		this->pushIndex(idxBRD);
		this->pushIndex(idxBLU);
		this->pushIndex(idxBLD);
		// +Z Up
		this->pushIndex(idxFRU);
		this->pushIndex(idxFLU);
		this->pushIndex(idxBRU);
		this->pushIndex(idxBRU);
		this->pushIndex(idxFLU);
		this->pushIndex(idxBLU);
		// -Z Down
		this->pushIndex(idxBRD);
		this->pushIndex(idxBLD);
		this->pushIndex(idxFRD);
		this->pushIndex(idxFRD);
		this->pushIndex(idxBLD);
		this->pushIndex(idxFLD);

	}

};
