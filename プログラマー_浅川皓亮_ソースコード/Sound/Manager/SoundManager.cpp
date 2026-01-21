#include "Sound/SoundManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "SaveManager.h"


// FMODの低レベルAPIのヘッダーをインクルードします。
// F_CALLBACK マクロが正しく定義されるように、FMOD_API_TRUE または FMOD_STUDIO_API_TRUE を定義します。
// これは、コンパイラがFMODのAPI関数の正しい呼び出し規約（例: __stdcall）を使用するように指示します。
#define FMOD_API_TRUE
#define FMOD_STUDIO_API_TRUE // FMOD Studio APIを使用している場合、これも定義します。

// FMODのコアAPIとStudio APIのヘッダー
#include "fmod_studio.h"
#include "fmod.h"
#include "fmod_common.h"       // F_CALLBACK マクロの定義が含まれることが多い
#include "fmod_studio.hpp"     // FMOD Studio APIのC++ラッパー
#include "fmod_studio_common.h" // FMOD Studioの共通定義（コールバック関連も含む）

// FMODAudioComponentの定義が必要なため、インクルードします。
// UFMODAudioComponent::EventInstance メンバーにアクセスするために必要です。
#include "FMODAudioComponent.h"
// =======================
// コンストラクタ
// =======================

USoundManager::USoundManager()
    : BGMVolume(1)
    , SEVolume(1)
    , mCurrentBGM(nullptr)
    , StartTime(0.f)

{
}
// =======================
// 初期化処理
// =======================

void USoundManager::Init()
{
    // 登録されているサウンドデータを AudioComponent に初期化
    for (auto& soundMap : SoundDataMap)
    {
        const ESoundKinds dataTag = soundMap.Key;
        FSoundData& soundData = soundMap.Value;
        soundData.AudioComponentMap.Reset();

        for (const auto& soundAssetPair : soundData.SoundAssetMap)
        {
            const FName waveTag = soundAssetPair.Key;
            USoundBase* sound = soundAssetPair.Value;
            if (waveTag.IsNone() || !sound)
                continue;

            // 既に登録済みならスキップ
            if (soundData.AudioComponentMap.Contains(waveTag))
                continue;

            // 2D サウンドコンポーネントを生成
            UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(this, sound);
            if (AudioComponent == nullptr)
                continue;

            AudioComponent->bAutoDestroy = false;

            soundData.AudioComponentMap.Add(waveTag, AudioComponent);
        }
    }

    SEVolume = USaveManager::GetSEVolume();
    BGMVolume = USaveManager::GetBGMVolume();
}

// 音量を保存
void USoundManager::SetVolume(float NewBGM, float NewSE)
{
    FVolumeSaveData data;
    data.BGMVolume = NewBGM;
    data.SEVolume = NewSE;
    USaveManager::SaveVolumeToJson(data);
}

// =======================
// 音量制御
// =======================

void USoundManager::SetBGMVolume(float NewVolume)
{
    const float PreviousBGMVolume = BGMVolume;
    BGMVolume = FMath::Clamp(NewVolume, 0.0f, 4.0f);

    if (!BGM)
    {
        return;
    }

    // 音量変更
    BGM->SetVolume(BGMVolume);

    // 音量が0から0以上になった場合は再生
    if (PreviousBGMVolume == 0.0f && BGMVolume > 0.0f && !BGM->IsPlaying())
    {
        BGM->Play();
    }
    // 音量が0になった場合は停止
    else if (BGMVolume == 0.0f && BGM->IsPlaying())
    {
        BGM->Stop();
    }
}

// SE 音量設定
void USoundManager::SetSEVolume(float vol)
{
    SEVolume = FMath::Clamp(vol, 0.0f, 4.0f);
}

// =======================
// サウンド再生処理
// =======================

