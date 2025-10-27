# 渲染器工作流程

在这里梳理 `main.cpp` 如何驱动整个渲染管线，直至最终输出图像的数据流。

## 1. 初始化 (main.cpp)

渲染始于 `main.cpp`，主程序在此阶段实例化并配置渲染所需的核心组件。

首先需要定义画面尺寸和每帧采样次数。根据画面尺寸初始化`FrameBuffer` (画布)，作为存储最终颜色的内存缓冲区。 然后是相机初始化，需要定义其在世界空间中的位置和朝向 (通过 `Transform` 变换)，并设置成像参数 (如视场角 `fov` 和投影方式、焦距光圈等)。与此同时，`Sampler` (如 `HaltonSampler`) 被创建以生成低差异样本点。最后，`Integrator` (如 `WhittedIntegrator`) 被实例化，它封装了渲染所需的核心着色算法。

## 2. 场景构建

初始化完成后，主程序 (`main.cpp`) 开始按流程构建场景。这个过程首先是加载或创建几何体 (`Shape`)，例如从 `.ply` 文件读取模型，或在代码中直接定义 `Sphere`。接着，为这些几何体实例化 `Material` (如 `MatteMaterial`) 来定义其光学属性。几何体和材质被关联并封装为 `Primitive` (图元)对象。随后，所有图元列表被提交给 `BVHAccel`，由其构造函数递归构建完整的 BVH 树。同时，`Light` (如 `PointLight` 或 `SkyBoxLight`) 也会被实例化。最终，构建完成的 BVH (代表所有几何体) 和光源列表被一同注册到 `Scene` 对象中，等待渲染。

## 3. 渲染循环 (Integrator::Render)

渲染的主循环由 `Integrator::Render` 方法驱动。`Integrator` 会遍历 `FrameBuffer` 上的所有像素 `(x, y)`。对于每个像素，它首先创建一个 `Sampler`的克隆，并请求一组 2D 样本点（用于抗锯齿）。接着，对于该像素内的每一个样本点，`Integrator` 会调用 `Camera::GenerateRay`，将 2D 像素坐标和样本点映射为一条 3D 主光线 (`Ray`)。这条光线随后被传递给核心着色函数 `Integrator::Li`，该函数负责计算这条光线所贡献的辐射率 (即颜色 `Spectrum`)。一个像素内的所有样本点返回的 `Spectrum` 值会被取平均，作为该像素的最终颜色，并写入 `FrameBuffer` 对应的 `(x, y)` 位置。

## 4. 着色 (Integrator::Li)

`Integrator::Li` (Light transport) 是 `Whitted-Style` 算法的核心，它通过递归计算单条光线的辐射率。该函数首先调用 `Scene::Intersect(ray)` (内部委托 `BVHAccel` 执行) 来查找光线与场景的最近交点。若光线未击中任何物体 (Miss)，则查询 `SkyBoxLight` 或返回背景色，作为该光路的终点。若光线击中物体 (Hit)，函数将获取该交点的 `Interaction` 详细信息 (位置, 法线, 材质等)。着色计算在此处开始：首先，检查交点处的图元是否为光源 (如 `DiffuseLight`)，如果是，则累加其自发光 `L_e`。随后，`Integrator` 会遍历场景中的所有 `Light`，从交点向光源发射**阴影光线** (`Shadow Ray`) 来计算直接光照贡献（前提是光线未被遮挡）。最后，若材质为 `Mirror` (镜面)，`Integrator` 会计算菲涅尔系数，生成一条新的**反射光线**，并递归调用 `Li`。递归返回的结果将与材质颜色和菲涅尔系数相乘后，累加到总颜色中。该函数最终返回在交点处累加的总辐射率 (自发光 + 直接光照 + 间接反射)。

## 5. 图像输出 

当 `Integrator::Render` 完成所有像素的计算后，进行图像输出