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
#include "map.h"
#include "player.h"
#include "wardrobe.h" 
#include "TestGhost.h"
#include "SoundManager.h"

// --- 전역 변수 ---
GLint width = 1600, height = 900;
GLuint shaderProgramID, vertexShader, fragmentShader;
GLuint VAO, VBO, EBO;

// 객체 인스턴스
Model model;     // cube.obj 데이터
MazeMap maze;    // 미로 맵 데이터
Player player;   // 플레이어 객체

// 옷장 벡터
std::vector<Wardrobe> wardrobes; 

// 유령 벡터
std::vector<Ghost> ghosts;

//사운드 매니저
SoundManager soundManager;

// 유령 BGM 재생 플래그
bool isGhostBGM = false;

// 유령충돌 플래그
bool isHit = false; 

// 게임 클리어 플래그
bool isGameClear = false;

// 조명 관련 변수
glm::vec3 lightPos(0.0f, 2.0f, 0.0f);
glm::vec3 lightDirection(0.0f, 0.0f, -1.0f);
#define MAIN_LIGHT 0.2f
#define GHOST_LIGHT 0.2f
float ambient = MAIN_LIGHT;

// 키보드 입력 상태 관리
bool keyState[256] = { false };


// --- 함수 선언 ---
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

// 큐브 그리기 함수
void DrawCube(glm::mat4 modelMat, glm::vec3 color) {
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint faceColorLoc = glGetUniformLocation(shaderProgramID, "faceColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3f(faceColorLoc, color.r, color.g, color.b);

    if (model.face_count > 0)
        glDrawElements(GL_TRIANGLES, model.face_count * 3, GL_UNSIGNED_INT, 0);
}

// 시작 화면용 전역 변수 및 함수
bool isGameStart = false; 
Ghost titleGhost(glm::vec3(5.0f, -0.5f, 5.0f)); // 타이틀 화면용 유령
float titleTime = 0.0f; // 애니메이션 시간

// 글자 그리기 함수
void DrawBitmapChar(const int* bitmap, glm::vec3 startPos, glm::vec3 color) {
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            if (bitmap[y * 5 + x] == 1) {
                glm::mat4 modelMat = glm::mat4(1.0f);
                modelMat = glm::translate(modelMat, startPos + glm::vec3(x * 0.5f, (4 - y) * 0.5f, 0.0f));
                modelMat = glm::scale(modelMat, glm::vec3(0.4f, 0.4f, 0.4f));
                DrawCube(modelMat, color);
            }
        }
    }
}

// 텍스트 비트맵 데이터
int bmp_M[] = { 1,0,0,0,1, 1,1,0,1,1, 1,0,1,0,1, 1,0,0,0,1, 1,0,0,0,1 };
int bmp_A[] = { 0,1,1,1,0, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1 };
int bmp_Z[] = { 1,1,1,1,1, 0,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0, 1,1,1,1,1 };
int bmp_E[] = { 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,0, 1,0,0,0,0, 1,1,1,1,1 };
int bmp_S[] = { 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1 };
int bmp_C[] = { 1,1,1,1,1, 1,0,0,0,0, 1,0,0,0,0, 1,0,0,0,0, 1,1,1,1,1 };
int bmp_P[] = { 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,0, 1,0,0,0,0 };
int bmp_L[] = { 1,0,0,0,0, 1,0,0,0,0, 1,0,0,0,0, 1,0,0,0,0, 1,1,1,1,1 };
int bmp_R[] = { 1,1,1,1,0, 1,0,0,0,1, 1,1,1,1,0, 1,0,1,0,0, 1,0,0,1,1 };
int bmp_Excl[] = { 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,0,0,0, 0,0,1,0,0 }; // 느낌표(!)


