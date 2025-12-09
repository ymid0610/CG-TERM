#pragma once
#include <string>

class SoundManager
{
public:
    SoundManager();
    ~SoundManager();

    // 배경음악 재생 (반복)
    void PlayBGM(std::string fileName);

    // 배경음악 정지
    void StopBGM();

    // 효과음 재생 (1회성 - 벽 부수기 등)
    void PlaySFX(std::string fileName);
};