bool USoundManager::PlaySound(
    ESoundKinds SoundType,
    FName SoundName,
    const bool SetVolume,
    const bool isLoop,
    float Volume,
    bool IsSpecifyLocation,
    FVector place)
{
    // BGM は別の専用関数で処理
    if (SoundType == ESoundKinds::BGM)
    {
        UE_LOG(LogTemp, Warning, TEXT("Use PlayBGM() for BGM playback"));
        return PlayBGM();
    }

    // サウンドデータの存在チェック
    if (!SoundDataMap.Contains(SoundType))
    {
        return false;
    }

    FSoundData& SoundData = SoundDataMap[SoundType];
    if (!SoundData.AudioComponentMap.Contains(SoundName))
    {
        return false;
    }

    UAudioComponent* AudioComponent = Cast<UAudioComponent>(SoundData.AudioComponentMap[SoundName]);
    if (!AudioComponent)
    {
        return false;
    }

    // 音量設定：カスタム音量 or デフォルト音量
    float FinalVolume = SetVolume ? Volume : SEVolume;
    FinalVolume = FMath::Clamp(FinalVolume, 0.0f, 1.0f);
    AudioComponent->SetVolumeMultiplier(FinalVolume);

    // 位置指定がある場合は3Dサウンドとして再生
    if (IsSpecifyLocation)
    {
        AudioComponent->SetWorldLocation(place);

        // 距離減衰を適用
        USoundAttenuation* AttenuationSettings = NewObject<USoundAttenuation>();
        if (AttenuationSettings)
        {
            AttenuationSettings->Attenuation.bAttenuate = true;
            AttenuationSettings->Attenuation.FalloffDistance = 2000.0f;
            AudioComponent->AttenuationSettings = AttenuationSettings;
        }
    }

    if (isLoop)
    {
        if (USoundWave* SoundWave = Cast<USoundWave>(AudioComponent->Sound))
        {
            SoundWave->bLooping = true;

            LoopSEMap.Add(SoundName, AudioComponent);
        }
    }

    // サウンド再生
    AudioComponent->Play();
    return true;
}


// =======================
// BGM 制御
// =======================

void USoundManager::StopBGM()
{
    if (BGM && BGM->IsPlaying())
    {
        BGM->Stop();
        UE_LOG(LogTemp, Log, TEXT("BGM stopped"));
    }
}

void USoundManager::StopSE(FName SoundName)
{
    if (SoundName.IsNone())
    {
        return;
    }

    // ループSEに登録されているかチェック
    UAudioComponent** FoundCompPtr = LoopSEMap.Find(SoundName);
    if (!FoundCompPtr)
    {
        return; // ループしていないSEなので無視
    }

    UAudioComponent* AudioComponent = *FoundCompPtr;
    if (AudioComponent)
    {
        // 再生中なら停止
        if (AudioComponent->IsPlaying())
        {
            AudioComponent->Stop();
        }

        // ループ解除（SoundWave の場合）
        if (USoundWave* SoundWave = Cast<USoundWave>(AudioComponent->Sound))
        {
            SoundWave->bLooping = false;
        }
    }

    // マップから削除
    LoopSEMap.Remove(SoundName);
}

bool USoundManager::PlayBGM()
{
    if (!BGMEventAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("BGMEventAsset is not set"));
        return false;
    }

    // 既存のBGMを停止
    if (BGM && BGM->IsPlaying())
    {
        BGM->Stop();
    }

    // BGMコンポーネントの初期化（初回のみ）
    if (!BGM)
    {
        BGM = NewObject<UFMODAudioComponent>(this);
        if (!BGM)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create FMOD Audio Component"));
            return false;
        }
        BGM->RegisterComponent();
    }


    // BGM再生
    BGM->SetEvent(BGMEventAsset);
    BGM->SetVolume(BGMVolume);
    BGM->Play();

    EventInstance = BGM->StudioInstance;
    StartTime = GetWorld()->GetTimeSeconds();
    return true;
}

void USoundManager::PauseBGM()
{
    if (BGM && BGM->IsPlaying())
    {
        BGM->SetPaused(true);
    }
}

void USoundManager::ResumeBGM()
{
    if (BGM && !BGM->IsPlaying())
    {
        BGM->SetPaused(false);
    }
}

