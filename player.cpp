#include "player.h"
#include <iostream>
#include <cmath> // sin, cos

Player::Player() {
    // 초기값 설정
    pos = glm::vec3(2.0f, -1.0f, 2.0f); // 미로 시작 위치
    rotY = 180.0f;

    cameraAngle = 0.0f;
    cameraDistance = 3.0f;
    pitch = 0.0f;
    sensitivity = 0.002f;
    viewMode = 3;

    currentAnim = IDLE;
    animTime = 0.0f;
    armAngle = 0.0f;
    legAngle = 0.0f;

    isFlashlightOn = false;
    isExhausted = false;

    maxStamina = 100.0f;
    currentStamina = 100.0f;
}

void Player::Update(bool keyState[], MazeMap& maze) {
    // 1. 이동 로직
    float inputZ = 0.0f; float inputX = 0.0f;
    if (keyState['w']) inputZ -= 1.0f;
    if (keyState['s']) inputZ += 1.0f;
    if (keyState['a']) inputX -= 1.0f;
    if (keyState['d']) inputX += 1.0f;

    bool isMoving = (inputX != 0.0f || inputZ != 0.0f);

    if (isMoving) {
        float length = sqrt(inputX * inputX + inputZ * inputZ);
        if (length > 0) { inputX /= length; inputZ /= length; }

        float rad = cameraAngle;
        float moveDirX = inputX * cos(rad) + inputZ * sin(rad);
        float moveDirZ = -inputX * sin(rad) + inputZ * cos(rad);

        float currentSpeed = 0.1f;

        // 달리기 & 스테미너 처리
        if (keyState[' '] && !isExhausted) {
            currentSpeed = 0.15f;
            currentAnim = RUN;
            currentStamina -= 1.0f;
            if (currentStamina <= 0.0f) {
                currentStamina = 0.0f;
                isExhausted = true;
            }
        }
        else {
            currentAnim = WALK;
            if (currentStamina < maxStamina) currentStamina += 0.2f;
        }

        // [충돌 처리]
        float nextX = pos.x + moveDirX * currentSpeed;
        float nextZ = pos.z + moveDirZ * currentSpeed;

        if (!maze.CheckCollision(nextX, pos.z)) {
            pos.x = nextX;
        }
        if (!maze.CheckCollision(pos.x, nextZ)) {
            pos.z = nextZ;
        }
    }
    else {
        currentAnim = IDLE;
        if (currentStamina < maxStamina) currentStamina += 0.2f;
    }

    // 지침 상태 해제
    if (isExhausted && currentStamina > 20.0f) {
        isExhausted = false;
    }
    if (currentStamina > maxStamina) currentStamina = maxStamina;

    // 회전 업데이트
    rotY = glm::degrees(cameraAngle) + 180.0f;

    // 2. 애니메이션 계산
    float speed = 0.0f; float amplitude = 0.0f;
    if (currentAnim == WALK) { speed = 0.2f; amplitude = 30.0f; }
    else if (currentAnim == RUN) { speed = 0.25f; amplitude = 60.0f; }
    else { if (animTime > 0) animTime = 0; speed = 0.05f; amplitude = 2.0f; }

    if (currentAnim != IDLE) {
        animTime += speed;
        armAngle = sin(animTime) * amplitude;
        legAngle = sin(animTime) * amplitude;
    }
    else {
        animTime += speed;
        armAngle = sin(animTime) * amplitude;
        legAngle = 0.0f;
    }
}

