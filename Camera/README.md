# 相机系统详解

本模块 (`Camera/`) 负责将2D的像素坐标转换为3D的世界空间光线，这是光线追踪的起点。本项目的相机系统设计具有良好的扩展性，围绕着 `Camera.h` 中的几个核心类展开。

## 1. 核心架构：三层继承

本系统的设计是逐层抽象的：

1. **`Camera` (基类)**: 定义了所有相机的最基本接口。
   - **`Transform CameraToWorld`**: 存储相机在世界空间中的位置和朝向。
   - **`virtual float GenerateRay(...)`**: 纯虚函数，是相机模块**最核心的函数**，负责根据采样信息生成一条光线。
2. **`ProjectiveCamera` (中间层)**: 继承自 `Camera`，处理所有投影相机共有的**坐标空间变换**。
   - **`Transform CameraToScreen`**: 存储投影矩阵。
   - **`Transform ScreenToRaster`**: 在构造时被预先计算，负责将浮点数的“屏幕空间”(`screenWindow`) 映射到 `[0, Width] x [0, Height]` 的光栅（像素）空间。
   - **`Transform RasterToCamera`**: 通过 `Inverse(CameraToScreen) * Inverse(ScreenToRaster)` 计算得出。`GenerateRay` 函数使用该矩阵，将像素坐标反向投影回相机空间中的一个3D点。
   - **`lensRadius`, `focalDistance`**: 存储景深参数。
3. **`PerspectiveCamera` / `OrthographicCamera` (实现层)**: 继承自 `ProjectiveCamera`，是可以实际创建和使用的具体相机。
   - 在构造时，负责创建**特定类型**的投影矩阵，并将其传递给 `ProjectiveCamera` 父类。
   - 实现了 `GenerateRay` 函数。

## 2. 结构体`CameraSample`

`Integrator` (积分器) 在渲染一个像素时，会先向 `Sampler` (采样器) 请求一组随机样本，并打包成 `CameraSample` 结构体：

```
struct CameraSample {
    Point2f pFilm;  // 胶片/像素上的2D点，用于抗锯齿
    Point2f pLens;  // 镜头上的2D点，用于景深
    float time;     // 时间点，用于运动模糊
};
```

这个 `CameraSample` 会被传递给 `GenerateRay`，作为生成光线的全部随机性来源。

## 3. 核心函数GenerateRay` (以透视为例)

`PerspectiveCamera::GenerateRay` 的执行流程是面试中考察的重点，它清晰地展示了空间变换：

1. **(输入)** CameraSample，其中`sample.pFilm`: 一个2D像素坐标，例如 `(300.7, 401.2)` (来自光栅空间)。
2. **`Point3f pCamera = RasterToCamera(pFilm);`**:
   - **(光栅空间 -> 相机空间)**。
   - 使用预先计算好的 `RasterToCamera` 矩阵，将2D像素点转换为相机坐标系中的一个3D点（该点位于近裁剪平面或 `z=1` 平面上）。
3. **`Ray ray = Ray(Point3f(0, 0, 0), Normalize(pCamera));`**:
   - **(相机空间)**。
   - 创建一条“针孔相机”光线。在相机空间中，光线起点 (Origin) 永远是 `(0,0,0)`，方向 (Direction) 就是上一步算出的 `pCamera` 向量。
4. **`if (lensRadius > 0)` (景深计算)**:
   - **`pLens = lensRadius \* ConcentricSampleDisk(sample.pLens);`**: 在相机空间的 `z=0` 平面上，根据 `lensRadius` 在镜头圆盘上采样一个随机点 `pLens`。
   - **`pFocus = ray(focalDistance / ray->d.z);`**: 计算“针孔光线”与焦平面 (`z = focalDistance`) 的交点 `pFocus`。
   - **`ray->o = Point3f(pLens.x, pLens.y, 0);`**: 将光线**起点**从 `(0,0,0)` 移动到镜头上的 `pLens` 点。
   - **`ray->d = Normalize(pFocus - ray->o);`**: 将光线**方向**修正为从 `pLens` 指向 `pFocus`。
5. **`ray = CameraToWorld(ray);`**:
   - **(相机空间 -> 世界空间)**。
   - 使用基类中的 `CameraToWorld` 变换，将这条在相机局部空间中定义好的光线，变换到世界空间中。
