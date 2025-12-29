#include <iostream>
#include <memory>
#include <vector>
#include <iomanip> // 用于格式化输出
#include <omp.h>   // 用于并行计算和计时

#include "Core/FrameBuffer.h"
#include "Core/PBR.h"
#include "Core/Transform.h"
#include "Core/Geometry.h"
#include "Shape/Shape.h"
#include "Shape/Sphere.h"
#include "Shape/Triangle.h"
#include "Shape/ModelLoad.h"
#include "Core/Interaction.h"
#include "Shape\plyRead.h"
#include "Accelerator\BVHAccel.h"
#include "Core/Primitive.h" // 确保包含了 Primitive.h
#include "Core/Spectrum.h"
#include "Core\Scene.h"

#include "Camera\Camera.h"
#include "Camera\Perspective.h"

#include "Sampler\Sampler.h"
#include "Sampler\ClockRand.h"
#include "Sampler\Halton.h"

#include "Integrator\Integrator.h"
#include "Integrator\WhittedIntegrator.h"
#include "Integrator\PathIntegrator.h"
#include "Integrator\DirectLightingIntegrator.h"
#include "Integrator\VolPathIntegrator.h"

#include "Material\Material.h"
#include "Material\MatteMaterial.h"
#include "Material\Mirror.h"
#include "Material\GlassMaterial.h"
#include "Material\MetalMaterial.h"
#include "Material\PlasticMaterial.h"

#include "Texture\Texture.h"
#include "Texture\ConstantTexture.h"
#include "Texture\ImageTexture.h"

#include "Sampler\TimeClockRandom.h"

#include "Light\Light.h"
#include "Light\PointLight.h"
#include "Light\DiffuseLight.h"
#include "Light\SkyBoxLight.h"
#include "Light\InfiniteAreaLight.h"

#include "Media\Medium.h"
#include "Media\HomogeneousMedium.h"
#include "Media\GridDensityMedium.h"

// 定义 stb_image_write 的实现
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
using namespace PBR;

