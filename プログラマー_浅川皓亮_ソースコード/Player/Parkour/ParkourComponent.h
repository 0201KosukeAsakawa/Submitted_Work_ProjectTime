// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ParkourComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParkourStateChanged);

UENUM(BlueprintType)
enum class EParkourMontageType : uint8
{
    None UMETA(DisplayName = "None"),
    Climb UMETA(DisplayName = "Climb"),
    GettingUp UMETA(DisplayName = "GettingUp"),
    Vault UMETA(DisplayName = "Vault"),
};

class UCapsuleComponent;
class UCharacterMovementComponent;

// 壁の検出情報を保持する構造体
USTRUCT(BlueprintType)
struct FWallDetectionInfo
{
    GENERATED_BODY()

    /** 壁との接触位置 */
    FVector ImpactLocation = FVector::ZeroVector;

    /** 壁の法線ベクトル */
    FVector SurfaceNormal = FVector::ZeroVector;

    /** 壁の上端位置 */
    FVector TopLocation = FVector::ZeroVector;

    /** 壁の内側上端位置 */
    FVector InnerTopLocation = FVector::ZeroVector;

    /** 壁の高さ */
    float Height = 0.0f;

    /** 壁が厚いかどうか */
    bool bIsThickWall = true;

    /** 登る必要がある高さかどうか */
    bool bRequiresClimbing = true;

    /** 内側の表面が検出されたか */
    bool bHasInnerSurface = false;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UParkourComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UParkourComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Parkour")
    bool Parkour();
    /** パルクール実行中かどうか */
    UFUNCTION(BlueprintPure, Category = "Parkour")
    bool IsPerformingParkour() const { return bIsPerformingParkour; }

    /** パルクール開始時に発行されるイベント */
    UPROPERTY(BlueprintAssignable, Category = "Parkour|Events")
    FOnParkourStateChanged OnParkourStarted;

    /** パルクール終了時に発行されるイベント */
    UPROPERTY(BlueprintAssignable, Category = "Parkour|Events")
    FOnParkourStateChanged OnParkourEnded;

private:
    // === 壁検出メソッド ===
    /** 壁の初期接触を検出 */
    bool DetectWallImpact(FWallDetectionInfo& OutWallInfo);

    /** 壁の上端を検出 */
    bool DetectWallTop(FWallDetectionInfo& InOutWallInfo);

    /** 壁の厚さを検出 */
    bool DetectWallThickness(FWallDetectionInfo& InOutWallInfo);

    /** 壁の高さと厚さを計算 */
    void CalculateWallProperties(FWallDetectionInfo& InOutWallInfo);

    // === アクション実行メソッド ===
    /** 登るアクションを実行 */
    bool ExecuteClimb();

    /** 乗り越えるアクションを実行 */
    bool ExecuteVault();

    /** 登ることが可能かチェック */
    bool CanPerformClimb();

    // === 物理・コリジョン制御 ===
    /** キャラクターの物理とコリジョンを無効化 */
    void DisablePhysicsAndCollision(UCapsuleComponent* CapsuleComp, UCharacterMovementComponent* MovementComp);

    /** キャラクターの物理とコリジョンを有効化 */
    void EnablePhysicsAndCollision();

    // === アニメーション制御 ===
    /** モンタージュを再生し、終了後のコールバックをスケジュール */
    void PlayMontageWithCallback(EParkourMontageType MontageType, TFunction<void()> OnComplete);

    // === 終了処理 ===
    /** 登りアクション終了処理 */
    void OnClimbComplete();

    /** パルクールアクション終了処理 */
    void OnParkourComplete();

    // === ヘルパーメソッド ===
    /** ライントレースを実行 */
    bool PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHitResult);

    /** オーナーキャラクターを取得 */
    ACharacter* GetOwnerCharacter() const;

    /** キャラクターの入力を無効化 */
    void DisableCharacterInput();

    /** キャラクターの入力を有効化 */
    void EnableCharacterInput();
private:
    // === 設定値 ===
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

    // === 状態変数 ===
    /** パルクールアクションが可能か */
    bool bCanParkour = true;
    /** パルクール実行中フラグ */
    bool bIsPerformingParkour = false;
    /** 現在検出している壁の情報 */
    FWallDetectionInfo CurrentWallInfo;
};