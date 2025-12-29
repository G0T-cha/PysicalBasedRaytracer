// 代码/Shape/ModelLoad.cpp (更新版)

#include "Shape\ModelLoad.h"
#include "Core\Geometry.h"
#include "Core\Transform.h"
#include "Core\primitive.h"
#include "Texture\ConstantTexture.h"
#include "Texture\ImageTexture.h"      // 确保 ImageTexture 被包含
#include "Material\MatteMaterial.h"
#include "Material\PlasticMaterial.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
// -----------------------
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>

/*namespace PBR {

    // (processMesh, processNode, loadModel, getDiffuseMaterial 保持不变)
    // ...
    std::shared_ptr<TriangleMesh> ModelLoad::processMesh(aiMesh* mesh, const aiScene* scene, const Transform& ObjectToWorld) {
        // (此函数保持原样)
        long nVertices = mesh->mNumVertices;
        long nTriangles = mesh->mNumFaces;
        int* vertexIndices = new int[nTriangles * 3];
        Point3f* P = new Point3f[nVertices];
        Vector3f* S = nullptr;
        Normal3f* N = new Normal3f[nVertices];
        Point2f* uv = new Point2f[nVertices];
        int* faceIndices = nullptr;
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            P[i].x = mesh->mVertices[i].x;
            P[i].y = mesh->mVertices[i].y;
            P[i].z = mesh->mVertices[i].z;
            if (mesh->HasNormals()) {
                N[i].x = mesh->mNormals[i].x;
                N[i].y = mesh->mNormals[i].y;
                N[i].z = mesh->mNormals[i].z;
            }
            if (mesh->mTextureCoords[0]) {
                uv[i].x = mesh->mTextureCoords[0][i].x;
                uv[i].y = mesh->mTextureCoords[0][i].y;
            }
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                vertexIndices[3 * i + j] = face.mIndices[j];
        }
        if (!mesh->HasNormals()) {
            delete[] N;
            N = nullptr;
        }
        if (!mesh->mTextureCoords[0]) {
            delete[] uv;
            uv = nullptr;
        }
        std::shared_ptr<TriangleMesh> trimesh =
            std::make_shared<TriangleMesh>(ObjectToWorld, nTriangles, vertexIndices, nVertices, P, S, N, uv, faceIndices);
        std::string meshName = mesh->mName.C_Str();
        meshNames.push_back(meshName);
        diffTexName.push_back("");
        specTexName.push_back("");
        delete[] vertexIndices;
        delete[] P;
        delete[] S;
        delete[] N;
        delete[] uv;
        return trimesh;
    }
    void ModelLoad::processNode(aiNode* node, const aiScene* scene, const Transform& ObjectToWorld) {
        // (此函数保持原样)
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, ObjectToWorld));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene, ObjectToWorld);
    }
    void ModelLoad::loadModel(std::string path, const Transform& ObjectToWorld) {
        // (此函数保持原样)
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate); // 已移除 FlipUVs
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "ERROR: Assimp failed to load model: " << path << std::endl;
            std::cerr << "Assimp error: " << import.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        std::cout << "INFO: Texture directory set to: " << directory << std::endl;
        processNode(scene->mRootNode, scene, ObjectToWorld);
    }
    inline std::shared_ptr<Material> getDiffuseMaterial(std::string filename) {
        // (此函数保持原样)
        if (filename.empty()) {
            return std::make_shared<MatteMaterial>(
                std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.5f)),
                std::make_shared<ConstantTexture<float>>(0.0f),
                std::make_shared<ConstantTexture<float>>(0.0f));
        }
        std::cout << "DEBUG: [getDiffuseMaterial] Attempting to load: " << filename << std::endl;
        std::unique_ptr<TextureMapping2D> map = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
        std::shared_ptr<Texture<Spectrum>> Kt = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(map), filename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        std::shared_ptr<Texture<float>> sigmaRed = std::make_shared<ConstantTexture<float>>(0.0f);
        std::shared_ptr<Texture<float>> bumpMap = std::make_shared<ConstantTexture<float>>(0.0f);
        return std::make_shared<MatteMaterial>(Kt, sigmaRed, bumpMap);
    }

    // ==========================================================
    // 1. 替换 getPlasticMaterial 为 createManualPbrMaterial
    // ==========================================================


    inline std::shared_ptr<Material> createManualPbrMaterial(
        const std::string& diffFilename,
        const std::string& metalFilename,
        const std::string& roughFilename,
        const std::string& normalFilename) // <-- 新增法线贴图
    {
        // --- 1. 加载 Diffuse (Kd) ---
        std::shared_ptr<Texture<Spectrum>> plasticKd;
        if (diffFilename.empty()) {
            plasticKd = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.5f));
        }
        else {
            std::cout << "DEBUG: [PBR] Loading Kd: " << diffFilename << std::endl;
            std::unique_ptr<TextureMapping2D> map1 = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            plasticKd = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(map1), diffFilename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        }

        // --- 2. 加载 Metalness (用作 Ks) ---
        std::shared_ptr<Texture<Spectrum>> plasticKr;
        if (metalFilename.empty()) {
            plasticKr = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.0f)); // 默认非金属
        }
        else {
            std::cout << "DEBUG: [PBR] Loading Kr (from Metal): " << metalFilename << std::endl;
            std::unique_ptr<TextureMapping2D> map2 = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            plasticKr = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(map2), metalFilename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        }

        // --- 3. 加载 Roughness (粗糙度) ---
        std::shared_ptr<Texture<float>> plasticRoughness;
        if (roughFilename.empty()) {
            std::cout << "DEBUG: [PBR] No roughness map. Using Constant 0.8." << std::endl;
            plasticRoughness = std::make_shared<ConstantTexture<float>>(0.8f);
        }
        else {
            // 【重要】: 粗糙度贴图是单通道 (float)，所以我们使用 ImageTexture<float, float>
            std::cout << "DEBUG: [PBR] Loading Roughness: " << roughFilename << std::endl;
            std::unique_ptr<TextureMapping2D> map3 = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            plasticRoughness = std::make_shared<ImageTexture<float, float>>(std::move(map3), roughFilename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        }

        // --- 4. 加载 Normal (法线) ---
        std::shared_ptr<Texture<Spectrum>> normalMap;
        if (normalFilename.empty()) {
            std::cout << "DEBUG: [PBR] No normal map. Using nullptr." << std::endl;
            normalMap = nullptr; // 传递一个空指针
        }
        else {
            // 【重要】: 法线贴图存储的是向量 (XYZ)，编码在 RGB 颜色中，所以我们使用 ImageTexture<RGBSpectrum, Spectrum>
            std::cout << "DEBUG: [PBR] Loading Normal: " << normalFilename << std::endl;
            std::unique_ptr<TextureMapping2D> map4 = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            normalMap = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(map4), normalFilename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        }

        // --- 5. 创建材质 ---
        // (注意我们将 normalMap 传递给了 bumpMap 插槽)
        return std::make_shared<PlasticMaterial>(plasticKd, plasticKr, plasticRoughness, normalMap, false);
    }


    // ... (buildNoTextureModel 保持不变) ...
    void ModelLoad::buildNoTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
        std::vector<std::shared_ptr<Primitive>>& prims, std::shared_ptr<Material> material) {
        // (此函数保持原样)
        std::vector<std::shared_ptr<Shape>> trisObj;
        Transform tri_World2Object = Inverse(tri_Object2World);
        for (int i = 0; i < meshes.size(); i++) {
            for (int j = 0; j < meshes[i]->nTriangles; ++j) {
                std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
                trisObj.push_back(std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j));
            }
        }
        for (int i = 0; i < trisObj.size(); ++i)
            prims.push_back(std::make_shared<GeometricPrimitive>(trisObj[i], material, nullptr, mediumInterface));
        meshes.clear();
        diffTexName.clear();
        specTexName.clear();
        meshNames.clear();
        directory = "";
    }

    // ==========================================================
    // 2. 更新 buildTextureModel
    // ==========================================================
    void ModelLoad::buildTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
        std::vector<std::shared_ptr<Primitive>>& prims) {

        std::cout << "--- Pre-loading materials ---" << std::endl;
        preloadedMaterials.clear();

        // 现在调用新函数，传入4个纹理
        preloadedMaterials["cloth"] = createManualPbrMaterial(
            directory + "/T_cloth_D.png",
            directory + "/T_cloth_M.png",
            directory + "/T_cloth_R.png",
            directory + "/T_cloth_N.png"
        );

        preloadedMaterials["metal"] = createManualPbrMaterial(
            directory + "/T_metal_D.png",
            directory + "/T_metal_M.png",
            directory + "/T_metal_R.png",
            directory + "/T_metal_N.png"
        );

        preloadedMaterials["sword"] = createManualPbrMaterial(
            directory + "/T_sword_D.png", // (假设 'l' 是多余的)
            directory + "/T_sword_M.png", // (假设 T_sword_M.png 存在)
            directory + "/T_sword_R.png",
            directory + "/T_sword_N.png"  // (假设 T_sword_N.png 存在)
        );

        preloadedMaterials["default"] = createManualPbrMaterial("", "", "", "");
        std::cout << "--- Material pre-loading complete ---" << std::endl;


        std::vector<std::shared_ptr<Shape>> trisObj;
        Transform tri_World2Object = Inverse(tri_Object2World);

        std::cout << "--- Assigning materials to meshes ---" << std::endl;
        for (int i = 0; i < meshes.size(); i++) {
            // (这部分的手动匹配逻辑保持原样)
            std::string meshName = meshNames[i];
            std::cout << "Processing Mesh (" << i << "/" << meshes.size() << "): " << meshName << std::endl;
            std::shared_ptr<Material> finalMaterial = preloadedMaterials["default"];
            std::string materialName = "default";
            if (meshName.rfind("Hood", 0) == 0 ||
                meshName.rfind("Padded", 0) == 0 ||
                meshName.rfind("Dress", 0) == 0)
            {
                finalMaterial = preloadedMaterials["cloth"];
                materialName = "cloth";
            }
            else if (meshName.rfind("Merged_Sword", 0) == 0)
            {
                finalMaterial = preloadedMaterials["sword"];
                materialName = "sword";
            }
            else if (meshName.rfind("Crown", 0) == 0 ||
                meshName.rfind("Head_Mask", 0) == 0 ||
                meshName.rfind("Chest", 0) == 0 ||
                meshName.rfind("Shoulder", 0) == 0 ||
                meshName.rfind("ArmStrap", 0) == 0 ||
                meshName.rfind("Bracer", 0) == 0 ||
                meshName.rfind("Glove", 0) == 0 ||
                meshName.rfind("Belt", 0) == 0 ||
                meshName.rfind("Boot", 0) == 0)
            {
                finalMaterial = preloadedMaterials["metal"];
                materialName = "metal";
            }
            std::cout << "  -> Match: '" << materialName << "'. Assigning material." << std::endl;
            if (mediumInterface.inside == nullptr && mediumInterface.outside == nullptr) {
                for (int j = 0; j < meshes[i]->nTriangles; ++j) {
                    std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
                    prims.push_back(std::make_shared<GeometricPrimitive>(
                        std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j),
                        finalMaterial,
                        nullptr, mediumInterface));
                }
            }
            else {
                for (int j = 0; j < meshes[i]->nTriangles; ++j) {
                    std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
                    prims.push_back(std::make_shared<GeometricPrimitive>(
                        std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j),
                        nullptr, nullptr, mediumInterface));
                }
            }
        }
        meshes.clear();
        diffTexName.clear();
        specTexName.clear();
        meshNames.clear();
        preloadedMaterials.clear();
        directory = "";
    }

}*/ // namespace PBR

