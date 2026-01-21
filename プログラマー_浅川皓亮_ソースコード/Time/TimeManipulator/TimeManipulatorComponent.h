#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "TimeManipulatorComponent.generated.h"

// Forward declarations
class UCharacterMovementComponent;
class UPlayerCameraControlComponent;

/**
 * @brief 記録モードの列挙型
 */
UENUM(BlueprintType)
enum class ERecordingMode : uint8
{
    Automatic UMETA(DisplayName = "Automatic"),                     // 自動記録（最大数まで記録）
    ManualStopAtMax UMETA(DisplayName = "Manual Stop"),             // 任意記録（Maxで停止）
    ManualClearAtMax UMETA(DisplayName = "Manual Clear"),           // 任意記録（Maxでクリア）
    ManualClearAndStopAtMax UMETA(DisplayName = "Manual Clear & Stop")     // 任意記録（Maxでクリア）
};

UENUM(BlueprintType)
enum class ERewindQuality : uint8
{
    Low      UMETA(DisplayName = "Low (軽い)"),      // FPS=20, Step=3
    Medium   UMETA(DisplayName = "Medium (普通)"),   // FPS=30, Step=2
    High     UMETA(DisplayName = "High (重い)"),     // FPS=60, Step=1
    Ultra    UMETA(DisplayName = "Ultra (最高)"),    // 最適化OFF
};



/**
 * @brief アクターの状態を記録するスナップショット構造体
 */
USTRUCT()
struct FTimeSnapshot
{
    GENERATED_BODY()

    /** アクターの位置 */
    FVector Location = FVector::ZeroVector;

    /** アクターの回転 */
    FRotator Rotation = FRotator::ZeroRotator;

    /** アクターの速度 */
    FVector Velocity = FVector::ZeroVector;

    /** 重力方向 */
    FVector GravityDirection = FVector::DownVector;

    /** 移動モード */
    TEnumAsByte<EMovementMode> MovementMode = MOVE_Walking;

    /** カスタム移動モード */
    uint8 CustomMovementMode = 0;

    /** 記録時のタイムスタンプ */
    float Timestamp = 0.0f;

    // ==================== カメラデータ ====================

    /** カメラデータが記録されているか */
    bool bHasCameraData = false;

    /** カメラの回転（ワールド空間） */
    FRotator CameraRotation = FRotator::ZeroRotator;

    /** カメラのロール値 */
    float CameraRoll = 0.0f;

    /** カメラのFOV */
    float CameraFOV = 90.0f;
};

// デリゲート宣言
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGravityDirectionChanged, FVector);
DECLARE_MULTICAST_DELEGATE(FOnRewindStarted);
DECLARE_MULTICAST_DELEGATE(FOnRewindStopped);
DECLARE_MULTICAST_DELEGATE(FOnRecordingStarted);
DECLARE_MULTICAST_DELEGATE(FOnRecordingStopped);
DECLARE_MULTICAST_DELEGATE(FOnRecordingMaxReached);

/**
 * @brief アクターの時間操作を制御するコンポーネント
 *
 * 記録データは新しいものから古いものへ遡って適用され、
 * リワインド終了時（完了または中断）に全データが自動消去されます。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UTimeManipulatorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimeManipulatorComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ==================== 記録制御 ====================

    void ApplyLerpedSnapshot(const FTimeSnapshot& From, const FTimeSnapshot& To, float Alpha);

    /** 記録を開始 */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation|Recording")
    void StartRecording();

    /** 記録を停止 */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation|Recording")
    void StopRecording();

    /** 記録データを全消去 */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation|Recording")
    void ClearRecording();

    // ==================== 巻き戻し制御 ====================

    /**
     * 巻き戻しを開始（新しいデータから古いデータへ遡る）
     * @param Duration 巻き戻しの継続時間（秒）
     * @note リワインド終了時（完了/中断）に全データが自動消去されます
     */
    void StartRewind(float Duration);

    /**
     * 巻き戻しを停止
     * @note 停止時に全記録データが消去されます
     */
    void StopRewind();

    // ==================== 時間制御 ====================

    /** グローバル時間停止/再開 */
    void SetTimeStop(bool bStop);

    /** グローバル時間スケール設定 */
    void SetTimeScale(float Scale);

    /** このアクターのみの時間スケール設定 */
    void SetCustomTimeDilation(float Scale);

    // ==================== 設定 ====================

    /** 最大スナップショット数を設定 */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void SetMaxSnapshots(int32 Max);

    /** 記録モードを設定 */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void SetRecordingMode(ERecordingMode Mode);

    /** リワインド速度を設定 */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void SetRewindSpeed(float Speed) { RewindSpeed = Speed; }

    /** スナップショット記録間隔を設定 */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void SetSnapshotInterval(float Interval) { SnapshotInterval = Interval; }

    /** 
    * 記録の適用間隔の設定
    * @param 適用するフレームMode
    */
    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void ApplyRewindQualityPreset(ERewindQuality rewindQuality);

    UFUNCTION(BlueprintCallable, Category = "Time Manipulator")
    bool UseFrameDistribution() const { return bUseFrameDistribution; }

    void ExecuteFrameDistributedUpdate(float DeltaTime);

    // ==================== 状態取得 ====================

    /** リワインド中かどうか */
    FORCEINLINE bool IsRewinding() const { return bIsRewinding; }

    /** 記録中かどうか */
    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    bool IsRecording() const { return bIsRecordingActive; }

    /** 現在の記録スナップショット数 */
    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    int32 GetSnapshotCount() const;

    /** 記録された総時間（秒） */
    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    float GetRecordedDuration() const;

    /** 最大スナップショット数を取得 */
    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    int32 GetMaxSnapshots() const { return MaxSnapshots; }

    /** 現在の記録モードを取得 */
    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    ERecordingMode GetRecordingMode() const { return RecordingMode; }

    // ==================== イベント ====================

    /** 重力方向が変更された時 */
    FOnGravityDirectionChanged OnGravityDirectionChanged;

    /** リワインド開始時 */
    FOnRewindStarted OnRewindStarted;

    /** リワインド停止時（データ消去後） */
    FOnRewindStopped OnRewindStopped;

    /** 記録開始時 */
    FOnRecordingStarted OnRecordingStarted;

    /** 記録停止時 */
    FOnRecordingStopped OnRecordingStopped;

    /** 最大スナップショット数到達時 */
    FOnRecordingMaxReached OnRecordingMaxReached;

