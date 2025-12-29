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

## 4. `PathIntegrator` (路径追踪)

实现了现代物理渲染的核心算法——**蒙特卡洛路径追踪**。它通过迭代追踪光线在场景中的多次弹射，无偏地求解渲染方程，从而实现全局光照 (Global Illumination)。

### 4.1. 核心成员变量
- **`maxDepth`**: 光线最大弹射次数，防止无限循环。
- **`rrThreshold`**: 俄罗斯轮盘赌 (Russian Roulette) 的阈值。当路径能量权重 (`beta`) 低于此值时，可能提前终止路径以节省计算资源。
- **`lightSampleStrategy`**: 光源采样策略（如 "spatial" 或 "uniform"）。
- **`lightDistribution`**: 在 `Preprocess` 阶段根据策略创建的加权分布，用于在着色点选择最合适的光源进行采样。

### 4.2. 核心工作流程 (`Li` 函数)
`Li` 函数使用**迭代循环**来追踪光路，维护一个路径吞吐量 `beta` (Throughput)。

1.  **初始化**: 设置累计辐射度 `L = 0`，路径吞吐量 `beta = 1`，镜面反射标记 `specularBounce = false`。
2.  **路径迭代 (bounces = 0 to maxDepth)**:
    * **场景求交**: 调用 `scene.Intersect(ray, &isect)`。
    * **累加自发光 (Le)**:
        * 如果光线击中光源（或逃逸出场景查询环境光），需要计算其自发光贡献。
        * 为了避免与直接光照计算重复计数，只有当光线是**从相机直接射出** (`bounces == 0`) 或**刚经过了镜面反射** (`specularBounce == true`) 时，才完整累加自发光。否则，需要按 **MIS 权重**进行衰减。
    * **准备着色**: 调用 `isect.ComputeScatteringFunctions`。
    * **直接光照 (Next Event Estimation, NEE)**:
        * 对于非镜面材质，显式地对场景光源进行采样（`UniformSampleOneLight`），这极大提高了找到光源的效率。
        * 此处集成了 **多重重要性采样 (MIS)**：它不仅计算“主动采样光源”的贡献，还通过**幂启发式 (Power Heuristic)** 权衡了“BSDF 偶然击中该光源”的概率。这种机制确保了无论是大面积光源（BSDF采样更有优势）还是小面积光源（光源采样更有优势），都能获得低噪点的结果。
    * **间接光照 (BSDF 采样)**:
        * 调用 `isect.bsdf->Sample_f`，根据材质的物理属性随机采样一个新的出射方向 `wi`。
        * 更新路径吞吐量：`beta *= f * AbsDot(wi, ns) / pdf`。
        * 如果采样的材质包含 `BSDF_SPECULAR`（如完美镜面），则标记 `specularBounce = true`，以便在下一次循环正确处理自发光。
    * **俄罗斯轮盘赌**:
        * 当路径弹射超过一定次数（如 3 次）且 `beta` 变暗时，以概率 $q$ 随机终止路径。存活的路径权重会除以 $(1-q)$ 以保持能量守恒。

### 4.3. 关键技术：多重重要性采样 (MIS)深度解析

在计算直接光照时（`EstimateDirect` 函数），单一的采样策略往往无法应对所有场景。例如，对光源采样适合小光源，但在高光表面上效率极低；对 BSDF 采样适合光滑表面，但很难击中小光源。

本项目使用了 **多重重要性采样 (MIS)**，通过 **幂启发式 (Power Heuristic)** 将两种策略的优势结合起来。

#### 策略一：对光源采样 (Light Sampling)
* **操作**: 主动在光源上选择一个点，并连接着色点与光源点。
* **计算**:
    * 调用 `light.Sample_Li` 得到入射方向 `wi` 和光源的概率密度 `lightPdf`。
    * **关键步骤**: 询问 BSDF，“如果我用 BSDF 采样策略，有多大可能会刚好选中这个方向？”。调用 `isect.bsdf->Pdf(wo, wi)` 得到 `scatteringPdf`。
    * 计算 MIS 权重：`weight = PowerHeuristic(1, lightPdf, 1, scatteringPdf)`。
    * 最终贡献：`f * Li * weight / lightPdf`。

