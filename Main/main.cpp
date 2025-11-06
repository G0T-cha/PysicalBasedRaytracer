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

inline std::shared_ptr<PBR::Material> getSmileFacePlasticMaterial() {

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


inline std::shared_ptr<PBR::Material> getGreeyMatteMaterial()
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
}

inline std::shared_ptr<PBR::Material> getYellowMetalMaterial() {
    PBR::Spectrum eta; eta[0] = 0.2f; eta[1] = 0.2f; eta[2] = 0.8f;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> etaM = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(eta);
    PBR::Spectrum k; k[0] = 0.11f; k[1] = 0.11f; k[2] = 0.11f;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> kM = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(k);
    std::shared_ptr<PBR::Texture<float>> Roughness = std::make_shared<PBR::ConstantTexture<float>>(0.15f);
    std::shared_ptr<PBR::Texture<float>> RoughnessU = std::make_shared<PBR::ConstantTexture<float>>(0.15f);
    std::shared_ptr<PBR::Texture<float>> RoughnessV = std::make_shared<PBR::ConstantTexture<float>>(0.15f);
    std::shared_ptr<PBR::Texture<float>> bumpMap = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    return 	std::make_shared<PBR::MetalMaterial>(etaM, kM, Roughness, RoughnessU, RoughnessV, bumpMap, false);
}

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
}


