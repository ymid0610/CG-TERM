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
#include <stdlib.h>sd
#include <stdio.h>

// 사용자 정의 헤더 파일 포함
#include "ReadObjFile.h"
#include "map.h"
#include "player.h" // [핵심] 분리한 플레이어 클래스 포함

// ---------- 사용자 정의 ---------- 
#define WINDOW_TITLE "Maze Escape" 
#define WINDOW_WIDTH 1600 
#define WINDOW_HEIGHT 900 
#define WINDOW_START 0, 0 
#define TIMER_INTERVAL 16 
#define BACKGROUND_COLOR 0.1f, 0.1f, 0.1f 

// --- 전역 변수 ---
GLint width = WINDOW_WIDTH, height = WINDOW_HEIGHT;
GLuint shaderProgramID, vertexShader, fragmentShader;
GLuint VAO, VBO, EBO;

// 객체 인스턴스
Model model;     // cube.obj 데이터
MazeMap maze;    // 미로 맵 데이터
Player player;   // [핵심] 플레이어 객체 (모든 상태값 포함)
bool isGameClear = false;

// 조명 관련 변수 (기본값)
glm::vec3 lightPos(0.0f, 2.0f, 0.0f);
glm::vec3 lightDirection(0.0f, 0.0f, -1.0f);

// 키보드 입력 상태 관리
bool keyState[256] = { false };

// --- 함수 선언 ---
void glewInitCheck();
void MakeVertexShaders();
void MakeFragmentShaders();
void InitBuffers();
GLuint MakeShaderProgram();
GLvoid DrawScene();
GLvoid Reshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void KeyboardUp(unsigned char key, int x, int y);
void Timer(int value);
void MouseMotion(int x, int y);
void LoadOBJ(const char* filename);
char* filetobuf(const char* file);
void DrawCube(glm::mat4 modelMat, glm::vec3 color); // 헬퍼 함수
void DrawStaminaBar(); // UI 그리기 함수

// ---------- 메인 함수 ----------
int main(int argc, char** argv) {
    srand(static_cast<unsigned>(time(0)));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(WINDOW_START);
    glutInitWindowSize(width, height);
    glutCreateWindow(WINDOW_TITLE);

    glewInitCheck();

    MakeVertexShaders();
    MakeFragmentShaders();
    shaderProgramID = MakeShaderProgram();

    LoadOBJ("cube.obj");

    InitBuffers();

    // 초기화 완료 후 메인 루프 시작
    glutDisplayFunc(DrawScene);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutKeyboardUpFunc(KeyboardUp);
    glutTimerFunc(16, Timer, 0);
    glutPassiveMotionFunc(MouseMotion);

    // 마우스 커서 숨기기 및 중앙 이동
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(width / 2, height / 2);

    glutMainLoop();
    return 0;
}

void DrawScene() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(VAO);

    // [수정] Player 객체에서 카메라 정보 가져오기
    glm::vec3 cameraPos = player.GetCameraPos();
    glm::vec3 cameraTarget = player.GetCameraTarget();
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    // 쉐이더 유니폼 전송
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
    GLint lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
    GLint lightOnLoc = glGetUniformLocation(shaderProgramID, "lightOn");
    GLint lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
    GLint lightDirLoc = glGetUniformLocation(shaderProgramID, "lightDir");
    GLint cutOffLoc = glGetUniformLocation(shaderProgramID, "cutOff");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    // [조명 업데이트] 손전등이 켜져있으면 플레이어 시선 방향으로 조명 설정
    if (player.isFlashlightOn) {
        lightPos = player.GetCameraPos(); // 눈 위치
        lightDirection = glm::normalize(player.GetCameraTarget() - player.GetCameraPos());
    }

    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
    glUniform1i(lightOnLoc, player.isFlashlightOn ? 1 : 0); // Player 상태 사용
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(lightDirLoc, lightDirection.x, lightDirection.y, lightDirection.z);
    glUniform1f(cutOffLoc, glm::cos(glm::radians(25.0f)));

    // 1. 플레이어 그리기 (DrawCube 함수 전달)
    player.Draw(shaderProgramID, model, DrawCube);

    // 2. 미로 그리기
    maze.Draw(shaderProgramID, DrawCube);

    // 3. 바닥 (Floor) - 미로 전체 크기만큼
    glm::mat4 floorMat = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, -2.0f, 10.0f));
    floorMat = glm::scale(floorMat, glm::vec3(20.0f, 0.1f, 20.0f));
    DrawCube(floorMat, glm::vec3(0.3f, 0.35f, 0.3f));

    // 4. 천장 (Ceiling)
    glm::mat4 ceilMat = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 3.0f, 10.0f));
    ceilMat = glm::scale(ceilMat, glm::vec3(20.0f, 0.1f, 20.0f));
    DrawCube(ceilMat, glm::vec3(0.4f, 0.4f, 0.4f));

    // 5. UI 그리기
    DrawStaminaBar();

    glutSwapBuffers();
}

