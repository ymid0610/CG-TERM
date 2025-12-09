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
	mapData[MAP_SIZE - 2][MAP_SIZE - 1] = 3; // 출구
}

// 맵 그리기
void MazeMap::Draw(GLuint shaderProgramID, void (*DrawCubeFunc)(glm::mat4, glm::vec3)) {
    for (int z = 0; z < MAP_SIZE; z++) {
        for (int x = 0; x < MAP_SIZE; x++) {
			if (mapData[z][x] == 0) continue; // 길은 그리지 않음
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
            else if (mapData[z][x] == 2) { // 부셔지는 벽 그리기
                float worldX = x * WALL_SIZE;
                float worldZ = z * WALL_SIZE;
                float wallHeight = WALL_HEIGHT;
                if (breakWallState[z][x].isBreaking) {
                    wallHeight = breakWallState[z][x].currentHeight;
                }
                glm::mat4 modelMat = glm::mat4(1.0f);

                if (!breakWallState[z][x].isBreaking) {
                    modelMat = glm::translate(modelMat, glm::vec3(worldX, 0.0f, worldZ));
                }
                else {
                    modelMat = glm::translate(modelMat, glm::vec3(worldX, (wallHeight / 2.0f) - (WALL_HEIGHT / 2.0f), worldZ));
                }
                modelMat = glm::scale(modelMat, glm::vec3(WALL_SIZE, wallHeight, WALL_SIZE));

                DrawCubeFunc(modelMat, glm::vec3(0.0f, 1.0f, 0.0f));
            }
        }
    }
}

// 충돌 감지 (매우 중요)
bool MazeMap::CheckCollision(float x, float z) {
    // 플레이어의 경계(반지름)를 고려한 충돌 체크
    float minX = x - PLAYER_RADIUS;
    float maxX = x + PLAYER_RADIUS;
    float minZ = z - PLAYER_RADIUS;
    float maxZ = z + PLAYER_RADIUS;

    // 4방향 경계 체크
    for (float testX : {minX, maxX}) {
        for (float testZ : {minZ, maxZ}) {
            int gridX = (int)((testX + WALL_SIZE / 2) / WALL_SIZE);
            int gridZ = (int)((testZ + WALL_SIZE / 2) / WALL_SIZE);

            if (gridX < 0 || gridX >= MAP_SIZE || gridZ < 0 || gridZ >= MAP_SIZE) return true;
            if (mapData[gridZ][gridX] == 1 || mapData[gridZ][gridX] == 2) return true;
        }
    }
    return false;
}

bool MazeMap::CheckVictory(float x, float z) {
    int gridX = (int)((x + WALL_SIZE / 2) / WALL_SIZE);
    int gridZ = (int)((z + WALL_SIZE / 2) / WALL_SIZE);

    if (mapData[gridZ][gridX] == 3) return true;
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

	// 벽을 부셔지는 벽(2)으로 설정
	GenerateBreakableWalls(0.2f); // 20% 확률로 부셔지는 벽 생성

	// 테두리는 항상 벽으로 설정
	for (int i = 0; i < MAP_SIZE; i++) {
		mapData[0][i] = 1;
		mapData[MAP_SIZE - 1][i] = 1;
		mapData[i][0] = 1;
		mapData[i][MAP_SIZE - 1] = 1;
	}

    return true;
}

bool MazeMap::GenerateBreakableWalls(float probability) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);
	for (int z = 1; z < MAP_SIZE - 1; ++z) {
		for (int x = 1; x < MAP_SIZE - 1; ++x) {
			if (mapData[z][x] == 1 && dis(gen) < probability) {
				mapData[z][x] = 2; // 부셔지는 벽으로 설정
			}
		}
	}
	return true;
}

bool MazeMap::BreakWall(glm::vec3 playerPos, glm::vec3 playerFront) {
    // 1. 상호작용 거리 설정 (벽 크기보다 약간 작게 설정하거나 비슷하게 설정)
    // 너무 길면 멀리서도 부서지고, 너무 짧으면 벽에 비벼야 부서집니다.
    float interactionDist = WALL_SIZE * 0.7f;

    // 2. 플레이어 위치에서 시선 방향으로 일정 거리만큼 앞선 좌표 계산
    // y축(높이)은 무시하고 x, z 평면에서의 위치만 계산합니다.
    glm::vec3 targetPos = playerPos + (glm::normalize(playerFront) * interactionDist);

    // 3. 월드 좌표 -> 배열 인덱스로 변환 (CheckCollision과 동일한 로직)
    int gridX = (int)((targetPos.x + WALL_SIZE / 2) / WALL_SIZE);
    int gridZ = (int)((targetPos.z + WALL_SIZE / 2) / WALL_SIZE);

    // 4. 배열 범위 체크 (인덱스가 맵을 벗어나지 않도록)
    if (gridX < 0 || gridX >= MAP_SIZE || gridZ < 0 || gridZ >= MAP_SIZE) {
        return false;
    }

    if (mapData[gridZ][gridX] == 2 && !breakWallState[gridZ][gridX].isBreaking) {
        breakWallState[gridZ][gridX].isBreaking = true;
        breakWallState[gridZ][gridX].currentHeight = WALL_HEIGHT;
        std::cout << "벽 파괴 애니메이션 시작! (" << gridX << ", " << gridZ << ")" << std::endl;
        return true;
    }

    return false; // 부실 수 있는 벽이 아님
}

void MazeMap::UpdateBreakWalls(float deltaTime) {
    float downSpeed = 0.02f * deltaTime; // 초당 2만큼 내려감 (조절 가능)
    for (int z = 0; z < MAP_SIZE; ++z) {
        for (int x = 0; x < MAP_SIZE; ++x) {
            if (mapData[z][x] == 2 && breakWallState[z][x].isBreaking) {
                breakWallState[z][x].currentHeight -= downSpeed;
                if (breakWallState[z][x].currentHeight <= 0.0f) {
                    mapData[z][x] = 0; // 완전히 내려가면 길로 변경
                    breakWallState[z][x].isBreaking = false;
                    breakWallState[z][x].currentHeight = 0.0f;
                }
            }
        }
    }
}

glm::vec3 MazeMap::GetRandomOpenPos() {
    std::vector<std::pair<int, int>> openSlots;

    // 맵 전체를 돌면서 '길(0)'인 곳의 좌표를 수집
    for (int z = 0; z < MAP_SIZE; z++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            if (mapData[z][x] == 0) {
                openSlots.push_back({ z, x });
            }
        }
    }

    // 갈 곳이 없다면 원점 반환 (예외 처리)
    if (openSlots.empty()) return glm::vec3(0.0f, 0.0f, 0.0f);

    // 랜덤하게 하나 뽑기
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, openSlots.size() - 1);

    int idx = dis(gen);
    int selectedZ = openSlots[idx].first;
    int selectedX = openSlots[idx].second;

    // 월드 좌표로 변환하여 반환 (높이는 0으로 설정, main에서 처리)
    return glm::vec3(selectedX * WALL_SIZE, 0.0f, selectedZ * WALL_SIZE);
}