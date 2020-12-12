#include "render/ModelSimple.hpp"

ui32 render::SimpleModel::pushVertex(ModelVertex const& vertex)
{
	auto idx = ui32(this->vertices.size());
	this->vertices.push_back(vertex);
	return idx;
}

void render::SimpleModel::pushTri(math::Vector3UInt const& tri)
{
	this->indices.push_back(tri[0]);
	this->indices.push_back(tri[1]);
	this->indices.push_back(tri[2]);
}

void render::createIcosahedronFaces(std::vector<math::Vector3> &points, std::vector<math::Vector3UInt> &tris)
{
	f32 ro = (1.0f + std::sqrt(5.0f)) / 2.0f;
	f32 ri = 1.0f;
	points = std::vector<math::Vector3>({
		{ -ri, +ro, 0 },
		{ +ri, +ro, 0 },
		{ -ri, -ro, 0 },
		{ +ri, -ro, 0 },
		{ 0, -ri, +ro },
		{ 0, +ri, +ro },
		{ 0, -ri, -ro },
		{ 0, +ri, -ro },
		{ +ro, 0, -ri },
		{ +ro, 0, +ri },
		{ -ro, 0, -ri },
		{ -ro, 0, +ri },
	});
	tris = std::vector<math::Vector3UInt>({
		{ 0, 11, 5 },
		{ 0, 5, 1 },
		{ 0, 1, 7 },
		{ 0, 7, 10 },
		{ 0, 10, 11 },
		{ 1, 5, 9 },
		{ 5, 11, 4 },
		{ 11, 10, 2 },
		{ 10, 7, 6 },
		{ 7, 1, 8 },
		{ 3, 9, 4 },
		{ 3, 4, 2 },
		{ 3, 2, 6 },
		{ 3, 6, 8 },
		{ 3, 8, 9 },
		{ 4, 9, 5 },
		{ 2, 4, 11 },
		{ 6, 2, 10 },
		{ 8, 6, 7 },
		{ 9, 8, 1 },
	});
}

render::SimpleModel render::createIcosphere(ui8 subdivisions)
{
	SimpleModel model = {};

	std::vector<math::Vector3> points;
	std::vector<math::Vector3UInt> tris;
	render::createIcosahedronFaces(points, tris);

	// Map of a&b to c where the point c is in between and colinear with a & b
	// a --- c ---- b
	auto pointSubdiv = std::map<std::pair<ui32, ui32>, ui32>();
	auto makeSubdivPair = [](ui32 const& a, ui32 const& b) { return a < b ? std::make_pair(a, b) : std::make_pair(b, a); };
	auto getOrAddPoint = [&pointSubdiv, &points](std::pair<ui32, ui32> const& subdivPair, math::Vector3 const& point) -> ui32
	{
		auto iter = pointSubdiv.find(subdivPair);
		if (iter != pointSubdiv.end()) return iter->second;
		else
		{
			auto idx = ui32(points.size());
			points.push_back(point);
			pointSubdiv.insert(std::make_pair(subdivPair, idx));
			return idx;
		}
	};

	// subdivide the icosahedron
	while (subdivisions-- > 0)
	{	
		auto newTris = std::vector<math::Vector3UInt>(tris.begin(), tris.end());
		newTris.reserve(tris.size() * 4);
		while (tris.size() > 0)
		{
			auto tri = *tris.begin();
			tris.erase(tris.begin());
			/*
							0
						 / \
					  /   \
				   a --- c 
					/ \   / \
         /   \ /   \
				1 --- b --- 2
			*/
			auto a = getOrAddPoint(makeSubdivPair(tri[0], tri[1]), (0.5f * points[tri[0]]) + (0.5f * points[tri[1]]));
			auto b = getOrAddPoint(makeSubdivPair(tri[1], tri[2]), (0.5f * points[tri[1]]) + (0.5f * points[tri[2]]));
			auto c = getOrAddPoint(makeSubdivPair(tri[2], tri[0]), (0.5f * points[tri[2]]) + (0.5f * points[tri[0]]));
			newTris.push_back({ c, tri[0], a });
			newTris.push_back({ a, tri[1], b });
			newTris.push_back({	b, tri[2], c });
			newTris.push_back({ c, a, b });
		}
		tris = std::move(newTris);
	}

	// Add all vertices and indices
	for (auto const& point : points)
	{
		auto normal = point.normalized();
		model.pushVertex({
			normal * 0.5f,
			normal,
			{}, {},
			{ 0, 0 }
		});
	}
	for (auto const& tri : tris)
	{
		model.pushTri(tri);
	}

	return model;
}

