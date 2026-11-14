#include "./Assimp_ModelLoader.h"
namespace FCT
{
    namespace
    {
        Mat4 convertAssimpMatrix(const aiMatrix4x4& from)
        {
            return Mat4(
                from.a1, from.a2, from.a3, from.a4,
                from.b1, from.b2, from.b3, from.b4,
                from.c1, from.c2, from.c3, from.c4,
                from.d1, from.d2, from.d3, from.d4
            );
        }

        int registerOrGetBoneIndex(ModelSkeletonData& skeletonData, const aiBone* bone)
        {
            std::string boneName = bone->mName.C_Str();
            auto it = skeletonData.boneNameToIndex.find(boneName);
            if (it != skeletonData.boneNameToIndex.end()) {
                return it->second;
            }
            int boneId = static_cast<int>(skeletonData.bones.size());
            BoneInfo info;
            info.name = boneName;
            info.id = boneId;
            info.offsetMatrix = convertAssimpMatrix(bone->mOffsetMatrix);
            skeletonData.bones.push_back(info);
            skeletonData.boneNameToIndex[boneName] = boneId;
            return boneId;
        }

        void populateMeshBoneWeights(const aiMesh* aiMesh, ModelMesh* mesh, ModelSkeletonData& skeletonData)
        {
            if (!aiMesh || !aiMesh->HasBones()) {
                return;
            }
            for (unsigned int b = 0; b < aiMesh->mNumBones; ++b) {
                const aiBone* bone = aiMesh->mBones[b];
                int boneId = registerOrGetBoneIndex(skeletonData, bone);
                if (!mesh) {
                    continue;
                }
                for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                    uint32_t vertexId = bone->mWeights[w].mVertexId;
                    float weight = bone->mWeights[w].mWeight;
                    if (vertexId < mesh->vertices.size()) {
                        mesh->vertices[vertexId].addBoneWeight(boneId, weight);
                    }
                }
            }
            if (mesh) {
                for (auto& v : mesh->vertices) {
                    v.normalizeBoneWeights();
                }
            }
        }