/*inline std::shared_ptr<PBR::Material> getSmileFacePlasticMaterial() {

    std::unique_ptr<PBR::TextureMapping2D> map = std::make_unique<PBR::UVMapping2D>(1.f, 1.f, 0.f, 0.f);
    std::string filename = "C:/Users/99531/Desktop/book/PBR-v1/Resources/awesomeface.jpg";
    PBR::ImageWrap wrapMode = PBR::ImageWrap::Repeat;
    bool trilerp = false;
    float maxAniso = 8.f;
    float scale = 1.f;
    bool gamma = false; //如果是tga和png就是true;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> Kt =
        std::make_shared<PBR::ImageTexture<PBR::RGBSpectrum, PBR::Spectrum>>(std::move(map), filename, trilerp, maxAniso, wrapMode, scale, gamma);

    std::shared_ptr<PBR::Texture<float>> plasticRoughness = std::make_shared<PBR::ConstantTexture<float>>(0.1f);
    std::shared_ptr<PBR::Texture<float>> bumpMap = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    return std::make_shared<PBR::PlasticMaterial>(Kt, Kt, plasticRoughness, bumpMap, true);
}


/*inline void getBox(PBR::Transform& tri_Object2World, float xlength, float ylength, float zlength,
    std::vector<std::shared_ptr<PBR::Primitive>>& prims, std::shared_ptr<PBR::Material>& mat, const MediumInterface& mediumInterface) {

    //墙和地板
    const int nTrianglesBox = 2 * 6;
    int vertexIndicesWall[nTrianglesBox * 3];
    for (int i = 0; i < nTrianglesBox * 3; i++)
        vertexIndicesWall[i] = i;
    const int nVerticesBox = nTrianglesBox * 3;
    float halfX = 0.5 * xlength, halfY = 0.5 * ylength, halfZ = 0.5 * zlength;
    PBR::Point3f P_box[nVerticesBox] = {
        //底板
        PBR::Point3f(-halfX,-halfY,halfZ), PBR::Point3f(-halfX,-halfY,-halfZ), PBR::Point3f(halfX,-halfY,halfZ),
        PBR::Point3f(halfX,-halfY,halfZ), PBR::Point3f(-halfX,-halfY,-halfZ), PBR::Point3f(halfX,-halfY,-halfZ),
        //顶板
        PBR::Point3f(-halfX,halfY,halfZ), PBR::Point3f(halfX,halfY,halfZ),PBR::Point3f(-halfX,halfY,-halfZ),
        PBR::Point3f(halfX,halfY,halfZ), PBR::Point3f(halfX,halfY,-halfZ),PBR::Point3f(-halfX,halfY,-halfZ),
        //后板
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(halfX,halfY,-halfZ),PBR::Point3f(halfX,-halfY,-halfZ),
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(-halfX,halfY,-halfZ), PBR::Point3f(halfX,halfY,-halfZ),
        //前板
        PBR::Point3f(-halfX,-halfY,halfZ),PBR::Point3f(halfX,-halfY,halfZ), PBR::Point3f(halfX,halfY,halfZ),
        PBR::Point3f(-halfX,-halfY,halfZ), PBR::Point3f(halfX,halfY,halfZ),PBR::Point3f(-halfX,halfY,halfZ),
        //右板
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(-halfX,-halfY,halfZ),PBR::Point3f(-halfX,halfY,halfZ),
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(-halfX,halfY,halfZ), PBR::Point3f(-halfX,halfY,-halfZ),
        //左板
        PBR::Point3f(halfX,-halfY,-halfZ),PBR::Point3f(halfX,-halfY,halfZ),PBR::Point3f(halfX,halfY,halfZ),
        PBR::Point3f(halfX,-halfY,-halfZ),PBR::Point3f(halfX,halfY,halfZ), PBR::Point3f(halfX,halfY,-halfZ)
    };
    const float uv_l = 1.f;
    PBR::Point2f UV_box[nVerticesBox] = {
        //底板
        PBR::Point2f(0.f,uv_l),PBR::Point2f(0.f,0.f),PBR::Point2f(uv_l,uv_l),
        PBR::Point2f(uv_l,uv_l),PBR::Point2f(0.f,0.f),PBR::Point2f(uv_l,0.f),
        //顶板
        PBR::Point2f(0.f,uv_l),PBR::Point2f(uv_l,uv_l),PBR::Point2f(0.f,0.f),
        PBR::Point2f(uv_l,uv_l),PBR::Point2f(uv_l,0.f),PBR::Point2f(0.f,0.f),
        //后板
        PBR::Point2f(0.f,0.f),PBR::Point2f(uv_l,uv_l),PBR::Point2f(uv_l,0.f),
        PBR::Point2f(0.f,0.f),PBR::Point2f(0.f,uv_l),PBR::Point2f(uv_l,uv_l),
        //前板
        PBR::Point2f(0.f,0.f),PBR::Point2f(uv_l,0.f),PBR::Point2f(uv_l,uv_l),
        PBR::Point2f(0.f,0.f),PBR::Point2f(uv_l,uv_l),PBR::Point2f(0.f,uv_l),
        //右板
        PBR::Point2f(0.f,0.f),PBR::Point2f(0.f,uv_l),PBR::Point2f(uv_l,uv_l),
        PBR::Point2f(0.f,0.f),PBR::Point2f(uv_l,uv_l),PBR::Point2f(uv_l,0.f),
        //左板
        PBR::Point2f(0.f,0.f),PBR::Point2f(0.f,uv_l),PBR::Point2f(uv_l,uv_l),
        PBR::Point2f(0.f,0.f),PBR::Point2f(uv_l,uv_l),PBR::Point2f(uv_l,0.f)

    };

    std::shared_ptr<PBR::TriangleMesh> meshBox = std::make_shared<PBR::TriangleMesh>
        (tri_Object2World, nTrianglesBox, vertexIndicesWall, nVerticesBox, P_box, nullptr, nullptr, UV_box, nullptr);

    PBR::Transform tri_World2Object = Inverse(tri_Object2World);
    std::vector<std::shared_ptr<PBR::Shape>> trisBox;
    for (int i = 0; i < 12; ++i)
        trisBox.push_back(std::make_shared<PBR::Triangle>(&tri_Object2World, &tri_World2Object, false, meshBox, i));

    for (int i = 0; i < trisBox.size(); ++i)
        prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisBox[i], mat, nullptr, mediumInterface));
}*/

