#version 330 core

layout(location = 0) in vec3 vPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 faceColor;

out vec3 FragPos;   // 프래그먼트 셰이더로 보낼 월드 좌표
out vec3 out_Color; // 프래그먼트 셰이더로 보낼 색상

void main(){
    // 모델 변환을 적용하여 월드 좌표 계산
    FragPos = vec3(model * vec4(vPos, 1.0));
    
    // 화면상의 좌표 계산 (필수)
    gl_Position = proj * view * vec4(FragPos, 1.0);
    
    out_Color = faceColor;
}