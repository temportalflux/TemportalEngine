#pragma once

#include "Model.hpp"

class ModelCube : public Model
{

public:
	ModelCube() : Model()
	{
		// TODO: The class says cube, but this is clearly a plane... :P
		auto idxTL = this->pushVertex({ {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} });
		auto idxTR = this->pushVertex({ {+0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} });
		auto idxBR = this->pushVertex({ {+0.5f, +0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} });
		auto idxBL = this->pushVertex({ {-0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} });
		this->pushIndex(idxTL);
		this->pushIndex(idxTR);
		this->pushIndex(idxBR);
		this->pushIndex(idxBR);
		this->pushIndex(idxBL);
		this->pushIndex(idxTL);
	}

};
