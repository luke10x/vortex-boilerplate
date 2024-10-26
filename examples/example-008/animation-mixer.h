#pragma once

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <map>
#include <vector>

#define MAX_BONES (200)

inline glm::mat4 assimpToGlmMatrix(aiMatrix4x4 mat)
{
    glm::mat4 m;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            m[x][y] = mat[y][x];
        }
    }
    return m;
}

struct AnimationMixer {
    const aiScene* m_scene;
    glm::mat4 m_globalInverseTransform;
    std::map<std::string, uint> m_boneNameToIndex;
    std::vector<glm::mat4> m_boneTransforms;

    void initBones(const aiScene* scene, const aiMesh* paiMesh);

    std::vector<glm::mat4> hydrateBoneTransforms(
        std::vector<glm::mat4>& Transforms,
        float currentSecond,
        unsigned int animationIndex0
    );

    aiVector3D calcInterpolatedPosition(
        float currentTick,
        const aiNodeAnim* pNodeAnim
    );
    uint findPosition(float currentTick, const aiNodeAnim* pNodeAnim);

    aiQuaternion calcInterpolatedRotation(
        float currentTick,
        const aiNodeAnim* pNodeAnim
    );
    uint findRotation(float currentTick, const aiNodeAnim* pNodeAnim);

    aiVector3D calcInterpolatedScaling(
        float currentTick,
        const aiNodeAnim* pNodeAnim
    );
    uint findScaling(float currentTick, const aiNodeAnim* pNodeAnim);

    const aiNodeAnim* FindNodeAnim(
        const aiAnimation& Animation,
        const std::string& NodeName
    );

    void applyBoneTransformsFromNodeTree(
        float AnimationTime,
        const aiNode* pNode,
        const glm::mat4& ParentTransform,
        const aiAnimation& Animation,
        std::vector<glm::mat4>& Transforms
    );
    float CalcAnimationTimeTicks(
        float TimeInSeconds,
        unsigned int animationIndex0
    );
};

void AnimationMixer::initBones(
    const aiScene* scene,
    const aiMesh* pMesh
)
{
    m_scene = scene;

    if (pMesh->mNumBones > MAX_BONES) {
        std::cerr << "This model has too many bones "
                  << pMesh->mNumBones << std::endl;
        assert(0);
    }

    for (uint i = 0; i < pMesh->mNumBones; i++) {
        auto pBone = pMesh->mBones[i];
        std::string boneName(pBone->mName.C_Str());

        m_boneNameToIndex[boneName] = i;

        m_boneTransforms.push_back(
            assimpToGlmMatrix(pBone->mOffsetMatrix)
        );
    }
}

aiVector3D AnimationMixer::calcInterpolatedPosition(
    float currentTick1,
    const aiNodeAnim* pNodeAnim
)
{
    aiVector3D Out;
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return Out;
    }

    uint PositionIndex     = findPosition(currentTick1, pNodeAnim);
    uint NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float) pNodeAnim->mPositionKeys[PositionIndex].mTime;
    if (t1 > currentTick1) {
        Out = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    } else {
        float t2 =
            (float) pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor    = (currentTick1 - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start =
            pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End =
            pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out              = Start + Factor * Delta;
    }
    return Out;
}

uint AnimationMixer::findPosition(
    float currentTick1,
    const aiNodeAnim* pNodeAnim
)
{
    for (uint i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
        float t = (float) pNodeAnim->mPositionKeys[i + 1].mTime;
        if (currentTick1 < t) {
            return i;
        }
    }
    return 0;
}

aiQuaternion AnimationMixer::calcInterpolatedRotation(
    float currentTick1,
    const aiNodeAnim* pNodeAnim
)
{
    aiQuaternion Out;
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return Out;
    }

    uint RotationIndex     = findRotation(currentTick1, pNodeAnim);
    uint NextRotationIndex = RotationIndex + 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float) pNodeAnim->mRotationKeys[RotationIndex].mTime;
    if (t1 > currentTick1) {
        Out = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    } else {
        float t2 =
            (float) pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor    = (currentTick1 - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiQuaternion& StartRotationQ =
            pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ =
            pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(
            Out, StartRotationQ, EndRotationQ, Factor
        );
    }

    Out.Normalize();
    return Out;
}

uint AnimationMixer::findRotation(
    float currentTick1,
    const aiNodeAnim* pNodeAnim
)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
        float t = (float) pNodeAnim->mRotationKeys[i + 1].mTime;
        if (currentTick1 < t) {
            return i;
        }
    }

    return 0;
}

aiVector3D AnimationMixer::calcInterpolatedScaling(
    float currentTick1,
    const aiNodeAnim* pNodeAnim
)
{
    aiVector3D Out;
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return Out;
    }

    uint ScalingIndex     = findScaling(currentTick1, pNodeAnim);
    uint NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float) pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    if (t1 > currentTick1) {
        Out = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    } else {
        float t2 =
            (float) pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor    = (currentTick1 - (float) t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start =
            pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End =
            pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out              = Start + Factor * Delta;
    }
    return Out;
}

