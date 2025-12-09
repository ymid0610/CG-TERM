#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <vector>
#include "ReadObjFile.h"
#include "map.h"

enum GhostState { NORMAL, GOING_UP, WAITING, GOING_DOWN };

class Ghost {
public:
    glm::vec3 pos;          // 위치
    float rotY;             // 회전
    float speed;            // 속도

    // 애니메이션 변수
    float floatTime;        // 둥둥 뜨는 시간
    float wobbleTime;       // 흐느적거리는 시간
    float baseHeight;       // 기준 높이

    // 색상 (vec3 유지)
    glm::vec3 baseColor;    // 기본 색상

    GhostState state = NORMAL;
    float targetY = -1.0f;      // 최초 y위치 저장
    float hideY = 20.0f;        // 숨었을 때 올라갈 y위치(원하는 값으로 조정)
    float yMoveSpeed = 0.1f;    // y축 이동 속도(조정 가능)

    Ghost(glm::vec3 startPos);
	bool Update(glm::vec3 playerPos, MazeMap& maze, bool playerIsHide);
    void Draw(GLuint shaderID, const Model& model);
    std::vector<std::pair<int, int>> FindPath(MazeMap& maze, glm::vec3 startPos, glm::vec3 endPos, int mapSize);
	glm::vec3 GetPos() const { return pos; }; // 현재 위치 반환

private:
    // 내부 그리기 함수
    void DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color);
};

bool IsCollideWithPlayer(const glm::vec3& ghostPos, const glm::vec3& playerPos);

glm::vec3 GetRandomPathPosition(const MazeMap& maze, int mapSize, float baseHeight);

struct Node {
    int r, c;
    int g; // 현재까지의 비용
    int f; // g + 휴리스틱
    Node(int r, int c, int g, int f) : r(r), c(c), g(g), f(f) {}
    bool operator>(const Node& other) const { return f > other.f; }
};