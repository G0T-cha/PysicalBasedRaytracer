#include "Accelerator\BVHAccel.h"
#include <memory>

namespace PBR {
	static long long treeBytes = 0;
	static long long totalPrimitives = 0;
	static long long totalLeafNodes = 0;
	static long long interiorNodes = 0;
	static long long leafNodes = 0;

    BVHAccel::~BVHAccel() { free(nodes); }
    struct BVHPrimitiveInfo {
        BVHPrimitiveInfo() {}
        BVHPrimitiveInfo(size_t primitiveNumber, const Bounds3f& bounds)
            : primitiveNumber(primitiveNumber),
            bounds(bounds),
            centroid(.5f * bounds.pMin + .5f * bounds.pMax) {}
        size_t primitiveNumber;
        Bounds3f bounds;
        Point3f centroid;
    };

	struct BVHBuildNode {
		void InitLeaf(int first, int n, const Bounds3f& b) {
			firstPrimOffset = first;
			nPrimitives = n;
			bounds = b;
			children[0] = children[1] = nullptr;
			++leafNodes;
			++totalLeafNodes;
			totalPrimitives += n;
		}
		void InitInterior(int axis, BVHBuildNode* c0, BVHBuildNode* c1) {
			children[0] = c0;
			children[1] = c1;
			bounds = Union(c0->bounds, c1->bounds);
			splitAxis = axis;
			nPrimitives = 0;
			++interiorNodes;
		}
		Bounds3f bounds;
		BVHBuildNode* children[2];
		int splitAxis, firstPrimOffset, nPrimitives;
	};

	struct LinearBVHNode {
		Bounds3f bounds;
		union {
			int primitivesOffset;   
			int secondChildOffset;  
		};
		uint16_t nPrimitives;  
		uint8_t axis;          
		uint8_t pad[1];        
	};

	BVHAccel::BVHAccel(std::vector<std::shared_ptr<Primitive>> p,
		int maxPrimsInNode, SplitMethod splitMethod)
		: maxPrimsInNode(std::min(255, maxPrimsInNode)),
		splitMethod(splitMethod),
		primitives(std::move(p)) {
		if (primitives.empty()) return;

		// 遍历图元，暂时存储包围盒和索引到primitiveInfo中
		std::vector<BVHPrimitiveInfo> primitiveInfo(primitives.size());
		for (size_t i = 0; i < primitives.size(); ++i)
			primitiveInfo[i] = { i, primitives[i]->WorldBound() };

		// 递归建树
		int totalNodes = 0;
		std::vector<std::shared_ptr<Primitive>> orderedPrims;
		orderedPrims.reserve(primitives.size());
		BVHBuildNode* root;
		root = recursiveBuild(primitiveInfo, 0, primitives.size(),
			&totalNodes, orderedPrims);

		// 图元重排，确保了叶子节点中的图元在内存中是连续的。
		primitives.swap(orderedPrims);
		primitiveInfo.resize(0);

		// 树的扁平化
		treeBytes += totalNodes * sizeof(LinearBVHNode) + sizeof(*this) +
			primitives.size() * sizeof(primitives[0]);
		nodes = new LinearBVHNode[totalNodes];
		int offset = 0;
		flattenBVHTree(root, &offset);
	}

	Bounds3f BVHAccel::WorldBound() const {
		return nodes ? nodes[0].bounds : Bounds3f();
	}
	struct BucketInfo {
		int count = 0;
		Bounds3f bounds;
	};

