#include "./ModelLoader.h"
namespace FCT {
    namespace {
        template<typename KeyType>
        size_t findKeyIndex(const std::vector<KeyType>& keys, double animationTime)
        {
            if (keys.size() <= 2) {
                return 0;
            }
            auto it = std::upper_bound(
                keys.begin(),
                keys.end(),
                animationTime,
                [](double t, const KeyType& key) {
                    return t < key.time;
                });
            if (it == keys.begin()) {
                return 0;
            }
            size_t idx = static_cast<size_t>(std::distance(keys.begin(), it) - 1);
            if (idx + 1 >= keys.size()) {
                idx = keys.size() - 2;
            }
            return idx;
        }

        float computeKeyFactor(double startTime, double endTime, double animationTime)
        {
            double delta = endTime - startTime;
            if (delta <= 1e-8) {
                return 0.0f;
            }
            double factor = (animationTime - startTime) / delta;
            if (factor < 0.0) factor = 0.0;
            if (factor > 1.0) factor = 1.0;
            return static_cast<float>(factor);
        }

        Quat extractRotationFromOrthonormal3x3(
            float r00, float r01, float r02,
            float r10, float r11, float r12,
            float r20, float r21, float r22)
        {
            const float trace = r00 + r11 + r22;
            Quat q;
            if (trace > 0.0f) {
                const float s = std::sqrt(trace + 1.0f) * 2.0f;
                const float invS = 1.0f / s;
                q.w = 0.25f * s;
                q.x = (r21 - r12) * invS;
                q.y = (r02 - r20) * invS;
                q.z = (r10 - r01) * invS;
            } else if ((r00 > r11) && (r00 > r22)) {
                const float s = std::sqrt(1.0f + r00 - r11 - r22) * 2.0f;
                const float invS = 1.0f / s;
                q.w = (r21 - r12) * invS;
                q.x = 0.25f * s;
                q.y = (r01 + r10) * invS;
                q.z = (r02 + r20) * invS;
            } else if (r11 > r22) {
                const float s = std::sqrt(1.0f + r11 - r00 - r22) * 2.0f;
                const float invS = 1.0f / s;
                q.w = (r02 - r20) * invS;
                q.x = (r01 + r10) * invS;
                q.y = 0.25f * s;
                q.z = (r12 + r21) * invS;
            } else {
                const float s = std::sqrt(1.0f + r22 - r00 - r11) * 2.0f;
                const float invS = 1.0f / s;
                q.w = (r10 - r01) * invS;
                q.x = (r02 + r20) * invS;
                q.y = (r12 + r21) * invS;
                q.z = 0.25f * s;
            }
            return q.normalize();
        }

        void flattenSkeletonNodeRecursive(
            const SkeletonNode& node,
            int32_t parentIdx,
            std::vector<FlattenedSkeletonNode>& outNodes,
            std::unordered_map<std::string, int32_t>& outNameMap)
        {
            const int32_t currentIndex = static_cast<int32_t>(outNodes.size());
            FlattenedSkeletonNode flat;
            flat.name = node.name;
            flat.bindTransform = node.transformation;
            flat.bindTRS = SkeletonTRS::fromMatrix(node.transformation);
            flat.parentIndex = parentIdx;
            flat.boneIndex = static_cast<int32_t>(node.boneIndex);
            outNodes.push_back(std::move(flat));

            if (!node.name.empty() && outNameMap.find(node.name) == outNameMap.end()) {
                outNameMap.emplace(node.name, currentIndex);
            }

            for (const auto& child : node.children) {
                flattenSkeletonNodeRecursive(child, currentIndex, outNodes, outNameMap);
            }
        }

        void evaluateNodeHierarchyFallback(
            const SkeletonNode& node,
            const Mat4& parentTransform,
            const ModelSkeletonData& skeleton,
            const AnimationClip* clip,
            double animationTimeTicks,
            std::vector<Mat4>& outBoneMatrices,
            size_t maxBones)
        {
            Mat4 nodeTransform = node.transformation;
            if (clip) {
                if (const BoneAnimationChannel* channel = clip->findChannel(node.name)) {
                    nodeTransform = channel->evaluateLocalTransform(animationTimeTicks);
                }
            }

            Mat4 globalTransform = parentTransform * nodeTransform;
            if (node.boneIndex >= 0) {
                size_t idx = static_cast<size_t>(node.boneIndex);
                if (idx < maxBones && idx < skeleton.bones.size()) {
                    outBoneMatrices[idx] = skeleton.globalInverseTransform * globalTransform * skeleton.bones[idx].offsetMatrix;
                }
            }

            for (const auto& child : node.children) {
                evaluateNodeHierarchyFallback(child, globalTransform, skeleton, clip, animationTimeTicks, outBoneMatrices, maxBones);
            }
        }
    }

