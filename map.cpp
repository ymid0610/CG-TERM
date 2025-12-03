// 맵
#include "map.h"
#include <iostream>
#include <gl/glew.h>
#include <gl/glm/ext.hpp>
#include <vector>
#include <random>


MazeMap::MazeMap() {
    Init();
}

void MazeMap::Init() {
	GenerateMaze(MAP_SIZE, MAP_SIZE);
	// 입구와 출구 설정
	mapData[1][0] = 0; // 입구
	mapData[MAP_SIZE - 2][MAP_SIZE - 1] = 2; // 출구
}

// 맵 그리기
void MazeMap::Draw(GLuint shaderProgramID, void (*DrawCubeFunc)(glm::mat4, glm::vec3)) {
    for (int z = 0; z < MAP_SIZE; z++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            if (mapData[z][x] == 1) { // 벽 그리기
                glm::mat4 modelMat = glm::mat4(1.0f);
                // 배열의 인덱스를 월드 좌표로 변환 (중앙 정렬을 위해 오프셋 조정 가능)
                float worldX = x * WALL_SIZE;
                float worldZ = z * WALL_SIZE;

                modelMat = glm::translate(modelMat, glm::vec3(worldX, 0.0f, worldZ));
                modelMat = glm::scale(modelMat, glm::vec3(WALL_SIZE, WALL_HEIGHT, WALL_SIZE)); // 높이 3짜리 벽

                // 회색 벽
                DrawCubeFunc(modelMat, glm::vec3(0.5f, 0.5f, 0.5f));
            }
            else if (mapData[z][x] == 2) { // 도착 지점 (초록색 바닥)
                glm::mat4 modelMat = glm::mat4(1.0f);
                float worldX = x * WALL_SIZE;
                float worldZ = z * WALL_SIZE;

                modelMat = glm::translate(modelMat, glm::vec3(worldX, -1.0f, worldZ)); // 바닥에 깔기
                modelMat = glm::scale(modelMat, glm::vec3(WALL_SIZE, 0.1f, WALL_SIZE));

                DrawCubeFunc(modelMat, glm::vec3(0.0f, 1.0f, 0.0f)); // 초록색
            }
        }
    }
}

// 충돌 감지 (매우 중요)
bool MazeMap::CheckCollision(float x, float z) {
    // 월드 좌표 -> 배열 인덱스로 변환
    // 큐브의 중심이 좌표이므로 반올림을 통해 가장 가까운 칸을 찾습니다.
    int gridX = (int)((x + WALL_SIZE / 2) / WALL_SIZE);
    int gridZ = (int)((z + WALL_SIZE / 2) / WALL_SIZE);

    // 배열 범위 체크
    if (gridX < 0 || gridX >= MAP_SIZE || gridZ < 0 || gridZ >= MAP_SIZE) return true;

    // 벽(1)이면 충돌로 간주
    if (mapData[gridZ][gridX] == 1) return true;

    return false;
}

bool MazeMap::CheckVictory(float x, float z) {
    int gridX = (int)((x + WALL_SIZE / 2) / WALL_SIZE);
    int gridZ = (int)((z + WALL_SIZE / 2) / WALL_SIZE);

    if (mapData[gridZ][gridX] == 2) return true;
    return false;
}

bool MazeMap::GenerateMaze(int rows, int cols) {
    std::random_device rd;
    std::mt19937 gen(rd());

    // maze 벡터 크기 초기화 (모든 칸을 벽으로 설정: 1)
    maze.assign(rows * cols, 1);

    std::vector<bool> visited(rows * cols, false);
    std::vector<std::pair<int, int>> stack;
    int dr[4] = { -1, 1, 0, 0 }, dc[4] = { 0, 0, -1, 1 };
    auto idx = [&](int r, int c) { return r * cols + c; };

    int sr = 1, sc = 1;
    stack.emplace_back(sr, sc);
    visited[idx(sr, sc)] = true;
    maze[idx(sr, sc)] = 0;

    while (!stack.empty()) {
        int r = stack.back().first, c = stack.back().second;
        std::vector<int> dirs = { 0, 1, 2, 3 };
        std::shuffle(dirs.begin(), dirs.end(), gen);
        bool moved = false;
        for (int d : dirs) {
            int nr = r + dr[d] * 2, nc = c + dc[d] * 2;
            if (nr > 0 && nr < rows && nc > 0 && nc < cols && !visited[idx(nr, nc)]) {
                maze[idx(r + dr[d], c + dc[d])] = 0;
                maze[idx(nr, nc)] = 0;
                visited[idx(nr, nc)] = true;
                stack.emplace_back(nr, nc);
                moved = true;
                break;
            }
        }
        if (!moved) stack.pop_back();
    }

    // maze 벡터를 mapData 2D 배열로 복사
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            mapData[r][c] = maze[idx(r, c)];
        }
    }

    return true;
}