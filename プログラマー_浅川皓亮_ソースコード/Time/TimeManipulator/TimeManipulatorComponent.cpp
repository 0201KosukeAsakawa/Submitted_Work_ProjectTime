// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/TimeManipulatorComponent.h"
#include "Component/PlayerCameraControlComponent.h"  
#include "Camera/CameraComponent.h"         

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/GameplayStatics.h"

#include "SubSystem/TimeManagerSubsystem.h"

#include "Player/PlayerCharacter.h"

namespace TimeManipulatorConstants
{
    constexpr float DEFAULT_REWIND_SPEED = 1.0f;
    constexpr float DEFAULT_SNAPSHOT_INTERVAL = 0.05f;
    constexpr int32 DEFAULT_MAX_SNAPSHOTS = 300;
    constexpr float GRAVITY_COMPARISON_TOLERANCE = 0.01f;
}

// 処理の流れ:
// 1. 全てのメンバ変数を初期化
// 2. Tickを有効化
UTimeManipulatorComponent::UTimeManipulatorComponent()
    : OwnerActor(nullptr)
    , CachedWorld(nullptr)
    , CurrentRewindIndex(-1)
    , RecordingMode(ERecordingMode::Automatic)
    , bAutoStartRecording(true)
    , MaxSnapshots(TimeManipulatorConstants::DEFAULT_MAX_SNAPSHOTS)
    , SnapshotInterval(TimeManipulatorConstants::DEFAULT_SNAPSHOT_INTERVAL)
    , RewindSpeed(TimeManipulatorConstants::DEFAULT_REWIND_SPEED)
    , bIsRewinding(false)
    , bIsRecordingActive(false)
    , bHasReachedMax(false)
    , bIsRewindOnWorld(true)
    , SnapshotAccumulator(0.0f)
    , SavedMovementMode(MOVE_Walking)
    , SavedCustomMovementMode(0)
    , bUseRewindOptimization(true)
    , RewindFrameStep(3)
    , RewindTargetFPS(40)
    , RewindApplicationFrameSkip(1)
    , RewindAccumulator(0.0f)
    , LerpAlpha(0.0f)
    , bHasValidLerpData(false)
{
    PrimaryComponentTick.bCanEverTick = true;
}

// 処理の流れ:
// 1. OwnerActorをキャッシュ
// 2. Worldをキャッシュ
// 3. CameraControlコンポーネントをキャッシュ
// 4. スナップショットバッファを初期化
// 5. Automaticモードで自動開始が有効なら記録を開始
// 6. ワールド巻き戻しが有効ならTimeManagerに登録
void UTimeManipulatorComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        UE_LOG(LogTemp, Error, TEXT("TimeManipulatorComponent: No owner actor found"));
        return;
    }

    CachedWorld = GetWorld();
    if (!CachedWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("TimeManipulatorComponent: World is null"));
        return;
    }

    CachedCameraControl = OwnerActor->FindComponentByClass<UPlayerCameraControlComponent>();
    if (!CachedCameraControl)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: CameraControl component not found"));
    }

    InitializeSnapshotBuffer();

    if (RecordingMode == ERecordingMode::Automatic && bAutoStartRecording)
    {
        StartRecording();
    }

    if (!bIsRewindOnWorld)
        return;

    UTimeManagerSubsystem* TimeManager = GetWorld()->GetSubsystem<UTimeManagerSubsystem>();
    if (TimeManager)
    {
        TimeManager->RegisterTimeComponent(this, false);
    }
}

