# 采样器系统详解

本模块 (`Sampler/`) 是渲染器的“随机性”来源。在蒙特卡洛光线追踪中，所有的高级效果，如抗锯齿 (Anti-aliasing)、景深 (Depth of Field)、软阴影、间接光照等，都依赖于积分。`Sampler` 模块为这些积分提供高质量的样本（Sample），样本质量决定了渲染结果收敛（噪点消除）的速度。

## 1. Sampler 

`Sampler.h` 定义了采样器的抽象基类，所有具体的采样器（如 Halton, Sobol）都继承自它。这个基类设计了渲染器与采样器协作的核心接口：

- **`Sampler(int64_t samplesPerPixel)`**: 构造函数，指定每个像素的采样数 (SPP)。
- **`virtual void StartPixel(const Point2i &p)`**:
  - 这是一个**状态管理**函数。在 `Integrator` (积分器) 开始渲染一个新像素 `p` 之前，必须调用此函数。作用是重置采样器。这对于**确定性**的低差异序列（如 Halton）至关重要，确保了渲染结果的可复现性。
- **`virtual bool StartNextSample()`**:
  - 另一个状态管理函数。`Integrator` 在一个像素内完成一次采样（一个 `Li` 路径）后调用它，使采样器前进到下一个样本索引 (sample index)。
- **`virtual float Get1D()` / `virtual Point2f Get2D()`**:
  - **最核心的接口**。渲染管线中的其他模块（如 `Camera`, `Light`, `Integrator`）通过这两个函数来请求“下一个”1D 或 2D 样本。渲染器不关心样本是如何生成的，只管通过这个接口来获取。

- **`CameraSample GetCameraSample(const Point2i &pRaster):`**: **组装**一个 `CameraSample` 结构体，内部会调用 `Get2D()` 和 `Get1D()` 来填充 `pFilm`、`pLens` 和 `time`。
- **`virtual std::unique_ptr<Sampler> Clone(int seed) = 0;`**:
  - 另一个**纯虚函数**，用于多线程渲染。每个渲染线程都必须获取一个唯一的 `Sampler` 实例（克隆体），以避免线程间对样本序列的竞争。这里

此外，还提供了 `Sampler` 的两种实现：

- `PixelSampler` 采样器（及其子类）通常在 `StartPixel` 时**一次性生成并存储**该像素所需的所有样本。
- `GlobalSampler` 采样器将整个图像的所有像素、所有样本索引（例如 `800*600*64` 个样本）映射到一个样本序列中，下面的 `Halton` 采样器和时钟采样器都是 `GlobalSampler`。

## 2. 采样器类型：PRNG vs QMC

本模块实现了两大类采样技术，这是面试中的关键知识点：

1. **PRNG (伪随机数生成器)**:
   - 相关文件: `RNG.h`, `ClockRand.h`, `TimeClockRandom.h`
   - `RNG` (Random Number Generator) 提供了标准的线性同余生成器 (LCG)。是一种“纯粹”的随机，样本之间没有关联。
   - **优点**: 实现简单。
   - **缺点**: 收敛慢。样本分布不均匀，容易产生“聚集” (Clumping) 和“空洞”，导致噪点消除得很慢。
2. **QMC (准蒙特卡洛 / 低差异序列)**:
   - 相关文件: `LowDiscrepancy.h`, `Halton.h`, `SobolMatrices.h`
   - 这是本项目中的**高级特性**。低差异序列（如 Halton 和 Sobol）是一种“更均匀”的随机数。它们被设计用来**刻意地**填补空间中的空隙，避免样本聚集。
   - **`Halton.h`**: 实现了 Halton 序列。它使用不同质数为基底（例如 2 和 3）来生成 2D 样本，能很好地覆盖 `[0,1)x[0,1)` 空间。
   - **`SobolMatrices.h`**: 提供了 Sobol 序列所需的初始化数据。Sobol 序列通常被认为在更高维度上（例如，当一个像素需要几十个1D/2D样本时）比 Halton 具有更好的分布特性。
   - **优点**: **收敛速度快得多**。在相同的 SPP 下，使用 QMC 得到的图像噪点远少于 PRNG。

## 3. 采样辅助函数 (`Sampling.h`)

`Sampler` 提供了在 `[0,1)x[0,1)` 空间中的“原始”样本，而 `Sampling.h` 中的函数负责将这些原始样本**映射 (Mapping)** 到有物理意义的几何形状上。

包括：圆盘采样（拒绝采样 、均匀采样、同心圆盘映射），半球采样（立体角、余弦加权采样）、球体采样、圆锥采样等。

## 4. 使用

`Sampler` 模块辅助多个核心模块进行采样：

1. **`Integrator` (积分器)**: 在 `Render` 函数中，`Integrator` 遍历每个像素，第一件事就是调用 `sampler->StartPixel(p)`。
2. **`Camera` (相机)**: 在 `GenerateRay` 中，调用 `sampler->Get2D()` 来获取 `pFilm` 样本（用于抗锯齿），并再次调用 `sampler->Get2D()` 来获取 `pLens` 样本（传递给 `ConcentricSampleDisk` 用于景深）。
3. **`Light` (光源)**: 在 `WhittedIntegrator` 中，`light->Sample_Li` (光源采样) 会调用 `sampler->Get2D()` 来在光源表面（如果是面光源）或方向上（如果是天空盒）随机选择一个采样点。

如：

```
Spectrum BxDF::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample, float* pdf, BxDFType* sampledType) const
	{
		*wi = CosineSampleHemisphere(sample);
		if (wo.z < 0) wi->z *= -1;
		*pdf = Pdf(wo, *wi);
		return f(wo,*wi);
	}
```