void Player::Draw(GLuint shaderID, Model& cubeModel, void (*DrawCubeFunc)(glm::mat4, glm::vec3)) {
    // 렌더링 조건 설정
    bool drawBodyParts = (viewMode == 3);

    glm::mat4 bodyMat = glm::mat4(1.0f);
    bodyMat = glm::translate(bodyMat, pos);
    bodyMat = glm::rotate(bodyMat, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));

    // 1. 몸통
    if (drawBodyParts) {
        glm::mat4 torsoScale = glm::scale(bodyMat, glm::vec3(0.5f, 0.7f, 0.25f));
        DrawCubeFunc(torsoScale, glm::vec3(0.0f, 0.0f, 0.8f));
    }

    // 2. 머리
    if (drawBodyParts) {
        glm::mat4 headMat = glm::translate(bodyMat, glm::vec3(0.0f, 0.55f, 0.0f));
        glm::mat4 headScale = glm::scale(headMat, glm::vec3(0.4f, 0.4f, 0.4f));
        DrawCubeFunc(headScale, glm::vec3(1.0f, 0.8f, 0.6f));
    }

    // 3. 왼쪽 팔
    {
        glm::mat4 lArmMat = glm::translate(bodyMat, glm::vec3(-0.35f, 0.35f, 0.0f));
        lArmMat = glm::rotate(lArmMat, glm::radians(armAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        lArmMat = glm::translate(lArmMat, glm::vec3(0.0f, -0.35f, 0.0f));
        glm::mat4 lArmScale = glm::scale(lArmMat, glm::vec3(0.2f, 0.7f, 0.2f));
        DrawCubeFunc(lArmScale, glm::vec3(1.0f, 0.8f, 0.6f));
    }

    // 4. 오른쪽 팔
    glm::mat4 rArmMat = glm::translate(bodyMat, glm::vec3(0.35f, 0.35f, 0.0f));
    float currentRArmAngle = -armAngle;

    if ((isFlashlightOn || viewMode == 1)) {
        currentRArmAngle = -90.0f + (armAngle * 0.1f);
        if (viewMode == 1) {
            currentRArmAngle -= glm::degrees(pitch);
        }
    }
    rArmMat = glm::rotate(rArmMat, glm::radians(currentRArmAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rArmMat = glm::translate(rArmMat, glm::vec3(0.0f, -0.35f, 0.0f));

    glm::mat4 rArmScale = glm::scale(rArmMat, glm::vec3(0.2f, 0.7f, 0.2f));
    DrawCubeFunc(rArmScale, glm::vec3(1.0f, 0.8f, 0.6f));

    // 손전등
    if (isFlashlightOn) {
        glm::mat4 lightMat = glm::translate(rArmMat, glm::vec3(0.0f, -0.5f, 0.0f));
        glm::mat4 lightModel = glm::scale(lightMat, glm::vec3(0.1f, 0.4f, 0.1f));
        DrawCubeFunc(lightModel, glm::vec3(0.1f, 0.1f, 0.1f));

        // 조명 위치 업데이트 (메인 루프에서 사용할 값을 여기서 업데이트하는 방식)
        // 실제 uniform 업데이트는 main에서 이루어짐
    }

    // 5. 다리
    if (drawBodyParts) {
        glm::mat4 lLegMat = glm::translate(bodyMat, glm::vec3(-0.15f, -0.35f, 0.0f));
        lLegMat = glm::rotate(lLegMat, glm::radians(legAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        lLegMat = glm::translate(lLegMat, glm::vec3(0.0f, -0.35f, 0.0f));
        glm::mat4 lLegScale = glm::scale(lLegMat, glm::vec3(0.2f, 0.7f, 0.2f));
        DrawCubeFunc(lLegScale, glm::vec3(0.2f, 0.2f, 0.5f));

        glm::mat4 rLegMat = glm::translate(bodyMat, glm::vec3(0.15f, -0.35f, 0.0f));
        rLegMat = glm::rotate(rLegMat, glm::radians(-legAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        rLegMat = glm::translate(rLegMat, glm::vec3(0.0f, -0.35f, 0.0f));
        glm::mat4 rLegScale = glm::scale(rLegMat, glm::vec3(0.2f, 0.7f, 0.2f));
        DrawCubeFunc(rLegScale, glm::vec3(0.2f, 0.2f, 0.5f));
    }
}

void Player::ProcessMouse(int x, int y, int width, int height) {
    if (viewMode == 1) {
        int centerX = width / 2;
        int centerY = height / 2;
        float xOffset = (x - centerX) * sensitivity;
        float yOffset = (y - centerY) * sensitivity;

        if (xOffset == 0 && yOffset == 0) return;

        cameraAngle -= xOffset;
        pitch -= yOffset;

        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;
    }
}

glm::vec3 Player::GetCameraPos() {
    if (viewMode == 3) {
        float camHeight = 2.0f;
        float camX = sin(cameraAngle) * cameraDistance;
        float camZ = cos(cameraAngle) * cameraDistance;
        return pos + glm::vec3(camX, camHeight, camZ);
    }
    else {
        return pos + glm::vec3(0.0f, 0.6f, 0.0f);
    }
}

glm::vec3 Player::GetCameraTarget() {
    if (viewMode == 3) {
        return pos;
    }
    else {
        glm::vec3 camPos = GetCameraPos();
        glm::vec3 direction;
        direction.x = -sin(cameraAngle) * cos(pitch);
        direction.y = sin(pitch);
        direction.z = -cos(cameraAngle) * cos(pitch);
        return camPos + glm::normalize(direction);
    }
}