- # 形状系统

  本模块 (`Shape/`) 定义了场景中所有**几何体**的抽象接口和具体实现，负责回答“光线击中了几何体吗？”

  在深入了解 `Shape` 之前，我们必须先了解 `Shape::Intersect` 函数的**输出**——`Interaction` 结构体。

  ## 1. Core/Interaction.h`

  `Interaction.h` (及其 `.cpp` 文件) 定义了渲染器中**重要的结构体**：`SurfaceInteraction` 。

  当 `Shape::Intersect` 函数返回 `true` 时，会填充一个 `SurfaceInteraction` 结构体，其中详细记录了光线与表面碰撞瞬间的所有必要信息，并将这些信息传递给后续的 `Material` (材质) 和 `Integrator` (积分器) 模块。

  ### `SurfaceInteraction` 存储了什么？

  `SurfaceInteraction` 继承自基类 `Interaction`，并添加了表面几何特有的信息：

  - **`Point3f p`**: 光线击中表面的**世界空间坐标**。
  - **`Normal3f n`**: 该点的**几何法线**（Geometric Normal）。
  - **`Point2f uv`**: 该点的 **UV 纹理坐标**（通过重心坐标插值或球面坐标计算得出）。
  - **`Vector3f wo`**: **出射光方向**（即指向光线来源的方向）。
  - **`const Primitive \*primitive`**: 一个指针，指向被击中的**图元** (`Primitive`)。这使得 `Integrator` 可以通过它获取 `Material` 信息。
  - **`Shading` 结构体**: 存储用于**着色 (Shading)** 的坐标系信息，包括 `n` (着色法线, Shading Normal)、`dpdu` (U方向切线)、`dpdv` (V方向切线)。
  - **`BSDF \*bsdf`**: 一个指针，用于存储该点的 BSDF (双向散射分布函数)。`Integrator` 会调用 `isect.ComputeScatteringFunctions()` (在 `Interaction.cpp` 中实现)，该函数会使用 `primitive` 指针找到 `Material`，并用 `Material` 来初始化这个 `bsdf` 成员。
  - 判断是否需要进行**法线翻转**。

  其中还包含几个函数：

  - **`Ray SpawnRay(const Vector3f& d) const`** : 生成**通用的“次级光线”**，例如**反射 **或**折射**光线，新光线的起点将是由当前点p沿着法线推离表面得到的点。
  - **`Ray SpawnRayTo(const Point3f& p2) const`**:  生成**阴影光线 (Shadow Ray)**，用于测试**当前点 `p`** 和**目标点 `p2`**（例如光源上的一个点）之间是否有遮挡。细节是最大距离 `tMax` 只被设置为 `1.0`，因为我们不关心后面是否还会击中其他物体。
  - **`Ray SpawnRayTo(const Interaction& it) const`**: 连接**两个表面**上的点，创建一条更安全的阴影光线。
  - **`ComputeScatteringFunctions(const Ray& ray, bool allowMultipleLobes, TransportMode mode)`**: **转发**给：`primitive->ComputeScatteringFunctions(this, ...)`，再转发给：`material->ComputeScatteringFunctions(this, ...)`，`Material`（例如 `MatteMaterial` 或 `MirrorMaterial`）最终执行实际工作：它会**创建**一个 `BSDF` 对象，并向其添加相应的 `BxDF`（如 `LambertianReflection` 或 `SpecularReflection`），最后将这个 `BSDF` 赋给 `isect->bsdf` 指针。
  - **`Spectrum SurfaceInteraction::Le(const Vector3f& w)`** : 回答光线所击中的这个表面，本身是否为光源，发出的光有多亮。会调用`primitive->ComputeScatteringFunctions(this, mode, allowMultipleLobes);`。

  **总结**: `Shape` 模块的职责是**计算**并**填充** `SurfaceInteraction` 中的几何信息 (p, n, uv, dpdu, dpdv)。`Integrator` 模块则负责**使用**这些信息（例如调用 `isect.ComputeScatteringFunctions()`）来计算光照和材质。

  ## 2. Shape.h

  `Shape.h` 文件定义了所有几何体都必须遵守的抽象基类 `Shape`。`Integrator` (积分器) 和 `BVHAccel` (加速结构) 只这个基类接口交互。

  ### 2.1. 核心接口 (纯虚函数)

  - **`virtual bool Intersect(const Ray &ray, float *tHit, SurfaceInteraction *isect) const = 0;`**:
    - **这是 `Shape` 最核心的函数**。
    - **作用**: 测试一条 `ray` 是否击中了该几何体。
    - **输出**: 如果击中，它必须返回 `true`，并通过指针填充三个关键信息：
      1. `tHit`: 光线到交点的距离 `t`。
      2. `isect`: 一个 `SurfaceInteraction` 结构体，包含交点的**所有**几何信息（3D位置、法线、UV坐标、切线等）。
  - **`virtual bool IntersectP(const Ray &ray) const = 0;`**:
    - **"P" 代表 "Predicate" (谓词)**。这是一个优化的“影子光线”测试。
    - **作用**: 只需判断**是否存在**交点，而**不需要**计算交点的详细信息 (`tHit` 或 `isect`)。
    - **应用**: 在计算直接光照时，我们只需要知道交点和光源之间是否有遮挡物 (`true`/`false`)。`IntersectP` 会跳过所有昂贵的 `SurfaceInteraction` 计算，因此速度快得多。
  - **`virtual Bounds3f ObjectBound() const = 0;` / `virtual Bounds3f WorldBound() const = 0;`**:
    - **作用**: 返回该几何体在**对象空间 (Object Space)** 和**世界空间 (World Space)** 中的轴对齐包围盒 (AABB)，用于BVH。

  - **`virtual Interaction Sample(const Point2f& u, float* pdf) const = 0;
    virtual float Pdf(const Interaction&) const { return 1 / Area(); }`**:
    - **作用**: 按面积 (Area) 采样，在表面积上均匀地选取一个随机点，pdf则为1/表面积。
  - **`virtual Interaction Sample(const Interaction& ref, const Point2f& u, float* pdf) const; virtual float Pdf(const Interaction& ref, const Vector3f& wi) const;`**:
    - **作用**: 执行光源采样，在外部一被着色点`ref`处，在立体角中选择一个该形状（光源）的采样点。对应的pdf在MIS中会用到。

  ### 2.2. 数据成员

  - **`const Transform *ObjectToWorld, *WorldToObject;`**:
    - 指向变换矩阵的指针。这允许一个 `Shape` (例如一个单位球体) 在场景中被多次实例化 (平移、旋转、缩放)，而无需存储多份几何数据。
  - **`const bool reverseOrientation;`**:
    - 如果为 `true`，则翻转该形状的法线。这对于创建天空盒或封闭房间（相机在内部）非常有用。

  ## 3. `Sphere` (球体)

  `Sphere` 是最基础的几何图元，继承自 `Shape`。

  ### 3.1. `Intersect` 逻辑

  `Sphere::Intersect` 是教科书式的光线追踪算法：

  1. **空间变换**: 为了简化计算，该函数首先将传入的 `ray` (在世界空间) 通过 `WorldToObject` 变换到球体的**对象空间**。在这个空间中，球体永远位于原点 `(0, 0, 0)` 且半径为 `radius`。
  2. **求解二次方程**: 光线 `Ray(o, d)` 和球体 `x^2+y^2+z^2 = r^2` 的交点，可以通过求解一个关于 `t` 的一元二次方程 `At^2 + Bt + C = 0` 得到。
     - `A = d · d` (光线方向的点积)
     - `B = 2 * (d · o)` (光线方向和原点的点积)
     - `C = o · o - radius^2` (光线原点的点积减半径平方)
  3. **判别式**: 计算判别式 `discriminant = B^2 - 4AC`。
     - 如果 `discriminant < 0`，方程无实数解，光线未击中球体。
  4. **查找 `tHit`**: 求解两个潜在的交点 `t0` 和 `t1`。算法会检查这两个 `t` 值是否在光线的有效范围内 (`t > 0` 且 `t < ray.tMax`)，并取**较近**的那个有效 `t` 作为 `tHit`。
  5. **填充 `SurfaceInteraction`**: 如果找到了 `tHit`，函数会计算：
     - `pHit`: 交点位置 (在对象空间中)，`ray(tHit)`。
     - `n`: 法线 (在对象空间中，就是 `pHit / radius`)。
     - `uv`: 球面坐标 `(phi, theta)`，用于纹理映射。
     - `dpdu`, `dpdv`: 偏导数（切线向量）。
     - 最后，将 `pHit` 和 `n` 等信息通过 `ObjectToWorld` 变换回**世界空间**，填充到 `isect` 结构体中。

  ## 4. `Triangle` (三角形)

  ### 4.1. `Intersect` 

  通过一系列变换（平移、置换和剪切），将三角形**变换到“光线空间”**中。在这个新空间里，光线被简化为从原点 `(0,0,0)` 沿着 `+Z` 轴发射。算法通过计算三个“**边函数**”（`e0, e1, e2`，本质是 2D 叉乘）来执行 2D 测试。如果 `e0, e1, e2` 全都同号，则光线命中。这些边函数值（除以它们的总和 `det`）还能**直接**提供**重心坐标**（`b0, b1, b2`）。`t` (距离) 则是通过使用这些重心坐标来**插值**三个变换后顶点的 `z` 坐标（`p0t.z` 等）计算得出。在返回 `true` 之前，算法还会执行一个关键的**浮点数精度测试**（`t > deltaT`），以防止“自相交”导致的阴影痤疮。最后，如果命中，函数会使用重心坐标 `(b0, b1, b2)` 来插值**原始的**顶点位置、UV 和法线，并计算切线 `dpdu/dpdv`，用以填充完整的 `SurfaceInteraction` 结构体。

  ### 4.2.  `Sample` 

  在三角形的表面积上，均匀地（uniformly）选取一个随机点，并返回该点的信息及其面积 PDF。输入的是采样器生成的一个2D点，返回 `Interaction` 结构体采样的点的位置 p、修正后的几何法线 n 和浮点数误差 pError，以及PDF。

  ## 5. 模型加载：`plyRead.h`

  - **作用**: 这是一个辅助工具，负责解析 `.ply` (Stanford Polygon Library) 3D 模型文件。它读取文件中的顶点列表 (Vertices)、面列表 (Faces)，并提取顶点位置 `p`、顶点法线 `n` (如果存在) 和顶点 UV `uv` (如果存在)。