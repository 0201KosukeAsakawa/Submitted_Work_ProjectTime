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
    , RewindFrameStep(2)            
    , RewindTargetFPS(40)          
    , RewindAccumulator(0.0f)     
{
    PrimaryComponentTick.bCanEverTick = true;
}

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

    // 自動記録モードかつ自動開始が有効な場合
    if (RecordingMode == ERecordingMode::Automatic && bAutoStartRecording)
    {
        StartRecording();
    }

    if (!bIsRewindOnWorld)
        return;

    UTimeManagerSubsystem* TimeManager = GetWorld()->GetSubsystem<UTimeManagerSubsystem>();
    if (TimeManager)
    {
        TimeManager->RegisterTimeComponent(this);
    }
}

void UTimeManipulatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UTimeManipulatorComponent::InitializeSnapshotBuffer()
{
    const int32 OldSize = SnapshotBuffer.Num();
    const int32 OldMemory = SnapshotBuffer.GetAllocatedSize();

    // 既存のデータを完全にクリア
    SnapshotBuffer.Empty(MaxSnapshots);  // Empty時にサイズを指定

    // 余剰メモリを確実に解放
    SnapshotBuffer.Shrink();

    CurrentRewindIndex = -1;
    bHasReachedMax = false;
    SnapshotAccumulator = 0.0f;
    RewindAccumulator = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Buffer initialized - Old: %d snapshots (%d bytes) → New: Reserved %d (%d bytes)"),
        OldSize, OldMemory, MaxSnapshots, SnapshotBuffer.GetAllocatedSize());
}


void UTimeManipulatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerActor || !CachedWorld) return;

    if (bIsRewinding)
    {
        if (bUseRewindOptimization)
        {
            // 最適化モード: 固定間隔で処理
            RewindAccumulator += DeltaTime;
            float TickInterval = GetRewindTickInterval();
            if (RewindAccumulator >= TickInterval)
            {
                ApplySnapshot(RewindAccumulator);
                RewindAccumulator = 0.0f;
            }

            // 停止チェック
            if (CurrentRewindIndex < 0)
            {
                StopRewind();
            }
        }
        else
        {
            // 通常モード: 毎フレーム処理（元のコードと同じ）
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

bool UTimeManipulatorComponent::CanRecord() const
{
    // 逆再生中は記録しない
    if (bIsRewinding)
    {
        return false;
    }

    switch (RecordingMode)
    {
    case ERecordingMode::Automatic:
        // 自動記録モードは最大数まで記録
        return SnapshotBuffer.Num() < MaxSnapshots;

    case ERecordingMode::ManualStopAtMax:
        // Maxに到達していない場合のみ記録
        return !bHasReachedMax;

    case ERecordingMode::ManualClearAtMax:
        // 常に記録可能（Maxで自動クリア）
        return true;
    case ERecordingMode::ManualClearAndStopAtMax:
        // 常に記録可能（Maxで自動クリア）
        return !bHasReachedMax;
    default:
        return false;
    }
}

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

void UTimeManipulatorComponent::RecordSnapshot(float DeltaTime)
{
    if (!OwnerActor || !CachedWorld) return;

    // 最大数チェック
    if (SnapshotBuffer.Num() >= MaxSnapshots)
    {
        if (!bHasReachedMax)
        {
            HandleMaxSnapshotsReached();
        }

        // Automaticモードの場合、古いデータを削除して新しいデータを追加
        if (RecordingMode == ERecordingMode::Automatic)
        {
            SnapshotBuffer.RemoveAt(0);  // 最古のデータを削除
            UE_LOG(LogTemp, VeryVerbose, TEXT("TimeManipulator: Removed oldest snapshot (ring buffer)"));
        }
        // ManualClearAtMaxの場合のみクリアして続行
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

    // カメラコンポーネントの記録
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

    // 新しいデータを配列の最後に追加（最新データが末尾）
    SnapshotBuffer.Add(NewSnapshot);

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, VeryVerbose, TEXT("TimeManipulator: Recorded snapshot %d/%d"),
        SnapshotBuffer.Num(), MaxSnapshots);
#endif
}
void UTimeManipulatorComponent::ApplySnapshot(float DeltaTime)
{
    if (!OwnerActor || CurrentRewindIndex < 0) return;

    const FTimeSnapshot& Snapshot = SnapshotBuffer[CurrentRewindIndex];

    OwnerActor->SetActorLocation(Snapshot.Location);
    OwnerActor->SetActorRotation(Snapshot.Rotation);

    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            MoveComp->Velocity = Snapshot.Velocity;

            // 重力方向の変更チェック
            FVector CurrentGravityDir = MoveComp->GetGravityDirection();
            if (!CurrentGravityDir.Equals(Snapshot.GravityDirection, TimeManipulatorConstants::GRAVITY_COMPARISON_TOLERANCE))
            {
                MoveComp->SetGravityDirection(Snapshot.GravityDirection);
                OnGravityDirectionChanged.Broadcast(Snapshot.GravityDirection);
            }

            // 移動モードの復元
            if (MoveComp->MovementMode != Snapshot.MovementMode)
            {
                MoveComp->SetMovementMode(Snapshot.MovementMode, Snapshot.CustomMovementMode);
            }
        }
    }

    // カメラコンポーネントの復元
    if (Snapshot.bHasCameraData)
    {
        if (CachedCameraControl)
        {
            if (APlayerController* PC = Cast<APlayerController>(OwnerActor->GetInstigatorController()))
            {
                PC->SetControlRotation(Snapshot.CameraRotation);
            }

            CachedCameraControl->SetCameraRoll(Snapshot.CameraRoll);
            CachedCameraControl->SetFOV(Snapshot.CameraFOV, true);
        }
    }

    // インデックスを進める（最適化モードなら間引く、通常モードなら1つずつ）
    if (bUseRewindOptimization)
    {
        CurrentRewindIndex -= RewindFrameStep;
        // 負の値になりすぎないように調整
        if (CurrentRewindIndex < -1)
        {
            CurrentRewindIndex = -1;
        }
    }
    else
    {
        // 1フレームずつ戻る
        CurrentRewindIndex--;
    }
}
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


