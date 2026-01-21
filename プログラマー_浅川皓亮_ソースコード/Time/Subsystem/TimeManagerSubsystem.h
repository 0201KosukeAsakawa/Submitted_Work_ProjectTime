// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeManagerSubsystem.generated.h"

class UTimeManipulatorComponent;
enum class ERewindQuality : uint8;
class IUIManagerProvider;

/**
 * @brief ワールド全体の時間操作を管理するサブシステム
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlowStopped);

UCLASS()
class UTimeManagerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UTimeManagerSubsystem();

    // 既存の関数
    void RegisterTimeComponent(UTimeManipulatorComponent* Component, bool bIsPlayer);
    void UnregisterTimeComponent(UTimeManipulatorComponent* Component);
    void RewindToWorld(float duration);
    void StartSlowMotion(float SlowScale);
    void ResetTimeDilation();
    UFUNCTION(BlueprintCallable)
    void ApplyRewindQualityPreset(ERewindQuality RewindQuality);
    void SetCustomTimeDilationWorld(float Scale);

    // フレーム分散制御
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool Tick(float DeltaTime);
    void ProcessRewindUpdates(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxUpdatesPerFrame = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableFrameDistribution = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bUseCameraPriority = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float PrioritySortInterval = 0.1f;

    static FOnSlowStopped OnSlowStopped;

private:
    UPROPERTY()
    TWeakObjectPtr<UTimeManipulatorComponent> PlayerComponent;

    UPROPERTY()
    TArray<TWeakObjectPtr<UTimeManipulatorComponent>> WorldComponents;

    float Duration;
    FTimerHandle SlowMotionTimerHandle;

    // フレーム分散用
    int32 CurrentUpdateIndex;
    FTSTicker::FDelegateHandle TickerHandle;

    // 優先度ソート用
    float TimeSinceLastSort;
    FVector LastCameraLocation;

    void SortComponentsByPriority();
    float CalculateComponentPriority(UTimeManipulatorComponent* Component) const;
};