#pragma once

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <map>
#include <vector>

#include "imgui.h"

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
    const aiScene* scene;
    const aiMesh* mesh;
    glm::mat4 globalInverseTransform;
    std::map<std::string, uint> boneNameToIndex;
    std::vector<glm::mat4> boneOffsets;

    void initBones(const aiScene* scene, const aiMesh* mesh);

    std::vector<glm::mat4> hydrateBoneTransforms(
        std::vector<glm::mat4>& Transforms,
        float currentSecond,
        unsigned int animationIndex0,
        float ticksPerSecond0,
        unsigned int animationIndex1,
        float ticksPerSecond1,
        float blendingFactor
    );

    aiVector3D calcInterpolatedPosition(
        float currentTick,
        const aiNodeAnim* pNodeAnim
    );

    aiQuaternion calcInterpolatedRotation(
        float currentTick,
        const aiNodeAnim* pNodeAnim
    );

    aiVector3D calcInterpolatedScaling(
        float currentTick,
        const aiNodeAnim* pNodeAnim
    );

    const aiNodeAnim* findChannel(
        const aiAnimation& Animation,
        const std::string& NodeName
    );

    void applyBoneTransformsFromNodeTree(
        const aiAnimation& Animation0,
        float currentTick0,
        const aiAnimation& Animation1,
        float currentTick1,
        float blendingFactor,
        const aiNode* pNode,
        const glm::mat4& parentTransform,
        std::vector<glm::mat4>& Transforms
    );

    float calcAnimationTick(
        float timeInSeconds,
        float ticksPerSecond,
        unsigned int animationIndex0
    );
};

struct AnimationMixerControls {
    AnimationMixer am;
    int selectedAnimation0 = 0;
    int selectedAnimation1 = 0;
    float ticksPerSecond0;
    float ticksPerSecond1;
    float blendingFactor;

    void initAnimationMixerControls(const AnimationMixer& am)
    {
        this->am = am;

        this->ticksPerSecond0 =
            this->am.scene->mAnimations[this->selectedAnimation0]
                        ->mTicksPerSecond == 0
                ? 30.0f
                : (float) this->am.scene
                      ->mAnimations[this->selectedAnimation0]
                      ->mTicksPerSecond;
        this->ticksPerSecond1 =
            this->am.scene->mAnimations[this->selectedAnimation1]
                        ->mTicksPerSecond == 0
                ? 30.0f
                : (float) this->am.scene
                      ->mAnimations[this->selectedAnimation1]
                      ->mTicksPerSecond;
    }

    void
    renderAnimationControls(AnimationMixer& am, float currentSecond);
};

void AnimationMixer::initBones(const aiScene* scene, const aiMesh* mesh)
{
    // scene and mesh will be stored forever
    this->scene = scene;
    this->mesh  = mesh;

    if (this->mesh->mNumBones > MAX_BONES) {
        std::cerr << "This model has too many bones "
                  << this->mesh->mNumBones << std::endl;
        assert(0);
    }

    // For each bone
    for (uint i = 0; i < this->mesh->mNumBones; i++) {
        auto pBone = this->mesh->mBones[i];
        std::string boneName(pBone->mName.C_Str());

        // Add bone to index mapping
        this->boneNameToIndex[boneName] = i;

        // reads binding pose offsets
        this->boneOffsets.push_back(
            assimpToGlmMatrix(pBone->mOffsetMatrix)
        );
    }
}

std::vector<glm::mat4> AnimationMixer::hydrateBoneTransforms(
    std::vector<glm::mat4>& Transforms,
    float currentSecond,
    unsigned int animationIndex0,
    float ticksPerSecond0,
    unsigned int animationIndex1,
    float ticksPerSecond1,
    float blendingFactor
)
{
    this->globalInverseTransform = glm::inverse(
        assimpToGlmMatrix(this->scene->mRootNode->mTransformation)
    );

    if (animationIndex0 >= this->scene->mNumAnimations) {
        printf(
            "Invalid animation index %d, max is %d\n", animationIndex0,
            this->scene->mNumAnimations
        );
        assert(0);
    }

    float currentTick0 = calcAnimationTick(
        currentSecond, ticksPerSecond0, animationIndex0
    );
    float currentTick1 = calcAnimationTick(
        currentSecond, ticksPerSecond1, animationIndex1
    );

    const aiAnimation& animation0 =
        *this->scene->mAnimations[animationIndex0];
    const aiAnimation& animation1 =
        *this->scene->mAnimations[animationIndex1];

    Transforms.resize(this->mesh->mNumBones);

    // Recurse starts here
    glm::mat4 rootParentTransform(1.0f);
    applyBoneTransformsFromNodeTree(
        animation0, currentTick0, animation1, currentTick1,
        blendingFactor, this->scene->mRootNode, rootParentTransform,
        Transforms
    );

    std::vector<glm::mat4> pose;
    return pose;
}

