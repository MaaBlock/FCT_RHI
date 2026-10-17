#include "./Assimp_ModelLoader.h"
namespace FCT
{
    /**
     * @brief 将Assimp纹理类型转换为ModelInfo::TextureType
     */
    ModelInfo::TextureType convertAssimpTextureType(aiTextureType assimpType)
    {
        switch (assimpType)
        {
            case aiTextureType_DIFFUSE: return ModelInfo::TextureType::diffuse;
            case aiTextureType_SPECULAR: return ModelInfo::TextureType::specular;
            case aiTextureType_NORMALS: return ModelInfo::TextureType::normals;
            case aiTextureType_HEIGHT: return ModelInfo::TextureType::height;
            case aiTextureType_EMISSIVE: return ModelInfo::TextureType::emissive;
            case aiTextureType_SHININESS: return ModelInfo::TextureType::shininess;
            case aiTextureType_OPACITY: return ModelInfo::TextureType::opacity;
            case aiTextureType_DISPLACEMENT: return ModelInfo::TextureType::displacement;
            case aiTextureType_LIGHTMAP: return ModelInfo::TextureType::lightmap;
            case aiTextureType_REFLECTION: return ModelInfo::TextureType::reflection;
            case aiTextureType_BASE_COLOR: return ModelInfo::TextureType::baseColor;
            case aiTextureType_NORMAL_CAMERA: return ModelInfo::TextureType::normalCamera;
            case aiTextureType_EMISSION_COLOR: return ModelInfo::TextureType::emissionColor;
            case aiTextureType_METALNESS: return ModelInfo::TextureType::metalness;
            case aiTextureType_DIFFUSE_ROUGHNESS: return ModelInfo::TextureType::diffuseRoughness;
            case aiTextureType_AMBIENT_OCCLUSION: return ModelInfo::TextureType::ambientOcclusion;
            case aiTextureType_SHEEN: return ModelInfo::TextureType::sheen;
            case aiTextureType_CLEARCOAT: return ModelInfo::TextureType::clearcoat;
            case aiTextureType_TRANSMISSION: return ModelInfo::TextureType::transmission;
            default: return ModelInfo::TextureType::unknown;
        }
    }
    std::unique_ptr<ModelData> Assimp_ModelLoader::loadModel(const std::string& path)
    {
        ZoneScoped();
        ZoneText(path.c_str(), path.size());
        Assimp::Importer importer;
        const auto aiScene = importer.ReadFile(path,
            aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

        auto ret = std::make_unique<ModelData>();
        for (unsigned int i = 0; i < aiScene->mNumMeshes; i++)
        {
            const auto aiMesh = aiScene->mMeshes[i];
            auto mesh = new ModelMesh();
            mesh->name = aiMesh->mName.C_Str();
            mesh->material = nullptr;

            mesh->vertices.resize(aiMesh->mNumVertices);

            if (aiMesh->HasPositions())
            {
                for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                    const auto& aiVertex = aiMesh->mVertices[j];
                    mesh->vertices[j].position = { aiVertex.x, aiVertex.y, aiVertex.z };
                }
            }

            if (aiMesh->HasNormals())
            {
                for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                    const auto& aiNormal = aiMesh->mNormals[j];
                    mesh->vertices[j].normal = { aiNormal.x, aiNormal.y, aiNormal.z };
                }
            }

            for (unsigned int texCoordSet = 0; texCoordSet < 8; texCoordSet++)
            {
                if (aiMesh->HasTextureCoords(texCoordSet))
                {
                    for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                        const auto& aiTexCoord = aiMesh->mTextureCoords[texCoordSet][j];
                        mesh->vertices[j].texCoords[texCoordSet] = { aiTexCoord.x, aiTexCoord.y };
                    }
                }
            }

