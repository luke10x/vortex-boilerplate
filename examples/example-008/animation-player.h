#pragma once

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
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

struct AnimationPlayer {
    const aiScene* m_pScene;
    glm::mat4 m_GlobalInverseTransform;
    std::map<std::string, uint> m_BoneNameToIndexMap;
    std::vector<glm::mat4> m_BoneInfo;

    void LoadMeshBones(uint MeshIndex, const aiMesh* paiMesh);
    std::vector<glm::mat4> GetBoneTransforms(
        float AnimationTimeSec,
        std::vector<glm::mat4>& Transforms,
        unsigned int AnimationIndex = 0
    );

    int GetBoneId(const aiBone* pBone);
    aiVector3D CalcInterpolatedScaling(
        float AnimationTime,
        const aiNodeAnim* pNodeAnim
    );
    aiQuaternion CalcInterpolatedRotation(
        float AnimationTime,
        const aiNodeAnim* pNodeAnim
    );
    aiVector3D CalcInterpolatedPosition(
        float AnimationTime,
        const aiNodeAnim* pNodeAnim
    );
    uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(
        const aiAnimation& Animation,
        const std::string& NodeName
    );
    void ReadNodeHierarchy(
        float AnimationTime,
        const aiNode* pNode,
        const glm::mat4& ParentTransform,
        const aiAnimation& Animation,
        std::vector<glm::mat4>& Transforms
    );
    float CalcAnimationTimeTicks(
        float TimeInSeconds,
        unsigned int AnimationIndex
    );
};

void AnimationPlayer::LoadMeshBones(uint MeshIndex, const aiMesh* pMesh)
{
    if (pMesh->mNumBones > MAX_BONES) {
        printf("This model has too many bones %d \n", pMesh->mNumBones);
        assert(0);
    }

    for (uint i = 0; i < pMesh->mNumBones; i++) {
        auto pBone = pMesh->mBones[i];
        int BoneId = GetBoneId(pBone);

        if (BoneId == m_BoneInfo.size()) {
            m_BoneInfo.push_back(assimpToGlmMatrix(pBone->mOffsetMatrix)
            );
        }
    }
}

int AnimationPlayer::GetBoneId(const aiBone* pBone)
{
    int BoneIndex = 0;
    std::string BoneName(pBone->mName.C_Str());

    if (m_BoneNameToIndexMap.find(BoneName) ==
        m_BoneNameToIndexMap.end()) {
        // Allocate an index for a new bone
        BoneIndex = (int) m_BoneNameToIndexMap.size();
        m_BoneNameToIndexMap[BoneName] = BoneIndex;
    } else {
        BoneIndex = m_BoneNameToIndexMap[BoneName];
    }

    return BoneIndex;
}

uint AnimationPlayer::FindPosition(
    float AnimationTimeTicks,
    const aiNodeAnim* pNodeAnim
)
{
    for (uint i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
        float t = (float) pNodeAnim->mPositionKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }
    return 0;
}

aiVector3D AnimationPlayer::CalcInterpolatedPosition(
    float AnimationTimeTicks,
    const aiNodeAnim* pNodeAnim
)
{
    aiVector3D Out;
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return Out;
    }

    uint PositionIndex = FindPosition(AnimationTimeTicks, pNodeAnim);
    uint NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float) pNodeAnim->mPositionKeys[PositionIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    } else {
        float t2 =
            (float) pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor    = (AnimationTimeTicks - t1) / DeltaTime;
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

uint AnimationPlayer::FindRotation(
    float AnimationTimeTicks,
    const aiNodeAnim* pNodeAnim
)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
        float t = (float) pNodeAnim->mRotationKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}

aiQuaternion AnimationPlayer::CalcInterpolatedRotation(
    float AnimationTimeTicks,
    const aiNodeAnim* pNodeAnim
)
{
    aiQuaternion Out;
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return Out;
    }

    uint RotationIndex = FindRotation(AnimationTimeTicks, pNodeAnim);
    uint NextRotationIndex = RotationIndex + 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float) pNodeAnim->mRotationKeys[RotationIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    } else {
        float t2 =
            (float) pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor    = (AnimationTimeTicks - t1) / DeltaTime;
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

uint AnimationPlayer::FindScaling(
    float AnimationTimeTicks,
    const aiNodeAnim* pNodeAnim
)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        float t = (float) pNodeAnim->mScalingKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}

