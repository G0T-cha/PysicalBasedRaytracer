# 材质与散射详解 (Material)

Material 模块定义了光线与场景表面交互的方式，是决定物体外观的核心。采用分层设计，将底层的物理散射模型 (BxDF) 与高层的材质接口 (Material) 分离。Integrator 通过 Material 工厂获取配置好的 BSDF 对象，然后与 BSDF 交互来执行光线散射计算。

## 1. 微表面理论 (Microfacet Theory)

为了模拟真实世界中非理想光滑表面的反射（Glossy Reflection），引入了基于物理的微表面模型。该理论认为宏观上粗糙的表面，在微观层面上是由无数个理想光滑的微平面（Microfacets）组成的。

微表面模型的核心 (`Microfacet.h/.cpp`) 实现了 **Cook-Torrance** BRDF 模型，其依靠统计学方法来描述这些微平面的整体行为。主要包含三个核心项：

- **法线分布函数D**:
  - 描述了微平面法线与半程向量一致的比例，决定了高光的形状和范围。
  - 实现了业界主流的 **GGX (Trowbridge-Reitz)** 分布，相比传统的分布具有更长的“拖尾”，能产生更真实的高光。
- **几何遮蔽函数 G**:
  - 描述了微平面之间互相遮挡（Shadowing）和掩蔽（Masking）的现象，确保了能量守恒。
  - 使用了与 GGX 分布相匹配的 **Smith** 模型。
- **菲涅尔项 F**:
  - 描述了光线与单个微平面发生碰撞时的反射比率（见下文菲涅尔效应章节）。

## 2. 散射框架 `Reflection.h/.cpp`

`Reflection` 文件是整个材质系统的基础，首先定义了 `BxDF`和 `BSDF`（`BxDF` 容器）这两个核心类。

### 2.1. `BxDF`：单一散射行为

`BxDF` 是一个抽象基类，代表一种单一的光线散射方式（如漫反射、镜面反射）。其关键接口和默认行为（由基类在 `.cpp` 文件中提供）如下：

- **`Spectrum f(wo, wi)` (纯虚函数)**: **求值**。子类必须实现此函数，返回从 `wi` 方向入射的光散射到 `wo` 方向的比例（BRDF 值）。
- **`Spectrum Sample_f(wo, *wi, sample, *pdf)` (虚函数)**: **采样**。给定出射方向 `wo`，根据 `BxDF` 的物理特性生成一个入射方向 `wi`。
  - **默认实现 (`BxDF::Sample_f`)**: 使用**余弦加权半球采样 (`CosineSampleHemisphere`)** 生成 `wi`，这对于漫反射是理想的重要性采样。
- **`float Pdf(wo, wi)` (虚函数)**: **查询 PDF**。返回 `Sample_f` 采样到 `wi` 方向的概率密度。
  - **默认实现 (`BxDF::Pdf`)**: 返回余弦加权采样的 PDF (`cos(theta) / PI`)。
- **`Spectrum rho(...)` (虚函数, 两个版本)**: 计算**半球反射率**（总反照率）。
  - **默认实现 (`BxDF::rho`)**: 使用**蒙特卡洛积分**（调用 `Sample_f`）来估算。子类如果能提供解析解（如 `LambertianReflection`）则应重写以提高效率。
- **`BxDFType type`**: 构造时传入的**位掩码**，用于标记 `BxDF` 的属性（反射/透射，漫反射/光泽/镜面）。`MatchesFlags()` 用于过滤。

### 2.2. `BSDF`：`BxDF` 容器

`BSDF` 是一个容器，管理一个或多个 `BxDF`。它是 `Integrator` 直接交互的对象。

- **构造函数 `BSDF(si, eta)`**: 从 `SurfaceInteraction si` 中提取 `ns` (着色法线), `ng` (几何法线), `ss` (切线), `ts` (副切线)，**构建本地 TBN 着色坐标系**。`eta` 存储折射率。
- **`Add(bxdf)`**: `Material` 工厂调用此方法将 `BxDF` 添加到 `bxdfs` 数组中。
- **`WorldToLocal(v)` / `LocalToWorld(v)`**:  使用 TBN 基向量在世界空间和 `BSDF` 的本地空间（`ns` 对齐到 Z 轴）之间转换向量。**所有 `BxDF` 的计算都在本地空间进行**。
- **`Spectrum f(woW, wiW, flags)`**: **聚合求值**。
  1. 转换 `woW`, `wiW` 到本地空间 `wo`, `wi`。
  2. 使用 `ng` 判断是反射还是透射 (`reflect` 标志)。
  3. 遍历 `bxdfs` 数组，**过滤**掉不匹配 `flags` 和 `reflect` 标志的 `BxDF`。
  4. **累加**所有匹配 `BxDF` 的 `f(wo, wi)` 结果。
