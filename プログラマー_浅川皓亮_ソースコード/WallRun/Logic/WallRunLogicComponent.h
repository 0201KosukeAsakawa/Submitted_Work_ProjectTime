// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WallRunLogicComponent.generated.h"
class UCharacterMovementComponent;

/**
 * @brief 壁走り中に使用される一時データ構造体
 * @details 壁の法線・移動方向・カメラ傾き・走行側（右/左）などを保持
 */
USTRUCT(BlueprintType)
struct FWallRunData
{
    GENERATED_BODY()

    /** 検出された壁の法線ベクトル */
    UPROPERTY()
    FVector WallNormal;

    /** 壁に沿った移動方向（前方） */
    UPROPERTY()
    FVector MoveDirection;

    /** カメラのロール角度（傾き） */
    UPROPERTY()
    float CameraRoll;

    /** 走行方向が右側の壁かどうか（true:右, false:左） */
    UPROPERTY()
    bool bIsRightSide;
};

/**
 * @brief 壁走りの設定パラメータ構造体
 * @details 壁走りの速度、重力スケール、カメラ傾き、ジャンプ挙動などを指定
 */
USTRUCT(BlueprintType)
struct FWallRunSettings
{
    GENERATED_BODY()

    /** 壁走り中の基本移動速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run")
    float Speed = 800.0f;

    /** 壁走り中の重力スケール（通常より軽くする） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run")
    float GravityScale = 0.0f;

    /** 壁走り中のカメラ傾き角度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run")
    float CameraRollAngle = 25.0f;

    // ------------------------------
    // 壁ジャンプ関連パラメータ
    // ------------------------------

    /** 壁ジャンプ時の基本ジャンプ力 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Jump")
    float JumpPower = 4000.0f;

    /** 壁ジャンプ時の前方方向への補正係数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Jump")
    float JumpForwardMultiplier = 1.2f;

    /** 壁ジャンプ時の上方向への補正係数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Jump")
    float JumpUpMultiplier = 1.0f;

    /** 壁ジャンプ時の壁法線方向補正係数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Jump")
    float JumpNormalMultiplier = 0.6f;

    /** 壁走り開始直後にジャンプを受け付ける猶予時間 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Jump")
    float JumpWindow = 0.2f;

    /** ジャンプ実行後、再ジャンプを許可するまでの遅延時間 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Jump")
    float JumpResetDelay = 0.5f;

    // ------------------------------
    // 壁走り条件関連
    // ------------------------------

    /** 壁走り開始に必要な最低速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Conditions")
    float MinimumSpeed = 300.0f;

    /** 壁走り終了時の速度閾値 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run|Conditions")
    float EndSpeedThreshold = 100.0f;
};

/**
 * @brief 壁走りのロジックを管理するコンポーネント
 * @details 壁走り中の物理計算、条件判定、状態フラグ管理を担当する
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UWallRunLogicComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** デフォルトコンストラクタ */
    UWallRunLogicComponent();

    // ============================================================
    // State Queries
    // ============================================================

    /**
     * @brief 壁走りを開始できるか判定
     * @param CharacterMovement キャラクターの移動コンポーネント
     * @param bHasMovementInput 移動入力があるか
     * @return 壁走り開始条件を満たす場合 true
     */
    bool CanStartWallRun(UCharacterMovementComponent* CharacterMovement, bool bHasMovementInput) const;

    /**
     * @brief 壁走りを継続できるか判定
     * @param CharacterMovement キャラクターの移動コンポーネント
     * @param bHasMovementInput 移動入力があるか
     * @param bWallStillDetected 壁がまだ検出されているか
     * @return 壁走り継続条件を満たす場合 true
     */
    bool CanContinueWallRun(
        UCharacterMovementComponent* CharacterMovement,
        bool bHasMovementInput,
        bool bWallStillDetected
    ) const;

    /**
     * @brief 現在壁走り中か
     */
    UFUNCTION(BlueprintPure, Category = "Wall Run")
    bool IsWallRunning() const { return bIsWallRunning; }