            if (aiMesh->HasTangentsAndBitangents())
            {
                for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                    const auto& aiTangent = aiMesh->mTangents[j];
                    const auto& aiBitangent = aiMesh->mBitangents[j];
                    mesh->vertices[j].tangent = { aiTangent.x, aiTangent.y, aiTangent.z };
                    mesh->vertices[j].bitangent = { aiBitangent.x, aiBitangent.y, aiBitangent.z };
                }
            }

            for (unsigned int colorSet = 0; colorSet < 8; colorSet++)
            {
                if (aiMesh->HasVertexColors(colorSet))
                {
                    for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                        const auto& aiColor = aiMesh->mColors[colorSet][j];
                        mesh->vertices[j].colors[colorSet] = { aiColor.r, aiColor.g, aiColor.b, aiColor.a };
                    }
                }
            }

            if (aiMesh->HasFaces())
            {
                mesh->indices.reserve(aiMesh->mNumFaces * 3);
                for (unsigned int j = 0; j < aiMesh->mNumFaces; j++)
                {
                    const aiFace& face = aiMesh->mFaces[j];
                    for (unsigned int k = 0; k < face.mNumIndices; k++)
                    {
                        mesh->indices.push_back(face.mIndices[k]);
                    }
                }
            }

            ret->meshes.push_back(mesh);
        }
        return ret;
    }
    ModelInfo::SceneInfo Assimp_ModelLoader::loadModelInfo(const std::string& path)
    {
        ZoneScoped();
        ZoneText(path.c_str(), path.size());
        ModelInfo::SceneInfo info;

        Assimp::Importer importer;
        const auto aiScene = importer.ReadFile(path,
            aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

        if (!aiScene) {
            return info;
        }
        info.name = aiScene->mName.C_Str();
        for (unsigned int i = 0; i < aiScene->mNumMeshes; i++)
        {
            const auto aiMesh = aiScene->mMeshes[i];
            ModelInfo::MeshInfo meshInfo;

            meshInfo.name = aiMesh->mName.C_Str();
            meshInfo.vertexCount = aiMesh->mNumVertices;
            meshInfo.hasPositions = aiMesh->HasPositions();
            meshInfo.hasNormals = aiMesh->HasNormals();
            meshInfo.hasTangents = aiMesh->HasTangentsAndBitangents();
            meshInfo.hasBitangents = aiMesh->HasTangentsAndBitangents();

            for (unsigned int texCoordSet = 0; texCoordSet < 8; texCoordSet++) {
                meshInfo.hasTexCoords[texCoordSet] = aiMesh->HasTextureCoords(texCoordSet);
            }

            for (unsigned int colorSet = 0; colorSet < 8; colorSet++) {
                meshInfo.hasVertexColors[colorSet] = aiMesh->HasVertexColors(colorSet);
            }

            meshInfo.indexCount = 0;
            meshInfo.triangleCount = 0;
            if (aiMesh->HasFaces()) {
                meshInfo.isIndexed = true;
                for (unsigned int j = 0; j < aiMesh->mNumFaces; j++) {
                    const aiFace& face = aiMesh->mFaces[j];
                    meshInfo.indexCount += face.mNumIndices;
                    if (face.mNumIndices == 3) {
                        meshInfo.triangleCount++;
                    }
                }
            }

            if (aiMesh->HasPositions()) {
                aiVector3D min = aiMesh->mVertices[0];
                aiVector3D max = aiMesh->mVertices[0];

                for (unsigned int j = 1; j < aiMesh->mNumVertices; j++) {
                    const aiVector3D& vertex = aiMesh->mVertices[j];
                    min.x = std::min(min.x, vertex.x);
                    min.y = std::min(min.y, vertex.y);
                    min.z = std::min(min.z, vertex.z);
                    max.x = std::max(max.x, vertex.x);
                    max.y = std::max(max.y, vertex.y);
                    max.z = std::max(max.z, vertex.z);
                }

                meshInfo.boundingBoxMin = Vec3(min.x, min.y, min.z);
                meshInfo.boundingBoxMax = Vec3(max.x, max.y, max.z);
                meshInfo.boundingBoxCenter = Vec3(
                    (min.x + max.x) * 0.5f,
                    (min.y + max.y) * 0.5f,
                    (min.z + max.z) * 0.5f
                );
            }

            info.meshInfos.push_back(meshInfo);
        }
          std::map<std::string, ModelInfo::TextureInfo> textureInfoMap;
        std::filesystem::path modelDir = std::filesystem::path(path).parent_path();

        for (unsigned int i = 0; i < aiScene->mNumMaterials; i++)
        {
            const auto aiMaterial = aiScene->mMaterials[i];

            std::vector<aiTextureType> textureTypes = {
                aiTextureType_DIFFUSE,
                aiTextureType_SPECULAR,
                aiTextureType_AMBIENT,
                aiTextureType_EMISSIVE,
                aiTextureType_HEIGHT,
                aiTextureType_NORMALS,
                aiTextureType_SHININESS,
                aiTextureType_OPACITY,
                aiTextureType_DISPLACEMENT,
                aiTextureType_LIGHTMAP,
                aiTextureType_REFLECTION,
                aiTextureType_BASE_COLOR,
                aiTextureType_NORMAL_CAMERA,
                aiTextureType_EMISSION_COLOR,
                aiTextureType_METALNESS,
                aiTextureType_DIFFUSE_ROUGHNESS,
                aiTextureType_AMBIENT_OCCLUSION,
                aiTextureType_SHEEN,
                aiTextureType_CLEARCOAT,
                aiTextureType_TRANSMISSION
            };

            for (auto textureType : textureTypes) {
                unsigned int textureCount = aiMaterial->GetTextureCount(textureType);

                for (unsigned int j = 0; j < textureCount; j++) {
                    aiString texturePath;
                    if (aiMaterial->GetTexture(textureType, j, &texturePath) == AI_SUCCESS) {
                        std::string texturePathStr = texturePath.C_Str();

                        if (texturePathStr.empty()) {
                            continue;
                        }

                        if (textureInfoMap.find(texturePathStr) == textureInfoMap.end()) {
                            ModelInfo::TextureInfo textureInfo;
                            textureInfo.path = texturePathStr;
                            textureInfo.isInner = (texturePathStr[0] == '*');
                            textureInfo.types.insert(convertAssimpTextureType(textureType));

                            textureInfoMap[texturePathStr] = textureInfo;
                        } else {
                            textureInfoMap[texturePathStr].types.insert(convertAssimpTextureType(textureType));
                        }
                    }
                }
            }
        }

        for (const auto& [path, textureInfo] : textureInfoMap) {
            info.textureInfos.push_back(textureInfo);
        }

        return info;
    }

    std::set<std::string> Assimp_ModelLoader::resolveTexturePaths(const std::string& modelPath) const
    {
        std::set<std::string> texturePaths;

        Assimp::Importer importer;
        const auto aiScene = importer.ReadFile(modelPath,
            aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

        if (!aiScene || !aiScene->HasMaterials()) {
            return texturePaths;
        }

        std::filesystem::path modelDir = std::filesystem::path(modelPath).parent_path();

        for (unsigned int i = 0; i < aiScene->mNumMaterials; i++) {
            const auto aiMaterial = aiScene->mMaterials[i];

            std::vector<aiTextureType> textureTypes = {
                aiTextureType_DIFFUSE,
                aiTextureType_SPECULAR,
                aiTextureType_AMBIENT,
                aiTextureType_EMISSIVE,
                aiTextureType_HEIGHT,
                aiTextureType_NORMALS,
                aiTextureType_SHININESS,
                aiTextureType_OPACITY,
                aiTextureType_DISPLACEMENT,
                aiTextureType_LIGHTMAP,
                aiTextureType_REFLECTION,
                aiTextureType_BASE_COLOR,
                aiTextureType_NORMAL_CAMERA,
                aiTextureType_EMISSION_COLOR,
                aiTextureType_METALNESS,
                aiTextureType_DIFFUSE_ROUGHNESS,
                aiTextureType_AMBIENT_OCCLUSION,
                aiTextureType_SHEEN,
                aiTextureType_CLEARCOAT,
                aiTextureType_TRANSMISSION
            };

            for (auto textureType : textureTypes) {
                unsigned int textureCount = aiMaterial->GetTextureCount(textureType);

                for (unsigned int j = 0; j < textureCount; j++) {
                    aiString texturePath;
                    if (aiMaterial->GetTexture(textureType, j, &texturePath) == AI_SUCCESS) {
                        std::string texturePathStr = texturePath.C_Str();

                        if (texturePathStr.empty()) {
                            continue;
                        }

                        if (texturePathStr[0] == '*') {
                            continue;
                        }

                        std::vector<std::string> candidatePaths;

                        candidatePaths.push_back(texturePathStr);

                        candidatePaths.push_back((modelDir / texturePathStr).string());

                        std::filesystem::path textureFileName = std::filesystem::path(texturePathStr).filename();
                        candidatePaths.push_back((modelDir / textureFileName).string());

                        std::vector<std::string> commonTextureDirs = {"textures", "texture", "images", "img", "maps"};
                        for (const auto& texDir : commonTextureDirs) {
                            candidatePaths.push_back((modelDir / texDir / textureFileName).string());
                        }

                        bool found = false;
                        for (const auto& candidatePath : candidatePaths) {
                            if (std::filesystem::exists(candidatePath)) {
                                try {
                                    std::string absolutePath = std::filesystem::canonical(candidatePath).string();
                                    texturePaths.insert(absolutePath);
                                    found = true;
                                    break;
                                } catch (const std::filesystem::filesystem_error&) {
                                    try {
                                        std::string absolutePath = std::filesystem::absolute(candidatePath).string();
                                        texturePaths.insert(absolutePath);
                                        found = true;
                                        break;
                                    } catch (const std::filesystem::filesystem_error&) {
                                        continue;
                                    }
                                }
                            }
                        }

                        if (!found) {
                            try {
                                std::string fallbackPath;
                                if (std::filesystem::path(texturePathStr).is_absolute()) {
                                    fallbackPath = std::filesystem::absolute(texturePathStr).string();
                                } else {
                                    fallbackPath = std::filesystem::absolute(modelDir / texturePathStr).string();
                                }
                                texturePaths.insert(fallbackPath);
                            } catch (const std::filesystem::filesystem_error&) {
                                texturePaths.insert(texturePathStr);
                            }
                        }
                    }
                }
            }
        }

        return texturePaths;
    }

    std::set<std::string> Assimp_ModelLoader::getPlatformSupportedExtensions() const
    {
        return {

        };
    }

    bool Assimp_ModelLoader::getEmbeddedTextureData(const std::string& modelPath, int textureIndex, 
                                                    std::vector<unsigned char>& outData) const
    {
        try {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate);
            
            if (!scene) {
                std::cerr << "Failed to load model for embedded texture: " << modelPath << std::endl;
                return false;
            }
            
            if (textureIndex < 0 || textureIndex >= static_cast<int>(scene->mNumTextures)) {
                std::cerr << "Invalid texture index: " << textureIndex << ", available textures: " << scene->mNumTextures << std::endl;
                return false;
            }
            
            const aiTexture* texture = scene->mTextures[textureIndex];
            if (!texture) {
                std::cerr << "Texture at index " << textureIndex << " is null" << std::endl;
                return false;
            }
            
            size_t dataSize;
            const unsigned char* sourceData = reinterpret_cast<const unsigned char*>(texture->pcData);

            if (texture->mHeight == 0) {
                dataSize = texture->mWidth;
            } else {
                dataSize = texture->mWidth * texture->mHeight * 4;
            }

            outData.resize(dataSize);
            std::memcpy(outData.data(), sourceData, dataSize);
            
            std::cout << "Successfully loaded embedded texture " << textureIndex << " from " << modelPath 
                      << ", size: " << dataSize << " bytes" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Exception while loading embedded texture: " << e.what() << std::endl;
            return false;
        }
    }
}
