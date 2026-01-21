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

UTimeManipulatorComponent::UTimeManipulatorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// 処理の流れ:
// 1. 参照をキャッシュ
// 2. スナップショットバッファを初期化
// 3. TimeManagerに登録
void UTimeManipulatorComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeComponent();
    InitializeSnapshotBuffer();

    if (!bIsSetSubsystem)
        return;

    if (UTimeManagerSubsystem* TimeManager = GetWorld()->GetSubsystem<UTimeManagerSubsystem>())
    {
        TimeManager->RegisterTimeComponent(this, false);
    }
    if (RecordingMode == ERecordingMode::Automatic)
        StartRecording();
}

void UTimeManipulatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopRecording();
    StopRewind();

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
// 1. Ownerとコンポーネントをキャッシュ
void UTimeManipulatorComponent::InitializeComponent()
{
    CachedOwner = GetOwner();
    CachedCharacter = Cast<ACharacter>(CachedOwner);

    if (CachedCharacter.IsValid())
    {
        CachedMovement = CachedCharacter->GetCharacterMovement();
        CachedCameraControl = CachedCharacter->FindComponentByClass<UPlayerCameraControlComponent>();
    }
}

// 処理の流れ:
// 1. バッファを確保してメモリを事前割り当て
void UTimeManipulatorComponent::InitializeSnapshotBuffer()
{
    SnapshotBuffer.SetNum(MaxSnapshots);
    SnapshotWriteIndex = 0;
    bBufferFull = false;

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Buffer initialized (%d snapshots, %d bytes)"),
        MaxSnapshots, SnapshotBuffer.GetAllocatedSize());
}

// 処理の流れ:
// 1. すでに記録中ならスキップ
// 2. フラグを立てて記録ループを開始
void UTimeManipulatorComponent::StartRecording()
{
    if (bIsRecording)
    {
        return;
    }

    bIsRecording = true;
    bShouldStopRecording = false;
    OnRecordingStarted.Broadcast();

    RecordingLoop();

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording started"));
}

// 処理の流れ:
// 1. 停止フラグを立てる
void UTimeManipulatorComponent::StopRecording()
{
    if (!bIsRecording)
    {
        return;
    }

    bShouldStopRecording = true;
    bIsRecording = false;
    OnRecordingStopped.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording stopped (%d snapshots)"),
        SnapshotBuffer.Num());
}

// 処理の流れ:
// 1. バッファをクリア
void UTimeManipulatorComponent::ClearRecording()
{
    SnapshotBuffer.Reset();
    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Recording cleared"));
}

// 処理の流れ:
// 1. スナップショットがなければスキップ
// 2. 移動状態を保存
// 3. 巻き戻しループを開始
void UTimeManipulatorComponent::StartRewind()
{
    if (bIsRewinding || SnapshotBuffer.Num() == 0)
    {
        return;
    }

    SaveMovementState();

    if (CachedMovement.IsValid())
    {
        CachedMovement->StopMovementImmediately();
        CachedMovement->DisableMovement();
    }

    bIsRewinding = true;
    bShouldStopRewinding = false;
    OnRewindStarted.Broadcast();
    CachedOwner->SetActorTickEnabled(false);
    RewindLoop();

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Rewind started"));
}

// 処理の流れ:
// 1. 停止フラグを立てる
// 2. バッファをクリア
// 3. 移動状態を復元
void UTimeManipulatorComponent::StopRewind()
{
    if (!bIsRewinding)
    {
        return;
    }

    bShouldStopRewinding = true;
    bIsRewinding = false;
    SnapshotWriteIndex = 0;
    SnapshotBuffer.Reset();
    RestoreMovementState();
    OnRewindStopped.Broadcast();

    if (RecordingMode == ERecordingMode::Automatic)
        StartRecording();

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Rewind stopped"));
    CachedOwner->SetActorTickEnabled(true);
}

