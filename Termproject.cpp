#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>
#include <stdlib.h>
#include <stdio.h>
#include "ReadObjFile.h"

// --- 전역 변수 및 상태 관리 ---
GLint width = 1600, height = 900;
GLuint shaderProgramID, vertexShader, fragmentShader;
GLuint VAO, VBO, EBO;

Model model; // cube.obj 데이터 저장

// 캐릭터 위치 및 회전
glm::vec3 stevePos = glm::vec3(0.0f, -1.0f, 0.0f);
float steveRotY = 180.0f; // 등을 보이게 설정

// [수정] 카메라 관련 변수
float cameraAngle = 0.0f;
float cameraDistance = 3.0f;
int viewMode = 1; // 1: 1인칭, 3: 3인칭 (기본값 1)

// 애니메이션 상태
enum AnimState { IDLE, WALK, RUN };
AnimState currentAnim = IDLE;
bool isFlashlightOn = false;

// 애니메이션 변수
float animTime = 0.0f;
float armAngle = 0.0f;
float legAngle = 0.0f;

// 조명 관련
glm::vec3 lightPos(0.0f, 2.0f, 0.0f);
glm::vec3 lightDirection(0.0f, 0.0f, -1.0f);

// [전역 변수 아래쪽에 추가]
float pitch = 0.0f;       // 위아래 각도 (Pitch)
float sensitivity = 0.002f; // 마우스 감도 (너무 빠르면 줄이세요)

// --- 전역 변수 ---
bool keyState[256] = { false }; // 키보드 눌림 상태 저장 (추가)
float moveSpeed = 0.1f; // 캐릭터 이동 속도

// --- 전역 변수 ---
bool isDoorOpen = false;    // 문이 열려있는지 상태 (True: 열림, False: 닫힘)
float currentDoorAngle = 0.0f; // 현재 문이 열린 각도 (0 ~ 90도)

// [추가] 스테미너 관련 변수
float maxStamina = 100.0f;      // 최대 스테미너
float currentStamina = 100.0f; // 현재 스테미너
bool isExhausted = false;      // [추가됨] 지침 상태 플래그 (True면 달리기 불가)

// --- 전역 변수 ---
// 0: 평상시 (Outside)
// 1: 문 열리는 중 (Entering - Opening Door)
// 2: 숨어있음 (Hidden)
// 3: 나가는 중 (Exiting - Opening Door)
int hidingState = 0;


// 함수 선언
void MakeVertexShaders();
void MakeFragmentShaders();
void InitBuffers();
GLuint MakeShaderProgram();
GLvoid DrawScene();
GLvoid Reshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void Timer(int value);
void LoadOBJ(const char* filename);
char* filetobuf(const char* file);

