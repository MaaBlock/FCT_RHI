#ifndef MODELLOADER_H
#define MODELLOADER_H
#include "../ThirdParty.h"
#include "../Context/Vertex.h"
namespace FCT
{
    namespace ModelInfo
    {
        /**
         * @cond CHINESE
         * @brief 纹理类型枚举
         * @endcond
         */
        enum class TextureType : int
        {
            diffuse = 0,        ///< @cond CHINESE 漫反射贴图 @endcond
            specular,           ///< @cond CHINESE 镜面反射贴图 @endcond
            ambient,            ///< @cond CHINESE 环境光贴图 @endcond
            emissive,           ///< @cond CHINESE 自发光贴图 @endcond
            height,             ///< @cond CHINESE 高度贴图 @endcond
            normals,            ///< @cond CHINESE 法线贴图 @endcond
            shininess,          ///< @cond CHINESE 光泽度贴图 @endcond
            opacity,            ///< @cond CHINESE 透明度贴图 @endcond
            displacement,       ///< @cond CHINESE 位移贴图 @endcond
            lightmap,           ///< @cond CHINESE 光照贴图 @endcond
            reflection,         ///< @cond CHINESE 反射贴图 @endcond
            baseColor,          ///< @cond CHINESE 基础颜色贴图(PBR) @endcond
            normalCamera,       ///< @cond CHINESE 相机空间法线贴图 @endcond
            emissionColor,      ///< @cond CHINESE 发射颜色贴图 @endcond
            metalness,          ///< @cond CHINESE 金属度贴图(PBR) @endcond
            diffuseRoughness,   ///< @cond CHINESE 漫反射粗糙度贴图 @endcond
            ambientOcclusion,   ///< @cond CHINESE 环境遮蔽贴图 @endcond
            sheen,              ///< @cond CHINESE 光泽贴图 @endcond
            clearcoat,          ///< @cond CHINESE 透明涂层贴图 @endcond
            transmission,       ///< @cond CHINESE 透射贴图 @endcond
            unknown             ///< @cond CHINESE 未知类型 @endcond
        };


        struct MeshInfo
        {
            std::string name;
            uint32_t vertexCount;
            uint32_t indexCount;
            uint32_t triangleCount;
            bool isIndexed;

            bool hasPositions;
            bool hasNormals;
            bool hasTangents;
            bool hasBitangents;
            bool hasBones;
            uint32_t boneCount;

            std::array<bool, 8> hasTexCoords;
            std::array<bool, 8> hasVertexColors;

            Vec3<> boundingBoxMin;
            Vec3<> boundingBoxMax;
            Vec3<> boundingBoxCenter;

            MeshInfo() : vertexCount(0), indexCount(0), triangleCount(0),
                         isIndexed(false), hasPositions(false), hasNormals(false),
                         hasTangents(false), hasBitangents(false),
                         hasBones(false), boneCount(0),
                         boundingBoxMin(0.0f), boundingBoxMax(0.0f), boundingBoxCenter(0.0f)
            {
                hasTexCoords.fill(false);
                hasVertexColors.fill(false);
            }

        private:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & name;
                ar & vertexCount;
                ar & indexCount;
                ar & triangleCount;
                ar & isIndexed;
                ar & hasPositions;
                ar & hasNormals;
                ar & hasTangents;
                ar & hasBitangents;
                ar & hasTexCoords;
                ar & hasVertexColors;
                ar & boundingBoxMin;
                ar & boundingBoxMax;
                ar & boundingBoxCenter;
                if (version >= 2) {
                    ar & hasBones;
                    ar & boneCount;
                }
            }
        };

        struct MaterialInfo
        {
        private:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                // 当MaterialInfo有成员时在这里添加序列化
            }
        };

        struct TextureInfo
        {
            std::string path;
            bool isInner;
            std::set<TextureType> types;
            TextureInfo() : isInner(false) {}
        private:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & path;
                ar & isInner;
                if (version >= 3) {
                    ar & types;
                }
            }
        };

        struct SceneInfo
        {
            std::string name;
            std::vector<MeshInfo> meshInfos;
            std::vector<MaterialInfo> materialInfos;
            std::vector<TextureInfo> textureInfos;
            std::vector<std::string> animationNames;
            uint32_t boneCount = 0;

        private:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & name;
                ar & meshInfos;
                ar & materialInfos;
                ar & textureInfos;
                if (version >= 2) {
                    ar & animationNames;
                    ar & boneCount;
                }
            }
        };
    }
}

