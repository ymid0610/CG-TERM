// Wardrobe.h
#pragma once

#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <vector>

// [중요] Model 구조체를 인식하기 위해 헤더 포함
#include "ReadObjFile.h" 

// 옷장의 현재 상태를 나타내는 열거형
enum HidingState {
    STATE_OUTSIDE = 0, // 평상시
    STATE_ENTERING,    // 들어가는 중 (문 열림 -> 플레이어 이동)
    STATE_HIDDEN,      // 숨어있음 (문 닫힘)
    STATE_EXITING      // 나오는 중 (문 열림 -> 플레이어 이동)
};

class Wardrobe {
private:
    // [추가] 옷장의 기준 위치 (문이 있는 앞면 중심 좌표)
    float posX, posZ;

    // 옷장 크기 설정
    float zWidth; // 가로 폭의 절반 (전체 폭은 zWidth * 2)
    float yBase;  // 바닥 높이

    // 문 애니메이션 관련
    float currentDoorAngle;
    float targetAngle;
    float doorSpeed;

    // 현재 상태
    HidingState currentState;

    // 내부 헬퍼 함수: 큐브 그리기
    void DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color);

public:
    // [수정] 생성자가 위치(x, z)를 입력받도록 변경
    Wardrobe(float x, float z);
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