void Timer(int value) {
    // 게임 클리어 시 멈춤
    if (isGameClear) {
        glutPostRedisplay();
        glutTimerFunc(16, Timer, 0);
        return;
    }

    // [핵심] 플레이어 업데이트 (키 입력과 맵 정보 전달)
    player.Update(keyState, maze);

    // 승리 조건 체크
    if (maze.CheckVictory(player.pos.x, player.pos.z)) {
        std::cout << "Game Clear!" << std::endl;
        isGameClear = true;
    }

    glutPostRedisplay();
    glutTimerFunc(16, Timer, 0);
}

void Keyboard(unsigned char key, int x, int y) {
    keyState[key] = true;

    // 플레이어 제어 및 시스템 키
    switch (key) {
    case '1': player.viewMode = 1; break; // Player 객체 변수 수정
    case '3': player.viewMode = 3; break;
    case 'e': player.isFlashlightOn = !player.isFlashlightOn; break;
    case 'j': player.cameraAngle += glm::radians(10.0f); break;
    case 'k': player.cameraDistance += 0.5f; break;
    case 'K': if (player.cameraDistance > 2.0f) player.cameraDistance -= 0.5f; break;
    case 'q': exit(0); break;
    }
}

void MouseMotion(int x, int y) {
    // 마우스 처리를 Player 클래스에 위임
    player.ProcessMouse(x, y, width, height);

    // 1인칭일 때만 마우스 포인터 중앙 고정
    if (player.viewMode == 1) {
        glutWarpPointer(width / 2, height / 2);
        glutPostRedisplay();
    }
}

void KeyboardUp(unsigned char key, int x, int y) {
    keyState[key] = false;
}

void Reshape(int w, int h) {
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

// 헬퍼 함수: 큐브 그리기 (Player와 MazeMap에 함수 포인터로 전달됨)
void DrawCube(glm::mat4 modelMat, glm::vec3 color) {
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint faceColorLoc = glGetUniformLocation(shaderProgramID, "faceColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3f(faceColorLoc, color.r, color.g, color.b);

    if (model.face_count > 0)
        glDrawElements(GL_TRIANGLES, model.face_count * 3, GL_UNSIGNED_INT, 0);
}

// UI(스테미너 바) 그리기
void DrawStaminaBar() {
    glDisable(GL_DEPTH_TEST); // UI는 맨 위에 그려야 함

    // 2D 렌더링을 위한 직교 투영 설정
    glm::mat4 ortho = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
    glm::mat4 viewUI = glm::mat4(1.0f);

    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewUI));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(ortho));

    // UI에는 조명 효과 끄기
    glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), 0);

    // [배경 바]
    float barWidth = 300.0f;
    float barHeight = 20.0f;
    float xPos = 50.0f;
    float yPos = height - 50.0f;

    glm::mat4 bgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + barWidth / 2, yPos, 0.0f));
    bgModel = glm::scale(bgModel, glm::vec3(barWidth, barHeight, 1.0f));
    DrawCube(bgModel, glm::vec3(0.3f, 0.3f, 0.3f));

    // [스테미너 게이지] - Player 객체의 데이터를 가져와 사용
    float ratio = player.currentStamina / player.maxStamina;
    float currentBarWidth = barWidth * ratio;

    glm::mat4 fgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + currentBarWidth / 2, yPos, 0.0f));
    fgModel = glm::scale(fgModel, glm::vec3(currentBarWidth, barHeight, 1.0f));

    // 색상 결정 (초록 -> 노랑 -> 빨강)
    glm::vec3 barColor;
    if (ratio > 0.5f) barColor = glm::vec3(0.0f, 1.0f, 0.0f);
    else if (ratio > 0.2f) barColor = glm::vec3(1.0f, 1.0f, 0.0f);
    else barColor = glm::vec3(1.0f, 0.0f, 0.0f);

    DrawCube(fgModel, barColor);

    glEnable(GL_DEPTH_TEST); // 3D 렌더링을 위해 다시 켜기
}

// ---------- 초기화 함수 정의 ----------
void glewInitCheck() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl; exit(-1);
    }
}

void LoadOBJ(const char* filename) {
	read_obj_file(filename, &model);
}

//  ---------- 파일을 버퍼로 읽어들이는 함수 ----------
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

// ---------- 셰이더 생성 함수 ----------
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