void InitWardrobes(int count) {
    wardrobes.clear();
    std::random_device rd;
    std::mt19937 gen(rd());

    // 맵 내부 좌표 (1 ~ MAP_SIZE-2)
    std::uniform_int_distribution<> dis(1, MAP_SIZE - 2);

    int placedCount = 0;
    int attempts = 0;

    // 옷장을 중심에서 얼마만큼 밀어야 벽에 딱 붙는지 계산
    float offset = (WALL_SIZE / 2.0f) - 0.8f;

    while (placedCount < count && attempts < 1000) {
        attempts++;
        int x = dis(gen);
        int z = dis(gen);

        // 현재 위치가 길(0)이어야 됨
        if (maze.mapData[z][x] != 0) continue;

        // 이미 옷장이 있는지 체크 (겹침 방지)
        bool alreadyExists = false;
        for (auto& w : wardrobes) {
            if (glm::distance(glm::vec3(x * WALL_SIZE, 0, z * WALL_SIZE), glm::vec3(w.GetPos().x, 0, w.GetPos().z)) < 2.0f)
                alreadyExists = true;
        }
        if (alreadyExists) continue;

        // 4방향을 검사하여 벽을 찾음
        float wx = x * WALL_SIZE;
        float wz = z * WALL_SIZE;
        bool placed = false;

        // [오른쪽(X+1)이 벽] -> 옷장 뒤(+X)가 오른쪽을 봐야 함 (Rot: 0도)
        if (maze.mapData[z][x + 1] == 1) {
            wardrobes.emplace_back(wx + offset, wz, 0.0f);
            placed = true;
        }
        // [왼쪽(X-1)이 벽] -> 옷장 뒤가 왼쪽(-X)을 봐야 함 (Rot: 180도)
        else if (maze.mapData[z][x - 1] == 1) {
            wardrobes.emplace_back(wx - offset, wz, 180.0f);
            placed = true;
        }
        // [아래(Z+1)가 벽] -> 옷장 뒤가 아래(+Z)를 봐야 함 (Rot: -90도)
        // GLM 회전: -90도 회전 시 Local +X 가 World +Z가 됨
        else if (maze.mapData[z + 1][x] == 1) {
            wardrobes.emplace_back(wx, wz + offset, -90.0f);
            placed = true;
        }
        // [위(Z-1)가 벽] -> 옷장 뒤가 위(-Z)를 봐야 함 (Rot: 90도)
        // GLM 회전: 90도 회전 시 Local +X 가 World -Z가 됨
        else if (maze.mapData[z - 1][x] == 1) {
            wardrobes.emplace_back(wx, wz - offset, 90.0f);
            placed = true;
        }

        if (placed) placedCount++;
    }
    std::cout << "옷장 " << placedCount << "개 생성 완료." << std::endl;
}



// UI(HP 바) 그리기 - 우상단
void DrawHPBar() {
    glDisable(GL_DEPTH_TEST); // 3D 깊이 테스트 끄기

    // 2D 투영 행렬 설정
    glm::mat4 ortho = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
    glm::mat4 viewUI = glm::mat4(1.0f);

    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewUI));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(ortho));

    // 조명 끄기
    glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), 0);

    // 설정 값
    float barWidth = 300.0f;
    float barHeight = 20.0f;
    float margin = 50.0f;

    // 우상단 좌표 계산
    float xPos = width - barWidth - margin;
    float yPos = height - margin;

    // 배경 바 (회색)
    glm::mat4 bgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + barWidth / 2, yPos, 0.0f));
    bgModel = glm::scale(bgModel, glm::vec3(barWidth, barHeight, 1.0f));
    DrawCube(bgModel, glm::vec3(0.3f, 0.3f, 0.3f));

    // HP 게이지 (빨간색)
    float ratio = (float)player.hp / 100.0f;
    if (ratio < 0.0f) ratio = 0.0f; // 음수 방지

    float currentBarWidth = barWidth * ratio;

    // 게이지 그리기
    // 게이지의 중심 = 시작점(xPos) + (현재 길이 / 2)
    glm::mat4 fgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + currentBarWidth / 2, yPos, 0.0f));
    fgModel = glm::scale(fgModel, glm::vec3(currentBarWidth, barHeight, 1.0f));

    DrawCube(fgModel, glm::vec3(1.0f, 0.0f, 0.0f)); // 빨간색

    glEnable(GL_DEPTH_TEST); // 깊이 테스트 다시 켜기
}