namespace PBR {

    static std::string sanitizePath(std::string path) {
        std::replace(path.begin(), path.end(), '\\', '/');
        return path;
    }

    static std::string extractEmbeddedTexture(const aiScene* scene, const std::string& rawPath, const std::string& baseDir) {
        if (rawPath.empty() || rawPath[0] != '*') return "";
        try {
            int texIndex = std::stoi(rawPath.substr(1));
            if (texIndex < 0 || texIndex >= scene->mNumTextures) return "";
            aiTexture* tex = scene->mTextures[texIndex];
            std::string ext = (tex->achFormatHint[0]) ? std::string(tex->achFormatHint) : "png";
            std::string fileName = "embedded_tex_" + std::to_string(texIndex) + "." + ext;
            std::string fullPath = baseDir + "/" + fileName;
            std::ifstream check(fullPath);
            if (check.good()) { check.close(); return sanitizePath(fullPath); }
            check.close();
            if (tex->mHeight == 0) {
                std::cout << "  [Texture Extract] Saving embedded texture [" << texIndex << "] to: " << fileName << std::endl;
                std::ofstream file(fullPath, std::ios::binary);
                if (file.is_open()) { file.write(reinterpret_cast<char*>(tex->pcData), tex->mWidth); file.close(); return sanitizePath(fullPath); }
                else { std::cerr << "  [Texture Extract] ERROR: Could not write file: " << fullPath << std::endl; }
            }
            else { std::cerr << "  [Texture Extract] WARNING: Uncompressed texture ignored." << std::endl; }
        }
        catch (...) {}
        return "";
    }