// 処理の流れ:
// 1. TimeManagerから登録解除
void UTimeManipulatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UTimeManagerSubsystem* TimeManager = World->GetSubsystem<UTimeManagerSubsystem>())
        {
            TimeManager->UnregisterTimeComponent(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

// 処理の流れ:
// 1. バッファを空にしてメモリを解放
// 2. 全てのフラグをリセット
// 3. ログ出力
void UTimeManipulatorComponent::InitializeSnapshotBuffer()
{
    const int32 OldSize = SnapshotBuffer.Num();
    const int32 OldMemory = SnapshotBuffer.GetAllocatedSize();

    SnapshotBuffer.Empty(MaxSnapshots);
    SnapshotBuffer.Shrink();

    CurrentRewindIndex = -1;
    bHasReachedMax = false;
    SnapshotAccumulator = 0.0f;
    RewindAccumulator = 0.0f;
    bHasValidLerpData = false;
    LerpAlpha = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Buffer initialized - Old: %d snapshots (%d bytes) → New: Reserved %d (%d bytes)"),
        OldSize, OldMemory, MaxSnapshots, SnapshotBuffer.GetAllocatedSize());
}

// 処理の流れ:
// 1. OwnerActorとWorldの有効性確認
// 2. 巻き戻し中の場合:
//    - フレーム分散が有効ならDeltaTimeを蓄積
//    - そうでなければスナップショットを適用
//    - インデックスが0未満なら巻き戻しを停止
// 3. 記録中の場合:
//    - 時間を蓄積
//    - インターバルに達したらスナップショットを記録
void UTimeManipulatorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerActor || !CachedWorld) return;

    if (bIsRewinding)
    {
        if (bUseFrameDistribution)
        {
            PendingDeltaTime += DeltaTime;
        }
        else
        {
            ApplySnapshot(DeltaTime);

            if (CurrentRewindIndex < 0)
            {
                StopRewind();
            }
        }
    }
    else if (bIsRecordingActive && CanRecord())
    {
        SnapshotAccumulator += DeltaTime;
        if (SnapshotAccumulator >= SnapshotInterval)
        {
            RecordSnapshot(SnapshotAccumulator);
            SnapshotAccumulator = 0.0f;
        }
    }
}

// 処理の流れ:
// 1. 巻き戻し中の場合はfalse
// 2. RecordingModeに応じて判定:
//    - Automatic: バッファが最大未満
//    - ManualStopAtMax: 最大に達していない
//    - ManualClearAtMax: 常にtrue
//    - ManualClearAndStopAtMax: 最大に達していない
bool UTimeManipulatorComponent::CanRecord() const
{
    if (bIsRewinding)
    {
        return false;
    }

    switch (RecordingMode)
    {
    case ERecordingMode::Automatic:
        return SnapshotBuffer.Num() < MaxSnapshots;

    case ERecordingMode::ManualStopAtMax:
        return !bHasReachedMax;

    case ERecordingMode::ManualClearAtMax:
        return true;

    case ERecordingMode::ManualClearAndStopAtMax:
        return !bHasReachedMax;

    default:
        return false;
    }
}

// 処理の流れ:
// 1. 最大到達フラグを立てる
// 2. イベントをブロードキャスト
// 3. RecordingModeに応じて処理:
//    - Automatic: ログ出力のみ
//    - ManualStopAtMax: 記録を停止
//    - ManualClearAtMax: バッファをクリア
//    - ManualClearAndStopAtMax: バッファをクリアして記録を停止
void UTimeManipulatorComponent::HandleMaxSnapshotsReached()
{
    bHasReachedMax = true;
    OnRecordingMaxReached.Broadcast();

    switch (RecordingMode)
    {
    case ERecordingMode::Automatic:
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Max snapshots reached (%d), stopping recording"), MaxSnapshots);
        break;

    case ERecordingMode::ManualStopAtMax:
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Max snapshots reached, stopping recording"));
        StopRecording();
        break;

    case ERecordingMode::ManualClearAtMax:
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Max snapshots reached, clearing buffer"));
        ClearRecording();
        break;

    case ERecordingMode::ManualClearAndStopAtMax:
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Max snapshots reached, clearing buffer and stopping recording"));
        ClearRecording();
        StopRecording();
        break;
    }
}