void UTimeManipulatorComponent::StopRecording()
{
    if (!bIsRecordingActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: Recording not active"));
        return;
    }

    bIsRecordingActive = false;
    SnapshotAccumulator = 0.0f;  // アキュムレータをリセット

    OnRecordingStopped.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording stopped (Snapshots: %d, Memory: %d bytes)"),
        SnapshotBuffer.Num(), SnapshotBuffer.GetAllocatedSize());
}


void UTimeManipulatorComponent::ClearRecording()
{
    InitializeSnapshotBuffer();
    bHasReachedMax = false;
    SnapshotAccumulator = 0.0f;
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording cleared"));
}

void UTimeManipulatorComponent::StartRewind(float Duration)
{
    if (!OwnerActor || bIsRewinding) return;

    if (SnapshotBuffer.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: No snapshots to rewind"));
        return;
    }

    bIsRewinding = true;
    RewindAccumulator = 0.0f;  // この行を追加

    // 最新データ（配列の最後）から開始
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

    // ログ部分を置き換え
#if !UE_BUILD_SHIPPING
    if (bUseRewindOptimization)
    {
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Rewind started with optimization (FrameStep: %d, TargetFPS: %d, Snapshots: %d)"),
            RewindFrameStep, RewindTargetFPS, SnapshotBuffer.Num());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Rewind started (Duration: %.2f, Snapshots: %d)"),
            Duration, SnapshotBuffer.Num());
    }
#endif
}
void UTimeManipulatorComponent::StopRewind()
{
    if (!bIsRewinding) return;

    bIsRewinding = false;

    // リワインド終了時に全データを消去
    bool bReachedEnd = (CurrentRewindIndex < 0);

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Rewind stopped (Reached end: %s, Clearing all data)"),
        bReachedEnd ? TEXT("Yes") : TEXT("No (Interrupted)"));

    // データを全消去してメモリ解放
    SnapshotBuffer.Empty();
    SnapshotBuffer.Shrink();  // 追加: メモリを確実に解放

    CurrentRewindIndex = -1;
    bHasReachedMax = false;
    SnapshotAccumulator = 0.0f;

    RestoreMovementState();
    OnRewindStopped.Broadcast();
}
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

void UTimeManipulatorComponent::RestoreMovementState()
{
    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            // 最古のスナップショットのMovementModeを復元（データがある場合）
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

void UTimeManipulatorComponent::ApplyRewindQualityPreset(ERewindQuality rewindQuality)
{
    // 現在の記録状態を保存
    bool bWasRecording = bIsRecordingActive;

    // 記録を完全に停止
    if (bIsRecordingActive)
    {
        StopRecording();
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Stopped recording for quality change"));
    }

    // 逆再生中なら強制停止
    if (bIsRewinding)
    {
        StopRewind();
        UE_LOG(LogTemp, Warning, TEXT("TimeManipulator: Stopped rewind for quality change"));
    }

    switch (rewindQuality)
    {
    case ERewindQuality::Low:
        bUseRewindOptimization = true;
        RewindTargetFPS = 20;
        RewindFrameStep = 3;
        MaxSnapshots = 300;
        SnapshotInterval = 0.05f;
        break;

    case ERewindQuality::Medium:
        bUseRewindOptimization = true;
        RewindTargetFPS = 30;
        RewindFrameStep = 2;
        MaxSnapshots = 600;
        SnapshotInterval = 0.033f;
        break;

    case ERewindQuality::High:
        bUseRewindOptimization = true;
        RewindTargetFPS = 60;
        RewindFrameStep = 1;
        MaxSnapshots = 900;
        SnapshotInterval = 0.016f;
        break;

    case ERewindQuality::Ultra:
        bUseRewindOptimization = false;
        RewindTargetFPS = 60;
        RewindFrameStep = 1;
        MaxSnapshots = 1200;
        SnapshotInterval = 0.016f;
        break;
    }

    // バッファを完全にクリア
    InitializeSnapshotBuffer();

    // 以前記録中だった場合は再開
    if (bWasRecording)
    {
        StartRecording();
        UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Restarted recording with new settings"));
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Quality preset applied - Quality: %d, FPS: %d, Step: %d, MaxSnapshots: %d, Interval: %.3f"),
        static_cast<int32>(rewindQuality), RewindTargetFPS, RewindFrameStep, MaxSnapshots, SnapshotInterval);
}

int32 UTimeManipulatorComponent::GetSnapshotCount() const
{
    return SnapshotBuffer.Num();
}

float UTimeManipulatorComponent::GetRecordedDuration() const
{
    if (SnapshotBuffer.Num() < 2)
    {
        return 0.0f;
    }

    return SnapshotBuffer.Last().Timestamp - SnapshotBuffer[0].Timestamp;
}