- **`float Pdf(woW, wiW, flags)`**: **聚合 PDF 查询** (用于 MIS)。
  1. 转换到本地空间。
  2. 遍历并**过滤** `BxDF`。
  3. **累加**所有匹配 `BxDF` 的 `Pdf(wo, wi)`。
  4. 返回**平均 PDF** (`pdf / matchingComps`)。
- **`Spectrum Sample_f(woW, \*wiW, u, \*pdf, type, \*sampledType)`**: **核心采样调度器**。
  1. **过滤**: 统计匹配 `type` 的组件数 `matchingComps`。
  2. **选择**: 使用 `u[0]` 在 `matchingComps` 个 `BxDF` 中**均匀随机选择一个** `bxdf`。
  3. **重映射**: 将 `u[0]` 重映射为 `uRemapped`，供 `bxdf` 使用。
  4. **调用**: 转换 `woW` 到 `wo`，调用 `bxdf->Sample_f(wo, &wi, uRemapped, pdf, ...)`。`wi` 和 `pdf` 是**本地空间**的结果。
  5. **转换**: 将 `wi` 转换回世界空间 `*wiW`。
  6. **计算最终 PDF (MIS)**: 如果不是镜面且 `matchingComps > 1`，则**重新计算** `*pdf` 为所有匹配 `BxDF` 的 `Pdf` 的**平均值**。
  7. **计算最终 f (MIS)**: 如果不是镜面，则**重新计算** `f` 为所有匹配且方向正确的 `BxDF` 的 `f()` 的**总和**。
  8. 返回最终的 `f` 值（镜面类型返回的是 `Sample_f` 计算的特殊值）。
- **`Spectrum rho(...)` (两个版本)**: **聚合反射率**。简单地遍历、过滤并**累加**所有匹配 `BxDF` 的 `rho()` 结果。

### 2.3. 具体 `BxDF` 实现

- **`LambertianReflection`**:
  - `f()` 实现为 `R * InvPi`。
  - **继承** `BxDF` 的默认 `Sample_f` (`CosineSampleHemisphere`) 和 `Pdf` (`cos/PI`)，这正好是其重要性采样。
  - **重写** `rho()` 直接返回 `R`（解析解）。
- **`OrenNayar`**:
  - 模拟粗糙表面的漫反射。在掠射角度或者当光源和观察者的方向比较接近时（逆反射），会显得比 Lambert 更亮。
  - 依赖粗糙度 `sigma` 计算特定的 A、B 系数来调整反射率。
- **`SpecularReflection`**:
  - `f()` 返回 0 (Delta 函数)。
  - `Pdf()` 返回 0。
  - **重写** `Sample_f`：
    - 忽略 `sample`，直接计算完美反射方向 `*wi`（本地空间）。
    - 设置 `*pdf = 1` (Delta 函数约定)。
    - 返回 `fresnel->Evaluate(...) * R / AbsCosTheta(*wi)`。除以 `cos` 是为了**抵消** `Integrator` 后续多余的 `cos` 乘法。
  - 持有 `Fresnel* fresnel` 指针来计算反射比例。
- **`SpecularTransmission`**:
  - 类似 `SpecularReflection`，但用于处理完美折射。
  - 使用斯涅尔定律计算折射方向，并处理全内反射 (TIR)。
- **`FresnelSpecular`**:
  - 统一处理完美光滑介质表面的反射和折射（如完美玻璃）。
  - 在 `Sample_f` 中根据菲涅尔项随机选择是反射还是折射。
- **`MicrofacetReflection`**:
  - 基于微表面理论的通用光泽反射模型。
  - 组合了 **D, G, F** 三项来计算 `f()`。
  - `Sample_f` 基于微表面分布 $D$ 进行可见法线的重要性采样。
- **`MicrofacetTransmission`**:
  - 基于微表面理论的光泽透射模型（用于磨砂玻璃）。
  - 考虑了光线进入不同密度介质时立体角的压缩或扩张效应 。

## 3. 菲涅尔效应 (`Fresnel.h/.cpp`)

`Fresnel` 类封装了菲涅尔方程，计算光在两种介质边界处的反射比例。

- **`Fresnel` (基类)**: 定义 `virtual Spectrum Evaluate(cosTheta) const = 0;` 接口。
- **`FresnelConductor` / `FresnelDielectric`**: 实现了不同材质（金属/绝缘体）的菲涅尔计算。
- **`FresnelNoOp`**: **总是返回 1.0**。用于 `MirrorMaterial` 来模拟完美反射。
- **`FrDielectric` / `FrConductor` (辅助函数)**: 提供了菲涅尔方程的具体数学实现。

