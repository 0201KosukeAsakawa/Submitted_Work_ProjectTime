// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interface/Soundable.h"
#include "SoundHandle.generated.h"

/**
 * 
 */
UCLASS()
class CARRY_API USoundHandle : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ==========================
    // ==== 音量制御 ====
    // ==========================

    /**
     * @brief BGM/SE音量を一括設定
     * @param WorldContext ワールドコンテキスト
     * @param NewBGM BGM音量（0-1）
     * @param NewSE SE音量（0-1）
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|Volume", meta = (WorldContext = "WorldContext"))
    static void SetVolume(UObject* WorldContext, float NewBGM, float NewSE);

    /**
     * @brief BGM音量設定
     * @param WorldContext ワールドコンテキスト
     * @param Volume 音量（0-1）
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|Volume", meta = (WorldContext = "WorldContext"))
    static void SetBGMVolume(UObject* WorldContext, float Volume);

    /**
     * @brief SE音量設定
     * @param WorldContext ワールドコンテキスト
     * @param Volume 音量（0-1）
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|Volume", meta = (WorldContext = "WorldContext"))
    static void SetSEVolume(UObject* WorldContext, float Volume);

    /**
     * @brief 現在のBGM音量を取得
     * @param WorldContext ワールドコンテキスト
     * @return float BGM音量（0-1）
     */
    UFUNCTION(BlueprintPure, Category = "Sound|Volume", meta = (WorldContext = "WorldContext"))
    static float GetBGMVolume(UObject* WorldContext);

    /**
     * @brief 現在のSE音量を取得
     * @param WorldContext ワールドコンテキスト
     * @return float SE音量（0-1）
     */
    UFUNCTION(BlueprintPure, Category = "Sound|Volume", meta = (WorldContext = "WorldContext"))
    static float GetSEVolume(UObject* WorldContext);

    // ==========================
    // ==== BGM制御 ====
    // ==========================

    /**
     * @brief BGMを停止
     * @param WorldContext ワールドコンテキスト
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|BGM", meta = (WorldContext = "WorldContext"))
    static void StopBGM(UObject* WorldContext);


    /**
     * @brief BGMを停止
     * @param WorldContext ワールドコンテキスト
     * @param SEName       停止させるSE名
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|SE", meta = (WorldContext = "WorldContext"))
    static void StopSE(UObject* WorldContext , FName SEName);
    // ==========================
    // ==== サウンド再生 ====
    // ==========================

    /**
     * @brief サウンドを再生
     * @param WorldContext ワールドコンテキスト
     * @param DataID サウンドデータID（"BGM", "SE"など）
     * @param SoundID 再生するサウンドのID
     * @param SetVolume カスタム音量を使用するか
     * @param Volume カスタム音量（0-1）
     * @param IsSpecifyLocation 位置指定再生するか
     * @param Location 再生位置
     * @return bool 再生成功か否か
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|Playback", meta = (WorldContext = "WorldContext", AdvancedDisplay = "SetVolume,Volume,IsSpecifyLocation,Location"))
    static bool PlaySound(
        UObject* WorldContext,
        ESoundKinds DataID,
        FName SoundID,
        bool isLoop = false,
        bool SetVolume = false,
        float Volume = 1.0f,
        bool IsSpecifyLocation = false,
        FVector Location = FVector::ZeroVector
    );
    // ==========================
    // ==== 便利関数 ====
    // ==========================

    /**
     * @brief SEを簡易再生（デフォルト音量で2D再生）
     * @param WorldContext ワールドコンテキスト
     * @param SoundID 再生するSEのID
     * @param isLoop 繰り返し再生するか
     * @return bool 再生成功か否か
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|Playback", meta = (WorldContext = "WorldContext"))
    static bool PlaySE(UObject* WorldContext, FName SoundID ,const bool isLoop = false);

    /**
     * @brief SEを位置指定で再生
     * @param WorldContext ワールドコンテキスト
     * @param SoundID 再生するSEのID
     * @param Location 再生位置
     * @param isLoop 繰り返し再生させるか
     * @return bool 再生成功か否か
     */
    UFUNCTION(BlueprintCallable, Category = "Sound|Playback", meta = (WorldContext = "WorldContext"))
    static bool PlaySEAtLocation(UObject* WorldContext, FName SoundID, FVector Location , bool isLoop);

private:
    /**
     * @brief SoundManagerインスタンスを取得
     * @param WorldContext ワールドコンテキスト
     * @return ISoundableProvider インターフェース
     */
    static TScriptInterface<ISoundManagerProvider> GetSoundManager(UObject* WorldContext);
};