/*inline std::shared_ptr<PBR::Material> getGreeyMatteMaterial()
{
    PBR::Spectrum whiteColor;
    whiteColor[0] = 0.41;
    whiteColor[1] = 0.41;
    whiteColor[2] = 0.41;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdWhite = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(whiteColor);
    std::shared_ptr<PBR::Texture<float>> sigma = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    std::shared_ptr<PBR::Texture<float>> bumpMap = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    // 材质
    return std::make_shared<PBR::MatteMaterial>(KdWhite, sigma, bumpMap);
}*/

/*inline std::shared_ptr<PBR::Material> getYellowMetalMaterial()

inline std::shared_ptr<PBR::Material> getWhiteGlassMaterial() {
    PBR::Spectrum c1; c1[0] = 0.98f; c1[1] = 0.98f; c1[2] = 0.98f;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> Kr = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(c1);
    PBR::Spectrum c2; c2[0] = 0.98f; c2[1] = 0.98f; c2[2] = 0.98f;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> Kt = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(c2);
    std::shared_ptr<PBR::Texture<float>> index = std::make_shared<PBR::ConstantTexture<float>>(1.5f);
    std::shared_ptr<PBR::Texture<float>> RoughnessU = std::make_shared<PBR::ConstantTexture<float>>(0.1f);
    std::shared_ptr<PBR::Texture<float>> RoughnessV = std::make_shared<PBR::ConstantTexture<float>>(0.1f);
    std::shared_ptr<PBR::Texture<float>> bumpMap = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    return 	std::make_shared<PBR::GlassMaterial>(Kr, Kt,
        RoughnessU, RoughnessV, index, bumpMap, false);
}*/

inline void getBox(PBR::Transform& tri_Object2World, float xlength, float ylength, float zlength,
    std::vector<std::shared_ptr<PBR::Primitive>>& prims, std::shared_ptr<PBR::Material>& mat, const MediumInterface& mediumInterface) {

    // (这个函数直接取自您的 main.cpp，用于创建12个三角形组成的盒子)
    const int nTrianglesBox = 2 * 6;
    int vertexIndicesWall[nTrianglesBox * 3];
    for (int i = 0; i < nTrianglesBox * 3; i++)
        vertexIndicesWall[i] = i;
    const int nVerticesBox = nTrianglesBox * 3;
    float halfX = 0.5 * xlength, halfY = 0.5 * ylength, halfZ = 0.5 * zlength;
    PBR::Point3f P_box[nVerticesBox] = {
        //底板
        PBR::Point3f(-halfX,-halfY,halfZ), PBR::Point3f(-halfX,-halfY,-halfZ), PBR::Point3f(halfX,-halfY,halfZ),
        PBR::Point3f(halfX,-halfY,halfZ), PBR::Point3f(-halfX,-halfY,-halfZ), PBR::Point3f(halfX,-halfY,-halfZ),
        //顶板
        PBR::Point3f(-halfX,halfY,halfZ), PBR::Point3f(halfX,halfY,halfZ),PBR::Point3f(-halfX,halfY,-halfZ),
        PBR::Point3f(halfX,halfY,halfZ), PBR::Point3f(halfX,halfY,-halfZ),PBR::Point3f(-halfX,halfY,-halfZ),
        //后板
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(halfX,halfY,-halfZ),PBR::Point3f(halfX,-halfY,-halfZ),
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(-halfX,halfY,-halfZ), PBR::Point3f(halfX,halfY,-halfZ),
        //前板
        PBR::Point3f(-halfX,-halfY,halfZ),PBR::Point3f(halfX,-halfY,halfZ), PBR::Point3f(halfX,halfY,halfZ),
        PBR::Point3f(-halfX,-halfY,halfZ), PBR::Point3f(halfX,halfY,halfZ),PBR::Point3f(-halfX,halfY,halfZ),
        //右板
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(-halfX,-halfY,halfZ),PBR::Point3f(-halfX,halfY,halfZ),
        PBR::Point3f(-halfX,-halfY,-halfZ),PBR::Point3f(-halfX,halfY,halfZ), PBR::Point3f(-halfX,halfY,-halfZ),
        //左板
        PBR::Point3f(halfX,-halfY,-halfZ),PBR::Point3f(halfX,-halfY,halfZ),PBR::Point3f(halfX,halfY,halfZ),
        PBR::Point3f(halfX,-halfY,-halfZ),PBR::Point3f(halfX,halfY,halfZ), PBR::Point3f(halfX,halfY,-halfZ)
    };

    // (我们不需要UV，所以最后一个参数传 nullptr)
    std::shared_ptr<PBR::TriangleMesh> meshBox = std::make_shared<PBR::TriangleMesh>
        (tri_Object2World, nTrianglesBox, vertexIndicesWall, nVerticesBox, P_box, nullptr, nullptr, nullptr, nullptr);

    PBR::Transform tri_World2Object = Inverse(tri_Object2World);
    std::vector<std::shared_ptr<PBR::Shape>> trisBox;
    for (int i = 0; i < 12; ++i)
        trisBox.push_back(std::make_shared<PBR::Triangle>(&tri_Object2World, &tri_World2Object, false, meshBox, i));

    for (int i = 0; i < trisBox.size(); ++i)
        prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisBox[i], mat, nullptr, mediumInterface));
}