        SkeletonNode buildSkeletonNodeTree(const aiNode* aiNode, const ModelSkeletonData& skeletonData)
        {
            SkeletonNode node;
            if (!aiNode) {
                return node;
            }
            node.name = aiNode->mName.C_Str();
            node.transformation = convertAssimpMatrix(aiNode->mTransformation);
            auto it = skeletonData.boneNameToIndex.find(node.name);
            if (it != skeletonData.boneNameToIndex.end()) {
                node.boneIndex = it->second;
            }
            node.children.reserve(aiNode->mNumChildren);
            for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
                node.children.push_back(buildSkeletonNodeTree(aiNode->mChildren[i], skeletonData));
            }
            return node;
        }

        bool findSkinnedMeshNodeGlobalTransform(
            const aiNode* node,
            const aiScene* scene,
            const Mat4& parentTransform,
            Mat4& outTransform)
        {
            if (!node || !scene) {
                return false;
            }
            Mat4 currentTransform = parentTransform * convertAssimpMatrix(node->mTransformation);
            for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
                unsigned int meshIdx = node->mMeshes[m];
                if (meshIdx < scene->mNumMeshes && scene->mMeshes[meshIdx]->HasBones()) {
                    outTransform = currentTransform;
                    return true;
                }
            }
            for (unsigned int c = 0; c < node->mNumChildren; ++c) {
                if (findSkinnedMeshNodeGlobalTransform(node->mChildren[c], scene, currentTransform, outTransform)) {
                    return true;
                }
            }
            return false;
        }

        BoneAnimationChannel extractAnimationChannel(const aiNodeAnim* aiChannel)
        {
            BoneAnimationChannel channel;
            channel.nodeName = aiChannel->mNodeName.C_Str();

            channel.positions.reserve(aiChannel->mNumPositionKeys);
            for (unsigned int k = 0; k < aiChannel->mNumPositionKeys; ++k) {
                const auto& key = aiChannel->mPositionKeys[k];
                channel.positions.push_back(KeyVec3{
                    key.mTime,
                    Vec3<>(key.mValue.x, key.mValue.y, key.mValue.z)
                });
            }

            channel.rotations.reserve(aiChannel->mNumRotationKeys);
            for (unsigned int k = 0; k < aiChannel->mNumRotationKeys; ++k) {
                const auto& key = aiChannel->mRotationKeys[k];
                channel.rotations.push_back(KeyQuat{
                    key.mTime,
                    Quat(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w)
                });
            }

            channel.scales.reserve(aiChannel->mNumScalingKeys);
            for (unsigned int k = 0; k < aiChannel->mNumScalingKeys; ++k) {
                const auto& key = aiChannel->mScalingKeys[k];
                channel.scales.push_back(KeyVec3{
                    key.mTime,
                    Vec3<>(key.mValue.x, key.mValue.y, key.mValue.z)
                });
            }
            return channel;
        }

        void extractSceneAnimations(const aiScene* aiScene, ModelSkeletonData& skeletonData)
        {
            if (!aiScene || !aiScene->HasAnimations()) {
                return;
            }
            skeletonData.animations.reserve(aiScene->mNumAnimations);
            for (unsigned int a = 0; a < aiScene->mNumAnimations; ++a) {
                const aiAnimation* aiAnim = aiScene->mAnimations[a];
                AnimationClip clip;
                clip.name = aiAnim->mName.C_Str();
                if (clip.name.empty()) {
                    clip.name = "Animation_" + std::to_string(a);
                }
                clip.duration = aiAnim->mDuration;
                clip.ticksPerSecond = aiAnim->mTicksPerSecond > 0.0 ? aiAnim->mTicksPerSecond : 25.0;
                clip.channels.reserve(aiAnim->mNumChannels);
                for (unsigned int c = 0; c < aiAnim->mNumChannels; ++c) {
                    BoneAnimationChannel channel = extractAnimationChannel(aiAnim->mChannels[c]);
                    clip.channelMap[channel.nodeName] = clip.channels.size();
                    clip.channels.push_back(std::move(channel));
                }
                skeletonData.animations.push_back(std::move(clip));
            }
        }

        void finalizeSkeletonHierarchy(const aiScene* aiScene, ModelSkeletonData& skeletonData)
        {
            if (!aiScene || !aiScene->mRootNode) {
                return;
            }
            Mat4 meshGlobalTransform;
            if (findSkinnedMeshNodeGlobalTransform(aiScene->mRootNode, aiScene, Mat4(), meshGlobalTransform)) {
                skeletonData.globalInverseTransform = meshGlobalTransform.inverse();
            } else {
                skeletonData.globalInverseTransform = convertAssimpMatrix(aiScene->mRootNode->mTransformation).inverse();
            }
            skeletonData.rootNode = buildSkeletonNodeTree(aiScene->mRootNode, skeletonData);
            extractSceneAnimations(aiScene, skeletonData);
            skeletonData.prepareRuntimeCache();
        }
    }

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
        importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 60.0f);
        const auto aiScene = importer.ReadFile(path,
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType |
            aiProcess_LimitBoneWeights);

        auto ret = std::make_unique<ModelData>();
        if (!aiScene) {
            return ret;
        }

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

            populateMeshBoneWeights(aiMesh, mesh, ret->skeletonData);

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

        if (ret->skeletonData.hasSkeleton() || aiScene->HasAnimations()) {
            finalizeSkeletonHierarchy(aiScene, ret->skeletonData);
        }

        return ret;
    }

    std::shared_ptr<ModelSkeletonData> Assimp_ModelLoader::loadSkeletonData(const std::string& path)
    {
        ZoneScoped();
        ZoneText(path.c_str(), path.size());
        Assimp::Importer importer;
        const auto aiScene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_LimitBoneWeights);

        if (!aiScene) {
            return nullptr;
        }

        auto skeletonData = std::make_shared<ModelSkeletonData>();
        for (unsigned int i = 0; i < aiScene->mNumMeshes; ++i) {
            populateMeshBoneWeights(aiScene->mMeshes[i], nullptr, *skeletonData);
        }
        if (skeletonData->hasSkeleton() || aiScene->HasAnimations()) {
            finalizeSkeletonHierarchy(aiScene, *skeletonData);
        }
        return skeletonData;
    }

    ModelInfo::SceneInfo Assimp_ModelLoader::loadModelInfo(const std::string& path)
    {
        ZoneScoped();
        ZoneText(path.c_str(), path.size());
        ModelInfo::SceneInfo info;

        Assimp::Importer importer;
        importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 60.0f);
        const auto aiScene = importer.ReadFile(path,
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType |
            aiProcess_LimitBoneWeights);

        if (!aiScene) {
            return info;
        }
        info.name = aiScene->mName.C_Str();
        std::unordered_set<std::string> uniqueBones;

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
            meshInfo.hasBones = aiMesh->HasBones();
            meshInfo.boneCount = aiMesh->mNumBones;

            for (unsigned int b = 0; b < aiMesh->mNumBones; ++b) {
                uniqueBones.insert(aiMesh->mBones[b]->mName.C_Str());
            }

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

        info.boneCount = static_cast<uint32_t>(uniqueBones.size());
        if (aiScene->HasAnimations()) {
            info.animationNames.reserve(aiScene->mNumAnimations);
            for (unsigned int a = 0; a < aiScene->mNumAnimations; ++a) {
                std::string animName = aiScene->mAnimations[a]->mName.C_Str();
                if (animName.empty()) {
                    animName = "Animation_" + std::to_string(a);
                }
                info.animationNames.push_back(animName);
            }
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

        for (const auto& [texPath, textureInfo] : textureInfoMap) {
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