6. **(输出)** `ray`: `Integrator` 拿到这条世界空间光线，开始调用 `Scene::Intersect` 进行追踪。

## 4. 一些细节

#### `screenWindow` vs `RasterSpace`

- **RasterSpace (光栅空间)**: 图像的像素坐标，是整数，例如 `[0, 800] x [0, 600]`。
- **ScreenSpace (屏幕空间)**: 相机“胶片”的物理尺寸，是浮点数，在相机空间中定义，例如 `[-1.33, 1.33] x [-1, 1]`。
- `CreatePerspectiveCamera` 中的逻辑是：根据 `RasterSpace` 的宽高比 `frame` 来设置 `ScreenSpace` 的大小，确保**较短的那条边**的尺寸被固定为 `2.0` (从 `-1.0` 到 `1.0`)。
- `ProjectiveCamera` 构造函数中的 `ScreenToRaster` 矩阵，就负责建立这两个空间之间的映射关系（包括Y轴翻转）。

#### `Perspective` vs `Orthographic` 投影

- **`Perspective(fov, near, far)`**: 创建一个透视投影矩阵。它会将相机空间中的 `z` 值映射到 `[near, far]` 范围，并且在变换过程中会除以 `z`（存储在 `m[3][2]=1, m[3][3]=0`），实现“近大远小”。
- **`Orthographic(near, far)`**: 创建一个正交投影矩阵。它**不会**改变 `x` 和 `y` 坐标，只会将 `z` 轴从 `[near, far]` 线性映射到 `[0, 1]`。它没有“近大远小”的效果。
- 这两个函数创建的矩阵都会被存储为 `CameraToScreen`，并用于计算 `RasterToCamera`。

## 5. 光线微分 (Ray Differentials) 技术深度解析

为了解决纹理采样中的走样问题（Anti-Aliasing），渲染器必须知道当前着色点在屏幕上所覆盖的像素区域大小（Footprint）。我们不能仅追踪一条无限细的光线，而是需要追踪一个“光束”。在工程实现上，我们通过追踪主光线以及两条稍有偏移的“辅助光线”来近似模拟这个光束。

### 5.1. 核心数据结构：`RayDifferential`

在 `Core/Geometry.h` 中，原始的 `Ray` 类被扩展为 `RayDifferential`。它实质上携带了**三条光线**的信息，共同定义了当前的光束：
1.  **主光线**: (o, d)，对应屏幕像素中心坐标 (x, y)
2.  **X 辅助光线**: (rxOrigin, rxDirection)：近似对应屏幕相邻像素坐标 (x+1, y)。
3.  **Y 辅助光线**: (ryOrigin, ryDirection)：近似对应屏幕相邻像素坐标 (x, y+1)。

### 5.2. 流程阶段一：微分的生成 (Camera)

光线微分的生成在 `Camera::GenerateRayDifferential` 基类中通过通用的**数值微分（有限差分）**方式实现，这确保了它能适用于所有类型的子类相机。

1. **主光线生成**: 首先调用 `GenerateRay(sample, rd)` 生成通过当前像素样本 $(x, y)$ 的主光线 $(o, d)$。
2. **尝试偏移采样**: 为了确定光线随像素坐标的变化率，算法尝试在胶片平面上将采样点向 X 方向偏移一个微小量 $\epsilon$（例如 0.05 或 -0.05）。
3. **计算偏导数并外推**:
   - 调用 `GenerateRay` 得到偏移后的临时光线 (o', d')。
   - 利用差商近似偏导数。
   - 利用一阶泰勒展开，外推得到水平偏移一个完整像素 (x+1, y) 处的辅助光线：
   - 即代码中的：`rd->rxOrigin = rd->o + (rx.o - rd->o) / eps;`
4. **Y 方向重复**: 对 Y 方向进行相同的操作，计算出 $(x, y+1)$ 处的辅助光线 `ry`。
5. **标记完成**: 设置 `rd->hasDifferentials = true`，此时 `rd` 结构体完整携带了一个近似的“光束”。

### 5.3. 流程阶段二：微分的传播与求交 (Intersection)

当光线与几何体相交时，我们需要计算出屏幕像素坐标的变化 (x, y) 如何引起纹理坐标 (u, v) 的变化。这部分逻辑在 `SurfaceInteraction::ComputeDifferentials` 中实现，它采用了一阶近似的方法来避免昂贵的额外求交计算。