    Mat4 SkeletonTRS::toMatrix() const
    {
        return Mat4::Translate(translation.x, translation.y, translation.z) *
               rotation.toMatrix() *
               Mat4::Scale(scale.x, scale.y, scale.z);
    }

    SkeletonTRS SkeletonTRS::fromMatrix(const Mat4& mat)
    {
        SkeletonTRS trs;
        trs.translation = Vec3<>(mat.m[3], mat.m[7], mat.m[11]);

        Vec3<> col0(mat.m[0], mat.m[4], mat.m[8]);
        Vec3<> col1(mat.m[1], mat.m[5], mat.m[9]);
        Vec3<> col2(mat.m[2], mat.m[6], mat.m[10]);

        float sx = col0.length();
        float sy = col1.length();
        float sz = col2.length();

        if (col0.cross(col1).dot(col2) < 0.0f) {
            sx = -sx;
        }
        trs.scale = Vec3<>(
            std::abs(sx) > 1e-6f ? sx : 1.0f,
            std::abs(sy) > 1e-6f ? sy : 1.0f,
            std::abs(sz) > 1e-6f ? sz : 1.0f);

        const float invSx = 1.0f / trs.scale.x;
        const float invSy = 1.0f / trs.scale.y;
        const float invSz = 1.0f / trs.scale.z;

        trs.rotation = extractRotationFromOrthonormal3x3(
            mat.m[0] * invSx, mat.m[1] * invSy, mat.m[2] * invSz,
            mat.m[4] * invSx, mat.m[5] * invSy, mat.m[6] * invSz,
            mat.m[8] * invSx, mat.m[9] * invSy, mat.m[10] * invSz);
        return trs;
    }

    SkeletonTRS SkeletonTRS::blend(const SkeletonTRS& a, const SkeletonTRS& b, float weight)
    {
        const float t = weight < 0.0f ? 0.0f : (weight > 1.0f ? 1.0f : weight);
        if (t <= 1e-5f) {
            return a;
        }
        if (t >= 1.0f - 1e-5f) {
            return b;
        }
        SkeletonTRS out;
        out.translation = a.translation + (b.translation - a.translation) * t;
        out.rotation = Quat::Slerp(a.rotation, b.rotation, t);
        out.scale = a.scale + (b.scale - a.scale) * t;
        return out;
    }

    Vec3<> BoneAnimationChannel::interpolatePosition(double animationTime) const
    {
        if (positions.empty()) {
            return Vec3<>(0.0f, 0.0f, 0.0f);
        }
        if (positions.size() == 1) {
            return positions[0].value;
        }
        size_t idx = findKeyIndex(positions, animationTime);
        size_t nextIdx = idx + 1;
        float factor = computeKeyFactor(positions[idx].time, positions[nextIdx].time, animationTime);
        return positions[idx].value + (positions[nextIdx].value - positions[idx].value) * factor;
    }

    Quat BoneAnimationChannel::interpolateRotation(double animationTime) const
    {
        if (rotations.empty()) {
            return Quat(0.0f, 0.0f, 0.0f, 1.0f);
        }
        if (rotations.size() == 1) {
            return rotations[0].value.normalize();
        }
        size_t idx = findKeyIndex(rotations, animationTime);
        size_t nextIdx = idx + 1;
        float factor = computeKeyFactor(rotations[idx].time, rotations[nextIdx].time, animationTime);
        return Quat::Slerp(rotations[idx].value, rotations[nextIdx].value, factor);
    }

    Vec3<> BoneAnimationChannel::interpolateScale(double animationTime) const
    {
        if (scales.empty()) {
            return Vec3<>(1.0f, 1.0f, 1.0f);
        }
        if (scales.size() == 1) {
            return scales[0].value;
        }
        size_t idx = findKeyIndex(scales, animationTime);
        size_t nextIdx = idx + 1;
        float factor = computeKeyFactor(scales[idx].time, scales[nextIdx].time, animationTime);
        return scales[idx].value + (scales[nextIdx].value - scales[idx].value) * factor;
    }

    SkeletonTRS BoneAnimationChannel::evaluateLocalTRS(double animationTime, const SkeletonTRS& fallbackBind) const
    {
        SkeletonTRS trs;
        trs.translation = positions.empty() ? fallbackBind.translation : interpolatePosition(animationTime);
        trs.rotation = rotations.empty() ? fallbackBind.rotation : interpolateRotation(animationTime);
        trs.scale = scales.empty() ? fallbackBind.scale : interpolateScale(animationTime);
        return trs;
    }

