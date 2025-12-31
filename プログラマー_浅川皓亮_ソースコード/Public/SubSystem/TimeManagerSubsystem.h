// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeManagerSubsystem.generated.h"

class UTimeManipulatorComponent;
enum class ERewindQuality : uint8;
class IUIManagerProvider;

DECLARE_MULTICAST_DELEGATE(FOnSlowStopped);

/**
 * @brief ワールド全体の時間操作を管理するサブシステム
 */
UCLASS()
class CARRY_API UTimeManagerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UTimeManagerSubsystem();

    // Component Registration
    void RegisterTimeComponent(UTimeManipulatorComponent* Component, bool bIsPlayer = false);
    void UnregisterTimeComponent(UTimeManipulatorComponent* Component);

    void RewindToWorld(float Duration);

    void StartSlowMotion(float duration);

    void ResetTimeDilation();

    UFUNCTION(BlueprintCallable, Category = "Time Manager")
    void ApplyRewindQualityPreset(ERewindQuality RewindQuality);

private:
    // Time Scale
    void SetCustomTimeDilationWorld(float Scale);

public:
    static FOnSlowStopped OnSlowStopped;
private:
    UPROPERTY()
    TArray<TWeakObjectPtr<UTimeManipulatorComponent>> WorldComponents;

    UPROPERTY()
    TWeakObjectPtr<UTimeManipulatorComponent> PlayerComponent;

    // Cached UI Manager
    TWeakInterfacePtr<IUIManagerProvider> CachedUIManager;

    FTimerHandle SlowMotionTimerHandle;

    float Duration;

    UPROPERTY()
    float ElapsedTime;

    UPROPERTY()
    float TimerDuration;
};