BOOST_CLASS_VERSION(FCT::ModelInfo::MeshInfo, 2);
BOOST_CLASS_VERSION(FCT::ModelInfo::MaterialInfo, 1);
BOOST_CLASS_VERSION(FCT::ModelInfo::TextureInfo, 3);
BOOST_CLASS_VERSION(FCT::ModelInfo::SceneInfo, 2);

namespace boost {
namespace serialization {
    template<class Archive>
    void serialize(Archive & ar, FCT::ModelInfo::TextureType & t, const unsigned int version)
    {
        using UnderlyingType = std::underlying_type_t<FCT::ModelInfo::TextureType>;
        UnderlyingType value = static_cast<UnderlyingType>(t);
        ar & value;
        t = static_cast<FCT::ModelInfo::TextureType>(value);
    }

}
}
namespace FCT
{
    struct BoneInfo
    {
        std::string name;
        int id = -1;
        Mat4 offsetMatrix;
    };

    struct SkeletonTRS
    {
        Vec3<> translation{0.0f, 0.0f, 0.0f};
        Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
        Vec3<> scale{1.0f, 1.0f, 1.0f};

        Mat4 toMatrix() const;
        static SkeletonTRS fromMatrix(const Mat4& mat);
        static SkeletonTRS blend(const SkeletonTRS& a, const SkeletonTRS& b, float weight);
    };

    struct SkeletonNode
    {
        std::string name;
        Mat4 transformation;
        int boneIndex = -1;
        std::vector<SkeletonNode> children;
    };

    struct FlattenedSkeletonNode
    {
        std::string name;
        Mat4 bindTransform;
        SkeletonTRS bindTRS;
        int32_t parentIndex = -1;
        int32_t boneIndex = -1;
    };

    struct KeyVec3
    {
        double time = 0.0;
        Vec3<> value;
    };

    struct KeyQuat
    {
        double time = 0.0;
        Quat value;
    };

    struct BoneAnimationChannel
    {
        std::string nodeName;
        std::vector<KeyVec3> positions;
        std::vector<KeyQuat> rotations;
        std::vector<KeyVec3> scales;

        Vec3<> interpolatePosition(double animationTime) const;
        Quat interpolateRotation(double animationTime) const;
        Vec3<> interpolateScale(double animationTime) const;
        SkeletonTRS evaluateLocalTRS(double animationTime, const SkeletonTRS& fallbackBind) const;
        Mat4 evaluateLocalTransform(double animationTime) const;
    };

    struct AnimationClip
    {
        std::string name;
        double duration = 0.0;
        double ticksPerSecond = 25.0;
        std::vector<BoneAnimationChannel> channels;
        std::unordered_map<std::string, size_t> channelMap;
        std::vector<int32_t> flatNodeToChannel;

        const BoneAnimationChannel* findChannel(const std::string& nodeName) const
        {
            auto it = channelMap.find(nodeName);
            if (it != channelMap.end() && it->second < channels.size()) {
                return &channels[it->second];
            }
            return nullptr;
        }

        const BoneAnimationChannel* findChannelByFlatNode(size_t flatNodeIndex) const noexcept
        {
            if (flatNodeIndex < flatNodeToChannel.size()) {
                const int32_t chIdx = flatNodeToChannel[flatNodeIndex];
                if (chIdx >= 0 && static_cast<size_t>(chIdx) < channels.size()) {
                    return &channels[static_cast<size_t>(chIdx)];
                }
            }
            return nullptr;
        }

        float getDurationInSeconds() const
        {
            double tps = ticksPerSecond > 0.0 ? ticksPerSecond : 25.0;
            return static_cast<float>(duration / tps);
        }

        double secondsToTicks(float timeInSeconds, bool loop) const;
    };