    /**
     * @brief 現在壁ジャンプが可能か
     */
    UFUNCTION(BlueprintPure, Category = "Wall Run")
    bool CanWallJump() const { return bCanWallJump; }

    // ============================================================
    // Calculations
    // ============================================================

    /**
     * @brief 壁走りに必要な情報を計算して返す
     * @param WallNormal 検出された壁の法線
     * @param CameraForward カメラの前方ベクトル
     * @param ActorUp アクターの上方向ベクトル
     * @return 壁走り情報（方向・カメラ傾きなど）
     */
    FWallRunData CalculateWallRunData(const FVector& WallNormal,
        const FVector& CameraForward,
        const FVector& ActorUp) const;

    /**
     * @brief 壁ジャンプの方向を算出
     * @param CameraForward カメラの前方ベクトル
     * @param ActorUp キャラクターの上方向ベクトル
     * @return 壁ジャンプ方向ベクトル
     */
    FVector CalculateWallJumpDirection(const FVector& CameraForward, const FVector& ActorUp) const;

    /**
     * @brief 壁ジャンプ時の初速度ベクトルを計算
     * @param JumpDirection ジャンプ方向ベクトル
     * @return 速度ベクトル
     */
    FVector CalculateWallJumpVelocity(const FVector& JumpDirection) const;

    // ============================================================
    // State Management
    // ============================================================

    /**
     * @brief 壁走り状態へ遷移
     * @param Data 壁走り情報構造体（壁法線・方向など）
     * @param InitialGravityScale 現在の重力スケール（後で復元に使用）
     */
    void EnterWallRun(const FWallRunData& Data, float InitialGravityScale);

    /**
     * @brief 壁走り終了処理
     * @details 重力スケールなどを復元し、フラグをリセット
     */
    void ExitWallRun();

    /**
     * @brief 壁ジャンプ実行後、ジャンプ可能フラグを消費する
     */
    void ConsumeWallJump();


    // ============================================================
    // Getters
    // ============================================================

    /** 現在の壁法線を取得 */
    FVector GetWallNormal() const { return CurrentWallNormal; }

    /** 壁に沿った移動方向を取得 */
    FVector GetMoveDirection() const { return CurrentMoveDirection; }

    /** 壁走り開始前の重力スケールを取得 */
    float GetPreviousGravityScale() const { return PreviousGravityScale; }

    /** 現在の設定構造体を取得 */
    const FWallRunSettings& GetSettings() const { return Settings; }

private:
    // ============================================================
    // Internal Calculations
    // ============================================================

    /**
     * @brief 壁の法線から右方向ベクトルを算出
     * @param WallNormal 壁の法線ベクトル
     * @return 壁に沿う右方向ベクトル
     */
    FVector CalculateWallRightVector(const FVector& WallNormal) const;

    /**
     * @brief カメラロール角度を算出
     * @param bIsRightSide 右側の壁か
     * @param ActorUp キャラクターの上方向ベクトル
     * @return ロール角度（度）
     */
    float CalculateCameraRoll(bool bIsRightSide, const FVector& ActorUp) const;

    /**
     * @brief 壁走り開始直後のジャンプ受付ウィンドウを開始
     * @param JumpWindow 許可時間（秒）
     */
    void StartWallJumpWindow(float JumpWindow);

private:
    // ============================================================
    // Internal State
    // ============================================================

        /** 壁走り設定パラメータ */
    UPROPERTY(EditAnywhere, Category = "Wall Run")
    FWallRunSettings Settings;

    /** 壁走り中フラグ */
    bool bIsWallRunning = false;

    /** 壁ジャンプ可能フラグ */
    bool bCanWallJump = false;

    /** 壁走り前の重力スケールを保存 */
    float PreviousGravityScale = 1.0f;

    /** 現在の壁法線ベクトル */
    FVector CurrentWallNormal = FVector::ZeroVector;

    /** 現在の壁に沿った移動方向 */
    FVector CurrentMoveDirection = FVector::ZeroVector;

    /** 壁ジャンプ受付ウィンドウのタイマー */
    FTimerHandle WallJumpWindowHandle;
};