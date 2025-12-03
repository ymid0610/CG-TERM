// Wardrobe.cpp
#include "wardrobe.h"
#include <iostream>

Wardrobe::Wardrobe() {
    // 초기 위치 및 크기 설정 (기존 코드 값 유지)
    xFront = 4.0f;
    xBack = 5.5f;
    zWidth = 1.5f;
    yBase = -0.25f;

    // 애니메이션 변수 초기화
    currentDoorAngle = 0.0f;
    targetAngle = 0.0f;
    doorSpeed = 5.0f;
    currentState = STATE_OUTSIDE;
}

Wardrobe::~Wardrobe() {
}

// 헬퍼 함수: 클래스 내부에서 큐브를 그리기 위해 사용
void Wardrobe::DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color) {
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint faceColorLoc = glGetUniformLocation(shaderID, "faceColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3f(faceColorLoc, color.r, color.g, color.b);

    if (model.face_count > 0)
        glDrawElements(GL_TRIANGLES, model.face_count * 3, GL_UNSIGNED_INT, 0);
}

void Wardrobe::Update(glm::vec3& playerPos, float& cameraAngle, float& pitch, int& viewMode, bool& isFlashlightOn) {
    // 1. 상태별 로직 (기존 Timer 함수의 switch문 이동)
    switch (currentState) {
    case STATE_OUTSIDE:
        targetAngle = 0.0f;
        break;

    case STATE_ENTERING:
        targetAngle = 90.0f;
        // 문이 다 열리면 플레이어를 안으로 이동시키고 문 닫기 시작
        if (currentDoorAngle >= 90.0f) {
            playerPos = glm::vec3(4.8f, -1.0f, 0.0f); // 옷장 안쪽 좌표
            cameraAngle = glm::radians(90.0f);
            pitch = 0.0f;
            viewMode = 1;         // 1인칭 고정
            isFlashlightOn = false; // 불 끄기
            currentState = STATE_HIDDEN;
        }
        break;

    case STATE_HIDDEN:
        targetAngle = 25.0f; // 틈새 확보
        viewMode = 1;        // 강제 1인칭
        playerPos = glm::vec3(4.8f, -1.0f, 0.0f); // 위치 고정
        break;

    case STATE_EXITING:
        targetAngle = 90.0f;
        // 문이 다 열리면 플레이어를 밖으로 이동
        if (currentDoorAngle >= 90.0f) {
            playerPos = glm::vec3(3.0f, -1.0f, 0.0f); // 옷장 앞 좌표
            currentState = STATE_OUTSIDE;
        }
        break;
    }

    // 2. 문 애니메이션 (부드러운 회전)
    if (abs(currentDoorAngle - targetAngle) > 1.0f) {
        if (currentDoorAngle < targetAngle) currentDoorAngle += doorSpeed;
        else currentDoorAngle -= doorSpeed;
    }
    else {
        currentDoorAngle = targetAngle;
    }
}

void Wardrobe::TryInteract(glm::vec3 playerPos) {
    // F키를 눌렀을 때 호출됨
    if (currentState == STATE_OUTSIDE) {
        // 상호작용 범위 체크 (x > 2.5 && z 범위 내)
        if (playerPos.x > 2.5f && abs(playerPos.z) < 3.0f) {
            currentState = STATE_ENTERING;
        }
    }
    else if (currentState == STATE_HIDDEN) {
        // 숨어있을 때는 나가기 상태로 전환
        currentState = STATE_EXITING;
    }
}

void Wardrobe::Draw(GLuint shaderID, const Model& model) {
    // 색상 정의
    glm::vec3 darkWood = glm::vec3(0.35f, 0.2f, 0.05f);
    glm::vec3 lightWood = glm::vec3(0.45f, 0.3f, 0.1f);
    glm::vec3 gold = glm::vec3(0.8f, 0.7f, 0.0f);

    // [상호작용 범위 시각화] - 투명한 초록 바닥
    // 원래 코드의 blending 로직 포함
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 zone = glm::translate(glm::mat4(1.0f), glm::vec3(3.25f, -1.95f, 0.0f));
    glm::mat4 zoneScale = glm::scale(zone, glm::vec3(1.5f, 0.01f, 6.0f));
    DrawBox(shaderID, model, zoneScale, glm::vec3(0.0f, 1.0f, 0.0f));

    glDisable(GL_BLEND);

    // --- 옷장 몸체 그리기 ---
    // 뒷면
    glm::mat4 back = glm::translate(glm::mat4(1.0f), glm::vec3(xBack, yBase, 0.0f));
    glm::mat4 backScale = glm::scale(back, glm::vec3(0.1f, 3.5f, zWidth * 2.0f));
    DrawBox(shaderID, model, backScale, darkWood);

    // 왼쪽면
    glm::mat4 left = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, yBase, -zWidth));
    glm::mat4 leftScale = glm::scale(left, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawBox(shaderID, model, leftScale, darkWood);

    // 오른쪽면
    glm::mat4 right = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, yBase, zWidth));
    glm::mat4 rightScale = glm::scale(right, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawBox(shaderID, model, rightScale, darkWood);

    // 윗면
    glm::mat4 top = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, 1.5f, 0.0f));
    glm::mat4 topScale = glm::scale(top, glm::vec3(1.5f, 0.1f, zWidth * 2.0f));
    DrawBox(shaderID, model, topScale, darkWood);

    // 바닥면
    glm::mat4 bottom = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, -2.0f, 0.0f));
    glm::mat4 bottomScale = glm::scale(bottom, glm::vec3(1.5f, 0.1f, zWidth * 2.0f));
    DrawBox(shaderID, model, bottomScale, darkWood);

    // --- 문 그리기 (애니메이션 적용) ---
    // 왼쪽 문
    glm::mat4 lHinge = glm::translate(glm::mat4(1.0f), glm::vec3(xFront, yBase, -zWidth));
    lHinge = glm::rotate(lHinge, glm::radians(-currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 lDoor = glm::translate(lHinge, glm::vec3(0.0f, 0.0f, zWidth / 2.0f));
    glm::mat4 lDoorScale = glm::scale(lDoor, glm::vec3(0.1f, 3.4f, zWidth));
    DrawBox(shaderID, model, lDoorScale, lightWood);

    // 왼쪽 문 손잡이
    glm::mat4 lKnob = glm::translate(lHinge, glm::vec3(-0.1f, 0.0f, zWidth - 0.2f));
    glm::mat4 knobScale = glm::scale(lKnob, glm::vec3(0.1f, 0.2f, 0.1f));
    DrawBox(shaderID, model, knobScale, gold);

    // 오른쪽 문
    glm::mat4 rHinge = glm::translate(glm::mat4(1.0f), glm::vec3(xFront, yBase, zWidth));
    rHinge = glm::rotate(rHinge, glm::radians(currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 rDoor = glm::translate(rHinge, glm::vec3(0.0f, 0.0f, -zWidth / 2.0f));
    glm::mat4 rDoorScale = glm::scale(rDoor, glm::vec3(0.1f, 3.4f, zWidth));
    DrawBox(shaderID, model, rDoorScale, lightWood);

    // 오른쪽 문 손잡이
    glm::mat4 rKnob = glm::translate(rHinge, glm::vec3(-0.1f, 0.0f, -zWidth + 0.2f));
    DrawBox(shaderID, model, rKnob, gold); // 크기는 위에서 계산한 knobScale 재사용은 안되므로 다시 계산하거나 변환행렬에 적용
    // (위 코드의 rKnob은 이미 스케일 전 변환행렬이므로 아래처럼 그립니다)
    // 원본 코드 로직상 rKnob 변환행렬에 scale을 적용해야 함.
    glm::mat4 rKnobScaled = glm::scale(rKnob, glm::vec3(0.1f, 0.2f, 0.1f));
    DrawBox(shaderID, model, rKnobScaled, gold);
}