# 积分器 Integrator

`Integrator` 模块负责实现具体的渲染算法（如 Whitted-Style），负责：从相机生成光线，追踪光线与场景的交互，计算光照，并将最终的像素颜色写入 `FrameBuffer` 。

## 1. 抽象基类 `Integrator.h`

这是所有积分器的顶层接口，定义了最基本的功能。

- **`virtual void Render(scene, &timeConsume) = 0;`**:
  - **核心纯虚函数**，渲染过程的总入口点。子类必须实现此函数，完成对整个场景 `scene` 的渲染，并将耗时写入 `timeConsume`。
- **`IntegratorRenderTime`**: 存储渲染时间的成员变量。

## 2. `SamplerIntegrator` 

此类继承自 `Integrator` ，为**基于采样器 **的渲染算法提供了一个通用框架。大多数物理渲染算法（包括 Whitted 和 Path Tracing）都属于此类。

- **构造函数 `SamplerIntegrator(camera, sampler, pixelBounds, frameBuffer)`**:
  - 存储渲染所需的核心组件：`camera` , `sampler`，渲染区域 `pixelBounds` ，以及输出目标 `m_FrameBuffer` 。
- **`void Render(scene, &timeConsume)`**:
  - 此函数驱动整个**渲染**过程，并利用 **OpenMP** 实现并行化：
    1. **并行设置**: `omp_set_num_threads()` 设置线程数。
    2. **并行循环**: `#pragma omp parallel for` 指令将外层像素行 (`j`) 的循环分配给多个线程并行处理。
    3. **采样器克隆**: `std::unique_ptr<Sampler> pixelSampler = sampler->Clone(offset);`
       - **关键**: 为**每个像素**（或每个线程处理的像素块中的像素）调用原型采样器 `sampler` 的 `Clone()` 方法。
       - 使用基于像素坐标的**唯一 `offset` **作为种子，确保：a) 每个线程拥有独立的采样器状态，避免竞争；b) 渲染结果是**确定性**的。
    4. **像素/样本循环**:
       - `pixelSampler->StartPixel(pixel);` 初始化像素。
       - `do { ... } while (pixelSampler->StartNextSample());` 执行 **SPP (每像素采样数)** 循环。
       - `CameraSample cs = pixelSampler->GetCameraSample(pixel);` 获取相机样本。
       - `camera->GenerateRay(cs, &r);` 生成主光线。
       - **`colObj += Li(r, scene, *pixelSampler, 0);`**:调用**子类必须实现**的 `Li()` 函数来计算该样本光线的颜色贡献，并累加到 `colObj`  中。
    5. **结果平均与写入**:
       - `colObj /= spp;` 计算样本平均颜色。
       - `m_FrameBuffer->set_uc(...)` 将最终颜色（注意 Y 轴翻转和 `float` 到 `uchar` 的转换）写入 `FrameBuffer` 。
    6. **进度报告**: 由主线程输出渲染进度。
- **`virtual Spectrum Li(ray, scene, sampler, depth) const = 0;`**:
  - **核心纯虚函数**，定义了**具体渲染算法**的接口。子类（如 `WhittedIntegrator`）必须实现此函数，计算给定光线 `ray` 的入射辐射度。
- **`Spectrum SpecularReflect(ray, isect, scene, sampler, depth) const`**:
  - **镜面反射辅助函数**。提供了一个处理理想镜面反射递归的通用实现。
  - **流程**:
    1. 指定 `type = BSDF_REFLECTION | BSDF_SPECULAR` 。
    2. 调用 `isect.bsdf->Sample_f(wo, &wi, ..., pdf, type)` 。`BSDF` 内部会过滤并调用 `SpecularReflection::Sample_f` ，得到完美反射方向 `wi` 、`pdf = 1` 和 `f = fresnel * R / cos`。
    3. 检查采样有效性 (`pdf > 0`, `!f.IsBlack()`)。
    4. 创建安全偏移的反射光线 `rd = isect.SpawnRay(wi)` 。
    5. **递归调用 `Li(rd, ..., depth + 1)`** 获取反射光线的颜色 `Li_recursive` 。
    6. 返回最终贡献 `f * Li_recursive * AbsDot(wi, ns) / pdf` ，其中 `AbsDot`  与 `f` 中的 `/ cos` 抵消，得到 `fresnel * R * Li_recursive` 。

## 3. `WhittedIntegrator` 

此类继承自 `SamplerIntegrator`，实现了经典的 **Whitted-Style 光线追踪**算法。

- **构造函数 `WhittedIntegrator`**:
  - 调用基类 `SamplerIntegrator` 构造函数传递核心组件。
  - 存储**最大递归深度 `maxDepth` **。
- **`Spectrum Li(ray, scene, sampler, depth) const`**:
  - **核心渲染算法实现**。
  - **流程**:
    1. **求交**: `scene.Intersect(ray, &isect)`查找最近交点。
    2. **未击中**: 若未击中，遍历光源调用 `light->Le(ray)` 获取背景/环境光并返回。
    3. **击中**:
       - **计算散射函数**: `isect.ComputeScatteringFunctions(ray)` ，初始化 `isect.bsdf` 。
       - **处理无 BSDF**: 若 `isect.bsdf == nullptr` ，递归调用 `Li`  沿原方向继续追踪。
       - **累加自发光**: `L += isect.Le(wo)` 。
       - **计算直接光照**:
         - **遍历所有光源 `light` **。
         - **光源采样**: `Spectrum Li_light = light->Sample_Li(...)` 获取光源贡献 `Li_light` 、方向 `wi`、PDF  `pdf` 和 `VisibilityTester vis` 。
         - **检查有效性**: 若 `Li_light` 黑或 `pdf == 0` ，跳过。
         - **求值 BSDF**: `Spectrum f = isect.bsdf->f(wo, wi)` 。
         - **阴影测试**: `if (!f.IsBlack() && visibility.Unoccluded(scene))` 。
         - **累加贡献**: `L += f * Li_light * AbsDot(wi, n) / pdf;`（**直接光照的单样本蒙特卡洛估算**）。
       - **计算间接光照 (递归)**:
         - **检查深度**: `if (depth + 1 < maxDepth)` 。
         - **调用镜面反射**: `L += SpecularReflect(ray, isect, scene, sampler, depth);` 。**此处无需 `if` 判断材质类型**，因为 `SpecularReflect` 函数内部会通过 `BSDF::Sample_f`  的 `type` 参数过滤，只对包含 `SPECULAR`  `BxDF`  的材质执行递归。若材质非镜面，`SpecularReflect` 会返回 0。
         - `L += SpecularTransmit(...)` 。
    4. **返回 `L` **。