inline std::shared_ptr<PBR::Material> getDarkGrayMatteMaterial()
{
    PBR::Spectrum darkGray; darkGray[0] = 0.4f; darkGray[1] = 0.4f; darkGray[2] = 0.4f;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> Kd = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(darkGray);
    std::shared_ptr<PBR::Texture<float>> sigma = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    return std::make_shared<PBR::MatteMaterial>(Kd, sigma, nullptr);
}

// 柱子 2: 浅灰金属
inline std::shared_ptr<PBR::Material> getLightGrayMetalMaterial() {
    // (使用类似铝的物理参数)
    PBR::Spectrum eta; eta[0] = 1.657f; eta[1] = 1.621f; eta[2] = 1.564f;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> etaM = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(eta);
    PBR::Spectrum k; k[0] = 9.223f; k[1] = 9.232f; k[2] = 9.096f;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> kM = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(k);
    std::shared_ptr<PBR::Texture<float>> Roughness = std::make_shared<PBR::ConstantTexture<float>>(0.05f);
    return 	std::make_shared<PBR::MetalMaterial>(etaM, kM, Roughness, Roughness, Roughness, nullptr, false);
}

inline std::shared_ptr<PBR::Material> getBluePurpleGlassMaterial() {
    // 反射(Kr) 依然是白色，这在物理上是正确的
    PBR::Spectrum white; white[0] = 1.0f; white[1] = 1.0f; white[2] = 1.0f;

    // 定义 蓝紫色 (高 R, 低 G, 高 B)
    PBR::Spectrum bluePurple;
    bluePurple[0] = 0.6f; // Red
    bluePurple[1] = 0.2f; // Green
    bluePurple[2] = 0.9f; // Blue

    std::shared_ptr<PBR::Texture<PBR::Spectrum>> Kr = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(white);
    // 将透射(Kt)的颜色从 褐色(brown) 改为 蓝紫色(bluePurple)
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> Kt = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(bluePurple);

    std::shared_ptr<PBR::Texture<float>> index = std::make_shared<PBR::ConstantTexture<float>>(1.5f); // 玻璃折射率
    std::shared_ptr<PBR::Texture<float>> Roughness = std::make_shared<PBR::ConstantTexture<float>>(0.1f); // 保持轻微粗糙

    return  std::make_shared<PBR::GlassMaterial>(Kr, Kt, Roughness, Roughness, index, nullptr, false);
}