// 処理の流れ:
// 1. OwnerActorとWorldの有効性確認
// 2. バッファが最大に達している場合:
//    - 最大到達処理を呼び出し
//    - Automaticモードなら最古のスナップショットを削除
//    - それ以外のモードなら記録をスキップ
// 3. 新しいスナップショットを作成:
//    - 位置、回転、速度、タイムスタンプを記録
//    - Characterの場合、重力方向と移動モードも記録
//    - CameraControlがある場合、カメラデータも記録
// 4. バッファに追加
void UTimeManipulatorComponent::RecordSnapshot(float DeltaTime)
{
    if (!OwnerActor || !CachedWorld) return;

    if (SnapshotBuffer.Num() >= MaxSnapshots)
    {
        if (!bHasReachedMax)
        {
            HandleMaxSnapshotsReached();
        }

        if (RecordingMode == ERecordingMode::Automatic)
        {
            SnapshotBuffer.RemoveAt(0);
            UE_LOG(LogTemp, VeryVerbose, TEXT("TimeManipulator: Removed oldest snapshot (ring buffer)"));
        }
        else if (RecordingMode != ERecordingMode::ManualClearAtMax)
        {
            return;
        }
    }

    FTimeSnapshot NewSnapshot;
    NewSnapshot.Location = OwnerActor->GetActorLocation();
    NewSnapshot.Rotation = OwnerActor->GetActorRotation();
    NewSnapshot.Velocity = OwnerActor->GetVelocity();
    NewSnapshot.Timestamp = CachedWorld->GetTimeSeconds();

    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            NewSnapshot.GravityDirection = MoveComp->GetGravityDirection();
            NewSnapshot.MovementMode = MoveComp->MovementMode;
            NewSnapshot.CustomMovementMode = MoveComp->CustomMovementMode;
        }
    }

    if (CachedCameraControl)
    {
        UCameraComponent* Camera = CachedCameraControl->GetCamera();
        if (Camera)
        {
            NewSnapshot.bHasCameraData = true;
            NewSnapshot.CameraRotation = Camera->GetComponentRotation();
            NewSnapshot.CameraRoll = CachedCameraControl->GetCurrentRoll();
            NewSnapshot.CameraFOV = Camera->FieldOfView;
        }
    }

    SnapshotBuffer.Add(NewSnapshot);

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, VeryVerbose, TEXT("TimeManipulator: Recorded snapshot %d/%d"),
        SnapshotBuffer.Num(), MaxSnapshots);
#endif
}

// 処理の流れ:
// 1. OwnerActorとインデックスの有効性確認
// 2. Ultraモード（最適化なし）の場合:
//    - 即座にスナップショットを適用
//    - インデックスを減算
// 3. Lerp補間モードの場合:
//    - アキュムレータを加算
//    - 次のターゲットフレームへ移動するかチェック
//    - RewindFrameStep分スキップした次のインデックスを計算
//    - Lerp補間を適用
void UTimeManipulatorComponent::ApplySnapshot(float DeltaTime)
{
    if (!OwnerActor || CurrentRewindIndex < 0) return;

    if (!bUseRewindOptimization)
    {
        ApplySnapshotImmediate(CurrentRewindIndex);
        CurrentRewindIndex--;
        return;
    }

    RewindAccumulator += DeltaTime;
    float TickInterval = GetRewindTickInterval();

    if (!bHasValidLerpData || RewindAccumulator >= TickInterval)
    {
        int32 NextIndex = CurrentRewindIndex - RewindFrameStep;

        if (NextIndex >= 0)
        {
            LastAppliedSnapshot = SnapshotBuffer[CurrentRewindIndex];
            TargetSnapshot = SnapshotBuffer[NextIndex];
            bHasValidLerpData = true;
            CurrentRewindIndex = NextIndex;
            RewindAccumulator = 0.0f;

#if !UE_BUILD_SHIPPING
            UE_LOG(LogTemp, VeryVerbose, TEXT("TimeManipulator: New lerp target - Steps: %d, Interval: %.3fs"),
                RewindFrameStep, TickInterval);
#endif
        }
        else
        {
            CurrentRewindIndex = -1;
            bHasValidLerpData = false;
            return;
        }
    }

    if (bHasValidLerpData)
    {
        LerpAlpha = FMath::Clamp(RewindAccumulator / TickInterval, 0.0f, 1.0f);
        ApplyLerpedSnapshot(LastAppliedSnapshot, TargetSnapshot, LerpAlpha);
    }
}

