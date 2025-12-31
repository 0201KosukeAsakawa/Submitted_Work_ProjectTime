// Fill out your copyright notice in the Description page of Project Settings.


#include "SoundHandle.h"
#include "LevelManager.h"

TScriptInterface<ISoundManagerProvider> USoundHandle::GetSoundManager(UObject* WorldContext)
{
    if (!WorldContext)
    {
        UE_LOG(LogTemp, Warning, TEXT("SoundHandle: WorldContext is null"));
        return nullptr;
    }

    ALevelManager* LevelManager = ALevelManager::GetInstance(WorldContext);
    if (!LevelManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("SoundHandle: LevelManager not found"));
        return nullptr;
    }

    return LevelManager->GetSoundManager();
}

// ==========================
// 音量制御
// ==========================

void USoundHandle::SetVolume(UObject* WorldContext, float NewBGM, float NewSE)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        SoundManager->SetVolume(NewBGM, NewSE);
    }
}

void USoundHandle::SetBGMVolume(UObject* WorldContext, float Volume)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        SoundManager->SetBGMVolume(Volume);
    }
}

void USoundHandle::SetSEVolume(UObject* WorldContext, float Volume)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        SoundManager->SetSEVolume(Volume);
    }
}

float USoundHandle::GetBGMVolume(UObject* WorldContext)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        return SoundManager->GetBGMVolume();
    }
    return 0.0f;
}

float USoundHandle::GetSEVolume(UObject* WorldContext)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        return SoundManager->GetSEVolume();
    }
    return 0.0f;
}

// ==========================
// BGM制御
// ==========================

void USoundHandle::StopBGM(UObject* WorldContext)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        SoundManager->StopBGM();
    }
}

void USoundHandle::StopSE(UObject* WorldContext, FName SEName)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        SoundManager->StopSE(SEName);
    }
}

// ==========================
// サウンド再生
// ==========================

bool USoundHandle::PlaySound(
    UObject* WorldContext,
    ESoundKinds DataID,
    FName SoundID,
    bool SetVolume,
    bool isLoop,
    float Volume,
    bool IsSpecifyLocation,
    FVector Location)
{
    if (TScriptInterface<ISoundManagerProvider> SoundManager = GetSoundManager(WorldContext))
    {
        return SoundManager->PlaySound(DataID, SoundID, isLoop, SetVolume, Volume, IsSpecifyLocation, Location);
    }
    return false;
}

// ==========================
// 便利関数
// ==========================

bool USoundHandle::PlaySE(UObject* WorldContext, FName SoundID , bool isLoop)
{
    return PlaySound(WorldContext, ESoundKinds::SE, SoundID,isLoop , false, 1.0f, false, FVector::ZeroVector);
}

bool USoundHandle::PlaySEAtLocation(UObject* WorldContext, FName SoundID, FVector Location, bool isLoop)
{
    return PlaySound(WorldContext, ESoundKinds::SE, SoundID, isLoop, false, 1.0f, true, Location);
}