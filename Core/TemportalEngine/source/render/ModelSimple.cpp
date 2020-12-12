#include "render/ModelSimple.hpp"

ui32 render::SimpleModel::pushVertex(ModelVertex const& vertex)
{
	auto idx = ui32(this->vertices.size());
	this->vertices.push_back(vertex);
	return idx;
}

render::SimpleModel render::createIcosphere()
{
	SimpleModel model = {};

	auto idxTL = model.pushVertex({ {-0.5, 0, -0.5}, {}, {}, {}, { 0.0f, 0.0f } });
	auto idxTR = model.pushVertex({ {+0.5, 0, -0.5}, {}, {}, {}, { 1.0f, 0.0f } });
	auto idxBR = model.pushVertex({ {+0.5, 0, +0.5}, {}, {}, {}, { 0.0f, 1.0f } });
	auto idxBL = model.pushVertex({ {-0.5, 0, +0.5}, {}, {}, {}, { 1.0f, 1.0f } });
	model.indices.push_back(idxBR);
	model.indices.push_back(idxTR);
	model.indices.push_back(idxTL);
	model.indices.push_back(idxTL);
	model.indices.push_back(idxBL);
	model.indices.push_back(idxBR);

	return model;
}
