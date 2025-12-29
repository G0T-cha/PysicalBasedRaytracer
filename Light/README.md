# 光源系统Light

`Light` 模块定义了场景中各种光源的接口和实现。光源是渲染方程中能量的来源，`Integrator` 通过调用光源接口来采样光照方向和计算光照强度，以模拟直接光照和（在路径追踪中）间接光照。本模块同样采用接口与实现分离的设计。

## 1. 基类 (`Light.h`)

- **`LightFlags` (枚举)**:
  - 使用位掩码 (`DeltaPosition`, `DeltaDirection`, `Area`, `Infinite`) 来标记光源的类型。这允许 `Integrator` 根据光源特性选择不同的采样策略（例如，点光源 `DeltaPosition` 不能直接被 BSDF 采样击中）。`IsDeltaLight()` 函数用于快速判断。
- **`VisibilityTester` (类)**:
  - 封装了**两个点**（通常是被着色点 `p0` 和光源采样点 `p1`）之间的可见性测试信息。
  - **`Unoccluded(scene)`**: 核心函数。其内部实现（在 `Light.cpp` 中）通过 `p0.SpawnRayTo(p1)` 创建一条具有 `tMax ≈ 1` 的阴影光线，并调用 `scene.IntersectP()` 进行遮挡测试，返回 `!scene.IntersectP()` 的结果。
- **`virtual Spectrum Sample_Li(ref, u, *wi, *pdf, *vis) const = 0;`**:
  - **光源采样 (Light Sampling)**。
  - 给定被着色点 `ref` 和 2D 样本 `u`，在光源上（或从光源方向）选择一个点（或方向），计算指向该点的入射方向 `*wi`、对应的**立体角 PDF** `*pdf`，创建 `VisibilityTester *vis`，并返回该方向入射到 `ref` 点的辐射度 `Li`。
- **`virtual float Pdf_Li(ref, wi) const = 0;`**:
  - **PDF 查询函数** (用于 MIS)。
  - 给定 `ref` 和一个入射方向 `wi`，返回光源采样策略 (`Sample_Li`) 采样到该 `wi` 方向的**立体角 PDF**。
- **`virtual Spectrum Power() const = 0;`**:
  - **计算总功率**。返回光源向整个场景发射的总辐射功率。用于高级积分器的光源选择策略。
- **`virtual Spectrum Le(ray) const;`**:
  - **查询自发光**。对于未击中场景或直接击中光源的光线 `ray`，返回其接收到的自发光辐射度。`Infinite` 光源会重写此函数。
- **`virtual Spectrum Sample_Le(u1, u2, time, *ray, *nLight, *pdfPos, *pdfDir) const = 0;`**:
  - **采样自发光** (用于高级算法，如双向路径追踪)。
  - 从光源自身出发，采样一条出射光线 `ray`，并返回起点法线 `nLight`、选择该起点的**面积 PDF** `pdfPos` 以及选择该方向的**立体角 PDF** `pdfDir`。
- **`virtual void Pdf_Le(ray, nLight, *pdfPos, *pdfDir) const = 0;`**:
  - **查询自发光 PDF** (用于高级算法)。
  - **职责**: 给定一条从光源发出的光线 `ray`，返回 `Sample_Le` 生成该光线的 `*pdfPos` 和 `*pdfDir`。

## 2. 具体光源实现

### 2.1. `PointLight` (点光源) (`PointLight.h/.cpp`)

- **类型**: `DeltaPosition`。
- **构造**: 计算并存储世界空间位置 `pLight` 和强度 `I`。
- **光源采样**`Sample_Li`:
  1. 计算从 `ref.p` 指向 `pLight` 的**唯一**方向 `*wi`。
  2. 设置 `*pdf = 1.0f` (Delta 分布约定)。
  3. 创建连接 `ref` 和 `pLight` 的 `VisibilityTester`。
  4. 返回 `I / DistanceSquared(pLight, ref.p)`（强度按平方反比衰减）。
- **查询光源采样PDF** `Pdf_Li`: 返回 0 (Delta 分布特性)。
- **能量 **`Power`: 返回 `4 * Pi * I` (强度乘以球面立体角)。
- **自发光采样** `Sample_Le`:
  1. 设置 `ray->o = pLight`。
  2. 使用 `UniformSampleSphere(u1)` 均匀采样 `ray->d`。
  3. 设置 `*pdfPos = 1` (Delta 位置约定)，`*pdfDir = UniformSpherePdf()` (`1 / (4*Pi)`)。
  4. 返回强度 `I`。
