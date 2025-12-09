#pragma once
#include <vector>

// [추가] GLuint 자료형을 인식하기 위해 glew.h를 포함해야 합니다.
#include <gl/glew.h> 
#include <gl/glm/glm.hpp>

// 맵의 크기와 셀 하나의 크기 정의
#define MAP_SIZE 13
#define WALL_SIZE 8.0f
#define WALL_HEIGHT 5.0f
#define PLAYER_RADIUS 0.25f // 플레이어의 반지름(혹은 박스의 절반 폭)

struct BreakWallInfo {
    bool isBreaking = false;
    float currentHeight = WALL_HEIGHT;
};

class MazeMap {
public:
    // 0: 길, 1: 벽, 2: 도착지점
    int mapData[MAP_SIZE][MAP_SIZE];
    std::vector<int> maze;
    BreakWallInfo breakWallState[MAP_SIZE][MAP_SIZE];

    MazeMap();
    void Init();
    // 이제 GLuint를 인식할 수 있어서 오류가 사라집니다.
    void Draw(GLuint shaderProgramID, void (*DrawCubeFunc)(glm::mat4, glm::vec3));
    bool CheckCollision(float x, float z); // 이동하려는 위치가 벽인지 확인
    bool CheckVictory(float x, float z);   // 도착했는지 확인
	bool GenerateMaze(int rows, int cols); // 미로 생성 함수
	bool GenerateBreakableWalls(float probability);
    bool BreakWall(glm::vec3 playerPos, glm::vec3 playerFront); // 벽 부수는 함수
	void UpdateBreakWalls(float deltaTime); // 부서지는 벽 상태 업데이트
	glm::vec3 GetRandomOpenPos(); // 빈 공간 랜덤 좌표 반환
};