// 헬퍼 함수: 큐브 그리기
void DrawCube(glm::mat4 modelMat, glm::vec3 color) {
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint faceColorLoc = glGetUniformLocation(shaderProgramID, "faceColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3f(faceColorLoc, color.r, color.g, color.b);

    if (model.face_count > 0)
        glDrawElements(GL_TRIANGLES, model.face_count * 3, GL_UNSIGNED_INT, 0);
}

void MouseMotion(int x, int y) {
    // 1인칭일 때만 작동
    if (viewMode == 1) {
        // 화면 중앙 좌표 계산
        int centerX = width / 2;
        int centerY = height / 2;

        // 중앙으로부터 마우스가 얼마나 이동했는지 계산
        float xOffset = (x - centerX) * sensitivity;
        float yOffset = (y - centerY) * sensitivity;

        // 마우스가 중앙에 있을 때는 계산하지 않음 (WarpPointer로 인한 재호출 방지)
        if (xOffset == 0 && yOffset == 0) return;

        // [수정] 각도 업데이트 방향 반전
        // 기존: cameraAngle += xOffset; (오른쪽 이동 시 왼쪽 회전)
        // 변경: cameraAngle -= xOffset; (오른쪽 이동 시 오른쪽 회전)
        cameraAngle -= xOffset;

        pitch -= yOffset;       // 상하 회전 (Pitch)

        // 위아래 각도 제한 (목이 꺾이지 않도록 -89 ~ 89도로 제한)
        if (pitch > 1.5f) pitch = 1.5f;  // 약 85도
        if (pitch < -1.5f) pitch = -1.5f;

        // 마우스를 다시 화면 중앙으로 강제 이동
        glutWarpPointer(centerX, centerY);

        // 화면 갱신 요청
        glutPostRedisplay();
    }
}

void KeyboardUp(unsigned char key, int x, int y) {
    // 키를 떼면 상태를 false로 변경
    keyState[key] = false;
}

int main(int argc, char** argv) {
    srand(static_cast<unsigned>(time(0)));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(width, height);
    glutCreateWindow("TP");

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패" << std::endl;
        return -1;
    }

    MakeVertexShaders();
    MakeFragmentShaders();
    shaderProgramID = MakeShaderProgram();

    LoadOBJ("cube.obj");
    InitBuffers();

    glutDisplayFunc(DrawScene);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);

    // [추가] 키를 뗐을 때를 감지하는 함수 등록
    glutKeyboardUpFunc(KeyboardUp);

    glutTimerFunc(16, Timer, 0);

    // [추가] 마우스 움직임 감지 함수 등록
    glutPassiveMotionFunc(MouseMotion);

    // [추가] 마우스 커서 숨기기 (게임처럼)
    // 필요하다면 특정 키(예: ESC)를 눌렀을 때 다시 보이게 할 수도 있음
    glutSetCursor(GLUT_CURSOR_NONE);

    // 마우스를 프로그램 시작 시 중앙으로 이동
    glutWarpPointer(width / 2, height / 2);

    glutMainLoop();
    return 0;
}