// 処理の流れ:
// 1. OwnerActorとインデックスの有効性確認
// 2. スナップショットから位置と回転を適用
// 3. Characterの場合:
//    - 速度を適用
//    - 重力方向を適用（変更があればイベント発火）
//    - 移動モードを適用
// 4. カメラデータがある場合:
//    - ControlRotationを設定
//    - カメラのロールとFOVを設定
void UTimeManipulatorComponent::ApplySnapshotImmediate(int32 SnapshotIndex)
{
    if (!OwnerActor || SnapshotIndex < 0 || SnapshotIndex >= SnapshotBuffer.Num()) return;

    const FTimeSnapshot& Snapshot = SnapshotBuffer[SnapshotIndex];

    OwnerActor->SetActorLocation(Snapshot.Location);
    OwnerActor->SetActorRotation(Snapshot.Rotation);

    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            MoveComp->Velocity = Snapshot.Velocity;

            FVector CurrentGravityDir = MoveComp->GetGravityDirection();
            if (!CurrentGravityDir.Equals(Snapshot.GravityDirection, TimeManipulatorConstants::GRAVITY_COMPARISON_TOLERANCE))
            {
                MoveComp->SetGravityDirection(Snapshot.GravityDirection);
                OnGravityDirectionChanged.Broadcast(Snapshot.GravityDirection);
            }

            if (MoveComp->MovementMode != Snapshot.MovementMode)
            {
                MoveComp->SetMovementMode(Snapshot.MovementMode, Snapshot.CustomMovementMode);
            }
        }
    }

    if (Snapshot.bHasCameraData && CachedCameraControl)
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerActor->GetInstigatorController()))
        {
            PC->SetControlRotation(Snapshot.CameraRotation);
        }

        CachedCameraControl->SetCameraRoll(Snapshot.CameraRoll);
        CachedCameraControl->SetFOV(Snapshot.CameraFOV, true);
    }
}

// 処理の流れ:
// 1. OwnerActorの有効性確認
// 2. 位置と回転を補間
// 3. Characterの場合:
//    - 速度を補間
//    - 重力方向を中間点で切り替え
//    - 移動モードを中間点で切り替え
// 4. カメラデータがある場合:
//    - カメラ回転、ロール、FOVを補間
void UTimeManipulatorComponent::ApplyLerpedSnapshot(const FTimeSnapshot& From, const FTimeSnapshot& To, float Alpha)
{
    if (!OwnerActor) return;

    FVector LerpedLocation = FMath::Lerp(From.Location, To.Location, Alpha);
    FRotator LerpedRotation = FMath::Lerp(From.Rotation, To.Rotation, Alpha);

    OwnerActor->SetActorLocation(LerpedLocation);
    OwnerActor->SetActorRotation(LerpedRotation);

    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            FVector LerpedVelocity = FMath::Lerp(From.Velocity, To.Velocity, Alpha);
            MoveComp->Velocity = LerpedVelocity;

            FVector TargetGravityDir = (Alpha < 0.5f) ? From.GravityDirection : To.GravityDirection;
            FVector CurrentGravityDir = MoveComp->GetGravityDirection();
            if (!CurrentGravityDir.Equals(TargetGravityDir, TimeManipulatorConstants::GRAVITY_COMPARISON_TOLERANCE))
            {
                MoveComp->SetGravityDirection(TargetGravityDir);
                OnGravityDirectionChanged.Broadcast(TargetGravityDir);
            }

            EMovementMode TargetMovementMode = (Alpha < 0.5f) ? From.MovementMode : To.MovementMode;
            uint8 TargetCustomMode = (Alpha < 0.5f) ? From.CustomMovementMode : To.CustomMovementMode;
            if (MoveComp->MovementMode != TargetMovementMode)
            {
                MoveComp->SetMovementMode(TargetMovementMode, TargetCustomMode);
            }
        }
    }

    if (From.bHasCameraData && To.bHasCameraData && CachedCameraControl)
    {
        FRotator LerpedCameraRotation = FMath::Lerp(From.CameraRotation, To.CameraRotation, Alpha);
        float LerpedCameraRoll = FMath::Lerp(From.CameraRoll, To.CameraRoll, Alpha);
        float LerpedFOV = FMath::Lerp(From.CameraFOV, To.CameraFOV, Alpha);

        if (APlayerController* PC = Cast<APlayerController>(OwnerActor->GetInstigatorController()))
        {
            PC->SetControlRotation(LerpedCameraRotation);
        }

        CachedCameraControl->SetCameraRoll(LerpedCameraRoll);
        CachedCameraControl->SetFOV(LerpedFOV, true);
    }
}