1.  **切平面近似**:
    * 算法并不真正将两条辅助光线与复杂的场景几何体进行求交。
    * 相反，它假设在交点 p 附近的微小区域内，表面是平坦的。它利用交点 p 和几何法线 n 定义了一个**切平面**。
    * 计算辅助光线 rx 和 ry 与该切平面的理论交点 px 和 py。
2.  **计算位置偏导数 (dpdx, dpdy)**:
    * 利用切平面上的辅助交点，直接计算出世界空间位置相对于屏幕坐标的变化向量：dpdx = px - p，以及 dpdy = py - p。
3.  **利用链式法则建立方程组**:
    * 我们已知位置 p 随纹理 u,v 的变化率（即几何体提供的切线向量 dpdu 和 dpdv）。
    * 根据多元微积分链式法则，3D位置对于屏幕 X 的变化率 (dpdx) 等于 “3D位置对纹理 U 的变化率乘以 U 对屏幕 X 的变化率” 加上 “3D位置对纹理 V 的变化率乘以 V 对屏幕 X 的变化率”。
    * 这构成了一个关于未知数 dudx 和 dvdx 的线性方程组。
4.  **求解线性系统**:
    * 上述方程是一个 3D 向量方程，为了数值稳定性，代码会将其投影到受法线影响最小的两个坐标轴平面上，简化为一个 2x2 的线性方程组。
    * 调用 `SolveLinearSystem2x2` 解出最终所需的 dudx, dvdx（以及 Y 方向的 dudy, dvdy）。这些值随后被用于 `MIPMap` 的层级计算。

### 5.4. 流程阶段三：反射/折射的传播

如果光线发生镜面反射或折射（如在 `SamplerIntegrator` 的 `SpecularReflect` 和 `SpecularTransmit` 中），这个“光束”必须继续传播，否则镜面中的倒影将无法进行正确的纹理抗锯齿。

这一步的计算相当复杂，本质是对反射和折射方程进行微分：

1.  **更新辅助光线起点**:
    * 这是最简单的一步。利用阶段二计算出的位置偏导数，直接得到新的辅助光线起点：rxOrigin = p + dpdx，ryOrigin = p + dpdy。
2.  **计算法线的变化率 (dndx, dndy)**:
    * 表面的弯曲程度会影响反射光束的发散。
    * 代码利用几何体固有的法线变化率 (dndu, dndv) 结合阶段二求得的纹理坐标变化率 (dudx, dvdx 等)，利用链式法则计算出法线相对于屏幕像素的变化率：dndx = dndu * dudx + dndv * dvdx。
3.  **计算入射方向的变化率 (dwodx, dwody)**:
    * 确定主光线与辅助光线在入射方向上的差异。
4.  **更新辅助光线方向**:
    * 将上述所有变化率代入到**微分后的反射/折射方程**中。
    * 例如，反射方向的微分不仅取决于入射方向怎么变 (dwodx)，还强烈取决于表面法线怎么变 (dndx)。
    * 最终计算出新的 rxDirection 和 ryDirection，生成新的 `RayDifferential` 继续递归追踪。

### 5.5. 最终目的：高质量纹理采样 (MIPMap)

在材质着色阶段，`ImageTexture` 利用这些信息进行抗锯齿采样，具体可见 texture 中的 readme：

1.  **传递偏导数**: `ImageTexture::Evaluate` 从 `SurfaceInteraction` 中读取四个关键偏导数 (dudx, dvdx, dudy, dvdy) 并传递给 `MIPMap::Lookup`。
2.  **计算 Footprint**: 在 `MIPMap` 中，估算当前像素在纹理空间中覆盖的区域“宽度”。为了保守地避免走样，它分别计算像素在 X 方向和 Y 方向上引起的纹理坐标变化量（向量模长），并取两者中的**最大值**作为最终过滤宽度。
3.  **MIP 层级选择与三线性插值**: `MIPMap` 根据这个宽度的对数值确定需要采样的 MIP 层级（浮点数）。宽度越大，层级越高（纹理越模糊）。最后，通过在相邻的两个离散 MIP 层级间进行**三线性插值** (Trilinear Filtering)，得到平滑、无噪点的最终纹理颜色。





