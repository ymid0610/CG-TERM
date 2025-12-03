#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <vector>
#include "ReadObjFile.h"
#include "map.h"

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

    Ghost(glm::vec3 startPos);
    void Update(glm::vec3 targetPos, MazeMap& maze);
    void Draw(GLuint shaderID, const Model& model);
    std::vector<std::pair<int, int>> FindPath(MazeMap& maze, glm::vec3 startPos, glm::vec3 endPos, int mapSize);

private:
    // 내부 그리기 함수
    void DrawBox(GLuint shaderID, const Model& model, glm::mat4 modelMat, glm::vec3 color);
};

bool IsCollideWithPlayer(const glm::vec3& ghostPos, const glm::vec3& playerPos);

glm::vec3 GetRandomPathPosition(const MazeMap& maze, int mapSize, float baseHeight);