#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE5Coro.h"
#include "PhysicsCalculatorComponent.generated.h"

class UBoxComponent;
using namespace UE5Coro;
/**
 * @brief 簡易物理演算コンポーネント（非同期最適化版）
 *
 * 重い物理演算を非同期処理に移行し、メインスレッドの負荷を軽減。
 * UE5Coroと非同期トレースを活用。
 *
 * **主な最適化**
 * - 毎フレームのSweep処理を非同期化
 * - Tick依存を排除
 * - キャッシュの活用
 * - 不要な計算の削減
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UPhysicsCalculatorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPhysicsCalculatorComponent();

protected:
    // ============================================
    // Unreal Overrides
    // ============================================

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ============================================
    // Public API
    // ============================================

    /**
     * @brief 物理計算を開始
     */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void StartPhysics();

    /**
     * @brief 物理計算を停止
     */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void StopPhysics();

    /**
     * @brief 力を加える
     */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void AddForce(FVector Direction, float Force, bool bSweep = true, bool bUseLocalOffset = true);

    /**
     * @brief 力をリセット
     */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void ResetForce();

    /**
     * @brief 接地しているか
     */
    UFUNCTION(BlueprintPure, Category = "Physics")
    bool IsOnGround() const { return bIsOnGround; }

    /**
     * @brief 着地した瞬間か
     */
    UFUNCTION(BlueprintPure, Category = "Physics")
    bool HasJustLanded() const { return bHasJustLanded; }

    /**
     * @brief 重力設定
     */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void SetGravityScale(bool bApplyGravity, float Scale = 9.8f, float Modifier = 1.0f);

private:
    // ============================================
    // Internal Logic
    // ============================================

    /** @brief コンポーネント初期化 */
    void InitializeComponent();

    /** @brief メイン物理ループ（コルーチン） */
    TCoroutine<> PhysicsUpdateLoop();

    /** @brief 非同期接地判定 */
    TCoroutine<bool> AsyncCheckGroundState();

    /** @brief 非同期衝突チェック */
    TCoroutine<FVector> AsyncGetBlockedAdjustedVector(const FVector& MoveVector);

    /** @brief 重力適用 */
    void ApplyGravity(float DeltaTime);

    /** @brief 力を適用 */
    void ApplyForce(float DeltaTime);

    /** @brief 接地状態を更新 */
    void UpdateGroundState(bool bNewGroundState);

private:
    // ============================================
    // Settings
    // ============================================

    UPROPERTY(EditAnywhere, Category = "Physics|Gravity")
    float GravityScale = 9.8f;

    UPROPERTY(EditAnywhere, Category = "Physics|Gravity")
    float MaxFallingSpeed = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Physics|Gravity")
    bool bShouldApplyGravity = true;

    UPROPERTY(EditAnywhere, Category = "Physics|Detection")
    FVector GroundCheckBoxExtent = FVector(40.0f, 20.0f, 15.0f);

    UPROPERTY(EditAnywhere, Category = "Physics|Detection")
    FVector CollisionCheckBoxExtent = FVector(20.0f, 20.0f, 20.0f);

    UPROPERTY(EditAnywhere, Category = "Physics|Detection")
    float GroundCheckDistance = 5.0f;

private:
    // ============================================
    // Cached References
    // ============================================

    UPROPERTY()
    TWeakObjectPtr<AActor> CachedOwner;

private:
    // ============================================
    // Runtime State
    // ============================================

    /** @brief 力の方向 */
    FVector ForceDirection = FVector::ZeroVector;

    /** @brief 力の大きさ */
    float ForceScale = 0.0f;

    /** @brief 重力タイマー */
    float GravityTimer = 0.0f;

    /** @brief 重力修正係数 */
    float GravityDivider = 1.0f;

    /** @brief 接地しているか */
    bool bIsOnGround = false;

    /** @brief 前フレームで接地していたか */
    bool bWasOnGround = false;

    /** @brief 今フレームで着地したか */
    bool bHasJustLanded = false;

    /** @brief 物理計算が有効か */
    bool bIsPhysicsActive = false;

    /** @brief 力適用時にSweepを使用するか */
    bool bUseSweep = false;

    /** @brief ローカル座標を使用するか */
    bool bUseLocalOffset = true;

    /** @brief コルーチン停止フラグ */
    bool bShouldStopPhysics = false;
};