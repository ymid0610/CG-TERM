#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <vector>
#include "ReadObjFile.h"

// 옷장 상태 상수
#define STATE_OUTSIDE 0
#define STATE_ENTERING 1
#define STATE_HIDDEN 2
#define STATE_EXITING 3

class Wardrobe
{
public:
    // [수정] 생성자에 rotation 추가 (기본값 0)
    Wardrobe(float x, float z, float rot);
    ~Wardrobe();

    void Update(glm::vec3& playerPos, float& cameraAngle, float& pitch, int& viewMode, bool& isFlashlightOn);
    void Draw(GLuint shaderID, const Model& model);
    int TryInteract(glm::vec3 playerPos);
    int GetState() { return currentState; }

    glm::vec3 GetPos() const { return glm::vec3(posX, -1.0f, posZ); }

private:
    void DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color);

    float posX, posZ;
    float rotation; // [추가] 옷장의 회전 각도 (Y축 기준)

    float zWidth;
    float yBase;

    // 애니메이션 관련
    float currentDoorAngle;
    float targetAngle;
    float doorSpeed;
    int currentState;
};