render::SimpleModel render::createCube()
{
	SimpleModel model = {};

	f32 r = 0.5f;
	// Top
	{
		auto idxTL = model.pushVertex({ { -r, +r, -r }, math::V3_UP, {}, {}, { 0, 0 } });
		auto idxTR = model.pushVertex({ { +r, +r, -r }, math::V3_UP, {}, {}, { 1, 0 } });
		auto idxBR = model.pushVertex({ { +r, +r, +r }, math::V3_UP, {}, {}, { 1, 1 } });
		auto idxBL = model.pushVertex({ { -r, +r, +r }, math::V3_UP, {}, {}, { 0, 1 } });
		model.pushTri({ idxTR, idxTL, idxBR });
		model.pushTri({ idxBR, idxTL, idxBL });
	}
	// Bottom
	{
		auto idxTL = model.pushVertex({ { +r, -r, -r }, -math::V3_UP, {}, {}, { 0, 0 } });
		auto idxTR = model.pushVertex({ { -r, -r, -r }, -math::V3_UP, {}, {}, { 1, 0 } });
		auto idxBR = model.pushVertex({ { -r, -r, +r }, -math::V3_UP, {}, {}, { 1, 1 } });
		auto idxBL = model.pushVertex({ { +r, -r, +r }, -math::V3_UP, {}, {}, { 0, 1 } });
		model.pushTri({ idxTR, idxTL, idxBR });
		model.pushTri({ idxBR, idxTL, idxBL });
	}
	// Front
	{
		auto idxTL = model.pushVertex({ { +r, +r, -r }, math::V3_FORWARD, {}, {}, { 0, 0 } });
		auto idxTR = model.pushVertex({ { -r, +r, -r }, math::V3_FORWARD, {}, {}, { 1, 0 } });
		auto idxBR = model.pushVertex({ { -r, -r, -r }, math::V3_FORWARD, {}, {}, { 1, 1 } });
		auto idxBL = model.pushVertex({ { +r, -r, -r }, math::V3_FORWARD, {}, {}, { 0, 1 } });
		model.pushTri({ idxTR, idxTL, idxBR });
		model.pushTri({ idxBR, idxTL, idxBL });
	}
	// Back
	{
		auto idxTL = model.pushVertex({ { -r, +r, +r }, -math::V3_FORWARD, {}, {}, { 0, 0 } });
		auto idxTR = model.pushVertex({ { +r, +r, +r }, -math::V3_FORWARD, {}, {}, { 1, 0 } });
		auto idxBR = model.pushVertex({ { +r, -r, +r }, -math::V3_FORWARD, {}, {}, { 1, 1 } });
		auto idxBL = model.pushVertex({ { -r, -r, +r }, -math::V3_FORWARD, {}, {}, { 0, 1 } });
		model.pushTri({ idxTR, idxTL, idxBR });
		model.pushTri({ idxBR, idxTL, idxBL });
	}
	// Right
	{
		auto idxTL = model.pushVertex({ { +r, +r, +r }, math::V3_RIGHT, {}, {}, { 0, 0 } });
		auto idxTR = model.pushVertex({ { +r, +r, -r }, math::V3_RIGHT, {}, {}, { 1, 0 } });
		auto idxBR = model.pushVertex({ { +r, -r, -r }, math::V3_RIGHT, {}, {}, { 1, 1 } });
		auto idxBL = model.pushVertex({ { +r, -r, +r }, math::V3_RIGHT, {}, {}, { 0, 1 } });
		model.pushTri({ idxTR, idxTL, idxBR });
		model.pushTri({ idxBR, idxTL, idxBL });
	}
	// Left
	{
		auto idxTL = model.pushVertex({ { -r, +r, -r }, -math::V3_RIGHT, {}, {}, { 0, 0 } });
		auto idxTR = model.pushVertex({ { -r, +r, +r }, -math::V3_RIGHT, {}, {}, { 1, 0 } });
		auto idxBR = model.pushVertex({ { -r, -r, +r }, -math::V3_RIGHT, {}, {}, { 1, 1 } });
		auto idxBL = model.pushVertex({ { -r, -r, -r }, -math::V3_RIGHT, {}, {}, { 0, 1 } });
		model.pushTri({ idxTR, idxTL, idxBR });
		model.pushTri({ idxBR, idxTL, idxBL });
	}

	return model;
}
