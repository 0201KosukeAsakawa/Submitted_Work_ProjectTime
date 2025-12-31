// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "PlayerCameraControlComponent.generated.h"


// Forward declarations
class UCameraComponent;
class UCharacterMovementComponent;

/**
 * @brief カメラの回転設定
 */
USTRUCT(BlueprintType)
struct FCameraRotationSettings
{
    GENERATED_BODY()

    /** マウス/スティックの感度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = "0.01", ClampMax = "3.0"))
    float Sensitivity = 0.1f;

    /** ピッチ（上下）の最小角度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = "-90.0", ClampMax = "0.0"))
    float MinPitch = -80.0f;

    /** ピッチ（上下）の最大角度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MaxPitch = 80.0f;

    /** ロール（傾き）の補間速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float RollInterpSpeed = 10.0f;
};

/**
 * @brief ヘッドボブの設定
 */
USTRUCT(BlueprintType)
struct FHeadBobSettings
{
    GENERATED_BODY()

    /** ヘッドボブを有効にするか */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob")
    bool bEnabled = true;

    /** ボブの振幅（高さ） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float Amplitude = 2.0f;

    /** ボブの周波数（速さ） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float Frequency = 10.0f;

    /** 停止時の補間速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float InterpSpeed = 5.0f;

    /** 移動速度の基準値（この速度で最大ボブ） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
    float SpeedReference = 300.0f;
};

/**
 * @brief FOV（視野角）の設定
 */
USTRUCT(BlueprintType)
struct FFOVSettings
{
    GENERATED_BODY()

    /** デフォルトのFOV */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FOV", meta = (ClampMin = "60.0", ClampMax = "120.0"))
    float DefaultFOV = 90.0f;

    /** FOV変更時の補間速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FOV", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float InterpSpeed = 8.0f;
};

/**
 * @brief プレイヤーカメラ制御コンポーネント
 * @details カメラコンポーネントを内部で作成・管理し、以下の機能を提供
 *          - マウス/スティック入力によるカメラ回転
 *          - 移動時のヘッドボブエフェクト
 *          - 動的なFOV変更（ブースト、壁走りなど）
 *          - カメラロール（傾き）制御
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UPlayerCameraControlComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UPlayerCameraControlComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ============================================
    // Camera Rotation
    // ============================================

    /**
     * @brief カメラ回転入力を処理
     * @param Value 入力値（X: Yaw, Y: Pitch）
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void ProcessLookInput(const FInputActionValue& Value);

    /**
     * @brief カメラのロール（傾き）を設定
     * @param Roll 目標ロール角度（度）
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void SetCameraRoll(float Roll);

    /**
     * @brief カメラのロールをリセット
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void ResetCameraRoll();

    /**
     * @brief カメラの反転設定
     * @param bInverted true: 上下反転
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void SetPitchInverted(bool bInverted);

    // ============================================
    // Head Bob
    // ============================================

    /**
     * @brief ヘッドボブを更新
     * @param MoveInput 移動入力ベクトル
     * @param IsMoving 移動中かどうか
     * @param IsFalling 落下中かどうか
     */
    void UpdateHeadBob(const FVector2D& MoveInput, bool IsMoving, bool IsFalling);

    /**
     * @brief ヘッドボブを有効/無効化
     * @param bEnabled true: 有効
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|HeadBob")
    void SetHeadBobEnabled(bool bEnabled);

    /**
     * @brief ヘッドボブをリセット
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|HeadBob")
    void ResetHeadBob();

    /**
     * @brief カメラが頭にアタッチされているかを設定
     * @param 揺れの有無を制御
     */
    void SetCameraAttachedToHead(bool bAttached);

    /**
     * @brief 現在のカメラアタッチ状態を取得
     */
    UFUNCTION(BlueprintPure, Category = "Camera")
    bool IsCameraAttachedToHead() const { return bIsCameraAttachedToHead; }

    // ============================================
    // FOV Control
    // ============================================

    /**
     * @brief FOVを変更
     * @param NewFOV 目標FOV
     * @param bInstant true: 即座に変更、false: 補間
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|FOV")
    void SetFOV(float NewFOV, bool bInstant = false);

    /**
     * @brief FOVをデフォルトに戻す
     * @param bInstant true: 即座に変更、false: 補間
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|FOV")
    void ResetFOV(bool bInstant = false);

    /**
     * @brief 現在のFOVを取得
     */
    UFUNCTION(BlueprintPure, Category = "Camera Control|FOV")
    float GetCurrentFOV() const { return CurrentFOV; }

    // ============================================
    // Camera Shake
    // ============================================

