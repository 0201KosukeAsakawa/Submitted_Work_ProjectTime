// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE5Coro.h"
#include "ParkourComponent.generated.h"

class UCharacterMovementComponent;
class UCapsuleComponent;

using namespace UE5Coro;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParkourStateChanged);

UENUM(BlueprintType)
enum class EParkourMontageType : uint8
{
    None UMETA(DisplayName = "None"),
    Climb UMETA(DisplayName = "Climb"),
    GettingUp UMETA(DisplayName = "GettingUp"),
    Vault UMETA(DisplayName = "Vault"),
};

/**
 * @brief 壁の検出情報を保持する構造体
 */
USTRUCT(BlueprintType)
struct FWallDetectionInfo
{
    GENERATED_BODY()

    FVector ImpactLocation = FVector::ZeroVector;
    FVector SurfaceNormal = FVector::ZeroVector;
    FVector TopLocation = FVector::ZeroVector;
    FVector InnerTopLocation = FVector::ZeroVector;
    float Height = 0.0f;
    bool bIsThickWall = true;
    bool bRequiresClimbing = true;
    bool bHasInnerSurface = false;
};

/**
 * @brief パルクール機能を提供するコンポーネント（UE5Coro最適化版）
 *
 * キャラクターの登る・乗り越えるアクションを自動判定・実行。
 * コルーチンベースで実装し、Timer/Tick依存を排除。
 *
 * **主な責務**
 * - 壁の検出と分析（高さ、厚さ）
 * - 適切なアクション選択（Climb/Vault）
 * - アニメーション制御
 * - 物理・入力の一時制御
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UParkourComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UParkourComponent();

protected:
    // ============================================
    // Unreal Overrides
    // ============================================

    virtual void BeginPlay() override;

public:
    // ============================================
    // Public API
    // ============================================

    /**
     * @brief パルクールを実行
     * @details 壁を自動検出し、適切なアクション（登る/乗り越える）を実行
     * @return 実行したか
     */
    UFUNCTION(BlueprintCallable, Category = "Parkour")
    bool Parkour();

    /**
     * @brief パルクール実行中かどうか
     */
    UFUNCTION(BlueprintPure, Category = "Parkour")
    bool IsPerformingParkour() const { return bIsPerformingParkour; }

    // ============================================
    // Delegates
    // ============================================

    UPROPERTY(BlueprintAssignable, Category = "Parkour|Events")
    FOnParkourStateChanged OnParkourStarted;

    UPROPERTY(BlueprintAssignable, Category = "Parkour|Events")
    FOnParkourStateChanged OnParkourEnded;

private:
    // ============================================
    // Internal Logic
    // ============================================

    /** @brief コンポーネントとキャッシュを初期化 */
    void InitializeComponent();

    /** @brief パルクールメインシーケンス（コルーチン） */
    TCoroutine<bool> ParkourSequence();

    /** @brief 登りシーケンス（コルーチン） */
    TCoroutine<> ExecuteClimbSequence();

    /** @brief 乗り越えシーケンス（コルーチン） */
    TCoroutine<> ExecuteVaultSequence();

    /** @brief モンタージュ再生を待機（コルーチン） */
    TCoroutine<> PlayMontageAsync(EParkourMontageType MontageType);

    /** @brief パルクール終了処理 */
    TCoroutine<> CleanupParkour();

    // ============================================
    // Detection Methods
    // ============================================

    /** @brief 壁の初期接触を検出 */
    bool DetectWallImpact(FWallDetectionInfo& OutWallInfo);

    /** @brief 壁の上端を検出 */
    bool DetectWallTop(FWallDetectionInfo& InOutWallInfo);

    /** @brief 壁の厚さを検出 */
    bool DetectWallThickness(FWallDetectionInfo& InOutWallInfo);

    /** @brief 壁の高さと厚さを計算 */
    void CalculateWallProperties(FWallDetectionInfo& InOutWallInfo);

    /** @brief 登ることが可能かチェック */
    bool CanPerformClimb() const;

    // ============================================
    // Physics & Input Control
    // ============================================

    /** @brief キャラクターの物理とコリジョンを無効化 */
    void DisablePhysicsAndCollision();

    /** @brief キャラクターの物理とコリジョンを有効化 */
    void EnablePhysicsAndCollision();

    /** @brief キャラクターの入力を無効化 */
    void DisableCharacterInput();

    /** @brief キャラクターの入力を有効化 */
    void EnableCharacterInput();

    // ============================================
    // Helper Methods
    // ============================================

    /** @brief ライントレースを実行 */
    bool PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHitResult) const;

private:
    // ============================================
    // Settings
    // ============================================

    UPROPERTY(EditAnywhere, Category = "Parkour|Animation")
    TMap<EParkourMontageType, UAnimMontage*> AnimMontageMap;

    UPROPERTY(EditAnywhere, Category = "Parkour|Detection")
    float WallDetectionDistance = 70.0f;

    UPROPERTY(EditAnywhere, Category = "Parkour|Detection")
    float MaxDetectionHeight = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Parkour|Detection")
    float ClimbHeightThreshold = 80.0f;

    UPROPERTY(EditAnywhere, Category = "Parkour|Detection")
    float ThicknessThreshold = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Parkour|Detection")
    float CharacterCenterOffset = 55.0f;

    UPROPERTY(EditAnywhere, Category = "Parkour|Movement")
    float NormalGravityScale = 5.0f;

private:
    // ============================================
    // Cached References
    // ============================================

    UPROPERTY()
    TWeakObjectPtr<ACharacter> CachedCharacter;

    UPROPERTY()
    TWeakObjectPtr<UCapsuleComponent> CachedCapsule;

    UPROPERTY()
    TWeakObjectPtr<UCharacterMovementComponent> CachedMovement;

    UPROPERTY()
    TWeakObjectPtr<UAnimInstance> CachedAnimInstance;

    /** @brief ライントレース用のObjectTypes（キャッシュ） */
    TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;

private:
    // ============================================
    // Runtime State
    // ============================================

    /** @brief パルクール実行中フラグ */
    bool bIsPerformingParkour = false;

    /** @brief 現在検出している壁の情報 */
    FWallDetectionInfo CurrentWallInfo;
};