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
		//��Ⱦ����ʱִ�У���������
		bool Intersect(const Ray& ray, SurfaceInteraction* isect) const;
		bool IntersectP(const Ray& ray) const;

	private:
		//�ݹ齨��
		BVHBuildNode* recursiveBuild(std::vector<BVHPrimitiveInfo>& primitiveInfo,
			int start, int end, int* totalNodes,
			std::vector<std::shared_ptr<Primitive>>& orderedPrims);
		//���ı�ƽ��
		int flattenBVHTree(BVHBuildNode* node, int* offset);
		const int maxPrimsInNode;
		const SplitMethod splitMethod;
		std::vector<std::shared_ptr<Primitive>> primitives;
		LinearBVHNode* nodes = nullptr;
	};

}
#endif