- **查询自发光 PDF** `Pdf_Le`: 设置 `*pdfPos = 0`（假设无法精确命中点光源位置的简化处理），`*pdfDir = UniformSpherePdf()`。

### 2.2. `DiffuseAreaLight` (漫射面光源) (`DiffuseLight.h/.cpp`)

- **类型**: `Area`。
- **构造**: 存储发射辐射度 `Lemit`、几何形状 `shape` 指针、`twoSided` 标志，并预计算存储 `area = shape->Area()`。
- **自发光辐照度** `L(intr, w)`: 实现 `AreaLight` 接口。检查 `twoSided` 或 `Dot(intr.n, w) > 0`，如果满足则返回 `Lemit`，否则返回 0。
- **光源采样** `Sample_Li`:
  1. **委托给 `shape->Sample(ref, u, pdf)`**，在 `shape` 表面按面积采样点 `pShape`，并获取转换后的**立体角 PDF** `*pdf`。
  2. 进行健壮性检查 (`pdf == 0` 或距离为 0)。
  3. 计算方向 `*wi = Normalize(pShape.p - ref.p)`。
  4. 创建连接 `ref` 和 `pShape` 的 `VisibilityTester`。
  5. 返回 `L(pShape, -*wi)`（查询采样点沿 `-wi` 方向的自发光）。**这里返回的是辐射度，距离衰减隐含在 `pdf` 中**。
- **光源采样PDF** `Pdf_Li`**: **委托给 `shape->Pdf(ref, wi)`**，**获取给定方向 `wi` 击中 `shape` 的**立体角 PDF**。
- **功率**`Power` : 返回 `(twoSided ? 2 : 1) * Lemit * area * Pi` (辐射度 * 面积 * Pi)。
- **自发光采样** `Sample_Le`:
  1. **委托给 `shape->Sample(u1, pdfPos)`** 按面积采样起点 `pShape`，获取**面积 PDF** `*pdfPos`。
  2. 使用 `CosineSampleHemisphere(u2)` 在 `pShape.n` 的半球内采样本地方向 `w`，获取**方向 PDF** `*pdfDir = CosineHemispherePdf(...)` (`cos/Pi`)。
  3. 将本地方向 `w` 转换到世界空间 `wWorld`。
  4. 创建光线 `*ray(pShape.p, wWorld)`。
  5. 返回 `L(pShape, wWorld)`。
- **查询自发光 PDF** `Pdf_Le`:
  1. **委托给 `shape->Pdf(Interaction(ray->o, ...))`** 获取起点 `ray->o` 的**面积 PDF** `*pdfPos`。
  2. 计算 `cosTheta = Dot(nLight, ray->d)`。
  3. 返回余弦采样的**方向 PDF** `*pdfDir = CosineHemispherePdf(cosTheta)`。

### 2.3. `SkyBoxLight` (天空盒/环境光) (`SkyBoxLight.h/.cpp`)

- **类型**: `Infinite`。
- **构造**: 标记类型，存储场景边界 `worldCenter`, `worldRadius`，并调用 `loadImage` 加载 HDR 图像到 `float* data`。
- **`loadImage`**: 使用 `stb_image::stbi_loadf` 加载浮点 HDR 图像，**启用垂直翻转**。
- **`get_sphere_uv(p, &u, &v)`**: 核心辅助函数，将**归一化方向向量 `p`** 通过 `atan2(p.z, p.x)` (phi) 和 `asin(p.y)` (theta) 转换为 `[0, 1]` 范围的 **UV 坐标**。
- **`getLightValue(u, v)`**: 根据 UV 坐标，在 `data` 数组中执行**最近邻查找**，获取 HDR 颜色值。**注意：实现中包含了一个 `HDRtoLDR` 的转换，这可能并非预期行为，应直接返回 HDR 值**。
- **查询自发光**`Le(ray)`:
  1. 获取归一化方向 `dir = Normalize(ray.d)`。
  2. 调用 `get_sphere_uv` 将 `dir` 转换为 `(u, v)`。
  3. 调用 `getLightValue(u, v)` 返回该方向的天空颜色。若 `data` 未加载，则返回基于方向向量的彩色背景（用于调试）。
