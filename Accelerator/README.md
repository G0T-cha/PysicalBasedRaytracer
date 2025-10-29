# BVHAccel

## Core/Primitive

`BVHAccel` 并不直接操作 `Shape` (形状)，而是操作 `Primitive` (图元) 。

`Primitive` (图元) 是场景中可渲染对象的基本单元，两个子类：

1. **`GeometricPrimitive` (几何图元)**: 将**几何** (`Shape` (形状))、**外观** (`Material` (材质)) 和**发光** (`AreaLight` (面光源)) 作为成员。我们场景中的每一个具体物体都是一个 `GeometricPrimitive` (几何图元)。
2. **`Aggregate` (聚合图元)**: 这是一个“容器”，它本身也是一个 `Primitive` (图元)，但它内部**持有**一个 `Primitive` (图元) 列表。

**`BVHAccel` (BVH加速结构) 本身就是 `Aggregate` (聚合图元) 的一个子类**。

这样，`BVHAccel` (BVH加速结构) 的 `primitives` (图元) 列表中就可以包含 `GeometricPrimitive` (几何图元)（例如一个球体），也可以包含**另一个 `BVHAccel` (BVH加速结构)**（例如一个包含一百万个三角形的模型）。这使得 `BVH` (包围盒层次结构) 可以被优雅地嵌套，`Integrator` (积分器) 只需调用顶层 `BVH` (包围盒层次结构) 的 `Intersect` (相交) 接口即可。

## BVHAccel

`BVHAccel` (Bounding Volume Hierarchy, 包围盒层次结构) 继承自 `Aggregate` (聚合)，负责高效地（以 `O(log N)` 复杂度）处理光线与大量 `Primitive` (图元) 的求交测试。

`BVHAccel` (BVH加速结构) 的工作流程分为两个截然不同的阶段：**构建 (Build)** 和**遍历 (Traversal)**。

### 1.1. 构建阶段 (BVHAccel 构造函数)

构建阶段在 `BVHAccel` (BVH加速结构) 的构造函数中**一次性**完成。它接收一个 `std::vector<std::shared_ptr<Primitive>>`（即场景中的所有图元），并执行算法来生成一棵高效的树。

- **分割策略 (`SplitMethod`)**: 构造函数允许指定一个 `SplitMethod` (分割方法)，默认使用 `SplitMethod::SAH` (Surface Area Heuristic, 表面积启发式)。SAH 是一种高质量的启发式算法，它会智能地评估所有可能的分割方案，选择一个能最小化未来遍历代价的分割平面。
- 其他两种策略是中点法（轴均分）、均分法（数量均分）
- **递归构建 (`recursiveBuild`)**: `recursiveBuild` 函数递归地执行：
  1. **判断叶子节点**: 如果当前图元列表小于 `maxPrimsInNode` (最大叶子节点图元数)，则停止递归，创建叶子节点。
  2. **选择分割轴**: 否则，根据 `SAH` 策略，选择最佳分割轴和位置。——创建桶、计算沿各个桶分割的代价：正比于数量和表面积。
  3. **分区 (Partition)**: 将图元列表分为“左”和“右”两组。
  4. **递归**: 对“左”组和“右”组分别递归调用 `recursiveBuild`。
- **树的扁平化 (`flattenBVHTree`)**: 递归构建产生的 `BVHBuildNode` 树（使用指针）对于 CPU 缓存**极其不友好**。因此，`flattenBVHTree` (扁平化BVH树) 函数会执行一次深度优先遍历，将 `BVHBuildNode` 树转换（“扁平化”）为一个**线性的 `LinearBVHNode` 数组**（存储在 `LinearBVHNode* nodes` 成员中）。
  - `LinearBVHNode` (线性BVH节点) 是一个专为遍历优化的紧凑结构，它**不使用指针**，而是使用**整数偏移量**（例如 `int secondChildOffset`）来定位其子节点，从而获得极佳的缓存局部性。

### 1.2. 遍历阶段 (Intersect / IntersectP)

遍历阶段在渲染时**每条光线**都会执行。`Integrator` (积分器) 调用 `BVHAccel::Intersect` 或 `IntersectP` (用于阴影光线)，触发 BVH 的遍历。

- **迭代式遍历**: 得益于 `LinearBVHNode` (线性BVH节点) 的扁平化设计，遍历过程是**迭代的**（使用一个小型栈），而不是递归的，这对性能至关重要。
- **遍历流程**:
  1. 算法从 `nodes` (节点) 数组的根节点 (`index = 0`) 开始循环。
  2. **测试包围盒**: 测试 `ray` (光线) 是否击中了当前 `LinearBVHNode` (线性BVH节点) 的包围盒。
  3. **剪枝 (Prune)**: 如果 `ray` (光线) **未**击中包围盒，则该节点及其所有子节点（可能包含数万个三角形）被**立即跳过**。
  4. **内部节点**: 如果击中了包围盒，且该节点是内部节点，则将其（一个或两个）被击中的子节点索引压入栈中(优先处理更近的节点)，继续循环。
  5. **叶子节点**: 如果击中了包围盒，且该节点是叶子节点，算法会遍历该叶子节点所对应的**一小段** `primitives` (图元) 列表（例如 `maxPrimsInNode` 个图元），并对它们逐一调用 `Primitive::Intersect`。