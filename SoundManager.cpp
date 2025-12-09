#include "SoundManager.h"
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "winmm.lib")

SoundManager::SoundManager() {
    currentSFXVolume = 1000;
}

SoundManager::~SoundManager() {
    // 종료 시 모든 소리 정지 및 닫기
    mciSendStringA("close all", NULL, 0, NULL);
}

void SoundManager::PlayBGM(std::string fileName) {
    // 기존 BGM이 있다면 닫기
    mciSendStringA("close bgm", NULL, 0, NULL);
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
    // 기존 sfx 닫기
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

void SoundManager::SetBGMVolume(int volume) {
    // volume 값의 범위는 0 (음소거) ~ 1000 (최대)
    if (volume < 0) volume = 0;
    if (volume > 1000) volume = 1000;

    std::string cmd = "setaudio bgm volume to " + std::to_string(volume);
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
}

void SoundManager::SetSFXVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 1000) volume = 1000;

    // 변수에 값을 저장
    currentSFXVolume = volume;

    std::string cmd = "setaudio sfx volume to " + std::to_string(volume);
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
}