    struct SkeletalBlendParams
    {
        std::string primaryClipName;
        float primaryTimeSec = 0.0f;
        bool primaryLoop = true;

        std::string targetClipName;
        float targetTimeSec = 0.0f;
        bool targetLoop = true;
        float blendWeight = 0.0f;

        std::string layerClipName;
        float layerTimeSec = 0.0f;
        bool layerLoop = true;
        float layerWeight = 0.0f;
        std::string layerMaskRootBone;

        bool lockRootPosition = false;
    };

    struct ModelSkeletonData
    {
        std::vector<BoneInfo> bones;
        std::unordered_map<std::string, int> boneNameToIndex;
        SkeletonNode rootNode;
        Mat4 globalInverseTransform;
        std::vector<AnimationClip> animations;

        std::vector<FlattenedSkeletonNode> flatNodes;
        std::unordered_map<std::string, int32_t> flatNodeNameToIndex;
        int32_t primaryMotionNodeIndex = -1;
        bool runtimeCachePrepared = false;

        bool hasSkeleton() const noexcept
        {
            return !bones.empty();
        }

        const AnimationClip* findAnimation(const std::string& clipName) const
        {
            if (animations.empty()) {
                return nullptr;
            }
            if (clipName.empty()) {
                return &animations[0];
            }
            for (const auto& clip : animations) {
                if (clip.name == clipName) {
                    return &clip;
                }
            }
            return &animations[0];
        }

        const AnimationClip* findAnimationExact(const std::string& clipName) const
        {
            if (clipName.empty()) {
                return nullptr;
            }
            for (const auto& clip : animations) {
                if (clip.name == clipName) {
                    return &clip;
                }
            }
            return nullptr;
        }

        void prepareRuntimeCache();
        int32_t findFlatNodeIndex(const std::string& nodeName) const;
        std::vector<std::string> getAnimationNames() const;
        std::vector<std::string> getNodeNames() const;

        void evaluateBindPose(std::vector<Mat4>& outBoneMatrices, size_t maxBones = 256) const;
        void evaluateAnimation(const std::string& clipName, float timeInSeconds, bool loop,
                               std::vector<Mat4>& outBoneMatrices, size_t maxBones = 256) const;
        void evaluateBlendedPose(
            const SkeletalBlendParams& params,
            std::vector<Mat4>& outBoneMatrices,
            std::vector<Mat4>* outGlobalNodeMatrices = nullptr,
            size_t maxBones = 256) const;
    };

    struct ModelVertex
    {
        ModelVertex() :  position(0.0f, 0.0f, 0.0f),
                         normal(0.0f, 0.0f, 1.0f),
                         tangent(1.0f, 0.0f, 0.0f),
                         bitangent(0.0f, 1.0f, 0.0f)
        {
            for (int i = 0; i < 8; i++) {
                texCoords[i] = Vec2(0.0f, 0.0f);
            }

            for (int i = 0; i < 8; i++) {
                colors[i] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            }

            for (int i = 0; i < 4; i++) {
                boneIds[i] = 0;
                boneWeights[i] = 0.0f;
            }
        }
        Vec3<> position;
        Vec3<> normal;
        Vec2 texCoords[8];
        Vec3<> tangent;
        Vec3<> bitangent;
        Vec4 colors[8];
        int32_t boneIds[4];
        float boneWeights[4];

        void addBoneWeight(int32_t boneId, float weight)
        {
            if (weight <= 0.0f) {
                return;
            }
            for (int i = 0; i < 4; ++i) {
                if (boneWeights[i] == 0.0f) {
                    boneIds[i] = boneId;
                    boneWeights[i] = weight;
                    return;
                }
            }
            int minIdx = 0;
            for (int i = 1; i < 4; ++i) {
                if (boneWeights[i] < boneWeights[minIdx]) {
                    minIdx = i;
                }
            }
            if (weight > boneWeights[minIdx]) {
                boneIds[minIdx] = boneId;
                boneWeights[minIdx] = weight;
            }
        }