- **光源采样**`Sample_Li`:
  1. 使用 `UniformSampleSphere(u)` **均匀采样**球面方向 `*wi`。**注意：未实现基于亮度的重要性采样**。
  2. 设置 `*pdf = UniformSpherePdf()` (`1 / (4*Pi)`)。
  3. 创建 `VisibilityTester`，将终点设置为沿 `wi` 方向推到场景边界 (`worldRadius`) 之外的点。
  4. 将采样方向 `*wi` 转换回 UV (`u_lookup`, `v_lookup`)。
  5. 调用 `getLightValue(u_lookup, v_lookup)` 返回该方向的辐射度 `Li`。
- **`Pdf_Li`**: 返回 0。**暂未实现 PDF 查询**。
- **`Power`**: 暂返回 0 (简化处理)。
- **`Sample_Le` / `Pdf_Le`**: 暂返回 0 或空实现 (简化处理)。

### 2.4. `InfiniteAreaLight` (基于物理的环境光源)

这是专为基于图像的照明 (IBL) 设计的高级环境光源。它不仅是 `SkyBoxLight` 的功能替代品，更引入了复杂的预计算机制，旨在解决蒙特卡洛积分中极其重要的**方差（噪点）问题**。

在渲染高动态范围 (HDR) 环境贴图时，绝大部分光能往往集中在极小的区域（例如天空中的太阳可能占据不到 1% 的像素，却提供了 90% 的照明）。如果使用传统的均匀球面采样，绝大部分样本会浪费在对场景贡献极小的“暗部”区域，导致渲染结果噪点极多，收敛极慢。

`InfiniteAreaLight` 通过以下机制彻底解决了这个问题：

#### 1. 智能预处理阶段 (构造函数)
在场景加载时，光源会进行一次昂贵的预计算，将“盲目”的纹理数据转化为可用于“智能”采样的数据结构。

* **构建 MIPMap (`Lmap`)**:
    * 原始的 HDR 图像数据并没有被简单地存为二维数组，而是被封装进了 **`MIPMap<RGBSpectrum>`**。
    * **目的**: 这不仅是为了节省后续查询的开销，更是为了抗锯齿。当光线查询环境贴图中极其细微的高频细节时，MIPMap 能够自动选择合适的层级进行滤波，避免了因点采样引起的闪烁和噪点。
* **构建亮度分布 (`Distribution2D`)**:
    * 这是重要性采样的核心。构造函数会遍历图像的每一个像素，计算其相对贡献。
    * **球面失真校正**: 在经纬度映射（Equirectangular Mapping）中，靠近极点的像素在球面上实际覆盖的面积要远小于赤道处的像素。因此，在计算像素贡献时，必须乘以一个球面校正因子。
    * **生成 CDF**: 利用校正后的亮度值，构建一个二维累积分布函数 (CDF) `Distribution2D`。这个结构本质上是一张“概率地图”，告诉采样器：“图像的这个区域非常亮，你应该更频繁地采样这里”。

#### 2. 高效渲染阶段 (运行时)
在积分器计算直接光照时，`InfiniteAreaLight` 能够提供高质量的样本。

* **重要性采样 (`Sample_Li`)**:
    * 当需要采样环境光时，它不再使用 `UniformSampleSphere`。
    * 它调用 `distribution->SampleContinuous(u, &mapPdf)`。这个函数会利用预计算的 CDF，将均匀分布的随机数 u 映射到高亮区域更密集的 UV 坐标上。
    * 结果是：太阳等强光源区域会被频繁选中，而黑暗的天空背景则很少被选中。这极大地降低了蒙特卡洛估计量的方差。
* **概率密度转换**:
    * `Distribution2D` 返回的是 2D 图像空间 $(u,v)$ 上的概率密度 ($PDF_{uv}$)。
    * 为了用于渲染方程的蒙特卡洛积分，必须将其转换为 3D 方向空间上的**立体角概率密度 ($PDF_{\omega}$)**。
    * 代码中应用了雅可比行列式进行转换：$PDF_{\omega} = \frac{PDF_{uv}}{2\pi^2 \sin\theta}$。这一步确保了数学上的严格正确性，否则渲染结果会出现偏差（Bias）。
* **辐射度查询**:
    * 最后，利用选定的 UV 坐标从 `MIPMap` 中查询精确的 HDR 颜色值，作为该方向的入射辐射度 $L_i$ 返回给积分器。