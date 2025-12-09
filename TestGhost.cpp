#include "TestGhost.h"
#include <iostream>
#include <cmath> // sin, cos, atan2
#include <queue>
#include <unordered_map>
#include <random>

Ghost::Ghost(glm::vec3 startPos) {
    pos = startPos;
    baseHeight = startPos.y;
    rotY = 0.0f;
    speed = 0.06f; // 이동 속도

    floatTime = 0.0f;
    wobbleTime = 0.0f;

    // [색상 설정]
    // 가산 혼합을 쓸 것이므로, 너무 밝으면 눈이 부십니다.
    // 어두운 파란색/하늘색을 쓰면 은은하게 빛나는 유령이 됩니다.
    baseColor = glm::vec3(0.1f, 0.3f, 0.5f);
}

void Ghost::DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color) {
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint faceColorLoc = glGetUniformLocation(shaderID, "faceColor");

    // [핵심 트릭] 셰이더 수정 없이 투명 효과 내기
    glEnable(GL_BLEND);

    // 1. 가산 혼합: 색을 더해서 밝게 만듦 (검은색은 투명, 밝은색은 발광)
    glBlendFunc(GL_ONE, GL_ONE);

    // 2. 깊이 쓰기 끄기: 유령 내부의 뒷면이 가려지지 않고 겹쳐 보이게 함
    glDepthMask(GL_FALSE);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3f(faceColorLoc, color.r, color.g, color.b);

    if (model.face_count > 0)
        glDrawElements(GL_TRIANGLES, model.face_count * 3, GL_UNSIGNED_INT, 0);

    // [상태 복구] 다음 물체(플레이어, 벽 등)를 위해 설정을 원래대로 돌려놓음
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 기본 블렌딩으로 복구
    glDisable(GL_BLEND);
}

bool Ghost::Update(glm::vec3 playerPos, MazeMap& maze) {

    // 플레이어와 충돌 체크
    if (IsCollideWithPlayer(pos, playerPos)) {
        pos = GetRandomPathPosition(maze, MAP_SIZE, baseHeight);
        floatTime = 0.0f; // 애니메이션 초기화(선택)
        wobbleTime = 0.0f;
        return true; // 이동 로직 생략
    }
    
	// 1. 플레이어 쪽으로 이동
    int mapSize = MAP_SIZE;
    auto path = FindPath(maze, pos, playerPos, mapSize);
    if (!path.empty()) {
        int nextIdx = (path.size() > 1) ? 1 : 0;
        int nextGridZ = path[nextIdx].first;
        int nextGridX = path[nextIdx].second;
        float targetX = nextGridX * WALL_SIZE;
        float targetZ = nextGridZ * WALL_SIZE;
        glm::vec3 direction = glm::normalize(glm::vec3(targetX, pos.y, targetZ) - pos);

        float nextX = pos.x + direction.x * speed;
        float nextZ = pos.z + direction.z * speed;

        // 충돌 체크: 벽이 아니면 이동
        if (!maze.CheckCollision(nextX, pos.z)) {
            pos.x = nextX;
        }
        if (!maze.CheckCollision(pos.x, nextZ)) {
            pos.z = nextZ;
        }
        rotY = glm::degrees(atan2(direction.x, direction.z));
    }
	else {
		// 경로가 없으면 천천히 플레이어 근처로 이동
		glm::vec3 direction = glm::normalize(glm::vec3(playerPos.x, pos.y, playerPos.z) - pos);
		pos.x += direction.x * (speed * 0.5f);
		pos.z += direction.z * (speed * 0.5f);
		rotY = glm::degrees(atan2(direction.x, direction.z));
	}

    // 2. 둥둥 뜨는 애니메이션
    floatTime += 0.05f;
    wobbleTime += 0.1f;
    pos.y = baseHeight + sin(floatTime) * 0.3f; // 위아래 움직임

	return false; // 충돌 없음
}

