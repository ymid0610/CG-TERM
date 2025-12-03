// 맵
#include "map.h"
#include <iostream>
#include <gl/glew.h>
#include <gl/glm/ext.hpp>

MazeMap::MazeMap() {
    Init();
}

void MazeMap::Init() {
    // 1은 벽, 0은 길, 2는 탈출구
    // 간단한 'ㄷ'자 형태의 미로 예시
    int tempMap[MAP_SIZE][MAP_SIZE] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 1, 0, 1, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 0, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 0, 1, 1, 0, 1},
        {1, 0, 0, 0, 1, 0, 1, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 1, 0, 2, 1}, // (8,8)이 도착지점
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            mapData[i][j] = tempMap[i][j];
        }
    }
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
                modelMat = glm::scale(modelMat, glm::vec3(WALL_SIZE, 3.0f, WALL_SIZE)); // 높이 3짜리 벽

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