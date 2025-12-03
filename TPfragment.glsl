#version 330 core

out vec4 FragColor;
in vec3 out_Color;
in vec3 FragPos;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform int lightOn;
uniform vec3 lightDir;    
uniform float cutOff;

void main(){
    vec3 result;
    
    // 1. 주변광 (Ambient) - 기본 밝기 약간 상향
    float ambientStrength = 0.05; 
    vec3 ambient = ambientStrength * lightColor;

    if(lightOn == 0){ 
        result = ambient * out_Color;
    } 
    else {
        vec3 norm = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
        vec3 lightDirectionVector = normalize(lightPos - FragPos);
        
        float theta = dot(lightDirectionVector, normalize(-lightDir));
        
        if(theta > cutOff) {
            // Diffuse (확산광) - 빛의 세기를 1.5배로 증폭
            float diff = max(dot(norm, lightDirectionVector), 0.0);
            vec3 diffuse = diff * lightColor * 1.5; 
            
            // [수정] 거리 감쇠 (빛이 더 멀리 가도록 수치 완화)
            float distance = length(lightPos - FragPos);
            // 기존: 1.0, 0.09, 0.032 -> 수정: 1.0, 0.045, 0.0075 (빛이 훨씬 멀리 감)
            float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * (distance * distance));
            
            // 부드러운 경계
            float epsilon = 0.15; 
            float intensity = clamp((theta - cutOff) / epsilon, 0.0, 1.0);
            
            diffuse *= intensity;
            diffuse *= attenuation;
            
            result = (ambient + diffuse) * out_Color;
        } else {
            result = ambient * out_Color;
        }
    }

    FragColor = vec4(result, 1.0);
}