aiVector3D AnimationMixer::calcInterpolatedPosition(
    float currentTick,
    const aiNodeAnim* pNodeAnim
)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        return pNodeAnim->mPositionKeys[0].mValue;
    }

    uint positionIndex = 0;
    for (uint i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
        float t = (float) pNodeAnim->mPositionKeys[i + 1].mTime;
        if (currentTick < t) {
            positionIndex = i;
            break;
        }
    }

    uint nextPositionIndex = positionIndex + 1;
    assert(nextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float) pNodeAnim->mPositionKeys[positionIndex].mTime;
    if (t1 > currentTick) {
        return pNodeAnim->mPositionKeys[positionIndex].mValue;
    }

    float t2 =
        (float) pNodeAnim->mPositionKeys[nextPositionIndex].mTime;
    float deltaTime = t2 - t1;
    float factor    = (currentTick - t1) / deltaTime;
    assert(factor >= 0.0f && factor <= 1.0f);
    const aiVector3D& start =
        pNodeAnim->mPositionKeys[positionIndex].mValue;
    const aiVector3D& end =
        pNodeAnim->mPositionKeys[nextPositionIndex].mValue;
    return start + factor * (end - start);
}

aiQuaternion AnimationMixer::calcInterpolatedRotation(
    float currentTick,
    const aiNodeAnim* pNodeAnim
)
{
    if (pNodeAnim->mNumRotationKeys == 1) {
        return pNodeAnim->mRotationKeys[0].mValue;
    }

    uint rotationIndex;
    assert(pNodeAnim->mNumRotationKeys > 0);
    for (uint i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
        float t = (float) pNodeAnim->mRotationKeys[i + 1].mTime;
        if (currentTick < t) {
            rotationIndex = i;
            break;
        }
    }

    uint nextRotationIndex = rotationIndex + 1;
    assert(nextRotationIndex < pNodeAnim->mNumRotationKeys);
    aiQuaternion quat;
    float t1 = (float) pNodeAnim->mRotationKeys[rotationIndex].mTime;
    if (t1 > currentTick) {
        quat = pNodeAnim->mRotationKeys[rotationIndex].mValue;
    } else {
        float t2 =
            (float) pNodeAnim->mRotationKeys[nextRotationIndex].mTime;
        float deltaTime = t2 - t1;
        float factor    = (currentTick - t1) / deltaTime;
        assert(factor >= 0.0f && factor <= 1.0f);
        const aiQuaternion& startRotationQ =
            pNodeAnim->mRotationKeys[rotationIndex].mValue;
        const aiQuaternion& endRotationQ =
            pNodeAnim->mRotationKeys[nextRotationIndex].mValue;
        aiQuaternion::Interpolate(
            quat, startRotationQ, endRotationQ, factor
        );
    }

    quat.Normalize();
    return quat;
}

aiVector3D AnimationMixer::calcInterpolatedScaling(
    float currentTick,
    const aiNodeAnim* pNodeAnim
)
{
    aiVector3D Out;
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        return pNodeAnim->mScalingKeys[0].mValue;
    }

    uint scalingIndex;
    assert(pNodeAnim->mNumScalingKeys > 0);
    for (uint i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        float t = (float) pNodeAnim->mScalingKeys[i + 1].mTime;
        if (currentTick < t) {
            scalingIndex = i;
            break;
        }
    }

    uint nextScalingIndex = scalingIndex + 1;
    assert(nextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float) pNodeAnim->mScalingKeys[scalingIndex].mTime;
    if (t1 > currentTick) {
        return pNodeAnim->mScalingKeys[scalingIndex].mValue;
    }
    float t2 = (float) pNodeAnim->mScalingKeys[nextScalingIndex].mTime;
    float deltaTime = t2 - t1;
    float factor    = (currentTick - (float) t1) / deltaTime;
    assert(factor >= 0.0f && factor <= 1.0f);
    const aiVector3D& start =
        pNodeAnim->mScalingKeys[scalingIndex].mValue;
    const aiVector3D& end =
        pNodeAnim->mScalingKeys[nextScalingIndex].mValue;
    return start + factor * (end - start);
}

