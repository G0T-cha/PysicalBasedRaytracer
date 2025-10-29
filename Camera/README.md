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