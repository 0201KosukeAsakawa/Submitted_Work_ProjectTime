#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "Components/ActorComponent.h"
#include "Interface/Soundable.h"
#include "fmod_studio.hpp"     // FMOD Studio APIのC++ラッパー
#include "SoundManager.generated.h"

class UFMODAudioComponent;
class UFMODEvent;

// サウンドデータを格納する構造体
USTRUCT()
struct FSoundData : public FTableRowBase
{
    GENERATED_USTRUCT_BODY()
public:
    // 再生対象の音（SoundWave or SoundCue）を保持
    UPROPERTY(EditAnywhere, Category = "Sound")
    TMap<FName, USoundBase*> SoundAssetMap;

    // AudioComponent（再生時に生成される）を保持
    UPROPERTY(Transient)
    TMap<FName, UAudioComponent*> AudioComponentMap;
};

// サウンドを管理するクラス
/**
 * @brief サウンド管理コンポーネント
 * BGM/SEの再生、音量管理、フェードイン/アウト、Beat連動処理を担当
 */
UCLASS(Blueprintable)
class CARRY_API USoundManager : public UObject, public ISoundManagerProvider
{
    GENERATED_BODY()

    friend class ALevelManager;

public:
    /** @brief コンストラクタ：デフォルト値設定 */
    USoundManager();

    /** @brief サウンドマネージャー初期化処理 */
    void Init();

private:
    // ==========================
    // ==== ボリューム管理関数 ===
    // ==========================

    /**
     * @brief BGM/SE音量を設定
     * @param NewBGM BGM音量（0-1）
     * @param NewSE SE音量（0-1）
     */
    UFUNCTION(BlueprintCallable)
    void SetVolume(float NewBGM, float NewSE)override;

    /**
     * @brief BGM音量設定
     * @param vol 音量（0-1）
     */
    UFUNCTION(BlueprintCallable)
    void SetBGMVolume(float vol)override;

    /**
     * @brief SE音量設定
     * @param vol 音量（0-1）
     */
    UFUNCTION(BlueprintCallable)
    virtual void SetSEVolume(float vol)override;

    /** @brief BGM停止 */
    void StopBGM()override;

    void StopSE(FName SoundName)override;

    void PauseBGM();

    void ResumeBGM();

    /** @brief 現在のBGM音量取得 */
    UFUNCTION(BlueprintCallable)
    float GetBGMVolume() const override { return BGMVolume; }

    /** @brief 現在のSE音量取得 */
    UFUNCTION(BlueprintCallable)
    float GetSEVolume() const override { return SEVolume; }

    // ==========================
    // ==== サウンド再生関数 ====
    // ==========================

    /**
     * @brief サウンド再生
     * @param SoundType BGM or SE
     * @param SoundName サウンド名
     * @param SetVolume 音量設定を反映するか
     * @param isLoop ループ再生させるか
     * @param Volume 音量（0-1）
     * @param IsSpecifyLocation 指定位置で再生するか
     * @param place 再生位置（IsSpecifyLocation=true時に使用）
     * @return bool 再生成功か否か
     */

    virtual bool PlaySound(
        ESoundKinds SoundType,
        FName SoundName,
        const bool isLoop = false,
        const bool SetVolume = false,
        float Volume = 1,
        bool IsSpecifyLocation = false,
        FVector place = FVector(0.0f, 0.0f, 0.0f))override;

    /** @brief BGM再生開始 */
    bool PlayBGM();

private:
    // ==========================
    // ==== サウンドデータ ======
    // ==========================

    /** @brief サウンドデータ保持マップ
     * Key: データID, Value: FSoundData
     */
    UPROPERTY(EditAnywhere, Category = "Sound")
    TMap<ESoundKinds, FSoundData> SoundDataMap;

    UPROPERTY()
    TMap<FName, UAudioComponent*> LoopSEMap;

    // ==========================
    // ==== BGM関連コンポーネント ====
    // ==========================

    /** @brief 現在再生中のBGM AudioComponent */
    UPROPERTY()
    UFMODAudioComponent* mCurrentBGM;

    /** @brief BGM用AudioComponent */
    UPROPERTY()
    UFMODAudioComponent* BGM;

    /** @brief FMOD EventAsset（BGM用） */
    UPROPERTY(EditAnywhere, Category = "FMOD")
    UFMODEvent* BGMEventAsset;

    /** @brief FMOD EventInstance */
    FMOD::Studio::EventInstance* EventInstance;

    /** @brief FMOD AudioComponent参照 */
    UPROPERTY()
    UFMODAudioComponent* FMODAudioComponent;

    /** @brief BGM音量 */
    float BGMVolume;

    /** @brief SE音量 */
    float SEVolume;

    /** @brief 再生開始時間 */
    float StartTime;
};