private:
    // ==================== 内部処理 ====================

    void InitializeSnapshotBuffer();
    void RecordSnapshot(float DeltaTime);
    void ApplySnapshot(float DeltaTime);
    void ApplySnapshotImmediate(int32 SnapshotIndex);
    void SaveCurrentMovementState();
    void RestoreMovementState();

    bool CanRecord() const;
    void HandleMaxSnapshotsReached();

    // FPSから間隔を計算
    float GetRewindTickInterval() const;
private:
    // ==================== コンポーネント参照 ====================

    UPROPERTY()
    AActor* OwnerActor;

    UPROPERTY()
    UWorld* CachedWorld;

    // ==================== スナップショットデータ ====================

    /**
     * スナップショット配列（動的配列）
     * - 新しいデータは末尾に追加
     * - リワインド時は末尾から先頭へ遡る
     * - リワインド終了時に全消去
     */
    UPROPERTY()
    TArray<FTimeSnapshot> SnapshotBuffer;

    /**
     * 現在のリワインドインデックス
     * - 最新データ（配列末尾）から開始
     * - 毎フレームデクリメント
     * - -1で終了
     */
    int32 CurrentRewindIndex;

    // ==================== 設定 ====================

    /** 記録モード */
    UPROPERTY(EditAnywhere, Category = "Time Manipulation|Recording")
    ERecordingMode RecordingMode;

    /** 自動記録開始（Automaticモードのみ） */
    UPROPERTY(EditAnywhere, Category = "Time Manipulation|Recording",
        meta = (EditCondition = "RecordingMode == ERecordingMode::Automatic"))
    bool bAutoStartRecording;

    /** 最大スナップショット数 */
    UPROPERTY(EditAnywhere, Category = "Time Manipulation", meta = (ClampMin = "1"))
    int32 MaxSnapshots;

    /** スナップショット記録間隔（秒） */
    UPROPERTY(EditAnywhere, Category = "Time Manipulation", meta = (ClampMin = "0.001"))
    float SnapshotInterval;

    /** リワインド速度（1.0 = 通常速度） */
    UPROPERTY()
    float RewindSpeed;

    float RewindAccumulator;

    /**
     * リワインド最適化設定
     * ビルド環境で逆再生時の負荷を軽減するための設定群
     */

     /**
      * リワインド処理の最適化を有効にするか
      * true: フレーム間引きとFPS制限を適用（推奨：ビルド環境）
      * false: 全フレーム処理（推奨：エディタでのデバッグ時）
      */
    UPROPERTY(EditAnywhere, Category = "Time Manipulation|Performance")
    bool bUseRewindOptimization;

    /**
     * スナップショットを何フレームごとに適用するか
     * 範囲: 1〜10
     * 推奨値: 2〜3（滑らかさと負荷のバランス）
     * 例: 2 = 2フレームごと、3 = 3フレームごと
     * 注意: 値が大きいほど高速だが動きが粗くなる
     */
    UPROPERTY(EditAnywhere, Category = "Time Manipulation|Performance", meta = (EditCondition = "bUseRewindOptimization", ClampMin = "1", ClampMax = "10"))
    int32 RewindFrameStep;

    /**
     * リワインド処理の目標FPS
     * 範囲: 10〜120
     * 推奨値: 30（負荷軽減）、60（滑らか）
     * 例: 30 = 約30fpsで処理、60 = 約60fpsで処理
     * 注意: 値が低いほど負荷は軽減されるがカクつく
     */
    UPROPERTY(EditAnywhere, Category = "Time Manipulation|Performance", meta = (EditCondition = "bUseRewindOptimization", ClampMin = "10", ClampMax = "120"))
    int32 RewindTargetFPS;
    // ==================== 状態フラグ ====================

    /** リワインド中フラグ */
    bool bIsRewinding;

    /** 記録中フラグ */
    bool bIsRecordingActive;

    /** 最大数到達フラグ */
    bool bHasReachedMax;

    UPROPERTY(EditAnywhere)
    bool bIsRewindOnWorld;

    // ==================== タイマー ====================

    /** スナップショット記録用アキュムレータ */
    float SnapshotAccumulator;

    // ==================== 保存された状態 ====================

    /** リワインド前の移動モード */
    TEnumAsByte<EMovementMode> SavedMovementMode;

    /** リワインド前のカスタム移動モード */
    uint8 SavedCustomMovementMode;

    UPROPERTY(EditAnywhere, Category = "Time Manipulation|Performance")
    ERewindQuality RewindQuality = ERewindQuality::Medium;

    UPROPERTY()
    UPlayerCameraControlComponent* CachedCameraControl;

    int32 RewindApplicationFrameSkip;

    UPROPERTY(EditAnywhere, Category = "Rewind|Performance")
    bool bUseFrameDistribution = true;

    float PendingDeltaTime = 0.0f;

    FTimeSnapshot LastAppliedSnapshot;
    FTimeSnapshot TargetSnapshot;
    float LerpAlpha;
    bool bHasValidLerpData;
};