        void normalizeBoneWeights()
        {
            float sum = boneWeights[0] + boneWeights[1] + boneWeights[2] + boneWeights[3];
            if (sum > 1e-6f) {
                float invSum = 1.0f / sum;
                for (int i = 0; i < 4; ++i) {
                    boneWeights[i] *= invSum;
                }
            }
        }
    };
    struct ModelMaterial
    {

    };
    struct ModelMesh
    {
        ModelMaterial* material;
        std::vector<ModelVertex> vertices;
        std::vector<uint32_t> indices;
        std::string name;
    };
    struct ModelObject
    {
        std::vector<ModelObject*> children;
        ModelMesh* mesh;
    };
    struct ModelData
    {
        std::vector<ModelMaterial*> materials;
        std::vector<ModelMesh*> meshes;
        std::vector<ModelObject*> objects;
        ModelSkeletonData skeletonData;

        ModelMesh* findMesh(const std::string &name)
        {
            auto it = std::find_if(meshes.begin(), meshes.end(), [&name](const ModelMesh* mesh)
            {
                return mesh->name == name;
            });

            return (it != meshes.end()) ? *it : nullptr;
        }
    };

    inline void setVertexAttributeFromModel(Vertex& vertex, size_t elementIndex,
                                    const VertexElement& element,
                                    const ModelVertex& modelVertex) {
        ModelVertexAttribute modelAttr = element.getModelAttribute();
        Format format = element.getFormat();

        if (modelAttr == ModelVertexAttribute::Position) {
            switch (format) {
            case Format::R32G32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec2(modelVertex.position.x, modelVertex.position.y));
                break;
            case Format::R32G32B32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec3(modelVertex.position.x, modelVertex.position.y, modelVertex.position.z));
                break;
            case Format::R32G32B32A32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec4(modelVertex.position.x, modelVertex.position.y, modelVertex.position.z, 1.0f));
                break;
            default:
                    break;
            }
            return;
        }

        if (modelAttr == ModelVertexAttribute::Normal) {
            switch (format) {
            case Format::R32G32B32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec3(modelVertex.normal.x, modelVertex.normal.y, modelVertex.normal.z));
                break;
            default:
                    break;
            }
            return;
        }

        if (modelAttr == ModelVertexAttribute::Tangent) {
            switch (format) {
            case Format::R32G32B32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec3(modelVertex.tangent.x, modelVertex.tangent.y, modelVertex.tangent.z));
                break;
            default:
                    break;
            }
            return;
        }

        if (modelAttr == ModelVertexAttribute::Bitangent) {
            switch (format) {
            case Format::R32G32B32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec3(modelVertex.bitangent.x, modelVertex.bitangent.y, modelVertex.bitangent.z));
                break;
            default:
                    break;
            }
            return;
        }

        if (modelAttr >= ModelVertexAttribute::TexCoord0 && modelAttr <= ModelVertexAttribute::TexCoord7) {
            size_t texCoordIndex = static_cast<size_t>(modelAttr) - static_cast<size_t>(ModelVertexAttribute::TexCoord0);
            if (texCoordIndex < 8) {
                switch (format) {
                case Format::R32G32_SFLOAT:
                    vertex.setAttribute(elementIndex, Vec2(modelVertex.texCoords[texCoordIndex].x, modelVertex.texCoords[texCoordIndex].y));
                    break;
                default:
                        break;
                }
            }
            return;
        }

        if (modelAttr >= ModelVertexAttribute::Color0 && modelAttr <= ModelVertexAttribute::Color7) {
            size_t colorIndex = static_cast<size_t>(modelAttr) - static_cast<size_t>(ModelVertexAttribute::Color0);
            if (colorIndex < 8) {
                switch (format) {
                case Format::R32G32B32_SFLOAT:
                    vertex.setAttribute(elementIndex, Vec3(modelVertex.colors[colorIndex].x, modelVertex.colors[colorIndex].y, modelVertex.colors[colorIndex].z));
                    break;
                case Format::R32G32B32A32_SFLOAT:
                    vertex.setAttribute(elementIndex, Vec4(modelVertex.colors[colorIndex].x, modelVertex.colors[colorIndex].y, modelVertex.colors[colorIndex].z, modelVertex.colors[colorIndex].w));
                    break;
                case Format::R8G8B8A8_UNORM:
                    {
                        Vector4<uint8_t> colorData(
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].x * 255.0f),
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].y * 255.0f),
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].z * 255.0f),
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].w * 255.0f)
                        );
                        vertex.setAttribute(elementIndex, colorData);
                    }
                    break;
                case Format::B8G8R8A8_UNORM:
                    {
                        Vector4<uint8_t> colorData(
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].z * 255.0f),
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].y * 255.0f),
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].x * 255.0f),
                            static_cast<uint8_t>(modelVertex.colors[colorIndex].w * 255.0f)
                        );
                        vertex.setAttribute(elementIndex, colorData);
                    }
                    break;
                default:
                        break;
                }
            }
            return;
        }

        if (modelAttr == ModelVertexAttribute::BoneIds) {
            switch (format) {
            case Format::R32G32B32A32_SINT:
                vertex.setAttribute(elementIndex, IVec4(
                    modelVertex.boneIds[0], modelVertex.boneIds[1],
                    modelVertex.boneIds[2], modelVertex.boneIds[3]));
                break;
            case Format::R32G32B32A32_UINT:
                vertex.setAttribute(elementIndex, UVec4(
                    static_cast<uint32_t>(std::max(0, modelVertex.boneIds[0])),
                    static_cast<uint32_t>(std::max(0, modelVertex.boneIds[1])),
                    static_cast<uint32_t>(std::max(0, modelVertex.boneIds[2])),
                    static_cast<uint32_t>(std::max(0, modelVertex.boneIds[3]))));
                break;
            case Format::R32G32B32A32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec4(
                    static_cast<float>(modelVertex.boneIds[0]),
                    static_cast<float>(modelVertex.boneIds[1]),
                    static_cast<float>(modelVertex.boneIds[2]),
                    static_cast<float>(modelVertex.boneIds[3])));
                break;
            default:
                break;
            }
            return;
        }

        if (modelAttr == ModelVertexAttribute::BoneWeights) {
            switch (format) {
            case Format::R32G32B32A32_SFLOAT:
                vertex.setAttribute(elementIndex, Vec4(
                    modelVertex.boneWeights[0], modelVertex.boneWeights[1],
                    modelVertex.boneWeights[2], modelVertex.boneWeights[3]));
                break;
            default:
                break;
            }
            return;
        }
    }

    // 转换时，用一个map存ModelMaterial
    // 转换时，用一个map存ModelMesh

    class ModelLoader {
    public:
        virtual std::unique_ptr<ModelData> loadModel(const std::string &path) = 0;
        virtual ModelInfo::SceneInfo loadModelInfo(const std::string &path) = 0;
        virtual std::shared_ptr<ModelSkeletonData> loadSkeletonData(const std::string& path);
        /**
         * @cond CHINESE
         * @brief 解析模型依赖的纹理位置
         * @param modelPath 模型路径
         * @return 纹理绝对路径
         * @endcond
         */
        virtual std::set<std::string> resolveTexturePaths(const std::string& modelPath) const = 0;
        /**
         * @brief 获取内嵌纹理数据
         * @param modelPath 模型文件路径
         * @param textureIndex 内嵌纹理索引（例如"*0"中的0）
         * @param outData 输出的纹理数据容器
         * @return 是否成功获取纹理数据
         * @endcond
         */
        virtual bool getEmbeddedTextureData(const std::string& modelPath, int textureIndex, 
                                           std::vector<unsigned char>& outData) const = 0;
        /**
         * @cond CHINESE
         * @brief 解析模型路径，获取模型文件的所有依赖路径
         * @param modelPath 模型文件路径
         * @return 依赖路径集合
         * @endcond
         */
        std::set<std::string> resolveModePaths(const std::string& modelPath);
        std::set<std::string> getSupportedExtensions() const;
    protected:
        std::set<std::string> getModelSpecificDependencies(const std::string& modelPath) const;
        virtual std::set<std::string> getPlatformSupportedExtensions() const = 0;;
    };

}
#endif //MODELLOADER_H
