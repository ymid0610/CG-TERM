#include "wardrobe.h"
#include <iostream>

Wardrobe::Wardrobe(float x, float z, float rot) {
    posX = x;
    posZ = z;
    rotation = rot; // 각도 저장

    zWidth = 1.5f;   // 옷장 절반 폭
    yBase = -0.25f;  // 높이 오프셋

    currentDoorAngle = 0.0f;
    targetAngle = 0.0f;
    doorSpeed = 5.0f;
    currentState = STATE_OUTSIDE;
}

Wardrobe::~Wardrobe() {
}

void Wardrobe::DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color) {
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint faceColorLoc = glGetUniformLocation(shaderID, "faceColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3f(faceColorLoc, color.r, color.g, color.b);

    if (model.face_count > 0)
        glDrawElements(GL_TRIANGLES, model.face_count * 3, GL_UNSIGNED_INT, 0);
}

void Wardrobe::Update(glm::vec3& playerPos, float& cameraAngle, float& pitch, int& viewMode, bool& isFlashlightOn) {
    glm::mat4 transMat = glm::mat4(1.0f);
    transMat = glm::rotate(transMat, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    // 옷장 내부(Local): 뒤쪽(+0.8 X방향)이 아니라 회전 고려 필요.
    // 모델 기준: Front는 -X방향, Back은 +X방향.
    // 숨을 위치: 중심(0,0)에서 약간 뒤(+0.5 X)
    glm::vec4 localHidePos = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
    glm::vec4 worldHidePos = transMat * localHidePos;

    float hiddenX = posX + worldHidePos.x;
    float hiddenZ = posZ + worldHidePos.z;

    switch (currentState) {
    case STATE_OUTSIDE:
        targetAngle = 0.0f;
        break;
    case STATE_ENTERING:
        targetAngle = 90.0f;
        if (currentDoorAngle >= 90.0f) {
            playerPos = glm::vec3(hiddenX, -1.0f, hiddenZ);
            cameraAngle = glm::radians(90.0f - rotation);
            

            pitch = 0.0f;           // 정면을 바라봄
            viewMode = 1;           // 1인칭 시점 강제
            isFlashlightOn = false; // 숨으면 불 끄기
            currentState = STATE_HIDDEN;
        }
        break;
    case STATE_HIDDEN:
        targetAngle = 25.0f;
        playerPos = glm::vec3(hiddenX, -1.0f, hiddenZ);
        break;
    case STATE_EXITING:
        targetAngle = 90.0f;
        if (currentDoorAngle >= 90.0f) {
            // 나올 때는 옷장 앞(-1.5f * Front방향)으로 이동
            glm::vec4 exitOffset = transMat * glm::vec4(-1.5f, 0.0f, 0.0f, 1.0f);
            playerPos = glm::vec3(posX + exitOffset.x, -1.0f, posZ + exitOffset.z);
            currentState = STATE_OUTSIDE;
        }
        break;
    }

    if (abs(currentDoorAngle - targetAngle) > 1.0f) {
        if (currentDoorAngle < targetAngle) currentDoorAngle += doorSpeed;
        else currentDoorAngle -= doorSpeed;
    }
    else {
        currentDoorAngle = targetAngle;
    }
}

int Wardrobe::TryInteract(glm::vec3 playerPos) {
    // 거리 기반 상호작용으로 변경
    float dist = glm::distance(playerPos, glm::vec3(posX, -1.0f, posZ));

    // 플레이어가 옷장 중심에서 2.5 거리 이내에 있으면 상호작용 가능
    if (dist < 2.5f) {
        if (currentState == STATE_OUTSIDE) {
            currentState = STATE_ENTERING;
            return 1;
        }
        else if (currentState == STATE_HIDDEN) {
            currentState = STATE_EXITING;
            return 1;
        }
    }
    return 0;
}

// Draw 함수를 로컬 좌표계 + Model Matrix 방식
void Wardrobe::Draw(GLuint shaderID, const Model& model) {
    // 기본 변환 행렬 (위치 이동 -> 회전)
    glm::mat4 baseMat = glm::mat4(1.0f);
    baseMat = glm::translate(baseMat, glm::vec3(posX, 0.0f, posZ));
    baseMat = glm::rotate(baseMat, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    // 색상
    glm::vec3 darkWood = glm::vec3(0.35f, 0.2f, 0.05f);
    glm::vec3 lightWood = glm::vec3(0.45f, 0.3f, 0.1f);
    glm::vec3 gold = glm::vec3(0.8f, 0.7f, 0.0f);

    // --- 로컬 좌표계 기준 그리기 (중심 0,0,0) ---
    // 이 모델의 정의: 앞(Front)은 -X 방향, 뒤(Back)는 +X 방향

    // 1. 뒷판 (Back Panel) : 로컬 X = +0.75 위치
    glm::mat4 back = glm::translate(baseMat, glm::vec3(0.75f, yBase, 0.0f));
    glm::mat4 backScale = glm::scale(back, glm::vec3(0.1f, 3.5f, zWidth * 2.0f));
    DrawBox(shaderID, model, backScale, darkWood);

    // 2. 왼쪽판 (Left Panel) : 로컬 Z = -1.5
    glm::mat4 left = glm::translate(baseMat, glm::vec3(0.0f, yBase, -zWidth));
    glm::mat4 leftScale = glm::scale(left, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawBox(shaderID, model, leftScale, darkWood);

    // 3. 오른쪽판 (Right Panel) : 로컬 Z = +1.5
    glm::mat4 right = glm::translate(baseMat, glm::vec3(0.0f, yBase, zWidth));
    glm::mat4 rightScale = glm::scale(right, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawBox(shaderID, model, rightScale, darkWood);

    // 4. 윗판/아랫판
    glm::mat4 top = glm::translate(baseMat, glm::vec3(0.0f, 1.5f, 0.0f));
    DrawBox(shaderID, model, glm::scale(top, glm::vec3(1.5f, 0.1f, zWidth * 2.0f)), darkWood);

    glm::mat4 bottom = glm::translate(baseMat, glm::vec3(0.0f, -2.0f, 0.0f));
    DrawBox(shaderID, model, glm::scale(bottom, glm::vec3(1.5f, 0.1f, zWidth * 2.0f)), darkWood);

    // 5. 문 (Doors)
    // 왼쪽 문 (Hinge at Z = -zWidth, X = -0.75)
    glm::mat4 lHinge = glm::translate(baseMat, glm::vec3(-0.75f, yBase, -zWidth));
    lHinge = glm::rotate(lHinge, glm::radians(-currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // 문 여는 회전

    glm::mat4 lDoor = glm::translate(lHinge, glm::vec3(0.0f, 0.0f, zWidth / 2.0f));
    DrawBox(shaderID, model, glm::scale(lDoor, glm::vec3(0.1f, 3.4f, zWidth)), lightWood);

    // 손잡이
    glm::mat4 lKnob = glm::translate(lHinge, glm::vec3(-0.1f, 0.0f, zWidth - 0.2f));
    DrawBox(shaderID, model, glm::scale(lKnob, glm::vec3(0.1f, 0.2f, 0.1f)), gold);

    // 오른쪽 문
    glm::mat4 rHinge = glm::translate(baseMat, glm::vec3(-0.75f, yBase, zWidth));
    rHinge = glm::rotate(rHinge, glm::radians(currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 rDoor = glm::translate(rHinge, glm::vec3(0.0f, 0.0f, -zWidth / 2.0f));
    DrawBox(shaderID, model, glm::scale(rDoor, glm::vec3(0.1f, 3.4f, zWidth)), lightWood);

    // 손잡이
    glm::mat4 rKnob = glm::translate(rHinge, glm::vec3(-0.1f, 0.0f, -zWidth + 0.2f));
    DrawBox(shaderID, model, glm::scale(rKnob, glm::vec3(0.1f, 0.2f, 0.1f)), gold);
}