    Mat4 BoneAnimationChannel::evaluateLocalTransform(double animationTime) const
    {
        Vec3<> pos = interpolatePosition(animationTime);
        Quat rot = interpolateRotation(animationTime);
        Vec3<> scl = interpolateScale(animationTime);
        return Mat4::Translate(pos.x, pos.y, pos.z) * rot.toMatrix() * Mat4::Scale(scl.x, scl.y, scl.z);
    }

    double AnimationClip::secondsToTicks(float timeInSeconds, bool loop) const
    {
        const double tps = ticksPerSecond > 0.0 ? ticksPerSecond : 25.0;
        double timeInTicks = static_cast<double>(timeInSeconds) * tps;
        if (duration > 0.0) {
            if (loop) {
                timeInTicks = std::fmod(timeInTicks, duration);
                if (timeInTicks < 0.0) {
                    timeInTicks += duration;
                }
            } else {
                if (timeInTicks < 0.0) timeInTicks = 0.0;
                if (timeInTicks > duration) timeInTicks = duration;
            }
        } else {
            timeInTicks = 0.0;
        }
        return timeInTicks;
    }

    void ModelSkeletonData::prepareRuntimeCache()
    {
        flatNodes.clear();
        flatNodeNameToIndex.clear();
        primaryMotionNodeIndex = -1;

        if (!rootNode.name.empty() || !rootNode.children.empty()) {
            flattenSkeletonNodeRecursive(rootNode, -1, flatNodes, flatNodeNameToIndex);
        }

        for (size_t i = 0; i < flatNodes.size(); ++i) {
            if (flatNodes[i].boneIndex >= 0) {
                primaryMotionNodeIndex = static_cast<int32_t>(i);
                break;
            }
        }

        for (auto& clip : animations) {
            if (clip.channelMap.empty() && !clip.channels.empty()) {
                for (size_t c = 0; c < clip.channels.size(); ++c) {
                    clip.channelMap[clip.channels[c].nodeName] = c;
                }
            }
            clip.flatNodeToChannel.assign(flatNodes.size(), -1);
            for (size_t nodeIdx = 0; nodeIdx < flatNodes.size(); ++nodeIdx) {
                auto it = clip.channelMap.find(flatNodes[nodeIdx].name);
                if (it != clip.channelMap.end() && it->second < clip.channels.size()) {
                    clip.flatNodeToChannel[nodeIdx] = static_cast<int32_t>(it->second);
                }
            }
        }

        runtimeCachePrepared = !flatNodes.empty();
    }

    int32_t ModelSkeletonData::findFlatNodeIndex(const std::string& nodeName) const
    {
        if (nodeName.empty()) {
            return -1;
        }
        auto it = flatNodeNameToIndex.find(nodeName);
        return it != flatNodeNameToIndex.end() ? it->second : -1;
    }

    std::vector<std::string> ModelSkeletonData::getAnimationNames() const
    {
        std::vector<std::string> names;
        names.reserve(animations.size());
        for (const auto& clip : animations) {
            names.push_back(clip.name);
        }
        return names;
    }

    std::vector<std::string> ModelSkeletonData::getNodeNames() const
    {
        std::vector<std::string> names;
        if (!flatNodes.empty()) {
            names.reserve(flatNodes.size());
            for (const auto& node : flatNodes) {
                if (!node.name.empty()) {
                    names.push_back(node.name);
                }
            }
            return names;
        }
        names.reserve(bones.size());
        for (const auto& bone : bones) {
            names.push_back(bone.name);
        }
        return names;
    }

    void ModelSkeletonData::evaluateBindPose(std::vector<Mat4>& outBoneMatrices, size_t maxBones) const
    {
        outBoneMatrices.assign(maxBones, Mat4());
        if (bones.empty()) {
            return;
        }
        if (runtimeCachePrepared && !flatNodes.empty()) {
            std::vector<Mat4> globalMatrices(flatNodes.size());
            for (size_t i = 0; i < flatNodes.size(); ++i) {
                const auto& node = flatNodes[i];
                globalMatrices[i] = (node.parentIndex >= 0)
                    ? (globalMatrices[static_cast<size_t>(node.parentIndex)] * node.bindTransform)
                    : node.bindTransform;
                if (node.boneIndex >= 0) {
                    const size_t bIdx = static_cast<size_t>(node.boneIndex);
                    if (bIdx < maxBones && bIdx < bones.size()) {
                        outBoneMatrices[bIdx] = globalInverseTransform * globalMatrices[i] * bones[bIdx].offsetMatrix;
                    }
                }
            }
            return;
        }
        evaluateNodeHierarchyFallback(rootNode, Mat4(), *this, nullptr, 0.0, outBoneMatrices, maxBones);
    }