void AnimationMixer::applyBoneTransformsFromNodeTree(
    const aiAnimation& animation0,
    float currentTick0,
    const aiAnimation& animation1,
    float currentTick1,
    float blendingFactor,
    const aiNode* pNode,
    const glm::mat4& parentTransform,
    std::vector<glm::mat4>& resultsBuffer
)
{
    std::string nodeName(pNode->mName.data);

    glm::mat4 nodeTransform(
        assimpToGlmMatrix(pNode->mTransformation)
    );

    const aiNodeAnim* channel0 = findChannel(animation0, nodeName);
    const aiNodeAnim* channel1 = findChannel(animation1, nodeName);
    if (channel0 && channel1) {
        // Get TRS components from animation
        aiVector3D aiPosition0 =
            calcInterpolatedPosition(currentTick0, channel0);
        aiVector3D aiPosition1 =
            calcInterpolatedPosition(currentTick1, channel1);
        aiVector3D blendedPosition =
            (1.0f - blendingFactor) * aiPosition0 +
            aiPosition1 * blendingFactor;
        glm::vec3 position(
            blendedPosition.x, blendedPosition.y, blendedPosition.z
        );

        aiQuaternion aiRotation0 =
            calcInterpolatedRotation(currentTick0, channel0);
        aiQuaternion aiRotation1 =
            calcInterpolatedRotation(currentTick1, channel1);
        aiQuaternion blendedRotation;
        aiQuaternion::Interpolate(
            blendedRotation, aiRotation0, aiRotation1, blendingFactor
        );
        glm::quat rotation(
            blendedRotation.w, blendedRotation.x, blendedRotation.y,
            blendedRotation.z
        );

        aiVector3D aiScaling0 =
            calcInterpolatedScaling(currentTick0, channel0);
        aiVector3D aiScaling1 =
            calcInterpolatedScaling(currentTick1, channel1);
        aiVector3D blendedScaling =
            (1.0f - blendingFactor) * aiScaling0 +
            aiScaling1 * blendingFactor;
        glm::vec3 scale(
            blendedScaling.x, blendedScaling.y, blendedScaling.z
        );

        // Inflate them into matrices
        glm::mat4 positionMat = glm::mat4(1.0f);
        positionMat           = glm::translate(positionMat, position);
        glm::mat4 rotationMat = glm::toMat4(rotation);
        glm::mat4 scaleMat    = glm::mat4(1.0f);
        scaleMat              = glm::scale(scaleMat, scale);

        // Multiply TRS matrices
        nodeTransform = positionMat * rotationMat * scaleMat;
    }

    glm::mat4 cascadeTransform =
        parentTransform * nodeTransform;

    auto isBoneNode = this->boneNameToIndex.find(nodeName) !=
                      this->boneNameToIndex.end();
    if (isBoneNode) {
        uint boneIndex = this->boneNameToIndex[nodeName];

        resultsBuffer[boneIndex] = glm::transpose(
            this->globalInverseTransform * cascadeTransform *
            boneOffsets[boneIndex]
        );
    } else {
        // Because there are some nodes at the root of the mesh,
        // that are not bones, but they have some transformations
        // Therefore, here I apply them to the globalTransformation
        cascadeTransform =
            cascadeTransform * nodeTransform;
    }

    for (uint i = 0; i < pNode->mNumChildren; i++) {
        // Go deeper into recursion
        applyBoneTransformsFromNodeTree(
            animation0, currentTick0, animation1, currentTick1,
            blendingFactor, pNode->mChildren[i], cascadeTransform,
            resultsBuffer
        );
    }
}

