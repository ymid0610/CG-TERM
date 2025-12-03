// Wardrobe.h
#pragma once
#include "ReadObjFile.h"
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <vector>

// 옷장의 현재 상태를 나타내는 열거형
enum HidingState {
    STATE_OUTSIDE = 0, // 평상시
    STATE_ENTERING,    // 들어가는 중 (문 열림 -> 플레이어 이동)
    STATE_HIDDEN,      // 숨어있음 (문 닫힘)
    STATE_EXITING      // 나오는 중 (문 열림 -> 플레이어 이동)
};

class Wardrobe {
private:
    // 옷장 위치 및 크기 설정
    float xFront, xBack;
    float zWidth;
    float yBase;

    // 문 애니메이션 관련
    float currentDoorAngle;
    float targetAngle;
    float doorSpeed;

    // 현재 상태
    HidingState currentState;

    // 내부 헬퍼 함수: 큐브 그리기 (기존 DrawCube와 동일한 역할)
    void DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color);

public:
    Wardrobe(); // 생성자
    ~Wardrobe();

    // 메인 루프에서 호출할 함수들
    void Update(glm::vec3& playerPos, float& cameraAngle, float& pitch, int& viewMode, bool& isFlashlightOn);
    void Draw(GLuint shaderID, const Model& model);

    // 상호작용 (F키 눌렀을 때)
    void TryInteract(glm::vec3 playerPos);

    // 상태 확인용 getter
    HidingState GetState() const { return currentState; }
    bool IsHidden() const { return currentState == STATE_HIDDEN; }
};