// 処理の流れ:
// 1. すでに記録中の場合は警告
// 2. 記録フラグを立てる
// 3. アキュムレータをリセット
// 4. イベントをブロードキャスト
void UTimeManipulatorComponent::StartRecording()
{
    if (bIsRecordingActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: Recording already active"));
        return;
    }

    bIsRecordingActive = true;
    bHasReachedMax = false;
    SnapshotAccumulator = 0.0f;

    OnRecordingStarted.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording started (Mode: %d, MaxSnapshots: %d, Interval: %.3f)"),
        static_cast<int32>(RecordingMode), MaxSnapshots, SnapshotInterval);
}

// 処理の流れ:
// 1. 記録中でない場合は警告
// 2. 記録フラグを下ろす
// 3. アキュムレータをリセット
// 4. イベントをブロードキャスト
void UTimeManipulatorComponent::StopRecording()
{
    if (!bIsRecordingActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: Recording not active"));
        return;
    }

    bIsRecordingActive = false;
    SnapshotAccumulator = 0.0f;

    OnRecordingStopped.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording stopped (Snapshots: %d, Memory: %d bytes)"),
        SnapshotBuffer.Num(), SnapshotBuffer.GetAllocatedSize());
}

// 処理の流れ:
// 1. バッファを初期化
// 2. フラグをリセット
void UTimeManipulatorComponent::ClearRecording()
{
    InitializeSnapshotBuffer();
    bHasReachedMax = false;
    SnapshotAccumulator = 0.0f;
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording cleared"));
}

// 処理の流れ:
// 1. OwnerActor有効性確認、巻き戻し中でないか確認
// 2. スナップショットがない場合は警告
// 3. 巻き戻しフラグを立てる
// 4. アキュムレータをリセット
// 5. 巻き戻しインデックスを最後に設定
// 6. 現在の移動状態を保存
// 7. Characterの移動を停止・無効化
// 8. イベントをブロードキャスト
void UTimeManipulatorComponent::StartRewind(float Duration)
{
    if (!OwnerActor || bIsRewinding) return;

    if (SnapshotBuffer.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: No snapshots to rewind"));
        return;
    }

    bIsRewinding = true;
    RewindAccumulator = 0.0f;
    PendingDeltaTime = 0.0f;
    bHasValidLerpData = false;
    LerpAlpha = 0.0f;

    CurrentRewindIndex = SnapshotBuffer.Num() - 1;

    SaveCurrentMovementState();

    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            MoveComp->StopMovementImmediately();
            MoveComp->DisableMovement();
        }
    }

    OnRewindStarted.Broadcast();

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Rewind started (Mode: %s, FrameStep: %d, Snapshots: %d)"),
        bUseRewindOptimization ? TEXT("Lerp") : TEXT("Immediate"), RewindFrameStep, SnapshotBuffer.Num());
