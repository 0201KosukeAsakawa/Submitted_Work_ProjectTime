// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Soundable.generated.h"

/**
 * @brief サウンドタイプ（BGM/SE）
 */
UENUM(BlueprintType)
enum class ESoundKinds : uint8
{
    BGM UMETA(DisplayName = "BGM"),
    SE  UMETA(DisplayName = "SE")
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USoundManagerProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class CARRY_API ISoundManagerProvider
{
    GENERATED_BODY()

    // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    /**
     * @brief BGM/SE音量を設定
     * @param NewBGM BGM音量（0-1）
     * @param NewSE SE音量（0-1）
     */
    virtual void SetVolume(float NewBGM, float NewSE) {}

    /**
     * @brief BGM音量設定
     * @param vol 音量（0-1）
     */
    virtual void SetBGMVolume(float vol) {}

    /**
     * @brief SE音量設定
     * @param vol 音量（0-1）
     */
    virtual void SetSEVolume(float vol) {}

    /** @brief BGM停止 */
    virtual void StopBGM() {}

    /**
    *@brief SE停止
    *
    * @param 停止させるSE名
    */
    virtual void StopSE(FName SoundName) {}

    /** @brief 現在のBGM音量取得 */
    virtual float GetBGMVolume() const { return 0.f; }

    /** @brief 現在のSE音量取得 */
    virtual float GetSEVolume() const { return 0.f; }

    // ==========================
    // ==== サウンド再生関数 ====
    // ==========================

    /**
     * @brief サウンド再生
     * @param SoundType BGM or SE
     * @param SoundName サウンド名
     * @param SetVolume 音量設定を反映するか
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
        FVector place = FVector(0.0f, 0.0f, 0.0f)) {
        return false;
    }

};
