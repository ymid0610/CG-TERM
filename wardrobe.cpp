// Wardrobe.cpp
#include "wardrobe.h"
#include <iostream>

// [수정] 생성자에서 위치를 받아서 초기화
Wardrobe::Wardrobe(float x, float z) {
    posX = x; // 옷장 앞면 X 좌표
    posZ = z; // 옷장 중심 Z 좌표

    // 옷장의 크기 및 높이 설정
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

// [수정] Update 함수: 플레이어 이동 위치를 posX, posZ 기준으로 계산
void Wardrobe::Update(glm::vec3& playerPos, float& cameraAngle, float& pitch, int& viewMode, bool& isFlashlightOn) {

    // 숨었을 때 플레이어가 위치할 좌표 (옷장 안쪽: 앞면보다 0.8만큼 뒤)
    float hiddenX = posX + 0.8f;
    float hiddenZ = posZ;

    switch (currentState) {
    case STATE_OUTSIDE:
        targetAngle = 0.0f;
        break;

    case STATE_ENTERING:
        targetAngle = 90.0f;
        // 문이 다 열리면 플레이어를 안으로 이동시키고 문 닫기 시작
        if (currentDoorAngle >= 90.0f) {
            playerPos = glm::vec3(hiddenX, -1.0f, hiddenZ); // 내부로 이동
            cameraAngle = glm::radians(90.0f); // 문 쪽을 바라봄
            pitch = 0.0f;
            viewMode = 1;         // 1인칭 고정
            isFlashlightOn = false; // 불 끄기
            currentState = STATE_HIDDEN;
        }
        break;

    case STATE_HIDDEN:
        targetAngle = 25.0f; // 틈새 확보
        // viewMode = 1;        // 강제 1인칭
        playerPos = glm::vec3(hiddenX, -1.0f, hiddenZ); // 위치 고정
        break;

    case STATE_EXITING:
        targetAngle = 90.0f;
        // 문이 다 열리면 플레이어를 밖으로 이동
        if (currentDoorAngle >= 90.0f) {
            playerPos = glm::vec3(posX - 1.0f, -1.0f, posZ); // 옷장 앞으로 이동
            currentState = STATE_OUTSIDE;
        }
        break;
    }

    // 문 애니메이션 (부드러운 회전)
    if (abs(currentDoorAngle - targetAngle) > 1.0f) {
        if (currentDoorAngle < targetAngle) currentDoorAngle += doorSpeed;
        else currentDoorAngle -= doorSpeed;
    }
    else {
        currentDoorAngle = targetAngle;
    }
}

// [수정] TryInteract: 상호작용 범위를 현재 옷장 위치(posX, posZ) 기준으로 체크
int Wardrobe::TryInteract(glm::vec3 playerPos) {
    // F키를 눌렀을 때 호출됨
    if (currentState == STATE_OUTSIDE) {
        // 옷장 앞(posX보다 작은 쪽)에 있고, Z축으로 옷장 폭 내에 있을 때
        // 예: posX가 10이라면 플레이어는 8.5 ~ 10.5 사이
        if (playerPos.x > (posX - 1.5f) && playerPos.x < (posX + 0.5f) && abs(playerPos.z - posZ) < 3.0f) {
            currentState = STATE_ENTERING;
            return 1;
        }
    }
    else if (currentState == STATE_HIDDEN) {
        // 숨어있을 때는 나가기 상태로 전환
        currentState = STATE_EXITING;
        return 1;
    }
}

// [수정] Draw 함수: 모든 좌표를 posX, posZ 기준으로 그리기
void Wardrobe::Draw(GLuint shaderID, const Model& model) {
    // 좌표 계산
    float xFront = posX;        // 앞면 위치
    float xBack = posX + 1.5f;  // 뒷면 위치 (두께 1.5)

    // 색상 정의
    glm::vec3 darkWood = glm::vec3(0.35f, 0.2f, 0.05f);
    glm::vec3 lightWood = glm::vec3(0.45f, 0.3f, 0.1f);
    glm::vec3 gold = glm::vec3(0.8f, 0.7f, 0.0f);

    //// [상호작용 범위 시각화] - 투명한 초록 바닥
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //// 상호작용 가능한 영역 표시 (옷장 앞쪽 바닥)
    //glm::mat4 zone = glm::translate(glm::mat4(1.0f), glm::vec3(posX - 0.75f, -1.95f, posZ));
    //glm::mat4 zoneScale = glm::scale(zone, glm::vec3(1.5f, 0.01f, 6.0f));
    //DrawBox(shaderID, model, zoneScale, glm::vec3(0.0f, 1.0f, 0.0f));

    //glDisable(GL_BLEND);

    // --- 옷장 몸체 그리기 ---
    // 뒷면
    glm::mat4 back = glm::translate(glm::mat4(1.0f), glm::vec3(xBack, yBase, posZ));
    glm::mat4 backScale = glm::scale(back, glm::vec3(0.1f, 3.5f, zWidth * 2.0f));
    DrawBox(shaderID, model, backScale, darkWood);

    // 왼쪽면
    glm::mat4 left = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, yBase, posZ - zWidth));
    glm::mat4 leftScale = glm::scale(left, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawBox(shaderID, model, leftScale, darkWood);

    // 오른쪽면
    glm::mat4 right = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, yBase, posZ + zWidth));
    glm::mat4 rightScale = glm::scale(right, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawBox(shaderID, model, rightScale, darkWood);

    // 윗면
    glm::mat4 top = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, 1.5f, posZ));
    glm::mat4 topScale = glm::scale(top, glm::vec3(1.5f, 0.1f, zWidth * 2.0f));
    DrawBox(shaderID, model, topScale, darkWood);

    // 바닥면
    glm::mat4 bottom = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, -2.0f, posZ));
    glm::mat4 bottomScale = glm::scale(bottom, glm::vec3(1.5f, 0.1f, zWidth * 2.0f));
    DrawBox(shaderID, model, bottomScale, darkWood);

    // --- 문 그리기 (애니메이션 적용) ---
    // 왼쪽 문
    glm::mat4 lHinge = glm::translate(glm::mat4(1.0f), glm::vec3(xFront, yBase, posZ - zWidth));
    lHinge = glm::rotate(lHinge, glm::radians(-currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 lDoor = glm::translate(lHinge, glm::vec3(0.0f, 0.0f, zWidth / 2.0f));
    glm::mat4 lDoorScale = glm::scale(lDoor, glm::vec3(0.1f, 3.4f, zWidth));
    DrawBox(shaderID, model, lDoorScale, lightWood);

    // 왼쪽 문 손잡이
    glm::mat4 lKnob = glm::translate(lHinge, glm::vec3(-0.1f, 0.0f, zWidth - 0.2f));
    glm::mat4 knobScale = glm::scale(lKnob, glm::vec3(0.1f, 0.2f, 0.1f));
    DrawBox(shaderID, model, knobScale, gold);

    // 오른쪽 문
    glm::mat4 rHinge = glm::translate(glm::mat4(1.0f), glm::vec3(xFront, yBase, posZ + zWidth));
    rHinge = glm::rotate(rHinge, glm::radians(currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 rDoor = glm::translate(rHinge, glm::vec3(0.0f, 0.0f, -zWidth / 2.0f));
    glm::mat4 rDoorScale = glm::scale(rDoor, glm::vec3(0.1f, 3.4f, zWidth));
    DrawBox(shaderID, model, rDoorScale, lightWood);

    // 오른쪽 문 손잡이
    glm::mat4 rKnob = glm::translate(rHinge, glm::vec3(-0.1f, 0.0f, -zWidth + 0.2f));
    // 오른쪽 손잡이는 변환 행렬에 스케일 적용이 안 되어 있었으므로 여기서 적용
    glm::mat4 rKnobScaled = glm::scale(rKnob, glm::vec3(0.1f, 0.2f, 0.1f));
    DrawBox(shaderID, model, rKnobScaled, gold);
}