    std::shared_ptr<TriangleMesh> ModelLoad::processMesh(aiMesh* mesh, const aiScene* scene, const Transform& ObjectToWorld) {
        long nVertices = mesh->mNumVertices;
        long nTriangles = mesh->mNumFaces;
        if (nVertices == 0 || nTriangles == 0) return nullptr;

        std::vector<int> vertexIndices(nTriangles * 3);
        std::vector<Point3f> P(nVertices);
        std::vector<Normal3f> N(nVertices);
        std::vector<Point2f> uv(nVertices);

        for (long i = 0; i < nVertices; i++) {
            P[i] = Point3f(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (mesh->HasNormals()) N[i] = Normal3f(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else N[i] = Normal3f(0, 1, 0);
            if (mesh->HasTextureCoords(0)) uv[i] = Point2f(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else uv[i] = Point2f(0.f, 0.f);
        }
        for (long i = 0; i < nTriangles; i++) {
            aiFace face = mesh->mFaces[i];
            if (face.mNumIndices != 3) continue;
            vertexIndices[3 * i + 0] = face.mIndices[0];
            vertexIndices[3 * i + 1] = face.mIndices[1];
            vertexIndices[3 * i + 2] = face.mIndices[2];
        }
        return std::make_shared<TriangleMesh>(ObjectToWorld, nTriangles, vertexIndices.data(), nVertices, P.data(), nullptr, mesh->HasNormals() ? N.data() : nullptr, mesh->HasTextureCoords(0) ? uv.data() : nullptr, nullptr);
    }

    void ModelLoad::processNode(aiNode* node, const aiScene* scene, const Transform& ObjectToWorld) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            std::shared_ptr<TriangleMesh> pbrtMesh = processMesh(mesh, scene, ObjectToWorld);
            if (pbrtMesh) { meshes.push_back(pbrtMesh); meshMaterialIndices.push_back(mesh->mMaterialIndex); }
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) processNode(node->mChildren[i], scene, ObjectToWorld);
    }

    void ModelLoad::loadModel(std::string path, const Transform& ObjectToWorld) {
        std::cout << "[ModelLoad] Starting load: " << path << std::endl;
        Assimp::Importer import;
        //const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace);
        const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) { std::cerr << "[ModelLoad] ERROR::ASSIMP::" << import.GetErrorString() << std::endl; return; }

        path = sanitizePath(path);
        directory = path.substr(0, path.find_last_of('/'));
        if (directory.empty()) directory = ".";

        std::cout << "[ModelLoad] Scene loaded. Materials: " << scene->mNumMaterials << ", Embedded Textures: " << scene->mNumTextures << std::endl;
        loadedMaterials.resize(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            auto getTexPath = [&](aiTextureType type) -> std::string {
                aiString str;
                if (mat->GetTexture(type, 0, &str) == AI_SUCCESS) {
                    std::string raw = str.C_Str();
                    if (raw.length() > 0 && raw[0] == '*') return extractEmbeddedTexture(scene, raw, directory);
                    return sanitizePath(directory + "/" + raw);
                }
                return "";
            };

            std::string diffPath = getTexPath(aiTextureType_DIFFUSE);
            // if (diffPath.empty()) diffPath = getTexPath(aiTextureType_BASE_COLOR); // Uncomment for newer Assimp if needed
            std::string metalPath = getTexPath(aiTextureType_UNKNOWN);
            if (metalPath.empty()) metalPath = getTexPath(aiTextureType_METALNESS);
            std::string normalPath = getTexPath(aiTextureType_NORMALS);

            if (i < 5) {
                std::cout << "  Mat[" << i << "] '" << mat->GetName().C_Str() << "': D='" << (diffPath.empty() ? "MISSING" : "OK") << "', N='" << (normalPath.empty() ? "MISSING" : "OK") << "'" << std::endl;
            }

            std::unique_ptr<TextureMapping2D> mapUV = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            std::shared_ptr<Texture<Spectrum>> kdTex, ksTex, normalTex;
            std::shared_ptr<Texture<float>> roughTex;

            if (!diffPath.empty()) kdTex = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(mapUV), diffPath, false, 8.f, ImageWrap::Repeat, 1.f, true);
            else { aiColor3D c(0.5f, 0.5f, 0.5f); mat->Get(AI_MATKEY_BASE_COLOR, c); kdTex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum::FromRGB(reinterpret_cast<float*>(&c))); }