    void ModelSkeletonData::evaluateAnimation(
        const std::string& clipName,
        float timeInSeconds,
        bool loop,
        std::vector<Mat4>& outBoneMatrices,
        size_t maxBones) const
    {
        SkeletalBlendParams params;
        params.primaryClipName = clipName;
        params.primaryTimeSec = timeInSeconds;
        params.primaryLoop = loop;
        evaluateBlendedPose(params, outBoneMatrices, nullptr, maxBones);
    }

    void ModelSkeletonData::evaluateBlendedPose(
        const SkeletalBlendParams& params,
        std::vector<Mat4>& outBoneMatrices,
        std::vector<Mat4>* outGlobalNodeMatrices,
        size_t maxBones) const
    {
        if (outBoneMatrices.size() != maxBones) {
            outBoneMatrices.resize(maxBones);
        }
        for (size_t i = 0; i < maxBones; ++i) {
            outBoneMatrices[i].identity();
        }
        if (bones.empty() && !outGlobalNodeMatrices) {
            return;
        }

        const AnimationClip* primaryClip = findAnimation(params.primaryClipName);
        if (!runtimeCachePrepared || flatNodes.empty()) {
            if (!primaryClip) {
                evaluateBindPose(outBoneMatrices, maxBones);
                return;
            }
            const double ticks = primaryClip->secondsToTicks(params.primaryTimeSec, params.primaryLoop);
            evaluateNodeHierarchyFallback(rootNode, Mat4(), *this, primaryClip, ticks, outBoneMatrices, maxBones);
            return;
        }

        const size_t nodeCount = flatNodes.size();
        std::vector<Mat4> localGlobalStorage;
        std::vector<Mat4>& globalMatrices = outGlobalNodeMatrices ? *outGlobalNodeMatrices : localGlobalStorage;
        if (globalMatrices.size() != nodeCount) {
            globalMatrices.resize(nodeCount);
        }

        const double primaryTicks = primaryClip
            ? primaryClip->secondsToTicks(params.primaryTimeSec, params.primaryLoop)
            : 0.0;

        const float clampedCrossfade = params.blendWeight < 0.0f
            ? 0.0f
            : (params.blendWeight > 1.0f ? 1.0f : params.blendWeight);
        const AnimationClip* targetClip = (clampedCrossfade > 1e-4f && !params.targetClipName.empty())
            ? findAnimationExact(params.targetClipName)
            : nullptr;
        const double targetTicks = targetClip
            ? targetClip->secondsToTicks(params.targetTimeSec, params.targetLoop)
            : 0.0;

        const float clampedLayerWeight = params.layerWeight < 0.0f
            ? 0.0f
            : (params.layerWeight > 1.0f ? 1.0f : params.layerWeight);
        const AnimationClip* layerClip = (clampedLayerWeight > 1e-4f && !params.layerClipName.empty())
            ? findAnimationExact(params.layerClipName)
            : nullptr;
        const double layerTicks = layerClip
            ? layerClip->secondsToTicks(params.layerTimeSec, params.layerLoop)
            : 0.0;
        const int32_t maskRootIdx = (layerClip && !params.layerMaskRootBone.empty())
            ? findFlatNodeIndex(params.layerMaskRootBone)
            : -1;

        std::vector<uint8_t> layerMask;
        if (layerClip) {
            layerMask.assign(nodeCount, maskRootIdx < 0 ? 1u : 0u);
            if (maskRootIdx >= 0) {
                for (size_t i = 0; i < nodeCount; ++i) {
                    const int32_t parentIdx = flatNodes[i].parentIndex;
                    if (static_cast<int32_t>(i) == maskRootIdx ||
                        (parentIdx >= 0 && layerMask[static_cast<size_t>(parentIdx)] != 0u)) {
                        layerMask[i] = 1u;
                    }
                }
            }
        }

        for (size_t i = 0; i < nodeCount; ++i) {
            const auto& node = flatNodes[i];
            const BoneAnimationChannel* primaryCh = primaryClip ? primaryClip->findChannelByFlatNode(i) : nullptr;
            const BoneAnimationChannel* targetCh = targetClip ? targetClip->findChannelByFlatNode(i) : nullptr;
            const bool applyLayer = layerClip && layerMask[i] != 0u;
            const BoneAnimationChannel* layerCh = applyLayer ? layerClip->findChannelByFlatNode(i) : nullptr;

            Mat4 localMat;
            if (!primaryCh && !targetCh && !layerCh) {
                localMat = node.bindTransform;
            } else {
                SkeletonTRS localTRS = primaryCh
                    ? primaryCh->evaluateLocalTRS(primaryTicks, node.bindTRS)
                    : node.bindTRS;

                if (targetClip) {
                    const SkeletonTRS targetTRS = targetCh
                        ? targetCh->evaluateLocalTRS(targetTicks, node.bindTRS)
                        : node.bindTRS;
                    localTRS = SkeletonTRS::blend(localTRS, targetTRS, clampedCrossfade);
                }

                if (applyLayer) {
                    const SkeletonTRS layerTRS = layerCh
                        ? layerCh->evaluateLocalTRS(layerTicks, node.bindTRS)
                        : node.bindTRS;
                    localTRS = SkeletonTRS::blend(localTRS, layerTRS, clampedLayerWeight);
                }

                if (params.lockRootPosition && static_cast<int32_t>(i) == primaryMotionNodeIndex) {
                    localTRS.translation.x = node.bindTRS.translation.x;
                    localTRS.translation.z = node.bindTRS.translation.z;
                }

                localMat = localTRS.toMatrix();
            }

            globalMatrices[i] = (node.parentIndex >= 0)
                ? (globalMatrices[static_cast<size_t>(node.parentIndex)] * localMat)
                : localMat;

            if (node.boneIndex >= 0) {
                const size_t bIdx = static_cast<size_t>(node.boneIndex);
                if (bIdx < maxBones && bIdx < bones.size()) {
                    outBoneMatrices[bIdx] = globalInverseTransform * globalMatrices[i] * bones[bIdx].offsetMatrix;
                }
            }
        }
    }

