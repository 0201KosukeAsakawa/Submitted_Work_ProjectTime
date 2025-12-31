// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerCharacter.h"
#include "Player/PlayerStateManager.h"
#include "Player/State/DefaultState.h"

#include "Player/State/LandingState.h"
#include "PostProcessEffectHandle.h"

#include "SubSystem/TimeManagerSubsystem.h"

#include "Interface/PlayerCharacterState.h"

#include "Component/BoostComponent.h"
#include "Component/TimeManipulatorComponent.h"
#include "Component/PlayerInputBinder.h"
#include "Component/WallRun/WallRunComponent.h"
#include "Component/PlayerCameraControlComponent.h"
#include "Component/ParkourComponent.h"

#include "SoundHandle.h"
#include "UIHandle.h"
// ============================================
// Constructor
// ============================================
APlayerCharacter::APlayerCharacter()
    : bPlayReplayWorld(false)
    ,bPlaySlowMotion(false)
    , SlowMotionTimer(0.0f)
    , SlowMotionDuration(0.0f)
    , SlowMotionScale(1.0f)
{
    PrimaryActorTick.bCanEverTick = true;

    // コンポーネント作成
    StateManager = CreateDefaultSubobject<UPlayerStateManager>(TEXT("StateManager"));
    InputBinder = CreateDefaultSubobject<UPlayerInputBinder>(TEXT("InputBinder"));
    WallRunComponent = CreateDefaultSubobject<UWallRunComponent>(TEXT("WallRunComponent"));
    TimeManipulator = CreateDefaultSubobject<UTimeManipulatorComponent>(TEXT("TimeManipulator"));
    CameraControl = CreateDefaultSubobject<UPlayerCameraControlComponent>(TEXT("CameraControl")); 
    BoostComponent = CreateDefaultSubobject<UBoostComponent>(TEXT("BoostComponent"));
    ParkourComponent = CreateDefaultSubobject<UParkourComponent>(TEXT("ParkourComponent"));

    CameraControl->SetupAttachment(GetMesh(), FName("head"));
    // 相対位置はソケットからのオフセット
    SavedLocalLocation = FVector(5.0f, 10.0f, 0.0f);
    CameraControl->SetRelativeLocation(FVector(15.0f, 0.0f, 0.0f)); // 目の位置調整

    // TimeManipulatorの初期設定
    if (TimeManipulator != nullptr)
    {
        TimeManipulator->SetRecordingMode(ERecordingMode::ManualClearAtMax);
    }
}

// ============================================
// Lifecycle
// ============================================
void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (StateManager != nullptr)
    {
        StateManager->Init();
    }
    // TimeManipulatorのイベントをバインド
    if (TimeManipulator != nullptr)
    {
        TimeManipulator->OnRewindStarted.AddUObject(this, &APlayerCharacter::OnRewindStarted);
        TimeManipulator->OnRewindStopped.AddUObject(this, &APlayerCharacter::OnRewindStopped);

        TimeManipulator->OnRecordingStopped.AddUObject(this, &APlayerCharacter::OnRewindStopped);
    }
    // デリゲートをバインド
    if (ParkourComponent && WallRunComponent)
    {
        ParkourComponent->OnParkourStarted.AddDynamic(this, &APlayerCharacter::OnParkourStarted);
        ParkourComponent->OnParkourEnded.AddDynamic(this, &APlayerCharacter::OnParkourEnded);

        UE_LOG(LogTemp, Log, TEXT("Parkour delegates bound successfully"));
    }
    else
    {
        if (!ParkourComponent)
            UE_LOG(LogTemp, Error, TEXT("ParkourComponent not found"));
        if (!WallRunComponent)
            UE_LOG(LogTemp, Error, TEXT("WallRunComponent not found"));
    }

    UTimeManagerSubsystem::OnSlowStopped.AddUObject(this, &APlayerCharacter::OnSlowStopped);
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ステート更新
    if (StateManager != nullptr)
    {
        StateManager->GetCurrentState()->OnUpdate(DeltaTime);
    }
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (InputBinder)
        {
            InputBinder->BindInputs(EnhancedInput, this);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to cast PlayerInputComponent to UEnhancedInputComponent in %s"), *GetName());
    }
}

// ============================================
// Time Manipulation Interface
// ============================================
void APlayerCharacter::StartTimeRecording()
{
    if (!TimeManipulator)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: TimeManipulator is null"));
        return;
    }

    TimeManipulator->StartRecording();
    ApplyRecordingPostProcess();

    UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Time recording started"));
}

void APlayerCharacter::StopTimeRecording()
{
    if (!TimeManipulator)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: TimeManipulator is null"));
        return;
    }

    TimeManipulator->StopRecording();
    RemoveRecordingPostProcess();

    UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Time recording stopped"));
}

void APlayerCharacter::StartTimeRewind(float Duration)
{
    if (!TimeManipulator)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: TimeManipulator is null"));
        return;
    }

    // 巻き戻し開始（ステートから呼ばれる）
    TimeManipulator->StartRewind(Duration);

    UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Time rewind started (Duration: %.2f)"), Duration);
}