uint AnimationMixer::findScaling(
    float currentTick1,
    const aiNodeAnim* pNodeAnim
)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        float t = (float) pNodeAnim->mScalingKeys[i + 1].mTime;
        if (currentTick1 < t) {
            return i;
        }
    }

    return 0;
}

void AnimationMixer::applyBoneTransformsFromNodeTree(
    float currentTick1,
    const aiNode* pNode,
    const glm::mat4& ParentTransform,
    const aiAnimation& Animation,
    std::vector<glm::mat4>& Transforms
)
{
    std::string NodeName(pNode->mName.data);

    glm::mat4 NodeTransformation(
        assimpToGlmMatrix(pNode->mTransformation)
    );

    const aiNodeAnim* pNodeAnim = FindNodeAnim(Animation, NodeName);
    if (pNodeAnim) {
        // Get TRS components from animation
        aiVector3D aiPosition =
            calcInterpolatedPosition(currentTick1, pNodeAnim);
        glm::vec3 position(aiPosition.x, aiPosition.y, aiPosition.z);

        aiQuaternion aiRotation =
            calcInterpolatedRotation(currentTick1, pNodeAnim);
        glm::quat rotation(
            aiRotation.w, aiRotation.x, aiRotation.y, aiRotation.z
        );

        aiVector3D aiScaling =
            calcInterpolatedScaling(currentTick1, pNodeAnim);
        glm::vec3 scale(aiScaling.x, aiScaling.y, aiScaling.z);

        // Inflate them into matrices
        glm::mat4 positionMat = glm::mat4(1.0f);
        positionMat           = glm::translate(positionMat, position);
        glm::mat4 rotationMat = glm::toMat4(rotation);
        glm::mat4 scaleMat    = glm::mat4(1.0f);
        scaleMat              = glm::scale(scaleMat, scale);

        // Multiply TRS matrices
        NodeTransformation = positionMat * rotationMat * scaleMat;
    }

    glm::mat4 GlobalTransformation =
        ParentTransform * NodeTransformation;

    auto isBoneNode =
        m_boneNameToIndex.find(NodeName) != m_boneNameToIndex.end();
    if (isBoneNode) {
        uint BoneIndex = m_boneNameToIndex[NodeName];

        Transforms[BoneIndex] = glm::transpose(
            m_globalInverseTransform * GlobalTransformation *
            m_boneTransforms[BoneIndex]
        );
    } else {
        // Because there are some nodes at the root of the mesh,
        // that are not bones, but they have some transformations
        // Therefore, here I apply them to the globalTransformation
        GlobalTransformation =
            GlobalTransformation * NodeTransformation;
    }

    for (uint i = 0; i < pNode->mNumChildren; i++) {
        // Go deeper into recursion
        applyBoneTransformsFromNodeTree(
            currentTick1, pNode->mChildren[i], GlobalTransformation,
            Animation, Transforms
        );
    }
}

std::vector<glm::mat4> AnimationMixer::hydrateBoneTransforms(
    std::vector<glm::mat4>& Transforms,
    float currentSecond,
    unsigned int animationIndex0
)
{
    m_globalInverseTransform = glm::inverse(
        assimpToGlmMatrix(m_scene->mRootNode->mTransformation)
    );

    if (animationIndex0 >= m_scene->mNumAnimations) {
        printf(
            "Invalid animation index %d, max is %d\n", animationIndex0,
            m_scene->mNumAnimations
        );
        assert(0);
    }

    float currentTick1 =
        CalcAnimationTimeTicks(currentSecond, animationIndex0);
    const aiAnimation& Animation =
        *m_scene->mAnimations[animationIndex0];

    Transforms.resize(m_scene->mMeshes[0]->mNumBones);

    // Recurse starts here
    glm::mat4 rootParentTransform(1.0f);
    applyBoneTransformsFromNodeTree(
        currentTick1, m_scene->mRootNode, rootParentTransform,
        Animation, Transforms
    );

    std::vector<glm::mat4> pose;
    return pose;
}

float AnimationMixer::CalcAnimationTimeTicks(
    float currentSecond,
    unsigned int animationIndex0
)
{
    float TicksPerSecond =
        m_scene->mAnimations[animationIndex0]->mTicksPerSecond == 0
            ? 30.0f
            : (float) m_scene->mAnimations[animationIndex0]
                  ->mTicksPerSecond;
    float TimeInTicks = currentSecond * TicksPerSecond;

    float Duration = 0.0f;
    float fraction = modf(
        (float) m_scene->mAnimations[animationIndex0]->mDuration,
        &Duration
    );
    float currentTick1 = fmod(TimeInTicks, Duration);
    return currentTick1;
}

const aiNodeAnim* AnimationMixer::FindNodeAnim(
    const aiAnimation& animation,
    const std::string& nodeName
)
{
    for (uint i = 0; i < animation.mNumChannels; i++) {
        const aiNodeAnim* pNodeAnim = animation.mChannels[i];

        if (std::string(pNodeAnim->mNodeName.data) == nodeName) {
            return pNodeAnim;
        }
    }

    return nullptr;
}