void DrawSteve() {
    // [렌더링 조건 설정]
    // 3인칭(viewMode == 3)일 때만 몸통, 머리, 다리를 그립니다.
    // 1인칭(viewMode == 1)일 때는 팔만 그립니다.
    bool drawBodyParts = (viewMode == 3);

    // 숨어있는 상태인지 확인 (hidingState가 2면 숨은 상태)
    bool isHidden = (hidingState == 2);

    glm::mat4 bodyMat = glm::mat4(1.0f);
    bodyMat = glm::translate(bodyMat, stevePos);
    bodyMat = glm::rotate(bodyMat, glm::radians(steveRotY), glm::vec3(0.0f, 1.0f, 0.0f));

    // 1. 몸통 (Body) - 3인칭일 때만 그림
    if (drawBodyParts) {
        glm::mat4 torsoScale = glm::scale(bodyMat, glm::vec3(0.5f, 0.7f, 0.25f));
        DrawCube(torsoScale, glm::vec3(0.0f, 0.0f, 0.8f));
    }

    // 2. 머리 (Head) - 3인칭일 때만 그림
    if (drawBodyParts) {
        glm::mat4 headMat = glm::translate(bodyMat, glm::vec3(0.0f, 0.55f, 0.0f));
        glm::mat4 headScale = glm::scale(headMat, glm::vec3(0.4f, 0.4f, 0.4f));
        DrawCube(headScale, glm::vec3(1.0f, 0.8f, 0.6f));
    }

    // --- 팔은 1인칭/3인칭 모두 그립니다 ---

    // 3. 왼쪽 팔 (Left Arm) - 항상 그림
    // (1인칭에서 팔이 흔들리는 모습을 보기 위해 조건문 없이 그립니다)
    {
        glm::mat4 lArmMat = glm::translate(bodyMat, glm::vec3(-0.35f, 0.35f, 0.0f));
        lArmMat = glm::rotate(lArmMat, glm::radians(armAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        lArmMat = glm::translate(lArmMat, glm::vec3(0.0f, -0.35f, 0.0f));
        glm::mat4 lArmScale = glm::scale(lArmMat, glm::vec3(0.2f, 0.7f, 0.2f));
        DrawCube(lArmScale, glm::vec3(1.0f, 0.8f, 0.6f));
    }

    // 4. 오른쪽 팔 (Right Arm) - 항상 그림
    // [중요] 숨어있을 때는 문 틈 시야 확보를 위해 팔을 내림
    glm::mat4 rArmMat = glm::translate(bodyMat, glm::vec3(0.35f, 0.35f, 0.0f));

    float currentRArmAngle = -armAngle;

    // 손전등을 켰거나 1인칭일 때 팔을 들어올림
    // 단, 숨어있을 때(!isHidden)는 들어올리지 않음!
    if ((isFlashlightOn || viewMode == 1) && !isHidden) {
        currentRArmAngle = -90.0f + (armAngle * 0.1f);
        if (viewMode == 1) {
            currentRArmAngle -= glm::degrees(pitch);
        }
    }

    rArmMat = glm::rotate(rArmMat, glm::radians(currentRArmAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rArmMat = glm::translate(rArmMat, glm::vec3(0.0f, -0.35f, 0.0f));

    glm::mat4 rArmScale = glm::scale(rArmMat, glm::vec3(0.2f, 0.7f, 0.2f));
    DrawCube(rArmScale, glm::vec3(1.0f, 0.8f, 0.6f));

    // ** 손전등 (Flashlight) **
    if (isFlashlightOn) {
        glm::mat4 lightMat = glm::translate(rArmMat, glm::vec3(0.0f, -0.5f, 0.0f));
        glm::mat4 lightModel = glm::scale(lightMat, glm::vec3(0.1f, 0.4f, 0.1f));
        DrawCube(lightModel, glm::vec3(0.1f, 0.1f, 0.1f));

        glm::vec4 lightWorldPos = lightMat * glm::vec4(0.0f, -0.5f, 0.0f, 1.0f);
        lightPos = glm::vec3(lightWorldPos);

        glm::vec3 dir = glm::normalize(glm::vec3(lightMat[1]));
        lightDirection = -dir;
    }

    // 5. 왼쪽 다리 (Left Leg) - 3인칭일 때만 그림
    if (drawBodyParts) {
        glm::mat4 lLegMat = glm::translate(bodyMat, glm::vec3(-0.15f, -0.35f, 0.0f));
        lLegMat = glm::rotate(lLegMat, glm::radians(legAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        lLegMat = glm::translate(lLegMat, glm::vec3(0.0f, -0.35f, 0.0f));
        glm::mat4 lLegScale = glm::scale(lLegMat, glm::vec3(0.2f, 0.7f, 0.2f));
        DrawCube(lLegScale, glm::vec3(0.2f, 0.2f, 0.5f));
    }

    // 6. 오른쪽 다리 (Right Leg) - 3인칭일 때만 그림
    if (drawBodyParts) {
        glm::mat4 rLegMat = glm::translate(bodyMat, glm::vec3(0.15f, -0.35f, 0.0f));
        rLegMat = glm::rotate(rLegMat, glm::radians(-legAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        rLegMat = glm::translate(rLegMat, glm::vec3(0.0f, -0.35f, 0.0f));
        glm::mat4 rLegScale = glm::scale(rLegMat, glm::vec3(0.2f, 0.7f, 0.2f));
        DrawCube(rLegScale, glm::vec3(0.2f, 0.2f, 0.5f));
    }
}

void DrawWardrobe() {
    // [원상복구] 옷장 위치를 다시 원래대로 (벽에서 떨어진 위치)
    float xFront = 4.0f;  // 옷장 앞면
    float xBack = 5.5f;   // 옷장 뒷면
    float zWidth = 1.5f;
    float yBase = -0.25f;

    glm::vec3 darkWood = glm::vec3(0.35f, 0.2f, 0.05f);
    glm::vec3 lightWood = glm::vec3(0.45f, 0.3f, 0.1f);
    glm::vec3 gold = glm::vec3(0.8f, 0.7f, 0.0f);

    // [원상복구] 상호작용 범위 시각화 (x=3.25 근처)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 원래 위치인 3.25로 되돌림
    glm::mat4 zone = glm::translate(glm::mat4(1.0f), glm::vec3(3.25f, -1.95f, 0.0f));
    glm::mat4 zoneScale = glm::scale(zone, glm::vec3(1.5f, 0.01f, 6.0f));
    DrawCube(zoneScale, glm::vec3(0.0f, 1.0f, 0.0f));

    glDisable(GL_BLEND);

    // --- (아래는 좌표 변수만 적용되므로 로직 동일) ---

    // 몸통
    glm::mat4 back = glm::translate(glm::mat4(1.0f), glm::vec3(xBack, yBase, 0.0f));
    glm::mat4 backScale = glm::scale(back, glm::vec3(0.1f, 3.5f, zWidth * 2.0f));
    DrawCube(backScale, darkWood);

    glm::mat4 left = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, yBase, -zWidth));
    glm::mat4 leftScale = glm::scale(left, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawCube(leftScale, darkWood);

    glm::mat4 right = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, yBase, zWidth));
    glm::mat4 rightScale = glm::scale(right, glm::vec3(1.5f, 3.5f, 0.1f));
    DrawCube(rightScale, darkWood);

    glm::mat4 top = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, 1.5f, 0.0f));
    glm::mat4 topScale = glm::scale(top, glm::vec3(1.5f, 0.1f, zWidth * 2.0f));
    DrawCube(topScale, darkWood);

    glm::mat4 bottom = glm::translate(glm::mat4(1.0f), glm::vec3((xFront + xBack) / 2.0f, -2.0f, 0.0f));
    glm::mat4 bottomScale = glm::scale(bottom, glm::vec3(1.5f, 0.1f, zWidth * 2.0f));
    DrawCube(bottomScale, darkWood);

    // 문
    glm::mat4 lHinge = glm::translate(glm::mat4(1.0f), glm::vec3(xFront, yBase, -zWidth));
    lHinge = glm::rotate(lHinge, glm::radians(-currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lDoor = glm::translate(lHinge, glm::vec3(0.0f, 0.0f, zWidth / 2.0f));
    glm::mat4 lDoorScale = glm::scale(lDoor, glm::vec3(0.1f, 3.4f, zWidth));
    DrawCube(lDoorScale, lightWood);

    glm::mat4 lKnob = glm::translate(lHinge, glm::vec3(-0.1f, 0.0f, zWidth - 0.2f));
    glm::mat4 knobScale = glm::scale(lKnob, glm::vec3(0.1f, 0.2f, 0.1f));
    DrawCube(knobScale, gold);

    glm::mat4 rHinge = glm::translate(glm::mat4(1.0f), glm::vec3(xFront, yBase, zWidth));
    rHinge = glm::rotate(rHinge, glm::radians(currentDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rDoor = glm::translate(rHinge, glm::vec3(0.0f, 0.0f, -zWidth / 2.0f));
    glm::mat4 rDoorScale = glm::scale(rDoor, glm::vec3(0.1f, 3.4f, zWidth));
    DrawCube(rDoorScale, lightWood);

    glm::mat4 rKnob = glm::translate(rHinge, glm::vec3(-0.1f, 0.0f, -zWidth + 0.2f));
    DrawCube(knobScale, gold);
}

// UI(스테미너 바) 그리기 함수
void DrawStaminaBar() {
    // 1. 깊이 테스트 비활성화 (UI가 항상 맨 위에 보이게 함)
    glDisable(GL_DEPTH_TEST);

    // 2. 직교 투영(Orthographic) 행렬 생성
    // 3D 원근감이 아닌, 2D 평면 좌표계(0 ~ width, 0 ~ height)를 사용
    glm::mat4 ortho = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
    glm::mat4 viewUI = glm::mat4(1.0f); // 뷰 행렬은 기본값(이동 없음)

    // 쉐이더에 2D용 행렬 전달
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewUI));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(ortho));

    // 조명 끄기 (UI는 빛의 영향을 받지 않아야 선명함)
    glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), 0);


    // --- [배경 바 (회색)] ---
    float barWidth = 300.0f;
    float barHeight = 20.0f;
    float xPos = 50.0f;           // 왼쪽 여백
    float yPos = height - 50.0f;  // 위쪽 여백 (화면 높이 - 50)

    // 위치: 바의 중심점 계산 (DrawCube는 중심 기준 1x1 크기임)
    glm::mat4 bgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + barWidth / 2, yPos, 0.0f));
    bgModel = glm::scale(bgModel, glm::vec3(barWidth, barHeight, 1.0f));
    DrawCube(bgModel, glm::vec3(0.3f, 0.3f, 0.3f)); // 회색 배경


    // --- [스테미너 바 (녹색)] ---
    float ratio = currentStamina / maxStamina; // 현재 비율 (0.0 ~ 1.0)
    float currentBarWidth = barWidth * ratio;  // 비율에 따른 너비

    // 위치: 줄어들 때 왼쪽 정렬이 되도록 중심점 이동
    // (배경의 왼쪽 끝 + 현재 너비의 절반)
    glm::mat4 fgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + currentBarWidth / 2, yPos, 0.0f));
    fgModel = glm::scale(fgModel, glm::vec3(currentBarWidth, barHeight, 1.0f));

    // 색상: 스테미너가 적으면 붉은색으로 변하게 연출 (선택사항)
    glm::vec3 barColor;
    if (ratio > 0.5f) barColor = glm::vec3(0.0f, 1.0f, 0.0f); // 녹색
    else if (ratio > 0.2f) barColor = glm::vec3(1.0f, 1.0f, 0.0f); // 노란색
    else barColor = glm::vec3(1.0f, 0.0f, 0.0f); // 빨간색

    DrawCube(fgModel, barColor);

    // 3. 깊이 테스트 다시 활성화 (다음 프레임 3D 렌더링을 위해)
    glEnable(GL_DEPTH_TEST);
}