bool APlayerCharacter::IsRecording() const
{
    return TimeManipulator && TimeManipulator->IsRecording();
}

// ============================================
// Post Process Effects
// ============================================
void APlayerCharacter::ApplyRecordingPostProcess()
{
    UPostProcessEffectHandle::ActivateEffect(this, EPostProcessEffectTag::Recording, true);
}

void APlayerCharacter::RemoveRecordingPostProcess()
{
    UPostProcessEffectHandle::DeactivateEffect(this, EPostProcessEffectTag::Recording, true);
}

void APlayerCharacter::ApplyRewindPostProcess()
{
    // 巻き戻し用ポストプロセスマテリアルを追加
    UPostProcessEffectHandle::ActivateEffect(this, EPostProcessEffectTag::Rewinding, true);
}

void APlayerCharacter::RemoveRewindPostProcess()
{
    // 巻き戻し用ポストプロセスマテリアルを削除
    UPostProcessEffectHandle::DeactivateEffect(this, EPostProcessEffectTag::Rewinding, true);
}

void APlayerCharacter::ApplySlowMotionPostProcess_Implementation()
{
}

void APlayerCharacter::RemoveSlowMotionPostProcess_Implementation()
{
  
}


// ============================================
// Time Manipulator Callbacks
// ============================================
void APlayerCharacter::OnRewindStarted()
{
    ApplyRewindPostProcess();
    UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Rewind post process applied"));
}

void APlayerCharacter::OnRewindStopped()
{
    RemoveRewindPostProcess();
    // ステート更新
    if (StateManager != nullptr)
    {
        StateManager->GetCurrentState()->SkillActionStop();
    }
}

void APlayerCharacter::OnSlowStopped()
{
    RemoveSlowMotionPostProcess();
    bPlaySlowMotion = false;
}

void APlayerCharacter::SubscribeToInteract(UObject* Object, FName FunctionName)
{
    if (Object)
    {
        FScriptDelegate Delegate;
        Delegate.BindUFunction(Object, FunctionName);
        OnInteractPressed.Add(Delegate);
    }
}

void APlayerCharacter::UnsubscribeFromInteract(UObject* Object, FName FunctionName)
{
    if (Object)
    {
        // FScriptDelegateを作成して削除
        FScriptDelegate Delegate;
        Delegate.BindUFunction(Object, FunctionName);
        OnInteractPressed.Remove(Delegate);
    }
}

void APlayerCharacter::SetCameraAttachToHead_Implementation(bool bAttachToHead)
{
    if (!CameraControl)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetCameraAttachToHead: CameraControl is null"));
        return;
    }

    USkeletalMeshComponent* mesh = GetMesh();
    if (!mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetCameraAttachToHead: Mesh is null"));
        return;
    }

    // 既に同じ状態なら何もしない
    if (bIsCameraAttachedToHead == bAttachToHead)
    {
        return;
    }

    bIsCameraAttachedToHead = bAttachToHead;


    if (bAttachToHead)
    {
        
        mesh->SetOwnerNoSee(false);
        // 頭のソケットにアタッチ（頭の動きに追従 = 揺れあり）
        CameraControl->AttachToComponent(
            mesh,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HeadSocketName
        );

        // ソケットからの相対位置を設定
        CameraControl->SetRelativeLocation(SavedLocalLocation);
        //CameraControl->SetRelativeRotation(FRotator::ZeroRotator);

        UE_LOG(LogTemp, Log, TEXT("Camera attached to head socket - FPS with head motion"));
    }
    else
    {
        // 常にFPSモード（メッシュは非表示）
        mesh->SetOwnerNoSee(true);
        // ルートコンポーネント（カプセル）にアタッチ（揺れなし）
        CameraControl->AttachToComponent(
            GetRootComponent(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale
        );

        // カプセルからの相対位置を設定（目線の高さを維持）
        CameraControl->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
        //CameraControl->SetRelativeRotation(FRotator::ZeroRotator);

        UE_LOG(LogTemp, Log, TEXT("Camera attached to root - FPS without head motion (stable)"));
    }

    // CameraControlコンポーネント側にも通知
    if (CameraControl)
    {
        CameraControl->SetCameraAttachedToHead(bAttachToHead);
    }
}

// ============================================
// Input Callbacks
// ============================================
void APlayerCharacter::OnMove(const FInputActionValue& Value)
{
    if (StateManager != nullptr && StateManager->GetCurrentState() != nullptr)
    {
        StateManager->GetCurrentState()->Movement(Value);
    }
}

void APlayerCharacter::OnJump(const FInputActionValue& Value)
{
    // ジャンプ処理
    if (StateManager != nullptr)
    {
        StateManager->GetCurrentState()->Jump(Value);
    }
}

void APlayerCharacter::OnReplayAction(const FInputActionValue& Value)
{
    if (StateManager != nullptr)
    {
        StateManager->GetCurrentState()->RePlayAction(Value);
    }
}

