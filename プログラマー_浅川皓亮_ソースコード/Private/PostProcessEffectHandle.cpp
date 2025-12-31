// Fill out your copyright notice in the Description page of Project Settings.


#include "PostProcessEffectHandle.h"
#include "Component/LevelEffectComponent.h"
#include "LevelManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

TMap<TWeakObjectPtr<UWorld>, UPostProcessEffectHandle::FRadialTransitionData> UPostProcessEffectHandle::TransitionDataMap;

UPostProcessEffectManager* UPostProcessEffectHandle::GetManager(UObject* WorldContext)
{
    if (!WorldContext)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectHandle: WorldContext is null"));
        return nullptr;
    }

    ALevelManager* LevelManager = ALevelManager::GetInstance(WorldContext);
    if (!LevelManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectHandle: LevelManager not found"));
        return nullptr;
    }

    // キーを生成してアクセス（このクラスのみ可能）
    FManagerAccessKey Key;
    return LevelManager->GetPostProcessEffectManager(Key);
};

void UPostProcessEffectHandle::ActivateEffect(UObject* WorldContext, EPostProcessEffectTag Tag, bool bInstant)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        Manager->ActivateEffect(Tag, bInstant);
    }
}

void UPostProcessEffectHandle::DeactivateEffect(UObject* WorldContext, EPostProcessEffectTag Tag, bool bInstant)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        Manager->DeactivateEffect(Tag, bInstant);
    }
}

void UPostProcessEffectHandle::ClearAllEffects(UObject* WorldContext, bool bInstant)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        Manager->ClearAllEffects(bInstant);
    }
}

bool UPostProcessEffectHandle::IsEffectActive(UObject* WorldContext, EPostProcessEffectTag Tag)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        return Manager->IsEffectActive(Tag);
    }
    return false;
}

void UPostProcessEffectHandle::SetEffectWeight(UObject* WorldContext, EPostProcessEffectTag Tag, float Weight)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        Manager->SetEffectWeight(Tag, Weight);
    }
}

void UPostProcessEffectHandle::SetEffectScalarParameter(UObject* WorldContext, EPostProcessEffectTag Tag, FName ParameterName, float Value)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        Manager->SetEffectScalarParameter(Tag, ParameterName, Value);
    }
}

void UPostProcessEffectHandle::SetEffectVectorParameter(UObject* WorldContext, EPostProcessEffectTag Tag, FName ParameterName, FLinearColor Value)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        Manager->SetEffectVectorParameter(Tag, ParameterName, Value);
    }
}

void UPostProcessEffectHandle::SetEffectTextureParameter(UObject* WorldContext, EPostProcessEffectTag Tag, FName ParameterName, UTexture* Value)
{
    if (UPostProcessEffectManager* Manager = GetManager(WorldContext))
    {
        Manager->SetEffectTextureParameter(Tag, ParameterName, Value);
    }
}

//エフェクトを広げる


void UPostProcessEffectHandle::StartRadialTransition(UObject* WorldContext, float Duration)
{
    if (!WorldContext)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectHandle: WorldContext is null"));
        return;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectHandle: World is null"));
        return;
    }

    // エフェクトをアクティブ化
    ActivateEffect(WorldContext, EPostProcessEffectTag::SlowMotion, true);

    // トランジションデータを取得または作成
    FRadialTransitionData& Data = TransitionDataMap.FindOrAdd(World);
    Data.CurrentProgress = 0.0f;
    Data.TargetProgress = 1.0f;
    Data.Duration = Duration;
    Data.ElapsedTime = 0.0f;
    Data.bIsActive = true;

    // 既存のタイマーをクリア
    if (Data.TimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(Data.TimerHandle);
    }

    // 初期値を設定
    SetEffectScalarParameter(WorldContext, EPostProcessEffectTag::SlowMotion, TEXT("Progress"), 0.0f);

    // 更新タイマーを設定
    World->GetTimerManager().SetTimer(
        Data.TimerHandle,
        FTimerDelegate::CreateLambda([WorldContext, World]()
            {
                if (World && WorldContext)
                {
                    UpdateRadialTransition(WorldContext, World->GetDeltaSeconds());
                }
            }),
        0.016f,  // 約60fps
        true
    );

    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectHandle: Started radial transition (Duration: %.2f)"), Duration);
}