aiVector3D AnimationPlayer::CalcInterpolatedScaling(
    float AnimationTimeTicks,
    const aiNodeAnim* pNodeAnim
)
{
    aiVector3D Out;
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return Out;
    }

    uint ScalingIndex     = FindScaling(AnimationTimeTicks, pNodeAnim);
    uint NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float) pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    } else {
        float t2 =
            (float) pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor    = (AnimationTimeTicks - (float) t1) / DeltaTime;
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

void AnimationPlayer::ReadNodeHierarchy(
    float AnimationTimeTicks,
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
            CalcInterpolatedPosition(AnimationTimeTicks, pNodeAnim);
        glm::vec3 position(aiPosition.x, aiPosition.y, aiPosition.z);

        aiQuaternion aiRotation =
            CalcInterpolatedRotation(AnimationTimeTicks, pNodeAnim);
        glm::quat rotation(
            aiRotation.w, aiRotation.x, aiRotation.y, aiRotation.z
        );

        aiVector3D aiScaling =
            CalcInterpolatedScaling(AnimationTimeTicks, pNodeAnim);
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

    if (m_BoneNameToIndexMap.find(NodeName) !=
        m_BoneNameToIndexMap.end()) {
        uint BoneIndex = m_BoneNameToIndexMap[NodeName];

        Transforms[BoneIndex] = glm::transpose(
            m_GlobalInverseTransform * GlobalTransformation *
            m_BoneInfo[BoneIndex]
        );
    }

    for (uint i = 0; i < pNode->mNumChildren; i++) {

        // Go deeper into recursion
        ReadNodeHierarchy(
            AnimationTimeTicks, pNode->mChildren[i],
            GlobalTransformation, Animation, Transforms
        );
    }
}

std::vector<glm::mat4> AnimationPlayer::GetBoneTransforms(
    float currentSecond,
    std::vector<glm::mat4>& Transforms,
    unsigned int AnimationIndex
)
{
    m_GlobalInverseTransform = glm::inverse(
        assimpToGlmMatrix(m_pScene->mRootNode->mTransformation)
    );

    if (AnimationIndex >= m_pScene->mNumAnimations) {
        printf(
            "Invalid animation index %d, max is %d\n", AnimationIndex,
            m_pScene->mNumAnimations
        );
        assert(0);
    }

    float AnimationTimeTicks =
        CalcAnimationTimeTicks(currentSecond, AnimationIndex);
    const aiAnimation& Animation =
        *m_pScene->mAnimations[AnimationIndex];

    Transforms.resize(m_pScene->mMeshes[0]->mNumBones);

    // Recurse starts here
    glm::mat4 Identity(1.0f);
    ReadNodeHierarchy(
        AnimationTimeTicks, m_pScene->mRootNode, Identity, Animation,
        Transforms
    );

    std::vector<glm::mat4> pose;
    return pose;
}

float AnimationPlayer::CalcAnimationTimeTicks(
    float currentSecond,
    unsigned int AnimationIndex
)
{
    float TicksPerSecond =
        m_pScene->mAnimations[AnimationIndex]->mTicksPerSecond == 0
            ? 30.0f
            : (float) m_pScene->mAnimations[AnimationIndex]
                  ->mTicksPerSecond;
    float TimeInTicks = currentSecond * TicksPerSecond;

    float Duration = 0.0f;
    float fraction = modf(
        (float) m_pScene->mAnimations[AnimationIndex]->mDuration,
        &Duration
    );
    float AnimationTimeTicks = fmod(TimeInTicks, Duration);
    return AnimationTimeTicks;
}

const aiNodeAnim* AnimationPlayer::FindNodeAnim(
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