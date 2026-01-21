#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "UE5Coro.h"
#include "TimeManipulatorComponent.generated.h"

// Forward declarations
class UCharacterMovementComponent;
class UPlayerCameraControlComponent;

using namespace UE5Coro;
using namespace UE5Coro::Latent;

namespace TimeConstants
{
    constexpr float DEFAULT_SNAPSHOT_INTERVAL = 0.05f;
    constexpr int32 DEFAULT_MAX_SNAPSHOTS = 300;
    constexpr float GRAVITY_TOLERANCE = 0.01f;
}

UENUM(BlueprintType)
enum class ERecordingMode : uint8
{
    Automatic,
    ManualStopAtMax,
    ManualClearAtMax,
    ManualClearAndStopAtMax
};

UENUM(BlueprintType)
enum class ERewindQuality : uint8
{
    Low,
    Medium,
    High,
    Ultra
};

/**
 * @brief 時間スナップショット（メモリ最適化版）
 */
USTRUCT()
struct FTimeSnapshot
{
    GENERATED_BODY()

    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    FVector Velocity = FVector::ZeroVector;
    FVector GravityDirection = FVector::DownVector;

    TEnumAsByte<EMovementMode> MovementMode = MOVE_Walking;
    uint8 CustomMovementMode = 0;

    float Timestamp = 0.0f;

    // カメラデータ（オプション）
    uint8 bHasCameraData : 1;
    FRotator CameraRotation = FRotator::ZeroRotator;
    float CameraRoll = 0.0f;
    float CameraFOV = 90.0f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGravityDirectionChanged, FVector);
DECLARE_MULTICAST_DELEGATE(FOnRewindStateChanged);

/**
 * @brief 時間操作コンポーネント（コルーチン最適化版）
 *
 * **主な最適化**
 * - Tick完全削除
 * - コルーチンベースの記録・巻き戻し
 * - スナップショットバッファの事前確保
 * - キャッシュの活用
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UTimeManipulatorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimeManipulatorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ============================================
    // Public API
    // ============================================

    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void StartRecording();

    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void StopRecording();

    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void ClearRecording();

    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void StartRewind();

    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void StopRewind();

    UFUNCTION(BlueprintCallable, Category = "Time Manipulation")
    void SetRewindQuality(ERewindQuality Quality);

    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    bool IsRecording() const { return bIsRecording; }

    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    bool IsRewinding() const { return bIsRewinding; }

    UFUNCTION(BlueprintPure, Category = "Time Manipulation")
    int32 GetSnapshotCount() const { return bBufferFull ? MaxSnapshots : SnapshotWriteIndex;}

    // ============================================
    // Delegates
    // ============================================

    FOnGravityDirectionChanged OnGravityDirectionChanged;
    FOnRewindStateChanged OnRewindStarted;
    FOnRewindStateChanged OnRewindStopped;
    FOnRewindStateChanged OnRecordingStarted;
    FOnRewindStateChanged OnRecordingStopped;

private:
    // ============================================
    // Internal Logic
    // ============================================

    void InitializeComponent();
    void InitializeSnapshotBuffer();

    /** @brief 記録ループ（コルーチン） */
    TCoroutine<> RecordingLoop();

    /** @brief 巻き戻しループ（コルーチン） */
    TCoroutine<> RewindLoop();

    /** @brief スナップショットを記録 */
    void CaptureSnapshot();

    /** @brief スナップショットを適用（Lerp補間） */
    void ApplySnapshotLerped(int32 FromIndex, int32 ToIndex, float Alpha);

    /** @brief 移動状態を保存 */
    void SaveMovementState();

    /** @brief 移動状態を復元 */
    void RestoreMovementState();
private:
    // ============================================
    // Settings
    // ============================================

    UPROPERTY(EditAnywhere, Category = "Time Manipulation")
    ERecordingMode RecordingMode = ERecordingMode::Automatic;

    UPROPERTY(EditAnywhere, Category = "Time Manipulation")
    int32 MaxSnapshots = TimeConstants::DEFAULT_MAX_SNAPSHOTS;

    UPROPERTY(EditAnywhere, Category = "Time Manipulation")
    float SnapshotInterval = TimeConstants::DEFAULT_SNAPSHOT_INTERVAL;

    UPROPERTY(EditAnywhere, Category = "Time Manipulation")
    ERewindQuality RewindQuality = ERewindQuality::Medium;

private:
    // ============================================
    // Cached References
    // ============================================

    UPROPERTY()
    TWeakObjectPtr<AActor> CachedOwner;

    UPROPERTY()
    TWeakObjectPtr<ACharacter> CachedCharacter;

    UPROPERTY()
    TWeakObjectPtr<UCharacterMovementComponent> CachedMovement;

    UPROPERTY()
    TWeakObjectPtr<UPlayerCameraControlComponent> CachedCameraControl;

private:
    // ============================================
    // Runtime State
    // ============================================

    /** @brief スナップショットバッファ（事前確保） */
    TArray<FTimeSnapshot> SnapshotBuffer;

    /** @brief 記録中フラグ */
    bool bIsRecording = false;

    /** @brief 巻き戻し中フラグ */
    bool bIsRewinding = false;

    /** @brief コルーチン停止フラグ */
    bool bShouldStopRecording = false;
    bool bShouldStopRewinding = false;

    /** @brief この配列が満タンであるか*/
    bool bBufferFull = false;

    /** @brief この配列が満タンであるか*/
    UPROPERTY(EditAnywhere)
    bool bIsSetSubsystem = true;

    /** @brief 保存された移動状態 */
    TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
    uint8 SavedCustomMovementMode = 0;

    /** @brief 巻き戻し設定（品質による） */
    int32 RewindFrameStep = 5;
    float RewindTargetFPS = 40.0f;

    int32 SnapshotWriteIndex = 0;
};