void Ghost::Draw(GLuint shaderID, const Model& model) {
    glm::mat4 bodyMat = glm::mat4(1.0f);
    bodyMat = glm::translate(bodyMat, pos);
    bodyMat = glm::rotate(bodyMat, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));

    // 1. 머리
    glm::mat4 headMat = glm::translate(bodyMat, glm::vec3(0.0f, 0.8f, 0.0f));
    glm::mat4 headScale = glm::scale(headMat, glm::vec3(0.6f, 0.6f, 0.6f));
    // 머리는 가장 밝게 (선명하게)
    DrawBox(shaderID, model, headScale, baseColor);

    // 눈 (검은색 눈은 가산 혼합에서 보이지 않으므로, 붉은색으로 빛나게 설정)
    glm::vec3 eyeColor = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::mat4 lEye = glm::translate(headMat, glm::vec3(-0.15f, 0.0f, 0.31f));
    DrawBox(shaderID, model, glm::scale(lEye, glm::vec3(0.1f, 0.1f, 0.05f)), eyeColor);
    glm::mat4 rEye = glm::translate(headMat, glm::vec3(0.15f, 0.0f, 0.31f));
    DrawBox(shaderID, model, glm::scale(rEye, glm::vec3(0.1f, 0.1f, 0.05f)), eyeColor);

    // 2. 꼬리 (아래로 갈수록 어둡게 -> 투명하게)
    int tailSegments = 5;
    for (int i = 1; i <= tailSegments; ++i) {
        float t = (float)i / tailSegments;

        float yOffset = 0.8f - (i * 0.25f);
        float wobbleX = sin(wobbleTime + i * 0.5f) * 0.1f;
        float wobbleZ = cos(wobbleTime + i * 0.5f) * 0.1f;

        glm::mat4 tailMat = glm::translate(bodyMat, glm::vec3(wobbleX, yOffset, wobbleZ));
        float scale = 0.5f * (1.0f - t * 0.5f);
        glm::mat4 tailScale = glm::scale(tailMat, glm::vec3(scale, 0.2f, scale));

        // [트릭] 색상을 어둡게 만들면 가산 혼합에서는 '투명'해 보입니다.
        glm::vec3 tailColor = baseColor * (1.0f - t * 0.9f); // 아래로 갈수록 어두워짐
        DrawBox(shaderID, model, tailScale, tailColor);
    }

    // 3. 팔 (흐느적거림)
    float armWobble = sin(wobbleTime * 0.8f) * 20.0f;

    glm::mat4 lArmMat = glm::translate(headMat, glm::vec3(-0.4f, -0.2f, 0.0f));
    lArmMat = glm::rotate(lArmMat, glm::radians(40.0f + armWobble), glm::vec3(0.0f, 0.0f, 1.0f));
    DrawBox(shaderID, model, glm::scale(lArmMat, glm::vec3(0.15f, 0.5f, 0.15f)), baseColor * 0.8f);

    glm::mat4 rArmMat = glm::translate(headMat, glm::vec3(0.4f, -0.2f, 0.0f));
    rArmMat = glm::rotate(rArmMat, glm::radians(-40.0f - armWobble), glm::vec3(0.0f, 0.0f, 1.0f));
    DrawBox(shaderID, model, glm::scale(rArmMat, glm::vec3(0.15f, 0.5f, 0.15f)), baseColor * 0.8f);
}

bool IsCollideWithPlayer(const glm::vec3& ghostPos, const glm::vec3& playerPos) {
    float threshold = 0.7f;
    float dx = ghostPos.x - playerPos.x;
    float dz = ghostPos.z - playerPos.z;
    float distSq = dx * dx + dz * dz;
    return distSq < (threshold * threshold);
}

glm::vec3 GetRandomPathPosition(const MazeMap& maze, int mapSize, float baseHeight) {
    std::vector<std::pair<int, int>> pathCells;
    for (int z = 0; z < mapSize; ++z) {
        for (int x = 0; x < mapSize; ++x) {
            if (maze.maze[z * mapSize + x] == 0) {
                pathCells.emplace_back(z, x);
            }
        }
    }
    if (pathCells.empty()) return glm::vec3(0.0f, baseHeight, 0.0f);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, pathCells.size() - 1);
    auto cell = pathCells[dis(gen)];
    float worldX = cell.second * WALL_SIZE;
    float worldZ = cell.first * WALL_SIZE;
    return glm::vec3(worldX, baseHeight, worldZ);
}

std::vector<std::pair<int, int>> Ghost::FindPath(MazeMap& maze, glm::vec3 startPos, glm::vec3 endPos, int mapSize) {
    int sx = static_cast<int>((startPos.x + WALL_SIZE / 2) / WALL_SIZE);
    int sz = static_cast<int>((startPos.z + WALL_SIZE / 2) / WALL_SIZE);
    int ex = static_cast<int>((endPos.x + WALL_SIZE / 2) / WALL_SIZE);
    int ez = static_cast<int>((endPos.z + WALL_SIZE / 2) / WALL_SIZE);

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    std::unordered_map<int, std::pair<int, int>> parent;
    std::vector<std::vector<int>> cost(mapSize, std::vector<int>(mapSize, INT_MAX));
    int dr[4] = { -1, 1, 0, 0 }, dc[4] = { 0, 0, -1, 1 };

    auto heuristic = [&](int r, int c) {
        return abs(r - ez) + abs(c - ex); // 맨해튼 거리
        };

    open.push(Node(sz, sx, 0, heuristic(sz, sx)));
    cost[sz][sx] = 0;

    while (!open.empty()) {
        Node node = open.top(); open.pop();
        int r = node.r, c = node.c;
        if (r == ez && c == ex) break;
        for (int d = 0; d < 4; ++d) {
            int nr = r + dr[d], nc = c + dc[d];
            if (nr >= 0 && nr < mapSize && nc >= 0 && nc < mapSize) {
                int cell = maze.mapData[nr][nc];
                if (cell != 0) continue; // 길(0)만 통과 가능
                int moveCost = 1;
                int newCost = cost[r][c] + moveCost;
                if (newCost < cost[nr][nc]) {
                    cost[nr][nc] = newCost;
                    parent[nr * mapSize + nc] = { r, c };
                    open.push(Node(nr, nc, newCost, newCost + heuristic(nr, nc)));
                }
            }
        }
    }

    // 경로 역추적
    std::vector<std::pair<int, int>> path;
    int r = ez, c = ex;
    while (!(r == sz && c == sx)) {
        path.push_back({ r, c });
        auto it = parent.find(r * mapSize + c);
        if (it == parent.end()) break;
        r = it->second.first;
        c = it->second.second;
    }
    std::reverse(path.begin(), path.end());
    return path;
}