// UI(스테미너 바) 그리기 - 좌상단
void DrawStaminaBar() {
    glDisable(GL_DEPTH_TEST);

    glm::mat4 ortho = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
    glm::mat4 viewUI = glm::mat4(1.0f);

    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewUI));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(ortho));

    glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), 0);

	// 배경 바 (회색)
    float barWidth = 300.0f;
    float barHeight = 20.0f;
    float xPos = 50.0f;
    float yPos = height - 50.0f;

    glm::mat4 bgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + barWidth / 2, yPos, 0.0f));
    bgModel = glm::scale(bgModel, glm::vec3(barWidth, barHeight, 1.0f));
    DrawCube(bgModel, glm::vec3(0.3f, 0.3f, 0.3f));

    // [스테미너 게이지]
    float ratio = player.currentStamina / player.maxStamina;
    float currentBarWidth = barWidth * ratio;

    glm::mat4 fgModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos + currentBarWidth / 2, yPos, 0.0f));
    fgModel = glm::scale(fgModel, glm::vec3(currentBarWidth, barHeight, 1.0f));

    glm::vec3 barColor;
    if (ratio > 0.5f) barColor = glm::vec3(0.0f, 1.0f, 0.0f);
    else if (ratio > 0.2f) barColor = glm::vec3(1.0f, 1.0f, 0.0f);
    else barColor = glm::vec3(1.0f, 0.0f, 0.0f);

    DrawCube(fgModel, barColor);

    glEnable(GL_DEPTH_TEST);
}

