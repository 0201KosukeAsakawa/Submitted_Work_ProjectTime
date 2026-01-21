// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeManagerSubsystem.generated.h"

class UTimeManipulatorComponent;
enum class ERewindQuality : uint8;
class IUIManagerProvider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlowStopped);

/**
 * @brief ワールド全体の時間操作管理（最適化版）
 *
 * **主な最適化**
 * - Ticker削除（不要）
 * - ソート処理の削減
 * - span使用で配列コピーを削減
 * - 非同期処理
 */
UCLASS()
class CARRY_API UTimeManagerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ============================================
    // Public API
    // ============================================

    void RegisterTimeComponent(UTimeManipulatorComponent* Component, bool bIsPlayer);
    void UnregisterTimeComponent(UTimeManipulatorComponent* Component);

    UFUNCTION(BlueprintCallable, Category = "Time Management")
    void RewindWorld();

    UFUNCTION(BlueprintCallable, Category = "Time Management")
    void StartSlowMotion(float SlowScale);

    UFUNCTION(BlueprintCallable, Category = "Time Management")
    void SetRewindQuality(ERewindQuality Quality);

    /** @brief スローモーション終了処理 */
    void ResetTimeDilation();

    static FOnSlowStopped OnSlowStopped;

private:
    // ============================================
    // Internal Logic
    // ============================================
    /** @brief 全コンポーネントにTimeDilationを設定 */
    void SetCustomTimeDilationWorld(float Scale);

private:
    // ============================================
    // Component Management
    // ============================================

    UPROPERTY()
    TWeakObjectPtr<UTimeManipulatorComponent> PlayerComponent;

    UPROPERTY()
    TArray<TWeakObjectPtr<UTimeManipulatorComponent>> WorldComponents;

private:
    // ============================================
    // Settings
    // ============================================

    UPROPERTY(EditAnywhere, Category = "Performance")
    float SlowMotionDuration = 10.0f;

    FTimerHandle SlowMotionTimerHandle;
};