    /**
     * @brief カメラシェイクを再生
     * @param ShakeClass シェイククラス
     * @param Scale シェイクの強度スケール
     * @param PlaySpace シェイクの適用空間
     * @param UserPlaySpaceRot カスタム回転
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|Shake")
    void PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.0f,
        ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal,
        FRotator UserPlaySpaceRot = FRotator::ZeroRotator);

    /**
     * @brief すべてのカメラシェイクを停止
     * @param bImmediately true: 即座に停止、false: フェードアウト
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|Shake")
    void StopAllCameraShakes(bool bImmediately = true);

    /**
     * @brief 特定のカメラシェイクを停止
     * @param ShakeClass 停止するシェイククラス
     * @param bImmediately true: 即座に停止、false: フェードアウト
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|Shake")
    void StopCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, bool bImmediately = true);

    /**
     * @brief 簡易カメラシェイク（プリセット: 弱）
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|Shake")
    void PlayLightShake();

    /**
     * @brief 簡易カメラシェイク（プリセット: 中）
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|Shake")
    void PlayMediumShake();

    /**
     * @brief 簡易カメラシェイク（プリセット: 強）
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|Shake")
    void PlayHeavyShake();

    /**
     * @brief カスタムカメラシェイク（パラメータ指定）
     * @param Duration シェイクの持続時間
     * @param Amplitude 振幅
     * @param Frequency 周波数
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control|Shake")
    void PlayCustomShake(float Duration = 0.5f, float Amplitude = 5.0f, float Frequency = 10.0f);

    // ============================================
    // Camera Shake Settings
    // ============================================

    /** プリセットシェイクのクラス */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Control|Shake Settings")
    TSubclassOf<UCameraShakeBase> LightShakeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Control|Shake Settings")
    TSubclassOf<UCameraShakeBase> MediumShakeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Control|Shake Settings")
    TSubclassOf<UCameraShakeBase> HeavyShakeClass;

    /**
     * @brief カメラコンポーネントを取得
     */
    UFUNCTION(BlueprintPure, Category = "Camera Control")
    UCameraComponent* GetCamera() const { return Camera; }

    /**
     * @brief 現在のカメラロールを取得
     */
    UFUNCTION(BlueprintPure, Category = "Camera Control")
    float GetCurrentRoll() const { return CurrentRoll; }

    /**
     * @brief カメラ感度を取得
     */
    UFUNCTION(BlueprintPure, Category = "Camera Control")
    float GetSensitivity() const { return RotationSettings.Sensitivity; }

    /**
     * @brief カメラ感度を設定
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void SetSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void AddSensitivity(float NewSensitivity);

    /**
     * @brief カメラの位置オフセットを設定
     * @param Offset ローカル空間でのオフセット
     */
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void SetCameraOffset(const FVector& Offset);

    /**
     * @brief カメラの位置オフセットを取得
     */
    UFUNCTION(BlueprintPure, Category = "Camera Control")
    FVector GetCameraOffset() const;

private:
    // ============================================
    // Initialization
    // ============================================

    /** コンポーネントの初期化 */
    void InitializeComponent();

    // ============================================
    // Camera Rotation Internal
    // ============================================

    /** カメラロールを補間更新 */
    void UpdateCameraRoll(float DeltaTime);

    // ============================================
    // Head Bob Internal
    // ============================================

    /** ヘッドボブのオフセットを計算 */
    float CalculateHeadBobOffset(float DeltaTime, float SpeedFactor);

    /** ヘッドボブのベース位置を取得 */
    FVector GetHeadBobBaseLocation() const;

    // ============================================
    // FOV Internal
    // ============================================

    /** FOVを補間更新 */
    void UpdateFOV(float DeltaTime);

    void UpdateBaseCameraLocation();

private:
    // ============================================
    // Camera Component
    // ============================================

    /**
     * @brief 内部で管理するカメラコンポーネント
     * @details このコンポーネントが自動的に作成・管理します
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Control", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* Camera;

    // ============================================
    // Cached References
    // ============================================

    /** キャッシュされたムーブメントコンポーネント */
    UPROPERTY()
    TWeakObjectPtr<UCharacterMovementComponent> CachedMovement;

    // ============================================
    // Camera Rotation State
    // ============================================

    /** 現在のカメラロール */
    float CurrentRoll;

    /** 目標カメラロール */
    float TargetRoll;

    /** ピッチ反転フラグ */
    bool bPitchInverted;

    // ============================================
    // Head Bob State
    // ============================================

    /** ヘッドボブの累積時間 */
    float HeadBobTime;

    /** ヘッドボブのベース位置（オフセット前） */
    FVector HeadBobBaseLocation;

    // ============================================
    // FOV State
    // ============================================

    /** 現在のFOV */
    float CurrentFOV;

    /** 目標FOV */
    float TargetFOV;

    /** カメラが頭にアタッチされているか（true: 頭の動きに追従、false: 揺れなし） */
    bool bIsCameraAttachedToHead = true;

    // ============================================
    // Settings
    // ============================================

    /** カメラ回転設定 */
    UPROPERTY(EditAnywhere, Category = "Camera Control|Settings")
    FCameraRotationSettings RotationSettings;

    /** ヘッドボブ設定 */
    UPROPERTY(EditAnywhere, Category = "Camera Control|Settings")
    FHeadBobSettings HeadBobSettings;

    /** FOV設定 */
    UPROPERTY(EditAnywhere, Category = "Camera Control|Settings")
    FFOVSettings FOVSettings;

    /** カメラの初期位置オフセット（ローカル空間） */
    UPROPERTY(EditAnywhere, Category = "Camera Control|Settings")
    FVector CameraLocationOffset = FVector(0.0f, 0.0f, 60.0f);

    /** カメラがPawn制御回転を使用するか */
    UPROPERTY(EditAnywhere, Category = "Camera Control|Settings")
    bool bUsePawnControlRotation = true;
};