void DrawScene() {
    // 1. 기본 설정
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(VAO);

    // 2. 카메라 설정
    glm::vec3 cameraPos;
    glm::vec3 cameraTarget;
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    if (viewMode == 3) {
        float camHeight = 2.0f;
        float camX = sin(cameraAngle) * cameraDistance;
        float camZ = cos(cameraAngle) * cameraDistance;
        cameraPos = stevePos + glm::vec3(camX, camHeight, camZ);
        cameraTarget = stevePos;
    }
    else {
        cameraPos = stevePos + glm::vec3(0.0f, 0.6f, 0.0f);
        glm::vec3 direction;
        direction.x = -sin(cameraAngle) * cos(pitch);
        direction.y = sin(pitch);
        direction.z = -cos(cameraAngle) * cos(pitch);
        cameraTarget = cameraPos + glm::normalize(direction);
    }

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
    GLint lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
    GLint lightOnLoc = glGetUniformLocation(shaderProgramID, "lightOn");
    GLint lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
    GLint lightDirLoc = glGetUniformLocation(shaderProgramID, "lightDir");
    GLint cutOffLoc = glGetUniformLocation(shaderProgramID, "cutOff");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
    glUniform1i(lightOnLoc, isFlashlightOn ? 1 : 0);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(lightDirLoc, lightDirection.x, lightDirection.y, lightDirection.z);
    glUniform1f(cutOffLoc, glm::cos(glm::radians(25.0f)));

    DrawSteve();
    DrawWardrobe();

    // --- [방 크기 확장: 10x10 -> 20x20] ---
    glm::vec3 wallColor = glm::vec3(0.6f, 0.6f, 0.6f);

    // (1) 앞쪽 벽 (Front) - z = -10
    glm::mat4 frontWall = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
    frontWall = glm::scale(frontWall, glm::vec3(20.0f, 10.0f, 1.0f));
    DrawCube(frontWall, wallColor);

    // (2) 뒤쪽 벽 (Back) - z = 10
    glm::mat4 backWall = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 10.0f));
    backWall = glm::scale(backWall, glm::vec3(20.0f, 10.0f, 1.0f));
    DrawCube(backWall, wallColor);

    // (3) 왼쪽 벽 (Left) - x = -10
    glm::mat4 leftWall = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.0f, 0.0f));
    leftWall = glm::scale(leftWall, glm::vec3(1.0f, 10.0f, 20.0f));
    DrawCube(leftWall, wallColor);

    // (4) 오른쪽 벽 (Right) - x = 10
    glm::mat4 rightWall = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));
    rightWall = glm::scale(rightWall, glm::vec3(1.0f, 10.0f, 20.0f));
    DrawCube(rightWall, wallColor);

    // (5) 바닥 (Floor)
    glm::mat4 floorMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    floorMat = glm::scale(floorMat, glm::vec3(20.0f, 0.1f, 20.0f));
    DrawCube(floorMat, glm::vec3(0.3f, 0.35f, 0.3f));

    // (6) 천장 (Ceiling)
    glm::mat4 ceilMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));
    ceilMat = glm::scale(ceilMat, glm::vec3(20.0f, 0.1f, 20.0f));
    DrawCube(ceilMat, glm::vec3(0.4f, 0.4f, 0.4f));

    // [추가됨] UI(스테미너 바) 그리기 - 반드시 3D 물체를 다 그린 뒤에 호출!
    DrawStaminaBar();

    glutSwapBuffers();
}

