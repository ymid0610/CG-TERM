#include "SoundManager.h"
#include <Windows.h>
#include <iostream>

// 윈도우 멀티미디어 라이브러리 링크 (별도 설정 없이 코드에서 링크)
#pragma comment(lib, "winmm.lib")

SoundManager::SoundManager() {
    // 초기화가 필요하다면 여기서 수행
}

SoundManager::~SoundManager() {
    // 종료 시 모든 소리 정지 및 닫기
    mciSendStringA("close all", NULL, 0, NULL);
}

void SoundManager::PlayBGM(std::string fileName) {
    // 기존 BGM이 있다면 닫기
    mciSendStringA("close bgm", NULL, 0, NULL);

    // 명령어 구성: 파일을 열고 별칭(alias)을 'bgm'으로 설정
    std::string cmdOpen = "open \"" + fileName + "\" type mpegvideo alias bgm";

    // 파일 열기 시도
    if (mciSendStringA(cmdOpen.c_str(), NULL, 0, NULL) == 0) {
        // 성공 시 반복 재생 (repeat)
        mciSendStringA("play bgm repeat", NULL, 0, NULL);
    }
    else {
        std::cerr << "[SoundManager] BGM 파일을 찾을 수 없거나 열 수 없습니다: " << fileName << std::endl;
    }
}

void SoundManager::StopBGM() {
    mciSendStringA("stop bgm", NULL, 0, NULL);
    mciSendStringA("close bgm", NULL, 0, NULL);
}

void SoundManager::PlaySFX(std::string fileName) {
    // 효과음은 겹쳐서 날 수 있어야 하므로, 간단한 구현을 위해 
    // 매번 고유한 alias를 쓰거나, 여기서는 'sfx'라는 별칭을 사용하여
    // 이전 효과음을 끊고 새 효과음을 재생하는 방식을 사용합니다.

    // 기존 sfx 닫기 (빠른 반응을 위해)
    mciSendStringA("close sfx", NULL, 0, NULL);

    std::string cmdOpen = "open \"" + fileName + "\" type mpegvideo alias sfx";

    if (mciSendStringA(cmdOpen.c_str(), NULL, 0, NULL) == 0) {
        // 한 번만 재생
        mciSendStringA("play sfx from 0", NULL, 0, NULL);
    }
    else {
        std::cerr << "[SoundManager] 효과음 파일을 찾을 수 없습니다: " << fileName << std::endl;
    }
}