void UPostProcessEffectHandle::CompleteRadialTransition(UObject* WorldContext)
{
    if (!WorldContext)
        return;

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
        return;

    FRadialTransitionData* Data = TransitionDataMap.Find(World);
    if (!Data)
        return;

    // 即座に完了
    SetEffectScalarParameter(WorldContext, EPostProcessEffectTag::SlowMotion, TEXT("Progress"), 1.0f);

    // タイマーをクリア
    if (Data->TimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(Data->TimerHandle);
    }

    Data->bIsActive = false;

    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectHandle: Completed radial transition instantly"));
}

void UPostProcessEffectHandle::ReverseRadialTransition(UObject* WorldContext, float Duration)
{
    if (!WorldContext)
        return;

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
        return;

    // トランジションデータを取得または作成
    FRadialTransitionData& Data = TransitionDataMap.FindOrAdd(World);
    Data.CurrentProgress = 1.0f;
    Data.TargetProgress = 0.0f;
    Data.Duration = Duration;
    Data.ElapsedTime = 0.0f;
    Data.bIsActive = true;

    // 既存のタイマーをクリア
    if (Data.TimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(Data.TimerHandle);
    }

    // 更新タイマーを設定
    World->GetTimerManager().SetTimer(
        Data.TimerHandle,
        FTimerDelegate::CreateLambda([WorldContext, World]()
            {
                if (World && WorldContext)
                {
                    UpdateRadialTransition(WorldContext, World->GetDeltaSeconds());
                }
            }),
        0.016f,
        true
    );

    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectHandle: Started reverse radial transition (Duration: %.2f)"), Duration);
}

void UPostProcessEffectHandle::SetTransitionProgress(UObject* WorldContext, float Progress)
{
    if (!WorldContext)
        return;

    Progress = FMath::Clamp(Progress, 0.0f, 1.0f);

    // Progressパラメータを直接設定
    SetEffectScalarParameter(WorldContext, EPostProcessEffectTag::SlowMotion, TEXT("Progress"), Progress);

    UE_LOG(LogTemp, Verbose, TEXT("PostProcessEffectHandle: Set transition progress to %.2f"), Progress);
}

void UPostProcessEffectHandle::UpdateRadialTransition(UObject* WorldContext, float DeltaTime)
{
    if (!WorldContext)
        return;

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
        return;

    FRadialTransitionData* Data = TransitionDataMap.Find(World);
    if (!Data || !Data->bIsActive)
        return;

    // 時間を進める
    Data->ElapsedTime += DeltaTime * 2;

    // 進行度を計算（イージング付き）
    float T = FMath::Clamp(Data->ElapsedTime / Data->Duration, 0.0f, 1.0f);

    // スムーズなイージング（EaseInOutCubic）
    float EasedT = T < 0.5f
        ? 4.0f * T * T * T
        : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 3.0f) / 2.0f;

    // 現在の進行度を計算
    Data->CurrentProgress = FMath::Lerp(
        Data->TargetProgress == 1.0f ? 0.0f : 1.0f,  // 開始値
        Data->TargetProgress,                         // 終了値
        EasedT
    );

    // マテリアルパラメータを更新
    SetEffectScalarParameter(WorldContext, EPostProcessEffectTag::SlowMotion, TEXT("Progress"), Data->CurrentProgress);

    // 完了チェック
    if (T >= 1.0f)
    {
        Data->bIsActive = false;

        // タイマーをクリア
        if (Data->TimerHandle.IsValid())
        {
            World->GetTimerManager().ClearTimer(Data->TimerHandle);
        }

        // 逆再生の場合はエフェクトを無効化
        if (Data->TargetProgress <= 0.0f)
        {
            DeactivateEffect(WorldContext, EPostProcessEffectTag::SlowMotion, true);
        }

        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectHandle: Radial transition completed (Progress: %.2f)"),
            Data->CurrentProgress);
    }
}


// ============================================
// プリセット関数
// ============================================

void UPostProcessEffectHandle::StartSlowMotionEffect(UObject* WorldContext)
{
    ActivateEffect(WorldContext, EPostProcessEffectTag::SlowMotion, false);
}

void UPostProcessEffectHandle::StopSlowMotionEffect(UObject* WorldContext)
{
    DeactivateEffect(WorldContext, EPostProcessEffectTag::SlowMotion, false);
}