void Timer(int value) {
    // ==========================================================
    // 1. 캐릭터 이동 & 스테미너 로직
    // ==========================================================
    if (hidingState == 0) {
        float inputZ = 0.0f; float inputX = 0.0f;
        if (keyState['w']) inputZ -= 1.0f;
        if (keyState['s']) inputZ += 1.0f;
        if (keyState['a']) inputX -= 1.0f;
        if (keyState['d']) inputX += 1.0f;

        bool isMoving = (inputX != 0.0f || inputZ != 0.0f);

        if (isMoving) {
            // 대각선 이동 시 속도 보정
            float length = sqrt(inputX * inputX + inputZ * inputZ);
            if (length > 0) { inputX /= length; inputZ /= length; }

            // 카메라 각도에 따른 이동 방향 계산
            float rad = cameraAngle;
            float moveDirX = inputX * cos(rad) + inputZ * sin(rad);
            float moveDirZ = -inputX * sin(rad) + inputZ * cos(rad);

            // [수정된 달리기 및 스테미너 로직]
            float currentSpeed = 0.1f; // 기본 걷기 속도

            // 조건 1: 스페이스바 누름
            // 조건 2: 지친 상태(isExhausted)가 아님
            // -> 이럴 때만 달리기 가능
            if (keyState[' '] && !isExhausted) {
                currentSpeed = 0.15f;
                currentAnim = RUN;

                // 달리는 중에는 스테미너 감소
                currentStamina -= 1.0f;

                // 스테미너가 바닥나면 지침 상태 발동
                if (currentStamina <= 0.0f) {
                    currentStamina = 0.0f;
                    isExhausted = true; // 이제 일정량 찰 때까지 못 달림
                }
            }
            else {
                // 스페이스바를 안 눌렀거나, 지쳐서 못 달리는 경우 -> 걷기
                currentAnim = WALK;

                // 걸을 때는 스테미너가 천천히 회복됨
                if (currentStamina < maxStamina) {
                    currentStamina += 0.2f;
                }
            }

            // 위치 적용
            stevePos.x += moveDirX * currentSpeed;
            stevePos.z += moveDirZ * currentSpeed;
        }
        else {
            // 움직이지 않을 때(IDLE)
            currentAnim = IDLE;

            // 멈춰있을 때는 스테미너가 천천히 회복됨
            if (currentStamina < maxStamina) {
                currentStamina += 0.2f;
            }
        }
    }
    else {
        // 숨어있을 때 (움직임 불가)
        currentAnim = IDLE;
        // 숨어있을 때도 회복
        if (currentStamina < maxStamina) {
            currentStamina += 1.0f;
        }
    }

    // [중요] 지침 상태 해제 로직
    // 지친 상태(true)라면, 스테미너가 20 이상 찼을 때만 다시 달릴 수 있게 해제
    if (isExhausted && currentStamina > 20.0f) {
        isExhausted = false;
    }

    // 스테미너 최대치 고정
    if (currentStamina > maxStamina) currentStamina = maxStamina;

    // 캐릭터 회전 (카메라 방향 등지기)
    steveRotY = glm::degrees(cameraAngle) + 180.0f;


    // ==========================================================
    // 2. 옷장 시퀀스 로직
    // ==========================================================
    float targetAngle = 0.0f;
    float doorSpeed = 5.0f;

    switch (hidingState) {
    case 0: // 평상시
        targetAngle = 0.0f;
        break;

    case 1: // 들어가는 중
        targetAngle = 90.0f;
        if (currentDoorAngle >= 90.0f) {
            // 옷장 안쪽 좌표
            stevePos = glm::vec3(4.8f, -1.0f, 0.0f);
            cameraAngle = glm::radians(90.0f);
            pitch = 0.0f;
            viewMode = 1;
            isFlashlightOn = false;
            hidingState = 2;
        }
        break;

    case 2: // 숨어있음
        targetAngle = 25.0f;
        viewMode = 1;
        stevePos = glm::vec3(4.8f, -1.0f, 0.0f); // 위치 고정
        break;

    case 3: // 나오는 중
        targetAngle = 90.0f;
        if (currentDoorAngle >= 90.0f) {
            // 옷장 앞 좌표
            stevePos = glm::vec3(3.0f, -1.0f, 0.0f);
            hidingState = 0;
        }
        break;
    }

    // 문 애니메이션
    if (abs(currentDoorAngle - targetAngle) > 1.0f) {
        if (currentDoorAngle < targetAngle) currentDoorAngle += doorSpeed;
        else currentDoorAngle -= doorSpeed;
    }
    else {
        currentDoorAngle = targetAngle;
    }


    // ==========================================================
    // 3. 팔/다리 애니메이션 계산
    // ==========================================================
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

    glutPostRedisplay();
    glutTimerFunc(16, Timer, 0);
}

