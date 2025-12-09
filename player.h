#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <vector>
#include "ReadObjFile.h"
#include "map.h" 

// 애니메이션 상태 열거형
enum AnimState { IDLE, WALK, RUN };

class Player {
public:
    // 멤버 변수
    glm::vec3 pos;          // 위치
    float rotY;             // 회전 (Y축)
	int hp;               // 체력

    // 카메라 관련
    float cameraAngle;
    float cameraDistance;
    float pitch;
    float sensitivity;
    int viewMode;           // 1: 1인칭, 3: 3인칭

    // 애니메이션 관련
    AnimState currentAnim;
    float animTime;
    float armAngle;
    float legAngle;

    // 상태 플래그
    bool isFlashlightOn;
    bool isExhausted;
    bool isHide; // 숨기 플래그 , true == 귀신이 인식 못함

    // 스테미너
    float maxStamina;
    float currentStamina;

    // 멤버 함수
    Player(); // 생성자 (초기화)

    // 업데이트: 키 입력 처리, 이동, 충돌 체크, 애니메이션 계산
    void Update(bool keyState[], MazeMap& maze);

    // 그리기: DrawCube 함수 포인터와 모델 데이터를 받아옴
    void Draw(GLuint shaderID, Model& cubeModel, void (*DrawCubeFunc)(glm::mat4, glm::vec3));

    // 마우스 입력 처리
    void ProcessMouse(int x, int y, int width, int height);

    // 카메라 뷰 행렬 계산용
    glm::vec3 GetCameraPos();
    glm::vec3 GetCameraTarget();
};