#endif
}

// 処理の流れ:
// 1. 巻き戻し中でない場合はスキップ
// 2. 巻き戻しフラグを下ろす
// 3. 終了まで到達したか判定
// 4. バッファをクリア
// 5. フラグをリセット
// 6. 移動状態を復元
// 7. イベントをブロードキャスト
void UTimeManipulatorComponent::StopRewind()
{
    if (!bIsRewinding) return;

    bIsRewinding = false;
    bool bReachedEnd = (CurrentRewindIndex < 0);

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Rewind stopped (Reached end: %s, Clearing all data)"),
        bReachedEnd ? TEXT("Yes") : TEXT("No (Interrupted)"));

    SnapshotBuffer.Empty();
    SnapshotBuffer.Shrink();

    CurrentRewindIndex = -1;
    bHasReachedMax = false;
    SnapshotAccumulator = 0.0f;
    bHasValidLerpData = false;

    RestoreMovementState();
    OnRewindStopped.Broadcast();
}

// 処理の流れ:
// 1. Characterの移動モードとカスタム移動モードを保存
void UTimeManipulatorComponent::SaveCurrentMovementState()
{
    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            SavedMovementMode = MoveComp->MovementMode;
            SavedCustomMovementMode = MoveComp->CustomMovementMode;
        }
    }
}

// 処理の流れ:
// 1. スナップショットが残っていれば最後のスナップショットの移動モードを復元
// 2. それ以外なら保存しておいた移動モードを復元
// 3. 最終的にFallingモードに設定
void UTimeManipulatorComponent::RestoreMovementState()
{
    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            if (SnapshotBuffer.Num() > 0 && CurrentRewindIndex >= 0)
            {
                const FTimeSnapshot& LastSnapshot = SnapshotBuffer[FMath::Max(0, CurrentRewindIndex)];
                MoveComp->SetMovementMode(LastSnapshot.MovementMode, LastSnapshot.CustomMovementMode);
            }
            else
            {
                MoveComp->SetMovementMode(SavedMovementMode, SavedCustomMovementMode);
            }

            MoveComp->SetMovementMode(MOVE_Falling);
        }
    }
}

// 処理の流れ:
// 1. グローバルタイムダイレーションを0.0または1.0に設定
// 2. Ownerのカスタムタイムダイレーションを1.0に設定
void UTimeManipulatorComponent::SetTimeStop(bool bStop)
{
    if (!OwnerActor || !CachedWorld) return;

    UGameplayStatics::SetGlobalTimeDilation(CachedWorld, bStop ? 0.0f : 1.0f);
    OwnerActor->CustomTimeDilation = 1.0f;
}

void UTimeManipulatorComponent::SetTimeScale(float Scale)
{
    if (!CachedWorld) return;
    UGameplayStatics::SetGlobalTimeDilation(CachedWorld, Scale);
}

void UTimeManipulatorComponent::SetCustomTimeDilation(float Scale)
{
    if (OwnerActor)
    {
        OwnerActor->CustomTimeDilation = Scale;
    }
}

// 処理の流れ:
// 1. 最大スナップショット数が0以下の場合は警告
// 2. 最大値を設定
// 3. バッファを初期化
void UTimeManipulatorComponent::SetMaxSnapshots(int32 Max)
{
    if (Max <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: Invalid MaxSnapshots value: %d"), Max);
        return;
    }

    MaxSnapshots = Max;
    InitializeSnapshotBuffer();
}

// 処理の流れ:
// 1. 記録中の場合は警告
// 2. RecordingModeを設定
// 3. バッファを初期化
void UTimeManipulatorComponent::SetRecordingMode(ERecordingMode Mode)
{
    if (bIsRecordingActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: Cannot change recording mode while recording is active"));
        return;
    }

    RecordingMode = Mode;
    InitializeSnapshotBuffer();
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording mode changed to %d"), static_cast<int32>(Mode));
}

