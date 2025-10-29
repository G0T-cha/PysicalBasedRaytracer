#pragma
#ifndef BVHACCEL_H
#define BVHACCEL_H

#include "Core\PBR.h"
#include "Core\primitive.h"

#include <vector>
#include <memory>

namespace PBR {
	struct BVHBuildNode;
	struct BVHPrimitiveInfo;
	struct LinearBVHNode;

	class BVHAccel : public Aggregate {
	public:
		enum class SplitMethod { SAH, HLBVH, Middle, EqualCounts };
		BVHAccel(std::vector<std::shared_ptr<Primitive>> p,
			int maxPrimsInNode = 1,
			SplitMethod splitMethod = SplitMethod::SAH);
		Bounds3f WorldBound() const;
		~BVHAccel();
		//渲染光线时执行，触发遍历
		bool Intersect(const Ray& ray, SurfaceInteraction* isect) const;
		bool IntersectP(const Ray& ray) const;

	private:
		//递归建树
		BVHBuildNode* recursiveBuild(std::vector<BVHPrimitiveInfo>& primitiveInfo,
			int start, int end, int* totalNodes,
			std::vector<std::shared_ptr<Primitive>>& orderedPrims);
		//树的扁平化
		int flattenBVHTree(BVHBuildNode* node, int* offset);
		const int maxPrimsInNode;
		const SplitMethod splitMethod;
		std::vector<std::shared_ptr<Primitive>> primitives;
		LinearBVHNode* nodes = nullptr;
	};

}
#endif