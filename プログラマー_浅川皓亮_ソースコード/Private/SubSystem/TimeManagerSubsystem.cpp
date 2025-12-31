// Fill out your copyright notice in the Description page of Project Settings.


#include "SubSystem/TimeManagerSubsystem.h"
#include "Component/TimeManipulatorComponent.h"
#include "LevelManager.h"
#include "Interface/UIManagerProvider.h"

#include "UIHandle.h"
#include "SoundHandle.h"
#include "SaveManager.h"

FOnSlowStopped UTimeManagerSubsystem::OnSlowStopped;

UTimeManagerSubsystem::UTimeManagerSubsystem()
    :Duration(10.f)
{
}

void UTimeManagerSubsystem::RegisterTimeComponent(UTimeManipulatorComponent* Component, bool bIsPlayer)
{
    if (!Component)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Attempted to register null component"));
        return;
    }

    if (bIsPlayer)
    {
        if (PlayerComponent.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Player component already registered, replacing"));
        }
        PlayerComponent = Component;
        UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Registered player time component"));
    }
    else
    {
        if (!WorldComponents.Contains(Component))
        {
            WorldComponents.Add(Component);
            UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Registered world time component (Total: %d)"), WorldComponents.Num());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Component already registered in WorldComponents"));
        }
    }
}

void UTimeManagerSubsystem::UnregisterTimeComponent(UTimeManipulatorComponent* Component)
{
    if (!Component)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Attempted to unregister null component"));
        return;
    }

    if (PlayerComponent == Component)
    {
        PlayerComponent.Reset();
        UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Unregistered player component"));
    }

    const int32 RemovedCount = WorldComponents.Remove(Component);
    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Unregistered world component (Remaining: %d)"), WorldComponents.Num());
    }
}


void UTimeManagerSubsystem::RewindToWorld(float duration)
{
    int32 ComponentsRewound = 0;
    int32 ComponentsFailed = 0;

    for (const TWeakObjectPtr<UTimeManipulatorComponent>& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->StartRewind(duration);
            ComponentsRewound++;
        }
        else
        {
            ComponentsFailed++;
        }
    }

    if (ComponentsFailed > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: %d components became invalid during rewind"), ComponentsFailed);
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Rewound %d world components"), ComponentsRewound);
}


void UTimeManagerSubsystem::StartSlowMotion(float SlowScale)
{
    SetCustomTimeDilationWorld(SlowScale);
    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Slow motion started (Scale=%.2f)"), SlowScale);

    if (GetWorld()->GetTimerManager().IsTimerActive(SlowMotionTimerHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(SlowMotionTimerHandle);
    }
    UUIHandle::ShowWidget(this, EWidgetCategory::Interactive, "Timer");
    UUIHandle::SetWidgetProperty<float>(this, EWidgetCategory::Interactive, "Timer", "Time", Duration);
    GetWorld()->GetTimerManager().SetTimer(
        SlowMotionTimerHandle,
        this,
        &UTimeManagerSubsystem::ResetTimeDilation,
        Duration,
        false 
    );

}

void UTimeManagerSubsystem::ResetTimeDilation()
{
    SetCustomTimeDilationWorld(1.0f);
    UUIHandle::HideWidget(this, EWidgetCategory::Interactive, "Timer");
    USoundHandle::StopSE(this, "SlowTime");
    OnSlowStopped.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Time dilation reset to normal"));
}

void UTimeManagerSubsystem::ApplyRewindQualityPreset(ERewindQuality RewindQuality)
{
    for (const TWeakObjectPtr<UTimeManipulatorComponent>& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->ApplyRewindQualityPreset(RewindQuality);
        }
    }

    USaveManager::SetRewindQuality(RewindQuality);
}

void UTimeManagerSubsystem::SetCustomTimeDilationWorld(float Scale)
{
    int32 ComponentsAffected = 0;

    for (const TWeakObjectPtr<UTimeManipulatorComponent>& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->SetCustomTimeDilation(Scale);
            ComponentsAffected++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Set time dilation %.2f for %d components"), Scale, ComponentsAffected);
}