// 処理の流れ:
// 1. 現在記録中なら停止
// 2. 巻き戻し中なら停止
// 3. 品質に応じて各パラメータを設定:
//    - Low: FPS20、大胆にスキップ
//    - Medium: FPS30、大きくスキップ
//    - High: FPS40、中程度にスキップ
//    - Ultra: FPS60、スキップなし
// 4. バッファを初期化
// 5. 記録中だった場合は再開
void UTimeManipulatorComponent::ApplyRewindQualityPreset(ERewindQuality rewindQuality)
{
    bool bWasRecording = bIsRecordingActive;

    if (bIsRecordingActive)
    {
        StopRecording();
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Stopped recording for quality change"));
    }

    if (bIsRewinding)
    {
        StopRewind();
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: Stopped rewind for quality change"));
    }

    switch (rewindQuality)
    {
    case ERewindQuality::Low:
        bUseRewindOptimization = true;
        bUseFrameDistribution = true;
        RewindTargetFPS = 20;
        RewindApplicationFrameSkip = 1;
        RewindFrameStep = 15;
        SnapshotInterval = 0.1f;
        break;

    case ERewindQuality::Medium:
        bUseRewindOptimization = true;
        bUseFrameDistribution = true;
        RewindTargetFPS = 30;
        RewindApplicationFrameSkip = 1;
        RewindFrameStep = 10;
        SnapshotInterval = 0.07f;
        break;

    case ERewindQuality::High:
        bUseRewindOptimization = true;
        bUseFrameDistribution = true;
        RewindTargetFPS = 40;
        RewindApplicationFrameSkip = 1;
        RewindFrameStep = 5;
        SnapshotInterval = 0.05f;
        break;

    case ERewindQuality::Ultra:
        bUseRewindOptimization = false;
        bUseFrameDistribution = false;
        RewindTargetFPS = 60;
        RewindApplicationFrameSkip = 1;
        RewindFrameStep = 1;
        SnapshotInterval = 0.016f;
        break;
    }

    InitializeSnapshotBuffer();

    if (bWasRecording)
    {
        StartRecording();
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Restarted recording with new settings"));
    }

    float ActualInterval = GetRewindTickInterval();
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Quality preset applied - Quality: %d, FrameStep: %d, Interval: %.3fs"),
        static_cast<int32>(RewindQuality), RewindFrameStep, ActualInterval);
}

// 処理の流れ:
// 1. ターゲットFPSとフレームスキップから実際のインターバルを計算
float UTimeManipulatorComponent::GetRewindTickInterval() const
{
    return (1.0f / static_cast<float>(RewindTargetFPS)) * static_cast<float>(RewindApplicationFrameSkip);
}

int32 UTimeManipulatorComponent::GetSnapshotCount() const
{
    return SnapshotBuffer.Num();
}

// 処理の流れ:
// 1. スナップショットが2つ未満なら0.0を返す
// 2. 最後と最初のタイムスタンプの差を返す
float UTimeManipulatorComponent::GetRecordedDuration() const
{
    if (SnapshotBuffer.Num() < 2)
    {
        return 0.0f;
    }

    return SnapshotBuffer.Last().Timestamp - SnapshotBuffer[0].Timestamp;
}

// 処理の流れ:
// 1. 巻き戻し中でない、またはPendingDeltaTimeが0以下ならスキップ
// 2. スナップショットを適用
// 3. PendingDeltaTimeをリセット
// 4. インデックスが0未満なら巻き戻しを停止
void UTimeManipulatorComponent::ExecuteFrameDistributedUpdate(float DeltaTime)
{
    if (!bIsRewinding || PendingDeltaTime <= 0.0f) return;

    ApplySnapshot(PendingDeltaTime);
    PendingDeltaTime = 0.0f;

    if (CurrentRewindIndex < 0)
    {
        StopRewind();
    }
}