#### 策略二：对 BSDF 采样 (BSDF Sampling)
* **操作**: 顺着材质的反射/折射方向发射光线，看是否偶然击中光源。
* **计算**:
    * 调用 `isect.bsdf->Sample_f` 得到出射方向 `wi` 和 BSDF 的概率密度 `scatteringPdf`。
    * 发射光线进行求交测试。如果击中光源，调用 `light.Pdf_Li` 询问：“如果我用光源采样策略，有多大可能刚好选中这个点？”，得到 `lightPdf`。
    * 计算 MIS 权重：`weight = PowerHeuristic(1, scatteringPdf, 1, lightPdf)`。
    * 最终贡献：`f * Li * weight / scatteringPdf`。

通过这种方式，MIS 能够自动“抑制”那些高方差（即 PDF 很小但贡献值可能很大）的样本，确保在任何场景下都能获得稳定、低噪点的直接光照结果。

## 5. `VolPathIntegrator` (体积路径追踪)

这是 `PathIntegrator` 的增强版，它不仅求解表面反射方程，还求解完整的**辐射传输方程 (RTE)**。它能够渲染参与介质（Participating Media），如雾、烟、云或次表面散射材质。

### 5.1. 核相较于路径追踪的扩展
与标准路径追踪相比，`VolPathIntegrator` 引入了光线与空间中微粒的交互：
* **吸收 (Absorption)**: 光能在介质中传播时被转换为热能，导致变暗。
* **发光 (Emission)**: 介质本身可能发光（如火焰）。
* **外散射 (Out-scattering)**: 光线碰撞到粒子后偏离原方向。
* **内散射 (In-scattering)**: 来自其他方向的光线碰撞粒子后汇聚到当前视线方向。

### 5.2. 核心工作流程 (`Li` 函数扩展)
`Li` 函数的主循环结构与 `PathIntegrator` 类似，但在每次追踪光线时增加了对介质的采样步骤。

1.  **初始化**: 同 `PathIntegrator`。
2.  **路径迭代 (bounces = 0 to maxDepth)**:
    * **介质采样**:
        * 在进行场景求交之前，首先发射射线。如果射线处于介质中 (`ray.medium != nullptr`)，则调用 `ray.medium->Sample` 尝试在射线上采样一个介质交互点。
        * 这可能会返回一个 `MediumInteraction`（击中介质粒子）或者仅返回光线在穿过介质时的透过率（未击中粒子）。
    * **分支一：发生介质交互 (Medium Hit)**:
        * 如果 `ray.medium->Sample` 采样到了一个交互点，说明光线在到达物体表面之前先撞到了介质粒子。
        * **更新吞吐量**: `beta` 乘以介质的透过率和散射反照率。
        * **累加自发光**: 如果介质本身发光，累加其贡献 (`beta * Le`)。
        * **介质直接光照 (NEE)**:
            * 在介质交互点调用 `UniformSampleOneLight`。
            * 光源采样时，`VisibilityTester` 会计算从光源到介质点的透过率 `Tr`，产生体积阴影效果。
        * **相位函数采样 (Phase Sampling)**:
            * 类似于表面的 BSDF，介质使用**相位函数 (Phase Function)** 来决定散射方向。
            * 调用 `mi.phase->Sample_p` 采样新的入射方向 `wi`，并更新 `beta`。
        * **生成新光线**: 从介质交互点发射新光线，继续下一轮循环（跳过后续的表面交互逻辑）。
    * **分支二：发生表面交互 (Surface Hit)**:
        * 如果光线穿过介质没有发生散射，最终击中了物体表面（或逃逸）。
        * **更新吞吐量**: `beta` 乘以光线从起点到表面这一段路径的介质透过率。
        * 接下来的流程（自发光、直接光照、BSDF 采样、俄罗斯轮盘赌）与标准 `PathIntegrator` 完全一致。唯一的区别是，在计算直接光照的可见性时，`VisibilityTester::Tr` 会考虑光线穿过介质的衰减。