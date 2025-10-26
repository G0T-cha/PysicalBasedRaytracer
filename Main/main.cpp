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

#include "Material\Material.h"
#include "Material\MatteMaterial.h"
#include "Material\Mirror.h"

#include "Texture\Texture.h"
#include "Texture\ConstantTexture.h"

#include "Sampler\TimeClockRandom.h"

#include "Light\Light.h"
#include "Light/PointLight.h"
#include "Light/DiffuseLight.h"
#include "Light/SkyBoxLight.h"

// 定义 stb_image_write 的实现
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
using namespace PBR;


int main(int argc, char* argv[]) {

    const int WIDTH = 1000;
    const int HEIGHT = 1000; // 使用正方形分辨率
    const int samples_per_pixel = 1000;

    // 2. 初始化帧缓冲区
    FrameBuffer* framebuffer = new FrameBuffer();
    framebuffer->InitBuffer(WIDTH, HEIGHT, 4); // 使用4通道 (RGBA)

    std::cout << "Framebuffer initialized." << std::endl;

    // 相机参数初始化
    std::shared_ptr<Camera> cam;
    Point3f eye(-3.0f, 1.5f, -3.0f), look(0.0, 0.0, 0.0f);
    Vector3f up(0.0f, 1.0f, 0.0f);
    Transform lookat = LookAt(eye, look, up);
    Transform Camera2World = Inverse(lookat);
    cam = std::shared_ptr<Camera>(CreatePerspectiveCamera(WIDTH, HEIGHT, Camera2World));

    PBR::Spectrum floorColor; floorColor[0] = 0.2; floorColor[1] = 0.3; floorColor[2] = 0.9;
    PBR::Spectrum dragonColor; dragonColor[0] = 1.0; dragonColor[1] = 1.0; dragonColor[2] = 0.0;
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdDragon = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(dragonColor);
    std::shared_ptr<PBR::Texture<PBR::Spectrum>> KdFloor = std::make_shared<PBR::ConstantTexture<PBR::Spectrum>>(floorColor);
    std::shared_ptr<PBR::Texture<float>> sigma = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    std::shared_ptr<PBR::Texture<float>> bumpMap = std::make_shared<PBR::ConstantTexture<float>>(0.0f);
    //材质
    std::shared_ptr<PBR::Material> dragonMaterial = std::make_shared<PBR::MatteMaterial>(KdDragon, sigma, bumpMap);
    std::shared_ptr<PBR::Material> floorMaterial = std::make_shared<PBR::MatteMaterial>(KdFloor, sigma, bumpMap);
    std::shared_ptr<PBR::Material> whiteLightMaterial = std::make_shared<PBR::MatteMaterial>(KdFloor, sigma, bumpMap);
    Spectrum mirrorColor(0.8f);
    std::shared_ptr<Texture<Spectrum>> KrMirror = std::make_shared<ConstantTexture<Spectrum>>(mirrorColor);
    std::shared_ptr<Material> mirrorMaterial = std::make_shared<MirrorMaterial>(KrMirror, bumpMap);
    std::cout << "Material initialized." << std::endl;

    int nTrianglesFloor = 2;
    int vertexIndicesFloor[6] = { 0,1,2,3,4,5 };
    int nVerticesFloor = 6;
    const float yPos_Floor = -2.0;
    Point3f P_Floor[6] = {
        PBR::Point3f(-6.0,yPos_Floor,6.0),PBR::Point3f(6.0,yPos_Floor,6.0),PBR::Point3f(-6.0,yPos_Floor,-6.0),
        PBR::Point3f(6.0,yPos_Floor,6.0),PBR::Point3f(6.0,yPos_Floor,-6.0),PBR::Point3f(-6.0,yPos_Floor,-6.0)
    };
    PBR::Transform floor_Object2World;
    PBR::Transform floor_World2Object = Inverse(floor_Object2World);
    std::shared_ptr<PBR::TriangleMesh> meshFloor = std::make_shared<PBR::TriangleMesh>
        (floor_Object2World, nTrianglesFloor, vertexIndicesFloor, nVerticesFloor, P_Floor, nullptr, nullptr, nullptr, nullptr);
    std::vector<std::shared_ptr<PBR::Shape>> trisFloor;
    for (int i = 0; i < nTrianglesFloor; ++i)
        trisFloor.push_back(std::make_shared<PBR::Triangle>(&floor_Object2World, &floor_World2Object, false, meshFloor, i));

    // 生成Mesh加速结构
    std::shared_ptr<PBR::TriangleMesh> mesh;
    std::vector<std::shared_ptr<PBR::Shape>> tris;
    std::vector<std::shared_ptr<PBR::Primitive>> prims;
    PBR::plyInfo* plyi;
    std::shared_ptr<Aggregate> agg;
    PBR::Transform tri_Object2World, tri_World2Object;

    tri_Object2World = Translate(Vector3f(0.0, -2.5, 0.0)) * tri_Object2World;
    tri_World2Object = Inverse(tri_Object2World);

    plyi = new PBR::plyInfo("C:/Users/99531/Desktop/book/0 - OriginProject/Resources/dragon.3d");

    mesh = std::make_shared<PBR::TriangleMesh>(tri_Object2World, plyi->nTriangles, plyi->vertexIndices, plyi->nVertices, plyi->vertexArray, nullptr, nullptr, nullptr, nullptr);
    tris.reserve(plyi->nTriangles);
    for (int i = 0; i < plyi->nTriangles; ++i)
        tris.push_back(std::make_shared<PBR::Triangle>(&tri_Object2World, &tri_World2Object, false, mesh, i));
    for (int i = 0; i < plyi->nTriangles; ++i)
        prims.push_back(std::make_shared<PBR::GeometricPrimitive>(tris[i], dragonMaterial,nullptr));
    for (int i = 0; i < nTrianglesFloor; ++i)
        prims.push_back(std::make_shared<GeometricPrimitive>(trisFloor[i], mirrorMaterial, nullptr));
    
    agg = std::make_shared<BVHAccel>(prims, 1, BVHAccel::SplitMethod::SAH);

    std::vector<std::shared_ptr<Light>> lights;

    Transform SkyBoxToWorld;
    Point3f SkyBoxCenter(0.f, 0.f, 0.f);
    float SkyBoxRadius = 10.0f;
    std::shared_ptr<SkyBoxLight> skyBoxLight =
        std::make_shared<SkyBoxLight>(SkyBoxToWorld, SkyBoxCenter,
            SkyBoxRadius, "C:/Users/99531/Desktop/book/0 - OriginProject/Resources/puresky_1k.hdr", 1);
    lights.push_back(skyBoxLight);

    /*int nTrianglesAreaLight = 2; //面光源数（三角形数）
    int vertexIndicesAreaLight[6] = { 0,1,2,3,4,5 }; //面光源顶点索引
    int nVerticesAreaLight = 6; //面光源顶点数
    const float yPos_AreaLight = 0.0;
    Point3f P_AreaLight[6] = { Point3f(-1.4,0.0,1.4), Point3f(-1.4,0.0,-1.4), Point3f(1.4,0.0,1.4),
        Point3f(1.4,0.0,1.4), Point3f(-1.4,0.0,-1.4), Point3f(1.4,0.0,-1.4) };
    //面光源的变换矩阵
    Transform tri_Object2World_AreaLight = Translate(Vector3f(0.7f, 5.0f, -2.0f));
    Transform tri_World2Object_AreaLight = Inverse(tri_Object2World_AreaLight);

    std::shared_ptr<TriangleMesh> meshAreaLight = std::make_shared<TriangleMesh>(
        tri_Object2World_AreaLight,
        nTrianglesAreaLight,
        vertexIndicesAreaLight,
        nVerticesAreaLight,
        P_AreaLight,
        nullptr, nullptr, nullptr, nullptr
        );

    std::vector<std::shared_ptr<Shape>> trisAreaLight;
    trisAreaLight.reserve(nTrianglesAreaLight);
    for (int i = 0; i < nTrianglesAreaLight; ++i) {
        trisAreaLight.push_back(std::make_shared<Triangle>(
            &tri_Object2World_AreaLight,
            &tri_World2Object_AreaLight,
            false,
            meshAreaLight,
            i
            ));
    }

    std::shared_ptr<AreaLight> areaLightInstance;

    for (int i = 0; i < nTrianglesAreaLight; ++i) {
        areaLightInstance = std::make_shared<DiffuseAreaLight>(
            tri_Object2World_AreaLight,
            Spectrum(10.f),
            5,
            trisAreaLight[i],
            false
            );

        lights.push_back(areaLightInstance);

        prims.push_back(std::make_shared<GeometricPrimitive>(
            trisAreaLight[i],
            whiteLightMaterial,
            areaLightInstance
            ));
    }*/

    std::cout << "Scene created. Starting render..." << std::endl;

    Bounds2i imageBound(Point2i(0, 0), Point2i(WIDTH, HEIGHT));
    std::shared_ptr<Sampler> mainSampler = std::make_shared<PBR::HaltonSampler>(
        samples_per_pixel, // 每个像素的样本数
        imageBound
        );

    std::unique_ptr<Scene> worldScene =
        std::make_unique<Scene>(agg, lights);
    Bounds2i ScreenBound(Point2i(0, 0), Point2i(WIDTH, HEIGHT));

    std::shared_ptr<Integrator> integrator = std::make_shared<WhittedIntegrator>(
        5, cam, mainSampler, ScreenBound, framebuffer
        );

    double frameTime;
    integrator->Render(*worldScene, frameTime);

    std::cout << std::endl;
    std::cout << "Render finished." << std::endl;

    // 6. 保存结果到 PNG 文件
    const char* filename = "render_final_parallel.png";
    int channels = 4;
    int stride_in_bytes = WIDTH * channels;
    stbi_flip_vertically_on_write(true);

    if (stbi_write_png(filename, WIDTH, HEIGHT, channels, framebuffer->getUCbuffer(), stride_in_bytes)) {
        std::cout << "Successfully saved image to " << filename << std::endl;
    }
    else {
        std::cerr << "Failed to save image." << std::endl;
    }

    // 7. 清理资源
    delete framebuffer;
    delete plyi;

    return 0;
}