void APlayerCharacter::OnLook(const FInputActionValue& Value)
{
    if (CameraControl)
    {
        CameraControl->ProcessLookInput(Value);
    }
}

void APlayerCharacter::OnSlowAction(const FInputActionValue& Value)
{
    // 特殊アクション処理
    UTimeManagerSubsystem* TimeManager = GetWorld()->GetSubsystem<UTimeManagerSubsystem>();
    if (TimeManager == nullptr)
        return;

    if (!bPlaySlowMotion)
    {
        TimeManager->StartSlowMotion(0.1);
        ApplySlowMotionPostProcess();
        bPlaySlowMotion = true;
        USoundHandle::PlaySE(this, "SlowTime", true);
        USoundHandle::PlaySE(this, "SlowTheWorld");
    }
    else
    {
        TimeManager->ResetTimeDilation();
        RemoveSlowMotionPostProcess();
        bPlaySlowMotion = false;
        USoundHandle::StopSE(this, "SlowTime");
    }
    

}

void APlayerCharacter::OnReplayToWorldAction(const FInputActionValue& Value)
{
    UTimeManagerSubsystem* TimeManager = GetWorld()->GetSubsystem<UTimeManagerSubsystem>();
    if (TimeManager == nullptr)
        return;
    TimeManager->RewindToWorld(10);
    USoundHandle::PlaySE(this, "RewindTheWorld");
}

void APlayerCharacter::OnBoost(const FInputActionValue& Value)
{
    if (StateManager != nullptr)
    {
        StateManager->GetCurrentState()->BoostAction(Value);
    }
}

void APlayerCharacter::OnInteractAction(const FInputActionValue& Value)
{
    OnInteractPressed.Broadcast(this);
}

void APlayerCharacter::OpenMenu(const FInputActionValue& Value)
{
    UUIHandle::ShowWidget(this, EWidgetCategory::Menu, "Menu");
}

// ============================================
// IPlayerInfoProvider Interface
// ============================================

bool APlayerCharacter::ChangeState(EPlayerStateType newStateType)
{
    return StateManager != nullptr && StateManager->ChangeState(newStateType);
}

bool APlayerCharacter::IsRewinding() const
{
    return TimeManipulator && TimeManipulator->IsRewinding();
}


UCameraComponent* APlayerCharacter::GetCamera()const
{
    return CameraControl ? CameraControl->GetCamera() : nullptr;
}


void APlayerCharacter::SetNewCameraRotation(float Roll)
{
    if (CameraControl)
    {
        CameraControl->SetCameraRoll(Roll);
    }
}

void APlayerCharacter::PlayBoost()
{
    UPostProcessEffectHandle::StartRadialTransition(this, 1.0f);
    if (BoostComponent)
    {
        BoostComponent->Boost();
    }
}

bool APlayerCharacter::PlayParkour()
{
       return  ParkourComponent && ParkourComponent->Parkour();
}

UPlayerCameraControlComponent* APlayerCharacter::GetCameraControl() const
{
    return CameraControl;
}

void APlayerCharacter::OnParkourStarted()
{
    if (WallRunComponent)
    {
        WallRunComponent->SetDetectionEnabled(false);
        if (WallRunComponent->IsWallRunning())
        {
            WallRunComponent->ExitWallRun();
        }

        UE_LOG(LogTemp, Log, TEXT("Parkour started - WallRun detection disabled"));
    }
}

void APlayerCharacter::OnParkourEnded()
{
    if (WallRunComponent)
    {
        WallRunComponent->SetDetectionEnabled(true);
        UE_LOG(LogTemp, Log, TEXT("Parkour ended - WallRun detection enabled"));
    }
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    // DefaultStateからのみ着地硬直を発生させる
    if (StateManager && StateManager->IsStateMatch(EPlayerStateType::Default))
    {
        // 現在の高度と記録されていた高度の差分を計算
        float CurrentHeight = GetActorLocation().Z;

        // DefaultStateから落下距離と硬直時間を計算
        if (UDefaultState* DefaultState = Cast<UDefaultState>(StateManager->GetCurrentState().GetObject()))
        {
            float FallDistance = DefaultState->GetLastGroundHeight() - CurrentHeight;
            float LagDuration = DefaultState->CalculateLandingLagDuration(FallDistance);

            UE_LOG(LogTemp, Log, TEXT("Landed: Fall distance = %f cm, Lag duration = %f seconds"),
                FallDistance, LagDuration);

            // 硬直時間が0より大きい場合のみLandingStateへ遷移
            if (LagDuration > 0.0f)
            {
                // LandingStateを取得して硬直時間を設定
                if (ULandingState* LandingState = Cast<ULandingState>(StateManager->ChangeState(EPlayerStateType::Landing).GetInterface()))
                {
                    LandingState->SetLagDuration(LagDuration);
                }
                CameraControl->PlayHeavyShake();
                
            }
        }
    }
}