## 4. `Material` 接口 (`Material.h/.cpp`)

`Material` 是一个抽象基类，充当“工厂”。

- **`virtual void ComputeScatteringFunctions(...) = 0;`**: **核心纯虚函数**。子类**必须**实现此方法。
  - **职责**:
    1. （可选）处理凹凸贴图或法线贴图，扰动 `si->shading.n`。
    2. `new BSDF(si, ...)` 创建 `BSDF` 容器。
    3. `new BxDF(...)` 创建一个或多个 `BxDF` 实例（可能需要调用 `texture->Evaluate(*si)` 来获取参数）。
    4. `si->bsdf->Add(bxdf)` 将 `BxDF` 添加到 `BSDF` 中。
    5. **确保**添加的 `BxDF` 组合是**能量守恒**的（例如通过加权或菲涅尔）。
  - `TransportMode mode`: 用于区分光线方向（您的实现目前可能只用到 `Radiance`）。
  - `allowMultipleLobes`: 优化提示。

## 5. 具体材质实现

### 5.1. `MatteMaterial` (`MatteMaterial.h/.cpp`)

- **构造函数**: 接收 `Kd` (颜色), `sigma` (粗糙度), `bumpMap` (凹凸贴图) 的 `Texture` 指针。
- **`ComputeScatteringFunctions` 实现**:
  1. `si->bsdf = std::make_shared<BSDF>(*si);` 创建 `BSDF`。
  2. `Spectrum r = Kd->Evaluate(*si).Clamp();` 获取颜色并**钳位 (Clamp)** 以保证能量守恒。
  3. `float sig = Clamp(sigma->Evaluate(*si), 0, 90);` 获取粗糙度。
  4. `if (!r.IsBlack())`: 如果颜色非黑。
  5. `if (sig == 0)`: 如果粗糙度为 0，`si->bsdf->Add(new LambertianReflection(r));` 添加理想漫反射。
  6. `else`: 否则（粗糙度 > 0），添加 `OrenNayar(r, sig)` BxDF 以模拟粗糙漫反射。

### 5.2. `MirrorMaterial` (`Mirror.h/.cpp`)

- **构造函数**: 接收 `Kr` (颜色/色调) 和 `bumpMap` (凹凸贴图) 的 `Texture` 指针。
- **`ComputeScatteringFunctions` 实现**:
  1. `si->bsdf = std::make_shared<BSDF>(*si);` 创建 `BSDF`。
  2. `Spectrum R = Kr->Evaluate(*si).Clamp();` 获取颜色并**钳位 (Clamp)**。
  3. `if (!R.IsBlack())`: 如果颜色非黑。
  4. `si->bsdf->Add(new SpecularReflection(R, new FresnelNoOp));` 添加镜面反射 `BxDF`，并使用 `FresnelNoOp` 来确保 100% 反射。

### 5.3. `MetalMaterial` (`MetalMaterial.h/.cpp`)

- **特性**: 纯粹的 PBR 金属工作流，没有漫反射分量。
- **构造**: 接收 eta 和 k 的光谱纹理（用于导体菲涅尔），以及粗糙度纹理。
- **实现**:
  - 使用 `FresnelConductor` 计算带有物理颜色的金属反射率。
  - 使用 `TrowbridgeReitzDistribution` (GGX) 作为微表面分布。
  - 组合为 `MicrofacetReflection` BxDF。

### 5.4. `GlassMaterial` (`GlassMaterial.h/.cpp`)

- **特性**: 能够模拟完美光滑的玻璃或粗糙的磨砂玻璃。
- **实现**:
  - 检查粗糙度，如果全为 0，则添加一个 `FresnelSpecular` BxDF，统一高效地处理完美反射和折射。
  - 如果粗糙度 > 0，则分别创建 `MicrofacetReflection` (用于反射部分) 和 `MicrofacetTransmission` (用于透射部分)，两者都使用相同的 GGX 分布来模拟磨砂效果。

### 5.5. `PlasticMaterial` (`PlasticMaterial.h/.cpp`)

- **特性**: 分层材质模型，模拟带有光泽涂层的漫反射基底（如塑料、上了清漆的木头）。
- **实现**:
  - 支持 **法线贴图 (Normal Map)**：从纹理中读取法线信息，并从切线空间转换到世界空间来扰动着色法线 `si->shading.n`。
  - 底层：使用 `LambertianReflection` 表现基底颜色 (`Kd`)。
  - 顶层：使用 `MicrofacetReflection` 表现涂层高光 (`Ks`)，并强制使用电介质菲涅尔（如 eta=1.5）来模拟涂层属性。