int main(int argc, char* argv[]) {

    const int WIDTH = 100;
    const int HEIGHT = 100; // 使用正方形分辨率
    const int samples_per_pixel = 100;

    // 2. 初始化帧缓冲区
    FrameBuffer* framebuffer = new FrameBuffer();
    framebuffer->InitBuffer(WIDTH, HEIGHT, 4); // 使用4通道 (RGBA)

    std::cout << "Framebuffer initialized." << std::endl;

    //相机参数初始化
    std::shared_ptr<Camera> cam;
    Point3f look(0.0f, 0.0f, 0.0f);
    Point3f eye(0.f, 0.f, 1.f);
    Vector3f up(0.0f, 1.0f, 0.0f);// 保持 Y 轴向
    Transform lookat = LookAt(eye, look, up);
    Transform Camera2World = Inverse(lookat);
    cam = std::shared_ptr<Camera>(CreatePerspectiveCamera(WIDTH, HEIGHT, Camera2World, nullptr));


    std::shared_ptr<Material> plasticMaterial = getDarkGrayMatteMaterial();
    std::shared_ptr<PBR::Texture<float>> sigma = std::make_shared<PBR::ConstantTexture<float>>(0.0f);

    PBR::HomogeneousMedium homoMedium(0.5, 4.4, -0.5);
    MediumInterface homoMediumInterface(&homoMedium, nullptr);
    MediumInterface noMedium;


    std::vector<std::shared_ptr<PBR::Primitive>> prims;

    PBR::ModelLoad fbxLoader;

    PBR::Transform fbx_Object2World = Translate(PBR::Vector3f(0.f, -75.f, -20.f)) * RotateX(90) * RotateZ(0) * PBR::Translate(PBR::Vector3f(0.f, -36.f, 30.f)) * Scale(2.0f, 2.0f, 2.0f);
    //PBR::Transform fbx_Object2World = Translate(PBR::Vector3f(0.f, -10.f, -20.f)) * RotateX(90) * RotateZ(0) * PBR::Translate(PBR::Vector3f(0.f, -36.f, 30.f))* Scale(2.0f, 2.0f, 2.0f);
    //PBR::Transform fbx_Object2World = Translate(PBR::Vector3f(0.f, 0.f, -50.f)) * RotateX(90) * RotateZ(0) * PBR::Translate(PBR::Vector3f(0.f, -36.f, 30.f));
    /*Vector3f currentRawCenter(1.52979f, 0.432972f, -2.86041f);
    Transform ResetToOrigin = Translate(-currentRawCenter);
    Transform StandUp = RotateX(90)* RotateY(30);
    Transform ScaleUp = Scale(2.0f, 2.0f, 2.0f);
    Transform PlaceInFront = Translate(Vector3f(0.0f, -1.0f, -8.0f));
    PBR::Transform fbx_Object2World = PlaceInFront * StandUp * ScaleUp * ResetToOrigin;;*/
    std::string fbxPath = "C:/Users/99531/Desktop/book/PBR-v1/Resources/apex/source/untitled.gltf";
    fbxLoader.loadModel(fbxPath, fbx_Object2World);
    fbxLoader.buildTextureModel(fbx_Object2World, noMedium, prims);

    /*PBR::Transform fbx_Object2World2 = Translate(PBR::Vector3f(-8.0f, -8.0f, -15.0f)) * RotateX(0) * RotateZ(0) * Scale(3, 3, 3);
    std::string fbxPath2 = "C:/Users/99531/Desktop/book/PBR-v1/Resources/apex_legends_car_smg.glb";
    fbxLoader.loadModel(fbxPath2, fbx_Object2World2);
    //fbxLoader.buildNoTextureModel(fbx_Object2World, noMedium, prims, plasticMaterial);
    fbxLoader.buildTextureModel(fbx_Object2World2, noMedium, prims);*/

    PBR::Spectrum whiteColor; whiteColor[0] = 1.0; whiteColor[1] = 1.0; whiteColor[2] = 1.0;
    std::shared_ptr<Material> mirrorMaterial;
    std::shared_ptr<PBR::Texture<float>> bumpMap = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdWhite = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(whiteColor);
    mirrorMaterial = std::make_shared<PBR::MirrorMaterial>(KdWhite, bumpMap);

    //灯光
    std::vector<std::shared_ptr<Light>> lights;

    std::shared_ptr<Material> whiteLightMaterial;
    whiteLightMaterial = std::make_shared<PBR::MatteMaterial>(KdWhite, sigma, bumpMap);
    int nTrianglesAreaLight = 2; 
    int vertexIndicesAreaLight[6] = { 0,1,2,3,4,5 }; 
    int nVerticesAreaLight = 6; 
    const float lightSize = 30.f; //
    Point3f P_AreaLight[6] = {
        PBR::Point3f(-lightSize, 0.0,  lightSize), PBR::Point3f(-lightSize, 0.0, -lightSize), PBR::Point3f(lightSize, 0.0,  lightSize),
        PBR::Point3f(lightSize, 0.0,  lightSize), PBR::Point3f(-lightSize, 0.0, -lightSize), PBR::Point3f(lightSize, 0.0, -lightSize)
    };
    PBR::Transform tri_Object2World_AreaLight =
        PBR::Translate(PBR::Vector3f(0.0f, 40.0f, 0.0f)) *
        PBR::RotateX(45.f);                               
    PBR::Transform tri_World2Object_AreaLight = PBR::Inverse(tri_Object2World_AreaLight);

    std::shared_ptr<PBR::TriangleMesh> meshAreaLight = std::make_shared<PBR::TriangleMesh>
        (tri_Object2World_AreaLight, nTrianglesAreaLight, vertexIndicesAreaLight, nVerticesAreaLight, P_AreaLight, nullptr, nullptr, nullptr, nullptr);

    std::vector<std::shared_ptr<PBR::Shape>> trisAreaLight;
    for (int i = 0; i < nTrianglesAreaLight; ++i)
        trisAreaLight.push_back(std::make_shared<PBR::Triangle>(&tri_Object2World_AreaLight, &tri_World2Object_AreaLight, false, meshAreaLight, i));

    PBR::Spectrum greyLight;
    greyLight[0] = 50.f; // R
    greyLight[1] = 50.f; // G
    greyLight[2] = 50.f; // B

    for (int i = 0; i < nTrianglesAreaLight; ++i) {
        std::shared_ptr<PBR::AreaLight> area =
            std::make_shared<PBR::DiffuseAreaLight>(tri_Object2World_AreaLight, noMedium, greyLight, 5, trisAreaLight[i], false);
        lights.push_back(area);
        prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisAreaLight[i], whiteLightMaterial, area, noMedium));
    }


    /*Transform InfinityLightToWorld = RotateX(90) * RotateY(-0) * RotateZ(-50);
    Spectrum power(1.f);
    std::shared_ptr<Light> skyBoxLight =
        std::make_shared<InfiniteAreaLight>(InfinityLightToWorld, power,
            10, "C:/Users/99531/Desktop/book/PBR-v1/Resources/sky3.png");
    lights.push_back(skyBoxLight);*/

    std::shared_ptr<Aggregate> agg;
    agg = std::make_shared<BVHAccel>(prims, 1, BVHAccel::SplitMethod::SAH);

    std::cout << "Scene created. Starting render..." << std::endl;
    //采样器
    Bounds2i imageBound(Point2i(0, 0), Point2i(WIDTH, HEIGHT));
    std::shared_ptr<Sampler> mainSampler = std::make_shared<PBR::HaltonSampler>(
        samples_per_pixel, // 每个像素的样本数
        imageBound
        );

    std::unique_ptr<Scene> worldScene =
        std::make_unique<Scene>(agg, lights);
    Bounds2i ScreenBound(Point2i(0, 0), Point2i(WIDTH, HEIGHT));
    //积分器
    std::shared_ptr<Integrator> integrator = std::make_shared<VolPathIntegrator>(
            15, cam, mainSampler, ScreenBound, 0.8f, "uniform", framebuffer);
    /*std::shared_ptr<Integrator> integrator = std::make_shared<PathIntegrator>(
            10, cam, mainSampler, ScreenBound, 0.8f, "uniform", framebuffer
            );*/
    /*std::shared_ptr<Integrator> integrator = std::make_shared<WhittedIntegrator>(
        5, cam, mainSampler, ScreenBound, framebuffer
        );*/
    /*std::shared_ptr<Integrator> integrator = std::make_shared<DirectLightingIntegrator>(
        LightStrategy::UniformSampleOne, 5, cam, mainSampler, ScreenBound, framebuffer
        );*/
     
    //开始渲染
    double frameTime;
    integrator->Render(*worldScene, frameTime);

    std::cout << std::endl;
    std::cout << "Render finished." << std::endl;

    // 保存结果到 PNG 文件
    const char* filename = "PathTracing26.png";
    int channels = 4;
    int stride_in_bytes = WIDTH * channels;
    stbi_flip_vertically_on_write(true);

    if (stbi_write_png(filename, WIDTH, HEIGHT, channels, framebuffer->getUCbuffer(), stride_in_bytes)) {
        std::cout << "Successfully saved image to " << filename << std::endl;
    }
    else {
        std::cerr << "Failed to save image." << std::endl;
    }

    // 清理资源
    delete framebuffer;
    // delete plyi;

    return 0;
}