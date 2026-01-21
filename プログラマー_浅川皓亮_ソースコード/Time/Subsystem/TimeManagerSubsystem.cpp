// Fill out your copyright notice in the Description page of Project Settings.


#include "SubSystem/TimeManagerSubsystem.h"
#include "Component/TimeManipulatorComponent.h"
#include "LevelManager.h"
#include "Interface/UIManagerProvider.h"

#include "UIHandle.h"
#include "SoundHandle.h"
#include "SaveManager.h"


FOnSlowStopped UTimeManagerSubsystem::OnSlowStopped;

void UTimeManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Initialized"));
}

void UTimeManagerSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// 処理の流れ:
// 1. Playerならプレイヤーコンポーネントに設定
// 2. それ以外ならワールドコンポーネントに追加
void UTimeManagerSubsystem::RegisterTimeComponent(UTimeManipulatorComponent* Component, bool bIsPlayer)
{
    if (!Component)
    {
        return;
    }

    if (bIsPlayer)
    {
        PlayerComponent = Component;
        UE_LOG(LogTemp, Log, TEXT("TimeManager: Registered player component"));
    }
    else
    {
        WorldComponents.AddUnique(Component);
        UE_LOG(LogTemp, Log, TEXT("TimeManager: Registered world component (Total: %d)"),
            WorldComponents.Num());
    }
}

// 処理の流れ:
// 1. 該当コンポーネントを削除
void UTimeManagerSubsystem::UnregisterTimeComponent(UTimeManipulatorComponent* Component)
{
    if (!Component)
    {
        return;
    }

    if (PlayerComponent == Component)
    {
        PlayerComponent.Reset();
    }

    WorldComponents.Remove(Component);
}

// 処理の流れ:
// 1. 無効なコンポーネントを削除
// 2. 全コンポーネントの巻き戻しを開始
void UTimeManagerSubsystem::RewindWorld()
{
    // 無効なコンポーネントを削除
    WorldComponents.RemoveAll([](const TWeakObjectPtr<UTimeManipulatorComponent>& Comp) {
        return !Comp.IsValid();
        });

    int32 ComponentsRewound = 0;

    // spanを使用してコピーを避ける（C++20）
    // または直接イテレート
    for (const auto& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->StartRewind();
            ComponentsRewound++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManager: Rewound %d components"), ComponentsRewound);
}

// 処理の流れ:
// 1. 全コンポーネントにTimeDilationを設定
// 2. タイマーで自動リセット
void UTimeManagerSubsystem::StartSlowMotion(float SlowScale)
{
    SetCustomTimeDilationWorld(SlowScale);

    if (GetWorld()->GetTimerManager().IsTimerActive(SlowMotionTimerHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(SlowMotionTimerHandle);
    }

    GetWorld()->GetTimerManager().SetTimer(
        SlowMotionTimerHandle,
        this,
        &UTimeManagerSubsystem::ResetTimeDilation,
        SlowMotionDuration,
        false
    );

    UE_LOG(LogTemp, Log, TEXT("TimeManager: Slow motion started (Scale=%.2f)"), SlowScale);
}

// 処理の流れ:
// 1. TimeDilationを通常に戻す
void UTimeManagerSubsystem::ResetTimeDilation()
{
    SetCustomTimeDilationWorld(1.0f);
    OnSlowStopped.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("TimeManager: Time dilation reset"));
}

// 処理の流れ:
// 1. 全コンポーネントに品質設定を適用
void UTimeManagerSubsystem::SetRewindQuality(ERewindQuality Quality)
{
    for (const auto& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->SetRewindQuality(Quality);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManager: Applied quality %d to all components"),
        static_cast<int32>(Quality));
}

// 処理の流れ:
// 1. 全コンポーネントにTimeDilationを設定
void UTimeManagerSubsystem::SetCustomTimeDilationWorld(float Scale)
{
    int32 ComponentsAffected = 0;

    for (const auto& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            if (AActor* Owner = Comp->GetOwner())
            {
                Owner->CustomTimeDilation = Scale;
                ComponentsAffected++;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManager: Set dilation %.2f for %d components"),
        Scale, ComponentsAffected);
}