void DrawScene() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(VAO);

    // 시작 화면 그리기
    if (!isGameStart) {
        // 카메라 고정
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 22.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "proj"), 1, GL_FALSE, glm::value_ptr(proj));

        // 조명 켜기
        glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), 0);
        glUniform3f(glGetUniformLocation(shaderProgramID, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgramID, "ambientStrength"), 1.0f); // 밝게

        // 텍스트 그리기
        glm::vec3 cWhite(1.0f, 1.0f, 1.0f);

        DrawBitmapChar(bmp_M, glm::vec3(-4.5f, 3.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_A, glm::vec3(-1.5f, 3.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_Z, glm::vec3(1.5f, 3.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_E, glm::vec3(4.5f, 3.5f, 0.0f), cWhite);

        DrawBitmapChar(bmp_E, glm::vec3(-7.5f, -0.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_S, glm::vec3(-4.5f, -0.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_C, glm::vec3(-1.5f, -0.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_A, glm::vec3(1.5f, -0.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_P, glm::vec3(4.5f, -0.5f, 0.0f), cWhite);
        DrawBitmapChar(bmp_E, glm::vec3(7.5f, -0.5f, 0.0f), cWhite);

        // 타이틀 유령 그리기
        titleGhost.Draw(shaderProgramID, model);

        glutSwapBuffers();
        return;
    }

    if (isGameClear) {
        // 카메라 고정
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 18.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "proj"), 1, GL_FALSE, glm::value_ptr(proj));

        // 조명 밝게 설정
        glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), 0);
        glUniform3f(glGetUniformLocation(shaderProgramID, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgramID, "ambientStrength"), 1.0f);

        // 텍스트 그리기
        // 노란색
        glm::vec3 cYellow(1.0f, 1.0f, 0.0f);

        // 좌표 계산
        DrawBitmapChar(bmp_C, glm::vec3(-7.5f, 0.0f, 0.0f), cYellow);
        DrawBitmapChar(bmp_L, glm::vec3(-4.5f, 0.0f, 0.0f), cYellow);
        DrawBitmapChar(bmp_E, glm::vec3(-1.5f, 0.0f, 0.0f), cYellow);
        DrawBitmapChar(bmp_A, glm::vec3(1.5f, 0.0f, 0.0f), cYellow);
        DrawBitmapChar(bmp_R, glm::vec3(4.5f, 0.0f, 0.0f), cYellow);

        DrawBitmapChar(bmp_Excl, glm::vec3(7.5f, 0.0f, 0.0f), cYellow);
        DrawBitmapChar(bmp_Excl, glm::vec3(9.0f, 0.0f, 0.0f), cYellow);
        DrawBitmapChar(bmp_Excl, glm::vec3(10.5f, 0.0f, 0.0f), cYellow);

        glutSwapBuffers();
        return; 
    }

    glm::vec3 cameraPos = player.GetCameraPos();
    glm::vec3 cameraTarget = player.GetCameraTarget();
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	if (player.viewMode == 3) {
		cameraUp = glm::vec3(0.0f, 0.0f, -1.0f);
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
    GLint ambientStrengthLoc = glGetUniformLocation(shaderProgramID, "ambientStrength");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    if (player.isFlashlightOn) {
        lightPos = player.GetCameraPos();
        lightDirection = glm::normalize(player.GetCameraTarget() - player.GetCameraPos());
    }

    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
    glUniform1i(lightOnLoc, player.isFlashlightOn ? 1 : 0);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(lightDirLoc, lightDirection.x, lightDirection.y, lightDirection.z);
    glUniform1f(cutOffLoc, glm::cos(glm::radians(25.0f)));
	glUniform1f(ambientStrengthLoc, ambient);

    // 플레이어 그리기
    player.Draw(shaderProgramID, model, DrawCube);

    // 미로 그리기
    maze.Draw(shaderProgramID, DrawCube);

    // 옷장 그리기
    for (auto& w : wardrobes) {
        w.Draw(shaderProgramID, model);
    }

    // 바닥 그리기
    glm::mat4 floorMat = glm::mat4(1.0f);
    floorMat = glm::translate(floorMat, glm::vec3((MAP_SIZE * WALL_SIZE) / 2 - WALL_SIZE / 2, -1.5f, (MAP_SIZE * WALL_SIZE) / 2 - WALL_SIZE / 2));
    floorMat = glm::scale(floorMat, glm::vec3(MAP_SIZE * WALL_SIZE, 0.1f, MAP_SIZE * WALL_SIZE));
    DrawCube(floorMat, glm::vec3(0.2f, 0.2f, 0.2f));

	glUniform1f(ambientStrengthLoc, ambient + GHOST_LIGHT); // 유령 주변 밝게
    for (auto& g : ghosts) {
        g.Draw(shaderProgramID, model);
    }

    // UI
    DrawStaminaBar(); // 좌상단
    DrawHPBar();      // 우상단

    glutSwapBuffers();
}

void Timer(int value) {

    if (!isGameStart) {
        // 유령 애니메이션 변수 업데이트
        titleGhost.wobbleTime += 0.1f;
        titleGhost.floatTime += 0.05f;
        float yPos = -5.0f + sin(titleGhost.floatTime) * 0.5f;

        // 위치 설정
        titleGhost.SetPos(glm::vec3(0.0f, yPos, 0.0f));

        glutPostRedisplay();
        glutTimerFunc(16, Timer, 0);
        return;
    }

    maze.UpdateBreakWalls(3);

    bool insideAny = false;
    Wardrobe* activeWardrobe = nullptr;


    if (!isGameClear) {
        // 모든 옷장 업데이트
        for (auto& w : wardrobes) {
            w.Update(player.pos, player.cameraAngle, player.pitch, player.viewMode, player.isFlashlightOn);
            if (w.GetState() != STATE_OUTSIDE) {
                insideAny = true;
                activeWardrobe = &w;
            }
        }

        if (!insideAny) {
            player.Update(keyState, maze);
        }
        else {
            player.currentAnim = IDLE;
            if (player.currentStamina < player.maxStamina) player.currentStamina += 0.5f;
        }

        // --- 유령(Ghost) 여러 마리 업데이트 ---
        isHit = false;
        float minDist = 1e9f;
        for (auto& g : ghosts) {
            bool hit = g.Update(player.pos, maze, player.isHide);
            if (hit) isHit = true;
            float dist = glm::distance(player.pos, g.GetPos());
            if (dist < minDist) minDist = dist;
        }

        // 감지 거리 설정
        float detectRange = 15.0f;
        if (minDist < detectRange) {
            if (!isGhostBGM) {
                soundManager.PlayBGM("NearByGhost.mp3");
                soundManager.SetBGMVolume(300);
                isGhostBGM = true;
            }
            // 거리 비례 밝기 어둡게
            ambient = MAIN_LIGHT * (minDist / detectRange);
        }
        else {
            if (isGhostBGM) {
                soundManager.PlayBGM("Dead_Silence_Soundtrack.mp3");
                soundManager.SetBGMVolume(300);
                isGhostBGM = false;
            }
            ambient = MAIN_LIGHT;
        }

        if (isHit) {
            soundManager.PlaySFX("HitGhost.mp3");
            soundManager.SetSFXVolume(500);

            glm::vec3 randPos = maze.GetRandomOpenPos();
            player.pos.x = randPos.x;
            player.pos.z = randPos.z;

            player.hp -= 10;
            std::cout << "유령과 충돌!" << std::endl;
            std::cout << "남은체력 : " << player.hp << std::endl;
            if (player.hp <= 0) {
                exit(0);
            }
            isHit = false;
        }

        if (maze.CheckVictory(player.pos.x, player.pos.z)) {
            std::cout << "Game Clear!" << std::endl;
            isGameClear = true;
        }
    
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, Timer, 0);
}

void Keyboard(unsigned char key, int x, int y) {

    // 엔터키 입력 시 게임 시작 및 초기화
    if (!isGameStart) {
        if (key == 13) { // Enter Key
            isGameStart = true;
            std::cout << "GAME START!" << std::endl;

            InitWardrobes(10);
            ghosts.clear(); // 중복 방지
            ghosts.emplace_back(glm::vec3((MAP_SIZE - 1) * WALL_SIZE, -1.0f, (MAP_SIZE - 2) * WALL_SIZE));
            ghosts.emplace_back(glm::vec3(1 * WALL_SIZE, -1.0f, (MAP_SIZE - 2) * WALL_SIZE));
            ghosts.emplace_back(glm::vec3((MAP_SIZE - 2) * WALL_SIZE, -1.0f, 1 * WALL_SIZE));

            // BGM 재생
            soundManager.PlayBGM("Dead_Silence_Soundtrack.mp3");
            soundManager.SetBGMVolume(300);
        }
        return;
    }

    keyState[key] = true;
    int hide;

    if (!isGameClear) {
        switch (key) {
        case '1': player.viewMode = 1; break;
        case '3': player.viewMode = 3; break;
        case 'e': player.isFlashlightOn = !player.isFlashlightOn;
            if (player.isFlashlightOn) {
                soundManager.PlaySFX("light-switch-on.mp3");
                soundManager.SetSFXVolume(500);
            }
            else {
                soundManager.PlaySFX("light-switch-off.mp3");
                soundManager.SetSFXVolume(500);
            }

            break;
        case 'f': // f키로 상호작용
            // 플레이어가 바라보는 방향(Vector) 계산
            glm::vec3 cameraFront = glm::normalize(player.GetCameraTarget() - player.GetCameraPos());
            // 위치와 방향을 함께 전달하여 벽 파괴 시도
            if (maze.BreakWall(player.pos, cameraFront)) soundManager.PlaySFX("BreakWall.aiff");

            for (auto& w : wardrobes) {
                int result = w.TryInteract(player.pos);
                if (result == 1) {
                    player.isHide = !player.isHide;
                    soundManager.PlaySFX("Closet.mp3");
                    break; // 한 번에 하나의 옷장만 상호작용
                }
            }

            break;
        case 'q': exit(0); break;
        case 'l':
			// 디버그용 : 출구 위치로 순간이동
			player.pos = glm::vec3((MAP_SIZE - 1) * WALL_SIZE, -1.0f, (MAP_SIZE - 2) * WALL_SIZE);
            soundManager.PlayBGM("Dead_Silence_Soundtrack.mp3");
            soundManager.SetBGMVolume(300);
            isGameClear = true;
			break;
        }
    }
    else {
        switch (key) {
		case 'q': exit(0); break;
        }
    }
   
}

void KeyboardUp(unsigned char key, int x, int y) {
    keyState[key] = false;
}

void MouseMotion(int x, int y) {
    player.ProcessMouse(x, y, width, height);
    if (player.viewMode == 1) {
        glutWarpPointer(width / 2, height / 2);
        glutPostRedisplay();
    }
}

void Reshape(int w, int h) {
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

// --- 셰이더 및 파일 로딩 ---
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

int main(int argc, char** argv) {
    srand(static_cast<unsigned>(time(0)));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(width, height);
    glutCreateWindow("Maze Game with Wardrobe");

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

    // 배경음악 
    soundManager.PlayBGM("Dead_Silence_Soundtrack.mp3");
    soundManager.SetBGMVolume(300);

    glutDisplayFunc(DrawScene);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutKeyboardUpFunc(KeyboardUp);
    glutTimerFunc(16, Timer, 0);
    glutPassiveMotionFunc(MouseMotion);

    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(width / 2, height / 2);

    glutMainLoop();
    return 0;
}