// 処理の流れ:
// 1. 品質に応じてパラメータを設定
void UTimeManipulatorComponent::SetRewindQuality(ERewindQuality Quality)
{
    RewindQuality = Quality;

    switch (Quality)
    {
    case ERewindQuality::Low:
        RewindFrameStep = 15;
        RewindTargetFPS = 20.0f;
        SnapshotInterval = 0.1f;
        break;
    case ERewindQuality::Medium:
        RewindFrameStep = 10;
        RewindTargetFPS = 30.0f;
        SnapshotInterval = 0.07f;
        break;
    case ERewindQuality::High:
        RewindFrameStep = 5;
        RewindTargetFPS = 40.0f;
        SnapshotInterval = 0.05f;
        break;
    case ERewindQuality::Ultra:
        RewindFrameStep = 1;
        RewindTargetFPS = 60.0f;
        SnapshotInterval = 0.016f;
        break;
    }

    InitializeSnapshotBuffer();

    UE_LOG(LogTemp, Log, TEXT("TimeManipulator: Quality set to %d (Step=%d, FPS=%.0f)"),
        static_cast<int32>(Quality), RewindFrameStep, RewindTargetFPS);
}

// ============================================
// Coroutines
// ============================================

// 処理の流れ:
// 1. 一定間隔でスナップショットを記録
// 2. 停止フラグが立つまで継続
TCoroutine<> UTimeManipulatorComponent::RecordingLoop()
{
    while (!bShouldStopRecording && CachedOwner.IsValid())
    {
        co_await Seconds(SnapshotInterval);

        if (RecordingMode != ERecordingMode::Automatic && bBufferFull)
        {
            break; // 停止
        }

        CaptureSnapshot();

        SnapshotWriteIndex = (SnapshotWriteIndex + 1) % MaxSnapshots;

        if (SnapshotWriteIndex == 0)
        {
            bBufferFull = true;
        }
    }

    bIsRecording = false;
}

// 処理の流れ:
// 1. 最新から古いスナップショットへLerp補間しながら適用
// 2. 停止フラグが立つまで継続
TCoroutine<> UTimeManipulatorComponent::RewindLoop()
{
    const float FrameTime = 1.0f / RewindTargetFPS;

    if (!bBufferFull)
    {
        // バッファが満杯でない場合: 単純に最新から0へ
        const int32 TotalSnapshots = SnapshotWriteIndex;

        if (TotalSnapshots < 2)
        {
            StopRewind();
            co_return;
        }

        for (int32 i = TotalSnapshots - 1; i >= 0 && !bShouldStopRewinding; i -= RewindFrameStep)
        {
            const int32 FromIndex = i;
            const int32 ToIndex = FMath::Max(0, i - RewindFrameStep);

            const float StepDuration = FrameTime * RewindFrameStep;
            float Elapsed = 0.0f;

            while (Elapsed < StepDuration && !bShouldStopRewinding)
            {
                co_await NextTick();
                if (!GetWorld()) { StopRewind(); co_return; }

                Elapsed += GetWorld()->GetDeltaSeconds();
                const float Alpha = FMath::Clamp(Elapsed / StepDuration, 0.0f, 1.0f);
                ApplySnapshotLerped(FromIndex, ToIndex, Alpha);
            }

            if (ToIndex == 0) break;
        }
    }
    else
    {
        // バッファが満杯の場合: 2パートに分けて逆再生
        const int32 StartIndex = (SnapshotWriteIndex - 1 + MaxSnapshots) % MaxSnapshots;

        // Part 1: 最新位置から0まで
        for (int32 i = StartIndex; i >= 0 && !bShouldStopRewinding; i -= RewindFrameStep)
        {
            const int32 FromIndex = i;
            const int32 ToIndex = FMath::Max(0, i - RewindFrameStep);

            const float StepDuration = FrameTime * RewindFrameStep;
            float Elapsed = 0.0f;

            while (Elapsed < StepDuration && !bShouldStopRewinding)
            {
                co_await NextTick();
                if (!GetWorld()) { StopRewind(); co_return; }

                Elapsed += GetWorld()->GetDeltaSeconds();
                const float Alpha = FMath::Clamp(Elapsed / StepDuration, 0.0f, 1.0f);
                ApplySnapshotLerped(FromIndex, ToIndex, Alpha);
            }

            if (ToIndex == 0) break;
        }

        if (bShouldStopRewinding)
        {
            StopRewind();
            co_return;
        }

        // Part 2: 配列の最後から書き込み位置まで（最古のデータまで）
        for (int32 i = MaxSnapshots - 1; i >= SnapshotWriteIndex && !bShouldStopRewinding; i -= RewindFrameStep)
        {
            const int32 FromIndex = i;
            const int32 ToIndex = FMath::Max(SnapshotWriteIndex, i - RewindFrameStep);

            const float StepDuration = FrameTime * RewindFrameStep;
            float Elapsed = 0.0f;

            while (Elapsed < StepDuration && !bShouldStopRewinding)
            {
                co_await NextTick();
                if (!GetWorld()) { StopRewind(); co_return; }

                Elapsed += GetWorld()->GetDeltaSeconds();
                const float Alpha = FMath::Clamp(Elapsed / StepDuration, 0.0f, 1.0f);
                ApplySnapshotLerped(FromIndex, ToIndex, Alpha);
            }

            if (ToIndex == SnapshotWriteIndex) break;
        }
        bBufferFull = false;
    }

    StopRewind();
}

