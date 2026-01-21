// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Component/TimeManipulatorComponent.h"
#include "SaveData.generated.h"
 

// =======================
// 音量設定のセーブデータ
// =======================
USTRUCT(BlueprintType)
struct FVolumeSaveData
{
    GENERATED_BODY()

    // BGM 音量
    UPROPERTY(BlueprintReadWrite)
    float BGMVolume = 2.0f;

    // 効果音(SE) 音量
    UPROPERTY(BlueprintReadWrite)
    float SEVolume = 2.0f;

    // JSON に変換
    TSharedPtr<FJsonObject> ToJson() const
    {
        TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
        Obj->SetNumberField("BGMVolume", BGMVolume);
        Obj->SetNumberField("SEVolume", SEVolume);
        return Obj;
    }

    // JSON から復元
    static FVolumeSaveData FromJson(const TSharedPtr<FJsonObject>& Obj)
    {
        FVolumeSaveData Data;
        Data.BGMVolume = Obj->GetNumberField("BGMVolume");
        Data.SEVolume = Obj->GetNumberField("SEVolume");
        return Data;
    }
};

// =======================
// カメラ設定のセーブデータ
// =======================
USTRUCT(BlueprintType)
struct FCameraSaveData
{
    GENERATED_BODY()

    // カメラ感度
    UPROPERTY(BlueprintReadWrite)
    float CameraSensitivity = 0.05f;

    // カメラシェイクのオン/オフ
    UPROPERTY(BlueprintReadWrite)
    bool bCameraShakeEnabled = true;

    // JSON に変換
    TSharedPtr<FJsonObject> ToJson() const
    {
        TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
        Obj->SetNumberField("CameraSensitivity", CameraSensitivity);
        Obj->SetBoolField("CameraShakeEnabled", bCameraShakeEnabled);
        return Obj;
    }

    // JSON から復元
    static FCameraSaveData FromJson(const TSharedPtr<FJsonObject>& Obj)
    {
        FCameraSaveData Data;
        Data.CameraSensitivity = Obj->GetNumberField("CameraSensitivity");
        Data.bCameraShakeEnabled = Obj->GetBoolField("CameraShakeEnabled");
        return Data;
    }
};

// =======================
// リワインド品質のセーブデータ
// =======================
USTRUCT(BlueprintType)
struct FRewindQualitySaveData
{
    GENERATED_BODY()

    // 逆再生のクオリティ
    UPROPERTY(BlueprintReadWrite)
    ERewindQuality RewindQuality = ERewindQuality::Ultra;

    // JSON に変換
    TSharedPtr<FJsonObject> ToJson() const
    {
        TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
        Obj->SetNumberField("RewindQuality", static_cast<int32>(RewindQuality));
        return Obj;
    }

    // JSON から復元
    static FRewindQualitySaveData FromJson(const TSharedPtr<FJsonObject>& Obj)
    {
        FRewindQualitySaveData Data;
        Data.RewindQuality = static_cast<ERewindQuality>(static_cast<int32>(Obj->GetNumberField("RewindQuality")));
        return Data;
    }
};