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

// 사용자 정의 헤더 파일 포함
#include "ReadObjFile.h"
#include "map.h"
#include "player.h"
#include "wardrobe.h" // [추가] 옷장 헤더 포함
#include "TestGhost.h"

// --- 전역 변수 ---
GLint width = 1600, height = 900;
GLuint shaderProgramID, vertexShader, fragmentShader;
GLuint VAO, VBO, EBO;

// 객체 인스턴스
Model model;     // cube.obj 데이터
MazeMap maze;    // 미로 맵 데이터
Player player;   // 플레이어 객체

// [추가] 옷장 객체 생성 (원하는 좌표 입력: x, z)
// 맵의 빈 공간 좌표를 잘 확인해서 넣어주세요. (예: 10.0, 4.0)
Wardrobe wardrobe(5.0f, 5.0f);

//test
Ghost ghost(glm::vec3(3.0f, -1.0f, 5.0f));

bool isGameClear = false;

// 조명 관련 변수
glm::vec3 lightPos(0.0f, 2.0f, 0.0f);
glm::vec3 lightDirection(0.0f, 0.0f, -1.0f);

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

// 헬퍼 함수: 큐브 그리기
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
    glDisable(GL_DEPTH_TEST);

    glm::mat4 ortho = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
    glm::mat4 viewUI = glm::mat4(1.0f);

    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewUI));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(ortho));

    glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), 0);

    // [배경 바]
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

    // 1. 플레이어 그리기
    player.Draw(shaderProgramID, model, DrawCube);

    // 2. 미로 그리기
    maze.Draw(shaderProgramID, DrawCube);

    // [추가] 3. 옷장 그리기
    wardrobe.Draw(shaderProgramID, model);

    // test
    ghost.Draw(shaderProgramID, model);

    // 5. UI
    DrawStaminaBar();

    glutSwapBuffers();
}

void Timer(int value) {
    if (isGameClear) {
        glutPostRedisplay();
        glutTimerFunc(16, Timer, 0);
        return;
    }

    // [중요 수정] 옷장 상태가 '평상시'일 때만 플레이어가 키보드로 움직임
    if (wardrobe.GetState() == STATE_OUTSIDE) {
        player.Update(keyState, maze);
    }
    else {
        // 숨어있거나 애니메이션 중일 때는 플레이어 걷기 애니메이션 중지
        player.currentAnim = IDLE;

        // 숨어있는 동안 스테미너 회복 (선택사항)
        if (player.currentStamina < player.maxStamina) {
            player.currentStamina += 0.5f;
        }
    }

    // [추가] 옷장 업데이트 (항상 호출)
    // 옷장이 플레이어의 위치, 시점 등을 제어할 수 있도록 참조 전달
    wardrobe.Update(
        player.pos,
        player.cameraAngle,
        player.pitch,
        player.viewMode,
        player.isFlashlightOn
    );

    // ==========================================================
    // [추가] 유령(Ghost) 업데이트
    // 플레이어의 위치(player.pos)를 타겟으로 주고, 충돌 체크용 맵(maze)을 넘깁니다.
    // ==========================================================
    if (player.isHide == false) {
        ghost.Update(player.pos, maze);
    }
    else {
		// 맵의 도착지점 좌표 계산
		glm::vec3 exitPos;
		for (int z = 0; z < MAP_SIZE; z++) {
			for (int x = 0; x < MAP_SIZE; x++) {
				if (maze.mapData[z][x] == 3) {
					exitPos = glm::vec3(x * WALL_SIZE, -1.0f, z * WALL_SIZE);
				}
			}
		}
		ghost.Update(exitPos, maze);
    }
    


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
    int hide;

    switch (key) {
    case '1': player.viewMode = 1; break;
    case '3': player.viewMode = 3; break;
    case 'e': player.isFlashlightOn = !player.isFlashlightOn; break;
    case 'j': player.cameraAngle += glm::radians(10.0f); break;
    case 'k': player.cameraDistance += 0.5f; break;
    case 'K': if (player.cameraDistance > 2.0f) player.cameraDistance -= 0.5f; break;

        
    case 'f': // f키로 상호작용
        // [수정] 1. 플레이어가 바라보는 방향(Vector) 계산
        // (Target - Position)을 정규화하면 바라보는 방향 벡터가 됩니다.
        glm::vec3 cameraFront = glm::normalize(player.GetCameraTarget() - player.GetCameraPos());
        // [수정] 2. 위치와 방향을 함께 전달하여 벽 파괴 시도
        maze.BreakWall(player.pos, cameraFront);
        hide = wardrobe.TryInteract(player.pos);
        if (hide == 1) player.isHide = !player.isHide;

        break;
    case 'q': exit(0); break;
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

// --- 셰이더 및 파일 로딩 (기존 유지) ---
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