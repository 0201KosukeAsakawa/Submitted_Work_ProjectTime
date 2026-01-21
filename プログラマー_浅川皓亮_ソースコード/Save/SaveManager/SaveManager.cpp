// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveManager.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "SaveData.h"



// =======================
// 音量データの保存・読み込み
// =======================
// 音量データを JSON ファイルに保存
void USaveManager::SaveVolumeToJson(const FVolumeSaveData& InData)
{
    FString SavePath = FPaths::ProjectSavedDir() + "VolumeSave.json";
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(InData.ToJson().ToSharedRef(), Writer);
    FFileHelper::SaveStringToFile(OutputString, *SavePath);
}

// 音量データを JSON ファイルから読み込み
FVolumeSaveData USaveManager::LoadVolumeFromJson()
{
    FString SavePath = FPaths::ProjectSavedDir() + "VolumeSave.json";
    FString Input;
    FVolumeSaveData LoadedData;

    // ファイルが存在すれば読み込み
    if (FFileHelper::LoadFileToString(Input, *SavePath))
    {
        TSharedPtr<FJsonObject> Json;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Input);
        if (FJsonSerializer::Deserialize(Reader, Json))
        {
            LoadedData = FVolumeSaveData::FromJson(Json);
        }
    }
    else
    {
        // ファイルが存在しない場合はデフォルト作成
        UE_LOG(LogTemp, Warning, TEXT("Volume save file not found, creating new with default values."));
        SaveVolumeToJson(LoadedData);
    }

    return LoadedData;
}

// =======================
// 音量データの取得・設定
// =======================
// BGM 音量を取得
float USaveManager::GetBGMVolume()
{
    return LoadVolumeFromJson().BGMVolume;
}

// SE 音量を取得
float USaveManager::GetSEVolume()
{
    return LoadVolumeFromJson().SEVolume;
}

// 音量を保存（BGM/SE 両方）
void USaveManager::SetVolume(float NewBGM, float NewSE)
{
    FVolumeSaveData VolumeData;
    VolumeData.BGMVolume = NewBGM;
    VolumeData.SEVolume = NewSE;

    // JSON に保存
    SaveVolumeToJson(VolumeData);
}

// =======================
// カメラデータの保存・読み込み
// =======================
// カメラデータを JSON ファイルに保存
void USaveManager::SaveCameraToJson(const FCameraSaveData& InData)
{
    FString SavePath = FPaths::ProjectSavedDir() + "CameraSave.json";
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(InData.ToJson().ToSharedRef(), Writer);
    FFileHelper::SaveStringToFile(OutputString, *SavePath);
}

// カメラデータを JSON ファイルから読み込み
FCameraSaveData USaveManager::LoadCameraFromJson()
{
    FString SavePath = FPaths::ProjectSavedDir() + "CameraSave.json";
    FString Input;
    FCameraSaveData LoadedData;

    // ファイルが存在すれば読み込み
    if (FFileHelper::LoadFileToString(Input, *SavePath))
    {
        TSharedPtr<FJsonObject> Json;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Input);
        if (FJsonSerializer::Deserialize(Reader, Json))
        {
            LoadedData = FCameraSaveData::FromJson(Json);
        }
    }
    else
    {
        // ファイルが存在しない場合はデフォルト作成
        UE_LOG(LogTemp, Warning, TEXT("Camera save file not found, creating new with default values."));
        SaveCameraToJson(LoadedData);
    }

    return LoadedData;
}

// =======================
// カメラ設定の取得・設定
// =======================
// カメラ感度を取得
float USaveManager::GetCameraSensitivity()
{
    return LoadCameraFromJson().CameraSensitivity;
}

// カメラシェイクのオン/オフ状態を取得
bool USaveManager::GetCameraShakeEnabled()
{
    return LoadCameraFromJson().bCameraShakeEnabled;
}

// カメラ設定を保存
void USaveManager::SetCameraSettings(float NewSensitivity, bool bNewShakeEnabled)
{
    FCameraSaveData CameraData;
    CameraData.CameraSensitivity = NewSensitivity;
    CameraData.bCameraShakeEnabled = bNewShakeEnabled;

    // JSON に保存
    SaveCameraToJson(CameraData);
}

// =======================
// リワインド品質データの保存・読み込み
// =======================

// リワインド品質データを JSON ファイルに保存
void USaveManager::SaveRewindQualityToJson(const FRewindQualitySaveData& InData)
{
    FString SavePath = FPaths::ProjectSavedDir() + "RewindQualitySave.json";
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(InData.ToJson().ToSharedRef(), Writer);
    FFileHelper::SaveStringToFile(OutputString, *SavePath);

    UE_LOG(LogTemp, Log, TEXT("SaveManager: Rewind quality saved (Quality: %d)"), static_cast<int32>(InData.RewindQuality));
}

// リワインド品質データを JSON ファイルから読み込み
FRewindQualitySaveData USaveManager::LoadRewindQualityFromJson()
{
    FString SavePath = FPaths::ProjectSavedDir() + "RewindQualitySave.json";
    FString Input;
    FRewindQualitySaveData LoadedData;

    // ファイルが存在すれば読み込み
    if (FFileHelper::LoadFileToString(Input, *SavePath))
    {
        TSharedPtr<FJsonObject> Json;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Input);
        if (FJsonSerializer::Deserialize(Reader, Json))
        {
            LoadedData = FRewindQualitySaveData::FromJson(Json);
            UE_LOG(LogTemp, Log, TEXT("SaveManager: Rewind quality loaded (Quality: %d)"), static_cast<int32>(LoadedData.RewindQuality));
        }
    }
    else
    {
        // ファイルが存在しない場合はデフォルト作成
        UE_LOG(LogTemp, Warning, TEXT("SaveManager: Rewind quality save file not found, creating new with default values."));
        SaveRewindQualityToJson(LoadedData);
    }

    return LoadedData;
}

// =======================
// リワインド品質データの取得・設定
// =======================

// リワインド品質を取得
ERewindQuality USaveManager::GetRewindQuality()
{
    return LoadRewindQualityFromJson().RewindQuality;
}

// リワインド品質を保存
void USaveManager::SetRewindQuality(ERewindQuality NewQuality)
{
    FRewindQualitySaveData QualityData;
    QualityData.RewindQuality = NewQuality;

    // JSON に保存
    SaveRewindQualityToJson(QualityData);
}