float AnimationMixer::calcAnimationTick(
    float currentSecond,
    float ticksPerSecond,
    unsigned int animationIndex0
)
{
    float tickSinceStarted = currentSecond * ticksPerSecond;

    float Duration = 0.0f;
    float fraction = modf(
        (float) this->scene->mAnimations[animationIndex0]->mDuration,
        &Duration
    );
    float currentTick1 = fmod(tickSinceStarted, Duration);
    return currentTick1;
}

const aiNodeAnim* AnimationMixer::findChannel(
    const aiAnimation& animation,
    const std::string& nodeName
)
{
    for (uint i = 0; i < animation.mNumChannels; i++) {
        const aiNodeAnim* channel = animation.mChannels[i];

        if (std::string(channel->mNodeName.data) == nodeName) {
            return channel;
        }
    }

    return nullptr;
}

void AnimationMixerControls::renderAnimationControls(
    AnimationMixer& am,
    float currentSecond
)
{
    const aiScene* scene = am.scene;
    if (!scene || scene->mNumAnimations == 0) {
        ImGui::Text("No animations available.");
        return;
    }

    ImGui::Columns(3, "AnimationControls");  // Start 3-column layout

    // First Column
    ImGui::Text("Animation 0");

    // Animation Drop-Down List for First Column
    if (ImGui::BeginCombo(
            "Animation##0",
            scene->mAnimations[selectedAnimation0]->mName.C_Str()
        )) {
        for (int i = 0; i < scene->mNumAnimations; ++i) {
            bool isSelected = (this->selectedAnimation0 == i);
            if (ImGui::Selectable(
                    scene->mAnimations[i]->mName.C_Str(), isSelected
                ))
                this->selectedAnimation0 = i;
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    float currentTick0 = this->am.calcAnimationTick(
        currentSecond, ticksPerSecond0, selectedAnimation0
    );
    ImGui::BeginDisabled();  // Disable any edits
    ImGui::SliderFloat(
        "Progress##0", &currentTick0, 0.0f,
        scene->mAnimations[selectedAnimation0]->mDuration, "%.3f"
    );
    ImGui::EndDisabled();  // Disable any edits
    ImGui::Text(
        "Length: %.1ft (%.3fs)",
        scene->mAnimations[selectedAnimation0]->mDuration,
        scene->mAnimations[selectedAnimation0]->mDuration /
            this->ticksPerSecond0
    );
    ImGui::InputFloat("Ticks per Second##0", &this->ticksPerSecond0);

    ImGui::NextColumn();

    // Middle Column
    ImGui::Text("Blending");

    // Blending factor Slider
    ImGui::SliderFloat(
        "Blending Factor", &this->blendingFactor, 0.0f, 1.0f, "%.3f"
    );

    ImGui::NextColumn();

    // Third Column - Same fields as the first column
    ImGui::Text("Animation 1");

    // Animation Drop-Down List for Third Column
    if (ImGui::BeginCombo(
            "Animation##1",
            scene->mAnimations[selectedAnimation1]->mName.C_Str()
        )) {
        for (int i = 0; i < scene->mNumAnimations; ++i) {
            bool isSelected = (selectedAnimation1 == i);
            if (ImGui::Selectable(
                    scene->mAnimations[i]->mName.C_Str(), isSelected
                ))
                selectedAnimation1 = i;
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    float currentTick1 = this->am.calcAnimationTick(
        currentSecond, ticksPerSecond1, selectedAnimation1
    );
    ImGui::BeginDisabled();  // Disable any edits
    ImGui::SliderFloat(
        "Progress##1", &currentTick1, 0.0f,
        scene->mAnimations[selectedAnimation1]->mDuration, "%.3f"
    );
    ImGui::EndDisabled();  // Disable any edits
    ImGui::Text(
        "Length: %.1ft (%.3fs)",
        scene->mAnimations[selectedAnimation1]->mDuration,
        scene->mAnimations[selectedAnimation1]->mDuration /
            this->ticksPerSecond1
    );
    ImGui::InputFloat("Ticks per Second##1", &this->ticksPerSecond1);

    ImGui::Columns(1);  // End columns layout
}