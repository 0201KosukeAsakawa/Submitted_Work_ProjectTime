// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE5Coro.h"
#include "BoostComponent.generated.h"

class IPlayerInfoProvider;
class UCameraComponent;
class UCharacterMovementComponent;
class UPlayerCameraControlComponent;

/**
 * @brief ブースト機能を提供するコンポーネント（UE5Coro版）
 *
 * キャラクターに一時的な速度上昇（ブースト）を付与し、FOV（視野角）を動的に変更することで演出を行う。
 * UE5Coroを使用してコルーチンベースで実装し、Timer/Tick依存を排除。
 *
 * **主な責務**
 * - ブースト発動条件のチェック（速度・クールダウンなど）
 * - FOV変化によるカメラ演出
 * - ブースト中の速度制御と復帰処理
 * - イベント発火（開始・終了）
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UBoostComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBoostComponent();

    // ============================================
    // Public API
    // ============================================

    /**
     * @brief ブーストを発動する
     *
     * 一定速度以上で移動しており、かつクールダウンが完了している場合のみ発動可能。
     * 発動時には速度強化と視野角(FOV)拡大を行い、一定時間後に自動的に解除される。
     */
    UFUNCTION(BlueprintCallable, Category = "Boost")
    void Boost();

    /**
     * @brief 現在ブースト中かを返す
     * @return ブースト中なら true
     */
    UFUNCTION(BlueprintPure, Category = "Boost")
    bool IsBoosting() const { return bIsBoosting; }

    // ============================================
    // Delegates
    // ============================================

    /** @brief ブースト開始時に発火するデリゲート */
    DECLARE_MULTICAST_DELEGATE(FOnBoostStarted);
    FOnBoostStarted OnBoostStarted;

    /** @brief ブースト終了時に発火するデリゲート */
    DECLARE_MULTICAST_DELEGATE(FOnBoostEnded);
    FOnBoostEnded OnBoostEnded;

protected:
    // ============================================
    // Unreal Overrides
    // ============================================

    /** @brief コンポーネント初期化 */
    virtual void BeginPlay() override;

private:
    // ============================================
    // Internal Logic
    // ============================================

    /** @brief 所有者・依存コンポーネントの初期化 */
    void InitializeComponent();

    /** @brief 所有キャラクターやカメラ制御などの参照をキャッシュ */
    void CacheComponents();

    /** @brief ブースト発動可能かを判定 */
    bool CanBoost() const;

    /** @brief ブーストのメインシーケンス（コルーチン） */
    UE5Coro::TCoroutine<> BoostSequence();

private:
    // ============================================
    // Configurable Parameters
    // ============================================

    /** @brief ブースト持続時間（秒） */
    UPROPERTY(EditAnywhere, Category = "Boost|Settings", meta = (ClampMin = "0.0"))
    float BoostDuration = 0.5f;

    /** @brief ブーストクールダウン時間（秒） */
    UPROPERTY(EditAnywhere, Category = "Boost|Settings", meta = (ClampMin = "0.0"))
    float BoostCooldownDuration = 1.0f;

    /** @brief ブースト時のカメラFOV（通常時より広く設定する） */
    UPROPERTY(EditAnywhere, Category = "Boost|Camera", meta = (ClampMin = "90.0", ClampMax = "140.0"))
    float BoostFOV = 110.0f;

private:
    // ============================================
    // Cached References
    // ============================================

    /** @brief 所有しているキャラクター（ACharacter） */
    UPROPERTY()
    TWeakObjectPtr<ACharacter> CachedOwner;

    /** @brief キャラクター移動コンポーネント */
    UPROPERTY()
    TWeakObjectPtr<UCharacterMovementComponent> CachedMovement;

    /** @brief カメラ制御コンポーネント */
    UPROPERTY()
    TWeakObjectPtr<UPlayerCameraControlComponent> CachedCameraControl;

private:
    // ============================================
    // Runtime State
    // ============================================

    /** @brief 現在ブースト中かどうか */
    bool bIsBoosting;

    /** @brief ブースト前の最大歩行速度（終了後に復元） */
    float OriginalMaxWalkSpeed;
};