void Keyboard(unsigned char key, int x, int y) {
    keyState[key] = true;

    switch (key) {
    case '1': viewMode = 1; break;
    case '3': viewMode = 3; break;
    case 'e': isFlashlightOn = !isFlashlightOn; break;
    case 'j': cameraAngle += glm::radians(10.0f); break;
    case 'k': cameraDistance += 0.5f; break;
    case 'K': if (cameraDistance > 2.0f) cameraDistance -= 0.5f; break;

    case 'f':
        if (hidingState == 0) {
            // [원상복구] 상호작용 범위: x > 2.5
            if (stevePos.x > 2.5f && abs(stevePos.z) < 3.0f) {
                hidingState = 1;
            }
        }
        else if (hidingState == 2) {
            hidingState = 3;
        }
        break;

    case 'q': exit(0); break;
    }
}

// ... 나머지 함수들 (유지) ...
void Reshape(int w, int h) {
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

char* filetobuf(const char* file) {
    FILE* fptr;
    fopen_s(&fptr, file, "rb");
    if (!fptr) return NULL;
    fseek(fptr, 0, SEEK_END);
    long length = ftell(fptr);
    char* buf = (char*)malloc(length + 1);
    fseek(fptr, 0, SEEK_SET);
    fread(buf, length, 1, fptr);
    fclose(fptr);
    buf[length] = 0;
    return buf;
}

void LoadOBJ(const char* filename) {
    read_obj_file(filename, &model);
}

void MakeVertexShaders() {
    GLchar* vertexSource = filetobuf("TPvertex.glsl");
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    free(vertexSource);
}

void MakeFragmentShaders() {
    GLchar* fragmentSource = filetobuf("TPfragment.glsl");
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    free(fragmentSource);
}

GLuint MakeShaderProgram() {
    GLuint shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderID);
    return shaderID;
}

void InitBuffers() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    if (model.vertex_count > 0) {
        std::vector<GLfloat> vData;
        std::vector<GLuint> iData;
        for (size_t i = 0; i < model.vertex_count; i++) {
            vData.push_back(model.vertices[i].x);
            vData.push_back(model.vertices[i].y);
            vData.push_back(model.vertices[i].z);
        }
        for (size_t i = 0; i < model.face_count; i++) {
            iData.push_back(model.faces[i].v1);
            iData.push_back(model.faces[i].v2);
            iData.push_back(model.faces[i].v3);
        }
        glBufferData(GL_ARRAY_BUFFER, vData.size() * sizeof(GLfloat), vData.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, iData.size() * sizeof(GLuint), iData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
    }
}