int main(int argc, char* argv[]) {

    const int WIDTH = 1200;
    const int HEIGHT = 1200; // 使用正方形分辨率
    const int samples_per_pixel = 1000;

    // 2. 初始化帧缓冲区
    FrameBuffer* framebuffer = new FrameBuffer();
    framebuffer->InitBuffer(WIDTH, HEIGHT, 4); // 使用4通道 (RGBA)

    std::cout << "Framebuffer initialized." << std::endl;

    //相机参数初始化
    std::shared_ptr<Camera> cam;
    Point3f eye(0.f, -100.f, 40.f), look(0.0, -102.0f, 0.0f);
    Vector3f up(0.0f, 1.0f, 0.0f);
    Transform lookat = LookAt(eye, look, up);
    Transform Camera2World = Inverse(lookat);
    cam = std::shared_ptr<Camera>(CreatePerspectiveCamera(WIDTH, HEIGHT, Camera2World, nullptr));

    //常量纹理、材质
    std::shared_ptr<Material> dragonMaterial;
    std::shared_ptr<Material> whiteWallMaterial;
    std::shared_ptr<Material> redWallMaterial;
    std::shared_ptr<Material> blueWallMaterial;
    std::shared_ptr<Material> whiteLightMaterial;
    std::shared_ptr<Material> mirrorMaterial;
    std::shared_ptr<Material> plasticMaterial;
    std::shared_ptr<PBR::Material> smileFaceMaterial = getSmileFacePlasticMaterial();
    std::shared_ptr<PBR::Material> yellowMetalMaterial = getYellowMetalMaterial();
    std::shared_ptr<PBR::Material> whiteGlassMaterial = getWhiteGlassMaterial();
    std::shared_ptr<PBR::Material> greeyDiffuseMaterial = getGreeyMatteMaterial();
    PBR::Spectrum whiteColor; whiteColor[0] = 1.0; whiteColor[1] = 1.0; whiteColor[2] = 1.0;
    PBR::Spectrum dragonColor; dragonColor[0] = 0.0; dragonColor[1] = 1.0; dragonColor[2] = 0.0;
    PBR::Spectrum redWallColor; redWallColor[0] = 0.9; redWallColor[1] = 0.1; redWallColor[2] = 0.17;
    PBR::Spectrum purpleColor; purpleColor[0] = 0.35; purpleColor[1] = 0.12; purpleColor[2] = 0.48;
    PBR::Spectrum blueWallColor; blueWallColor[0] = 0.14; blueWallColor[1] = 0.21; blueWallColor[2] = 0.87;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdDragon = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(dragonColor);
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KrDragon = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(dragonColor);
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdWhite = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(whiteColor);
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdRed = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(redWallColor);
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdBlue = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(blueWallColor);
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> plasticKd = std::make_shared<ConstantTexture<Spectrum>>(purpleColor);
    std::shared_ptr<Texture<Spectrum>> plasticKr = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(1.f) - purpleColor);
    std::shared_ptr<Texture<float>> plasticRoughness = std::make_shared<ConstantTexture<float>>(0.1f);
    std::shared_ptr<PBR::Texture<float>> sigma = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    std::shared_ptr<PBR::Texture<float>> bumpMap = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    dragonMaterial = std::make_shared<PBR::MatteMaterial>(KdDragon, sigma, bumpMap);
    whiteWallMaterial = std::make_shared<PBR::MatteMaterial>(KdWhite, sigma, bumpMap);
    redWallMaterial = std::make_shared<PBR::MatteMaterial>(KdRed, sigma, bumpMap);
    blueWallMaterial = std::make_shared<PBR::MatteMaterial>(KdBlue, sigma, bumpMap);
    whiteLightMaterial = std::make_shared<PBR::MatteMaterial>(KdWhite, sigma, bumpMap);
    mirrorMaterial = std::make_shared<PBR::MirrorMaterial>(KdWhite, bumpMap);
    plasticMaterial = std::make_shared<PBR::PlasticMaterial>(plasticKd, plasticKr, plasticRoughness, bumpMap, true);
    std::cout << "Material initialized." << std::endl;

    PBR::HomogeneousMedium homoMedium(0.5, 4.4, -0.5);
    MediumInterface homoMediumInterface(&homoMedium, nullptr);
    MediumInterface noMedium;


    std::vector<std::shared_ptr<PBR::Primitive>> prims;
    PBR::ModelLoad fbxLoader;
    PBR::Transform fbx_Object2World = PBR::Translate(PBR::Vector3f(0.f, -200.f, -120.f)); //* PBR::Scale(0.5f, 0.5f, 0.5f);
    std::string fbxPath = "C:/Users/99531/Desktop/book/PBR-v1/Resources/dark-knight/source/Knight_All.fbx";
    fbxLoader.loadModel(fbxPath, fbx_Object2World);
    //fbxLoader.buildNoTextureModel(fbx_Object2World, noMedium, prims, plasticMaterial);
    fbxLoader.buildTextureModel(fbx_Object2World, noMedium, prims);

    const int nTrianglesWall = 2;
    int vertexIndicesWall[nTrianglesWall * 3];
    for (int i = 0; i < nTrianglesWall * 3; i++)
        vertexIndicesWall[i] = i;
    const int nVerticesWall = nTrianglesWall * 3;
    const float length_Wall = 200.f;
    const float groundY = -200.f;
    PBR::Point3f P_Wall[nVerticesWall] = {
        // 底座
        PBR::Point3f(-length_Wall, groundY, length_Wall),
        PBR::Point3f(length_Wall, groundY, length_Wall),
        PBR::Point3f(-length_Wall, groundY, -length_Wall),
        PBR::Point3f(length_Wall, groundY, length_Wall),
        PBR::Point3f(length_Wall, groundY, -length_Wall),
        PBR::Point3f(-length_Wall, groundY, -length_Wall),
    };
    PBR::Transform tri_Floor2World = PBR::Transform();
    PBR::Transform tri_World2Floor = PBR::Inverse(tri_Floor2World);
    std::shared_ptr<PBR::TriangleMesh> meshConBox = std::make_shared<PBR::TriangleMesh>(tri_Floor2World, nTrianglesWall, vertexIndicesWall, nVerticesWall, P_Wall, nullptr, nullptr, nullptr, nullptr);
    std::vector<std::shared_ptr<PBR::Shape>> trisFloor;
    for (int i = 0; i < nTrianglesWall; ++i)
        trisFloor.push_back(std::make_shared<PBR::Triangle>(&tri_Floor2World, &tri_World2Floor, false, meshConBox, i));

    // 将物体填充到基元
    for (int i = 0; i < nTrianglesWall; ++i)
    {
        prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisFloor[i], mirrorMaterial, nullptr, nullptr));
    }

    //墙和地板
    /*const int nTrianglesWall = 2 * 5;
    int vertexIndicesWall[nTrianglesWall * 3];
    for (int i = 0; i < nTrianglesWall * 3; i++)
        vertexIndicesWall[i] = i;
    const int nVerticesWall = nTrianglesWall * 3;
    const float length_Wall = 5.0f;
    Point3f P_Wall[nVerticesWall] = {
        //底座
        PBR::Point3f(0.f,0.f,length_Wall),PBR::Point3f(length_Wall,0.f,length_Wall), PBR::Point3f(0.f,0.f,0.f),
        PBR::Point3f(length_Wall,0.f,length_Wall),PBR::Point3f(length_Wall,0.f,0.f),PBR::Point3f(0.f,0.f,0.f),
        //天花板
        PBR::Point3f(0.f,length_Wall,length_Wall),PBR::Point3f(0.f,length_Wall,0.f),PBR::Point3f(length_Wall,length_Wall,length_Wall),
        PBR::Point3f(length_Wall,length_Wall,length_Wall),PBR::Point3f(0.f,length_Wall,0.f),PBR::Point3f(length_Wall,length_Wall,0.f),
        //后墙
        PBR::Point3f(0.f,0.f,0.f),PBR::Point3f(length_Wall,0.f,0.f), PBR::Point3f(length_Wall,length_Wall,0.f),
        PBR::Point3f(0.f,0.f,0.f), PBR::Point3f(length_Wall,length_Wall,0.f),PBR::Point3f(0.f,length_Wall,0.f),
        //右墙
        PBR::Point3f(0.f,0.f,0.f),PBR::Point3f(0.f,length_Wall,length_Wall), PBR::Point3f(0.f,0.f,length_Wall),
        PBR::Point3f(0.f,0.f,0.f), PBR::Point3f(0.f,length_Wall,0.f),PBR::Point3f(0.f,length_Wall,length_Wall),
        //左墙
        PBR::Point3f(length_Wall,0.f,0.f),PBR::Point3f(length_Wall,length_Wall,length_Wall), PBR::Point3f(length_Wall,0.f,length_Wall),
        PBR::Point3f(length_Wall,0.f,0.f), PBR::Point3f(length_Wall,length_Wall,0.f),PBR::Point3f(length_Wall,length_Wall,length_Wall)
    };
    PBR::Transform tri_ConBox2World = PBR::Translate(PBR::Vector3f(-0.5 * length_Wall, -0.5 * length_Wall, -0.5 * length_Wall));
    PBR::Transform tri_World2ConBox = PBR::Inverse(tri_ConBox2World);
    std::shared_ptr<PBR::TriangleMesh> meshConBox = std::make_shared<PBR::TriangleMesh>
        (tri_ConBox2World, nTrianglesWall, vertexIndicesWall, nVerticesWall, P_Wall, nullptr, nullptr, nullptr, nullptr);
    std::vector<std::shared_ptr<PBR::Shape>> trisConBox;
    for (int i = 0; i < nTrianglesWall; ++i)
        trisConBox.push_back(std::make_shared<PBR::Triangle>(&tri_ConBox2World, &tri_World2ConBox, false, meshConBox, i));

    //将物体填充到基元
    for (int i = 0; i < nTrianglesWall; ++i) {
        if (i == 6 || i == 7)
            prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisConBox[i], redWallMaterial, nullptr, noMedium));
        else if (i == 8 || i == 9)
            prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisConBox[i], blueWallMaterial, nullptr, noMedium));
        else
            prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisConBox[i], whiteWallMaterial, nullptr, noMedium));
    }

    /*
    
    //PBR::Transform tri_Box2World = PBR::Translate(PBR::Vector3f(0.0f, -2.f, 0.0f));
    //getBox(tri_Box2World, 4.0, 4.0, 4.0, prims, smileFaceMaterial, nullptr);

    // 生成Mesh加速结构
    std::shared_ptr<PBR::TriangleMesh> mesh;
    std::vector<std::shared_ptr<PBR::Shape>> tris;
    PBR::plyInfo* plyi;
    std::shared_ptr<Aggregate> agg;
    PBR::Transform tri_Object2World, tri_World2Object;

    tri_Object2World = Translate(Vector3f(0.0, -2.9, 0.0));
    tri_World2Object = Inverse(tri_Object2World);

    plyi = new PBR::plyInfo("C:/Users/99531/Desktop/book/PBR-v1/Resources/dragon.3d");

    mesh = std::make_shared<PBR::TriangleMesh>(tri_Object2World, plyi->nTriangles, plyi->vertexIndices, plyi->nVertices, plyi->vertexArray, nullptr, nullptr, nullptr, nullptr);
    tris.reserve(plyi->nTriangles);
    for (int i = 0; i < plyi->nTriangles; ++i)
        tris.push_back(std::make_shared<PBR::Triangle>(&tri_Object2World, &tri_World2Object, false, mesh, i));
    for (int i = 0; i < plyi->nTriangles; ++i)
        prims.push_back(std::make_shared<PBR::GeometricPrimitive>(tris[i], whiteGlassMaterial, nullptr, homoMediumInterface));*/

    //灯光
    std::vector<std::shared_ptr<Light>> lights;

    /*int nTrianglesAreaLight = 2; //面光源数（三角形数）
    int vertexIndicesAreaLight[6] = { 0,1,2,3,4,5 }; //面光源顶点索引
    int nVerticesAreaLight = 6; //面光源顶点数
    const float yPos_AreaLight = 0.0;
    Point3f P_AreaLight[6] = { PBR::Point3f(-1.4,0.0,1.4), PBR::Point3f(-1.4,0.0,-1.4), PBR::Point3f(1.4,0.0,1.4),
        PBR::Point3f(1.4,0.0,1.4), PBR::Point3f(-1.4,0.0,-1.4), PBR::Point3f(1.4,0.0,-1.4) };
    //面光源的变换矩阵
    PBR::Transform tri_Object2World_AreaLight = PBR::Translate(PBR::Vector3f(0.0f, 2.45f, 0.0f));
    PBR::Transform tri_World2Object_AreaLight = PBR::Inverse(tri_Object2World_AreaLight);
    //构造三角面片集
    std::shared_ptr<PBR::TriangleMesh> meshAreaLight = std::make_shared<PBR::TriangleMesh>
        (tri_Object2World_AreaLight, nTrianglesAreaLight, vertexIndicesAreaLight, nVerticesAreaLight, P_AreaLight, nullptr, nullptr, nullptr, nullptr);
    std::vector<std::shared_ptr<PBR::Shape>> trisAreaLight;
    //生成三角形数组
    for (int i = 0; i < nTrianglesAreaLight; ++i)
        trisAreaLight.push_back(std::make_shared<PBR::Triangle>(&tri_Object2World_AreaLight, &tri_World2Object_AreaLight, false, meshAreaLight, i));
    //填充光源类物体到基元
    for (int i = 0; i < nTrianglesAreaLight; ++i) {
        std::shared_ptr<PBR::AreaLight> area =
            std::make_shared<PBR::DiffuseAreaLight>(tri_Object2World_AreaLight, noMedium, PBR::Spectrum(5.f), 5, trisAreaLight[i], false);
        lights.push_back(area);
        prims.push_back(std::make_shared<PBR::GeometricPrimitive>(trisAreaLight[i], whiteLightMaterial, area, noMedium));
    }*/

    Transform InfinityLightToWorld = RotateX(-90) * RotateY(-0) * RotateZ(-50);
    Spectrum power(1.f);
    std::shared_ptr<Light> skyBoxLight =
        std::make_shared<InfiniteAreaLight>(InfinityLightToWorld, power,
            10, "C:/Users/99531/Desktop/book/PBR-v1/Resources/Free8kalienatmosphereHDRI.hdr");
    lights.push_back(skyBoxLight);

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
            10, cam, mainSampler, ScreenBound, 1.f, "uniform", framebuffer);
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
    const char* filename = "PathTracing21.png";
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