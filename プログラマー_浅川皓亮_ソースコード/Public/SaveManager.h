// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SaveData.h"
#include "SaveManager.generated.h"

UCLASS()
class CARRY_API USaveManager : public UObject
{
    GENERATED_BODY()
public:
    // =======================
    // 音量設定
    // =======================
    /**
     * @brief ボリュームデータを JSON 形式で保存する。
     *
     * @param InData 保存対象のボリューム設定データ。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static void SaveVolumeToJson(const FVolumeSaveData& InData);

    /**
     * @brief JSON 形式のボリューム設定データをロードする。
     *
     * @return 読み込んだボリューム設定データ。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static FVolumeSaveData LoadVolumeFromJson();

    /**
     * @brief 現在の BGM 音量を取得する。
     *
     * @return BGM 音量（0.0f ～ 1.0f）。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static float GetBGMVolume();

    /**
     * @brief 現在の SE 音量を取得する。
     *
     * @return SE 音量（0.0f ～ 1.0f）。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static float GetSEVolume();

    /**
     * @brief BGM と SE の音量を設定する。
     *
     * @param NewBGM 設定する BGM 音量（0.0f ～ 1.0f）。
     * @param NewSE 設定する SE 音量（0.0f ～ 1.0f）。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static void SetVolume(float NewBGM, float NewSE);

    // =======================
    // カメラ設定
    // =======================
    /**
     * @brief カメラ設定データを JSON 形式で保存する。
     *
     * @param InData 保存対象のカメラ設定データ。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static void SaveCameraToJson(const FCameraSaveData& InData);

    /**
     * @brief JSON 形式のカメラ設定データをロードする。
     *
     * @return 読み込んだカメラ設定データ。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static FCameraSaveData LoadCameraFromJson();

    /**
     * @brief カメラ感度を取得する。
     *
     * @return カメラ感度。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static float GetCameraSensitivity();

    /**
     * @brief カメラシェイクのオン/オフ状態を取得する。
     *
     * @return カメラシェイクが有効な場合 true。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static bool GetCameraShakeEnabled();

    /**
     * @brief カメラ設定を保存する。
     *
     * @param NewSensitivity 新しいカメラ感度。
     * @param bNewShakeEnabled 新しいカメラシェイクのオン/オフ状態。
     */
    UFUNCTION(BlueprintCallable, Category = "Save")
    static void SetCameraSettings(float NewSensitivity, bool bNewShakeEnabled);

    // =======================
    // リワインド品質データの保存・読み込み
    // =======================

    // リワインド品質データを JSON ファイルに保存
    UFUNCTION(BlueprintCallable, Category = "Save Manager|Rewind")
    static void SaveRewindQualityToJson(const FRewindQualitySaveData& InData);

    // リワインド品質データを JSON ファイルから読み込み
    UFUNCTION(BlueprintCallable, Category = "Save Manager|Rewind")
    static FRewindQualitySaveData LoadRewindQualityFromJson();

    // =======================
    // リワインド品質データの取得・設定
    // =======================

    // リワインド品質を取得
    UFUNCTION(BlueprintPure, Category = "Save Manager|Rewind")
    static ERewindQuality GetRewindQuality();

    // リワインド品質を保存
    UFUNCTION(BlueprintCallable, Category = "Save Manager|Rewind")
    static void SetRewindQuality(ERewindQuality NewQuality);
};