    std::shared_ptr<ModelSkeletonData> ModelLoader::loadSkeletonData(const std::string& path)
    {
        auto modelData = loadModel(path);
        if (!modelData) {
            return nullptr;
        }
        modelData->skeletonData.prepareRuntimeCache();
        return std::make_shared<ModelSkeletonData>(std::move(modelData->skeletonData));
    }

    std::set<std::string> ModelLoader::resolveModePaths(const std::string& modelPath)
    {
        std::set<std::string> allPaths;

        std::string absoluteModelPath;
        try {
            if (std::filesystem::exists(modelPath)) {
                absoluteModelPath = std::filesystem::canonical(modelPath).string();
            } else {
                absoluteModelPath = std::filesystem::absolute(modelPath).string();
            }
            allPaths.insert(absoluteModelPath);
        } catch (const std::filesystem::filesystem_error&) {
            allPaths.insert(modelPath);
            absoluteModelPath = modelPath;
        }

        std::filesystem::path modelFilePath(absoluteModelPath);
        std::string extension = modelFilePath.extension().string();

        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        std::set<std::string> texturePaths = resolveTexturePaths(absoluteModelPath);
        allPaths.insert(texturePaths.begin(), texturePaths.end());

        std::set<std::string> specificDependencies = getModelSpecificDependencies(absoluteModelPath);

        std::filesystem::path modelDir = std::filesystem::path(absoluteModelPath).parent_path();
        for (const auto& relativePath : specificDependencies) {
            try {
                std::string absolutePath = std::filesystem::canonical(modelDir / relativePath).string();
                allPaths.insert(absolutePath);
            } catch (const std::filesystem::filesystem_error&) {
                std::string absolutePath = std::filesystem::absolute(modelDir / relativePath).string();
                allPaths.insert(absolutePath);
            }
        }

        return allPaths;
    }

    std::set<std::string> ModelLoader::getModelSpecificDependencies(const std::string& modelPath) const
    {
        std::set<std::string> dependencies;

        std::filesystem::path modelFilePath(modelPath);
        std::string extension = modelFilePath.extension().string();
        std::string baseName = modelFilePath.stem().string();

        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        if (extension == ".obj") {
            dependencies.insert(baseName + ".mtl");
        }
        else if (extension == ".gltf")
        {
            dependencies.insert(baseName + ".bin");
        }
        else if (extension == ".fbx")
        {

        } else if (extension == ".glb")
        {

        } else if (extension == ".usdz")
        {

        }
        return dependencies;
    }

    std::set<std::string> ModelLoader::getSupportedExtensions() const
    {
        auto platform = getPlatformSupportedExtensions();
        std::set<std::string> supportedExtensions = {
            ".obj",
            ".gltf",
            ".fbx",
            ".glb",
            ".usdz"
        };
        return supportedExtensions;
    }
}