            if (!metalPath.empty()) {
                mapUV = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f); ksTex = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(mapUV), metalPath, false, 8.f, ImageWrap::Repeat, 1.f, false);
                mapUV = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f); roughTex = std::make_shared<ImageTexture<float, float>>(std::move(mapUV), metalPath, false, 8.f, ImageWrap::Repeat, 1.f, false);
            }
            else { ksTex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.0f)); roughTex = std::make_shared<ConstantTexture<float>>(0.5f); }

            if (!normalPath.empty()) { mapUV = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f); normalTex = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(mapUV), normalPath, false, 8.f, ImageWrap::Repeat, 1.f, false); }

            loadedMaterials[i] = std::make_shared<PlasticMaterial>(kdTex, ksTex, roughTex, normalTex, false);
        }
        processNode(scene->mRootNode, scene, ObjectToWorld);
        std::cout << "[ModelLoad] Load Finished. Total Meshes: " << meshes.size() << std::endl;
    }

    void ModelLoad::buildNoTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
        std::vector<std::shared_ptr<Primitive>>& prims, std::shared_ptr<Material> material) {

        std::cout << "[ModelLoad] Building NO-TEXTURE primitives..." << std::endl;
        Transform tri_World2Object = Inverse(tri_Object2World);
        Bounds3f totalBounds; // 包围盒
        size_t startPrimCount = prims.size();

        for (size_t i = 0; i < meshes.size(); i++) {
            std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
            for (int j = 0; j < meshPtr->nTriangles; ++j) {
                auto tri = std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j);
                prims.push_back(std::make_shared<GeometricPrimitive>(tri, material, nullptr, mediumInterface));
                totalBounds = Union(totalBounds, tri->WorldBound());
            }
        }

        Point3f c = (totalBounds.pMin + totalBounds.pMax) * 0.5f;
        std::cout << "=======================================================" << std::endl;
        std::cout << "[ModelLoad] NO-TEXTURE BUILD SUMMARY:" << std::endl;
        std::cout << "  > Added Primitives: " << (prims.size() - startPrimCount) << std::endl;
        std::cout << "  > Total World Bounds: Min(" << totalBounds.pMin.x << "," << totalBounds.pMin.y << "," << totalBounds.pMin.z << ") - "
            << "Max(" << totalBounds.pMax.x << "," << totalBounds.pMax.y << "," << totalBounds.pMax.z << ")" << std::endl;
        std::cout << "  > SUGGESTED Camera LookAt: (" << c.x << ", " << c.y << ", " << c.z << ")" << std::endl;
        std::cout << "=======================================================" << std::endl;

        meshes.clear(); meshMaterialIndices.clear(); loadedMaterials.clear(); directory = "";
    }

    void ModelLoad::buildTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
        std::vector<std::shared_ptr<Primitive>>& prims) {

        std::cout << "[ModelLoad] Building TEXTURED primitives..." << std::endl;
        Transform tri_World2Object = Inverse(tri_Object2World);
        Bounds3f totalBounds; // 包围盒
        size_t startPrimCount = prims.size();

        for (size_t i = 0; i < meshes.size(); ++i) {
            std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
            std::shared_ptr<Material> material = loadedMaterials[meshMaterialIndices[i]];
            if (!material) material = std::make_shared<MatteMaterial>(std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.5f)), nullptr, nullptr);

            for (int j = 0; j < meshPtr->nTriangles; ++j) {
                auto tri = std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j);
                prims.push_back(std::make_shared<GeometricPrimitive>(tri, material, nullptr, mediumInterface));
                totalBounds = Union(totalBounds, tri->WorldBound());
            }
        }

        Point3f c = (totalBounds.pMin + totalBounds.pMax) * 0.5f;
        std::cout << "=======================================================" << std::endl;
        std::cout << "[ModelLoad] TEXTURED BUILD SUMMARY:" << std::endl;
        std::cout << "  > Added Primitives: " << (prims.size() - startPrimCount) << std::endl;
        std::cout << "  > Total World Bounds: Min(" << totalBounds.pMin.x << "," << totalBounds.pMin.y << "," << totalBounds.pMin.z << ") - "
            << "Max(" << totalBounds.pMax.x << "," << totalBounds.pMax.y << "," << totalBounds.pMax.z << ")" << std::endl;
        std::cout << "  > SUGGESTED Camera LookAt: (" << c.x << ", " << c.y << ", " << c.z << ")" << std::endl;
        std::cout << "=======================================================" << std::endl;

        meshes.clear(); meshMaterialIndices.clear(); loadedMaterials.clear(); directory = "";
    }

} // namespace PBR