// 処理の流れ:
// 1. 現在のアクター状態をキャプチャ
void UTimeManipulatorComponent::CaptureSnapshot()
{
    if (!CachedOwner.IsValid()) return;

    FTimeSnapshot Snapshot;
    Snapshot.Location = CachedOwner->GetActorLocation();
    Snapshot.Rotation = CachedOwner->GetActorRotation();
    Snapshot.Velocity = CachedOwner->GetVelocity();
    Snapshot.Timestamp = GetWorld()->GetTimeSeconds();

    if (CachedMovement.IsValid())
    {
        Snapshot.GravityDirection = CachedMovement->GetGravityDirection();
        Snapshot.MovementMode = CachedMovement->MovementMode;
        Snapshot.CustomMovementMode = CachedMovement->CustomMovementMode;
    }

    if (CachedCameraControl.IsValid())
    {
        if (UCameraComponent* Camera = CachedCameraControl->GetCamera())
        {
            Snapshot.bHasCameraData = true;
            Snapshot.CameraRotation = Camera->GetComponentRotation();
            Snapshot.CameraRoll = CachedCameraControl->GetCurrentRoll();
            Snapshot.CameraFOV = Camera->FieldOfView;
        }
    }
    SnapshotBuffer[SnapshotWriteIndex] = Snapshot;

}

// 処理の流れ:
// 1. 2つのスナップショット間を補間
// 2. アクターに適用
void UTimeManipulatorComponent::ApplySnapshotLerped(int32 FromIndex, int32 ToIndex, float Alpha)
{
    if (!CachedOwner.IsValid() ||
        !SnapshotBuffer.IsValidIndex(FromIndex) ||
        !SnapshotBuffer.IsValidIndex(ToIndex))
    {
        return;
    }

    const FTimeSnapshot& From = SnapshotBuffer[FromIndex];
    const FTimeSnapshot& To = SnapshotBuffer[ToIndex];

    // 位置・回転を補間
    CachedOwner->SetActorLocation(FMath::Lerp(From.Location, To.Location, Alpha));
    CachedOwner->SetActorRotation(FMath::Lerp(From.Rotation, To.Rotation, Alpha));

    if (CachedMovement.IsValid())
    {
        CachedMovement->Velocity = FMath::Lerp(From.Velocity, To.Velocity, Alpha);

        // 重力方向は中間点で切り替え
        const FVector& TargetGravity = (Alpha < 0.5f) ? From.GravityDirection : To.GravityDirection;
        if (!CachedMovement->GetGravityDirection().Equals(TargetGravity, TimeConstants::GRAVITY_TOLERANCE))
        {
            CachedMovement->SetGravityDirection(TargetGravity);
            OnGravityDirectionChanged.Broadcast(TargetGravity);
        }
    }

    // カメラ補間
    if (From.bHasCameraData && To.bHasCameraData && CachedCameraControl.IsValid())
    {
        const FRotator CameraRot = FMath::Lerp(From.CameraRotation, To.CameraRotation, Alpha);
        const float CameraRoll = FMath::Lerp(From.CameraRoll, To.CameraRoll, Alpha);
        const float CameraFOV = FMath::Lerp(From.CameraFOV, To.CameraFOV, Alpha);

        if (APlayerController* PC = Cast<APlayerController>(CachedOwner->GetInstigatorController()))
        {
            PC->SetControlRotation(CameraRot);
        }

        CachedCameraControl->SetCameraRoll(CameraRoll);
        CachedCameraControl->SetFOV(CameraFOV, true);
    }
}

void UTimeManipulatorComponent::SaveMovementState()
{
    if (CachedMovement.IsValid())
    {
        SavedMovementMode = CachedMovement->MovementMode;
        SavedCustomMovementMode = CachedMovement->CustomMovementMode;
    }
}

void UTimeManipulatorComponent::RestoreMovementState()
{
    if (CachedMovement.IsValid())
    {
        CachedMovement->SetMovementMode(MOVE_Falling);
    }
}