	BVHBuildNode* BVHAccel::recursiveBuild(
		std::vector<BVHPrimitiveInfo>& primitiveInfo, int start, int end, int* totalNodes,
		std::vector<std::shared_ptr<Primitive>>& orderedPrims) {
		//递归调用的第一步：为当前子树创建临时根节点、节点总数递增、总包围盒
		BVHBuildNode* node = new BVHBuildNode;
		(*totalNodes)++;
		Bounds3f bounds;
		for (int i = start; i < end; ++i)
			bounds = Union(bounds, primitiveInfo[i].bounds);
		int nPrimitives = end - start;
		//图元数量为1：叶子节点
		if (nPrimitives == 1) {		
			int firstPrimOffset = orderedPrims.size();
			for (int i = start; i < end; ++i) {
				int primNum = primitiveInfo[i].primitiveNumber;
				orderedPrims.push_back(primitives[primNum]);
			}
			node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
			return node;
		}
		else {
			//所有图元中心点重合，无法继续分割：叶子节点
			Bounds3f centroidBounds;
			for (int i = start; i < end; ++i)
				centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
			int dim = centroidBounds.MaximumExtent();
			int mid = (start + end) / 2;
			if (centroidBounds.pMax[dim] == centroidBounds.pMin[dim]) {
				int firstPrimOffset = orderedPrims.size();
				for (int i = start; i < end; ++i) {
					int primNum = primitiveInfo[i].primitiveNumber;
					orderedPrims.push_back(primitives[primNum]);
				}
				node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
				return node;
			}
			else {
				// 选择分割策略
				switch (splitMethod) {
				case SplitMethod::Middle: {
					// 中点法：找到中心点包围盒 (centroidBounds) 在 dim 轴上的空间中点 pmid。
					// 数组就地重排
					float pmid =
						(centroidBounds.pMin[dim] + centroidBounds.pMax[dim]) / 2;
					BVHPrimitiveInfo* midPtr = std::partition(
						&primitiveInfo[start], &primitiveInfo[end - 1] + 1,
						[dim, pmid](const BVHPrimitiveInfo& pi) {
						return pi.centroid[dim] < pmid;
					});
					mid = midPtr - &primitiveInfo[0];
					// 所有图元都在中点同侧-落入均分法
					if (mid != start && mid != end) break;
				}
				case SplitMethod::EqualCounts: {
					// 均分法：找到数量中点
					mid = (start + end) / 2;
					std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
						&primitiveInfo[end - 1] + 1,
						[dim](const BVHPrimitiveInfo& a,
							const BVHPrimitiveInfo& b) {
						return a.centroid[dim] < b.centroid[dim];
					});
					break;
				}
				case SplitMethod::SAH:
				default: {
					// 表面积启发法SAH
					if (nPrimitives <= 2) {
						// 图元过少，退化为均分法
						mid = (start + end) / 2;
						std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
							&primitiveInfo[end - 1] + 1,
							[dim](const BVHPrimitiveInfo& a,
								const BVHPrimitiveInfo& b) {
							return a.centroid[dim] <
								b.centroid[dim];
						});
					}
					else {
						// 创建桶
						constexpr int nBuckets = 12;
						BucketInfo buckets[nBuckets];

						// 遍历图元，根据相对位置放入桶中
						for (int i = start; i < end; ++i) {
							int b = nBuckets *
								centroidBounds.Offset(
									primitiveInfo[i].centroid)[dim];
							if (b == nBuckets) b = nBuckets - 1;
							buckets[b].count++;
							buckets[b].bounds =
								Union(buckets[b].bounds, primitiveInfo[i].bounds);
						}

						// 计算代价（1 是遍历内部节点的代价，
						//count0 * b0.SurfaceArea() 是与左子节点求交的期望代价（代价正比于数量和表面积）。）
						float cost[nBuckets - 1];
						for (int i = 0; i < nBuckets - 1; ++i) {
							Bounds3f b0, b1;
							int count0 = 0, count1 = 0;
							for (int j = 0; j <= i; ++j) {
								b0 = Union(b0, buckets[j].bounds);
								count0 += buckets[j].count;
							}
							for (int j = i + 1; j < nBuckets; ++j) {
								b1 = Union(b1, buckets[j].bounds);
								count1 += buckets[j].count;
							}
							cost[i] = 1 +
								(count0 * b0.SurfaceArea() +
									count1 * b1.SurfaceArea()) /
								bounds.SurfaceArea();
						}

						// 找到最小代价对应的最佳分割
						float minCost = cost[0];
						int minCostSplitBucket = 0;
						for (int i = 1; i < nBuckets - 1; ++i) {
							if (cost[i] < minCost) {
								minCost = cost[i];
								minCostSplitBucket = i;
							}
						}

						// 执行分割，找出分割点
						float leafCost = nPrimitives;
						if (nPrimitives > maxPrimsInNode || minCost < leafCost) {
							BVHPrimitiveInfo* pmid = std::partition(
								&primitiveInfo[start], &primitiveInfo[end - 1] + 1,
								[=](const BVHPrimitiveInfo& pi) {
								int b = nBuckets *
									centroidBounds.Offset(pi.centroid)[dim];
								if (b == nBuckets) b = nBuckets - 1;
								return b <= minCostSplitBucket;
							});
							mid = pmid - &primitiveInfo[0];
						}
						else {
							// SAH判定分割不划算
							int firstPrimOffset = orderedPrims.size();
							for (int i = start; i < end; ++i) {
								int primNum = primitiveInfo[i].primitiveNumber;
								orderedPrims.push_back(primitives[primNum]);
							}
							node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
							return node;
						}
					}
					break;
				}

				}
				//递归处理左半部分/右半部分
				node->InitInterior(dim,
					recursiveBuild(primitiveInfo, start, mid,
						totalNodes, orderedPrims),
					recursiveBuild(primitiveInfo, mid, end,
						totalNodes, orderedPrims));
			}
		}
		return node;
	}
	//深度优先：树的扁平化
	//offset是一个全局偏移量
	int BVHAccel::flattenBVHTree(BVHBuildNode* node, int* offset) {
		LinearBVHNode* linearNode = &nodes[*offset];
		linearNode->bounds = node->bounds;
		//递增偏移量作为新的槽位
		int myOffset = (*offset)++;
		//递归中止
		if (node->nPrimitives > 0) {
			linearNode->primitivesOffset = node->firstPrimOffset;
			linearNode->nPrimitives = node->nPrimitives;
		}
		else {
			// 复制分割轴
			linearNode->axis = node->splitAxis;
			linearNode->nPrimitives = 0;
			//递归调用左子节点，会填充 myOffset + 1以及所有后续槽位。
			flattenBVHTree(node->children[0], offset);
			//递归调用右子节点。这个调用会返回右子节点被放置的起始索引
			//并将其存储在当前节点的 secondChildOffset (第二个子节点偏移) 字段中。
			linearNode->secondChildOffset =
				flattenBVHTree(node->children[1], offset);
		}
		return myOffset;
	}

	bool BVHAccel::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
		if (!nodes) return false;
		bool hit = false;
		// 预先计算除法
		Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
		int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };
		// 栈，currentNodeIndex是正在处理的节点
		int toVisitOffset = 0, currentNodeIndex = 0;
		int nodesToVisit[64];
		while (true) {
			const LinearBVHNode* node = &nodes[currentNodeIndex];
			// 光线是否击中了包围盒？
			if (node->bounds.IntersectP(ray, invDir, dirIsNeg)) {
				//击中叶子节点
				if (node->nPrimitives > 0) {
					// 遍历图元，调用图元的相交函数
					for (int i = 0; i < node->nPrimitives; ++i)
						if (primitives[node->primitivesOffset + i]->Intersect(ray, isect))
							hit = true;
					// 弹出栈
					if (toVisitOffset == 0) break;
					currentNodeIndex = nodesToVisit[--toVisitOffset];
				}
				// 击中内部节点
				else {
					//先测试较近的子节点，较远 (Far) 的那个子节点压入 (Push) 栈中，后续处理
					if (dirIsNeg[node->axis]) {
						nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
						currentNodeIndex = node->secondChildOffset;
					}
					else {
						nodesToVisit[toVisitOffset++] = node->secondChildOffset;
						currentNodeIndex = currentNodeIndex + 1;
					}
				}
			}
			//未击中，栈空遍历结束，栈不空弹出继续循环
			else {
				if (toVisitOffset == 0) break;
				currentNodeIndex = nodesToVisit[--toVisitOffset];
			}
		}
		return hit;
	}

	bool BVHAccel::IntersectP(const Ray& ray) const {
		if (!nodes) return false;
		Vector3f invDir(1.f / ray.d.x, 1.f / ray.d.y, 1.f / ray.d.z);
		int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };
		int nodesToVisit[64];
		int toVisitOffset = 0, currentNodeIndex = 0;
		while (true) {
			const LinearBVHNode* node = &nodes[currentNodeIndex];
			if (node->bounds.IntersectP(ray, invDir, dirIsNeg)) {
				if (node->nPrimitives > 0) {
					for (int i = 0; i < node->nPrimitives; ++i) {
						if (primitives[node->primitivesOffset + i]->IntersectP(
							ray)) {
							return true;
						}
					}
					if (toVisitOffset == 0) break;
					currentNodeIndex = nodesToVisit[--toVisitOffset];
				}
				else {
					if (dirIsNeg[node->axis]) {
						nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
						currentNodeIndex = node->secondChildOffset;
					}
					else {
						nodesToVisit[toVisitOffset++] = node->secondChildOffset;
						currentNodeIndex = currentNodeIndex + 1;
					}
				}
			}
			else {
				if (toVisitOffset == 0) break;
				currentNodeIndex = nodesToVisit[--toVisitOffset];
			}
		}
		return false;
	}

}