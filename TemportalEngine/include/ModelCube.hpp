#pragma once

#include "Model.hpp"

class ModelCube : public Model
{

public:
	ModelCube() : Model()
	{
		// <x, y, z> = <Right/Left, Forward/Backward, Up/Down>
		auto idxFLU = this->pushVertex({ {-0.5f, +0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}, { 0.0f, 0.0f} });
		auto idxFRU = this->pushVertex({ {+0.5f, +0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, { 1.0f, 0.0f} });
		auto idxBRU = this->pushVertex({ {+0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, { 1.0f, 1.0f} });
		auto idxBLU = this->pushVertex({ {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, { 0.0f, 1.0f} });
		auto idxFLD = this->pushVertex({ {-0.5f, +0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f} });
		auto idxFRD = this->pushVertex({ {+0.5f, +0.5f, 1.0f}, {1.0f, 1.0f, 0.0f}, { 1.0f, 0.0f} });
		auto idxBRD = this->pushVertex({ {+0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}, { 1.0f, 1.0f} });
		auto idxBLD = this->pushVertex({ {-0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 0.0f}, { 0.0f, 1.0f} });
		// +X Right
		///*
		this->pushIndex(idxFRU);
		this->pushIndex(idxFRD);
		this->pushIndex(idxBRD);
		this->pushIndex(idxBRD);
		this->pushIndex(idxBRU);
		this->pushIndex(idxFRU);
		//*/
		// -X Left
		///*
		this->pushIndex(idxFLD);
		this->pushIndex(idxFLU);
		this->pushIndex(idxBLU);
		this->pushIndex(idxBLU);
		this->pushIndex(idxBLD);
		this->pushIndex(idxFLD);
		//*/
		// +Y Forward
		///*
		this->pushIndex(idxFLD);
		this->pushIndex(idxFRD);
		this->pushIndex(idxFRU);
		this->pushIndex(idxFRU);
		this->pushIndex(idxFLU);
		this->pushIndex(idxFLD);
		//*/
		// -Y Backward
		///*
		this->pushIndex(idxBLU);
		this->pushIndex(idxBRU);
		this->pushIndex(idxBRD);
		this->pushIndex(idxBRD);
		this->pushIndex(idxBLD);
		this->pushIndex(idxBLU);
		//*/
		// +Z Up
		this->pushIndex(idxFLU);
		this->pushIndex(idxFRU);
		this->pushIndex(idxBRU);
		this->pushIndex(idxBRU);
		this->pushIndex(idxBLU);
		this->pushIndex(idxFLU);
		// -Z Down
		this->pushIndex(idxBLD);
		this->pushIndex(idxBRD);
		this->pushIndex(idxFRD);
		this->pushIndex(idxFRD);
		this->pushIndex(idxFLD);
		this->pushIndex(idxBLD);

	}

};
