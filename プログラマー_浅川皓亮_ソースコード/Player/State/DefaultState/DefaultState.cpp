// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/State/DefaultState.h"
#include "Player/PlayerCharacter.h"
#include "Player/PlayerStateManager.h"
#include "PostProcessEffectHandle.h"

#include "Components/PrimitiveComponent.h"
#include "Component/PlayerCameraControlComponent.h"
#include "Camera/CameraComponent.h"
#include "Component/WallRun/WallRunComponent.h"

#include "Gameframework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Interface/Soundable.h"
#include "Interface/PlayerInfoProvider.h"

#include "LevelManager.h"

#include "UI/UIManager.h"

#include "SoundHandle.h"
namespace DefaultStateConstants
{
    /** 入力を無視する閾値（スティックの遊び範囲） */
    static constexpr float INPUT_DEADZONE = 0.2f;

    /** Head Bob の補間速度（停止時の戻り速度） */
    static constexpr float BOB_INTERP_SPEED = 5.0f;

    /** 移動速度からボブ強度を算出する際の基準値 */
    static constexpr float BOB_SPEED_REFERENCE = 300.0f;

    /** 移動速度からボブ強度を算出する際の基準値 */
    static constexpr int MAX_JUMPCOUNT = 2;

    /** 着地硬直が発生し始める最小落下距離（単位: cm） */
    static constexpr float MIN_FALL_DISTANCE_FOR_LAG = 800.0f;

    /** 最大着地硬直時間を発生させる落下距離（単位: cm） */
    static constexpr float MAX_FALL_DISTANCE_FOR_LAG = 2000.0f;

    /** 最小着地硬直時間（秒） */
    static constexpr float MIN_LANDING_LAG = 0.0f;

    /** 最大着地硬直時間（秒） */
    static constexpr float MAX_LANDING_LAG = 1.f;
}


// ============================================
// Constructor
// ============================================
UDefaultState::UDefaultState()
    : MoveSpeed(600.0f)
    , CameraBobFrequency(20.0f)
    , CameraBobAmplitude(5.0f)
    , JumpCount(DefaultStateConstants::MAX_JUMPCOUNT)
    , IsRecording(false)
    , LastGroundHeight(0.f)
{

}

// ============================================
// State Lifecycle
// ============================================
bool UDefaultState::OnEnter(AActor* owner)
{
    if (owner == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("DefaultState: Owner is Nullptr"));
        return false;
    }
    OwnerActor = owner;
    // オーナーキャラクターが IPlayerInfoProvider を実装していれば登録
    if (OwnerActor->GetClass()->ImplementsInterface(UPlayerInfoProvider::StaticClass()))
    {
        OwnerInterface.SetInterface(Cast<IPlayerInfoProvider>(OwnerActor));
        OwnerInterface.SetObject(OwnerActor);
    }
    else
    {
        return false;
    }

    // デフォルト移動速度設定
    MoveSpeed = 5.0f;
    IsRecording = false;

    // 初期地面高度を記録
    LastGroundHeight = OwnerActor->GetActorLocation().Z;

    return true;
}
bool UDefaultState::OnUpdate(float DeltaTime)
{
    if (!OwnerInterface)
        return false;
    // --------------------------------------------
    // 接地状態のチェックとジャンプ回数リセット処理
    // --------------------------------------------
    ACharacter* Character = Cast<ACharacter>(OwnerActor);
    if (Character)
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            // 地面に接地している場合
            if (MoveComp->IsMovingOnGround())
            {
                JumpCount = DefaultStateConstants::MAX_JUMPCOUNT;
                // 接地中は常に現在の高度を記録
                LastGroundHeight = OwnerActor->GetActorLocation().Z;
            }
            // 落下中の場合は高度を記録しない（落下開始時の高度を保持）
        }
    }
    return true;
}

bool UDefaultState::OnExit()
{
    //// ★ 記録停止
    //if (ITimeControllable* TimeControllable = Cast<ITimeControllable>(OwnerActor))
    //{
    //   TimeControllable->StopTimeRecording();
    //    UE_LOG(LogTemp, Log, TEXT("DefaultState: Time recording stopped"));
    //}

    return true;
}


// ============================================
// Input Handling
// ============================================
bool UDefaultState::RePlayAction(const FInputActionValue& Value)
{
    if (!OwnerInterface)
    {
        UE_LOG(LogTemp, Error, TEXT("RewindingState:OwnerInterface is null"));
        return false;
    }

    ITimeControllable* TimeControllable = Cast<ITimeControllable>(OwnerActor);

    if (TimeControllable == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RewindingState:TimeControllable is null"));
        return false;
    }

    if (!TimeControllable->IsRecording())
    {
        TimeControllable->StartTimeRecording();
        USoundHandle::PlaySE(this, "StartRecording");
        IsRecording = true;
        UE_LOG(LogTemp, Log, TEXT("RewindingState: Rewind started via PlayerCharacter"));
    }
    else
    {
        IsRecording = false;
        TimeControllable->StopTimeRecording();
        USoundHandle::PlaySE(this, "Replay" , true);
        // ★ 巻き戻しステートへ遷移（記録停止 → Rewindingへ）
        OwnerInterface->ChangeState(EPlayerStateType::Rewinding);
    }

    return true;
}

bool UDefaultState::Movement(const FInputActionValue& Value)
{
    if (!OwnerInterface || !GetWorld())
        return false;

    if (UWallRunComponent* runCom = OwnerActor->GetComponentByClass<UWallRunComponent>())
    {
        if (runCom->IsWallRunning())
            return true;
    }

    FVector2D MoveInput = Value.Get<FVector2D>();

    ACharacter* Character = Cast<ACharacter>(OwnerActor);
    if (!Character)
        return false;

    // 移動処理
    FVector MoveDir = CalculateMoveDirection(Character, MoveInput);
    Character->AddMovementInput(MoveDir, MoveSpeed);

    // カメラコントロールを使用してヘッドボブを更新
    if (UPlayerCameraControlComponent* CameraControl = OwnerActor->FindComponentByClass<UPlayerCameraControlComponent>())
    {
        UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
        bool IsMoving = MoveInput.Size() >= DefaultStateConstants::INPUT_DEADZONE;
        bool IsFalling = MoveComp ? MoveComp->IsFalling() : false;

        CameraControl->UpdateHeadBob(MoveInput, IsMoving, IsFalling);
    }

    return true;
}

bool UDefaultState::Jump(const FInputActionValue& Value)
{
    if (OwnerInterface->PlayParkour())
        return true;

    if (UWallRunComponent* runCom = OwnerActor->GetComponentByClass<UWallRunComponent>())
    {
        if (runCom->HandleJumpPressed())
        {
            JumpCount = DefaultStateConstants::MAX_JUMPCOUNT;
            return true;
        }
    }
    // Check if can perform regular jump
    if (JumpCount <= 0)
        return false;
    if (OwnerActor == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("DefaultState: Owner is Nullptr"));
        return false;
    }
    UCharacterMovementComponent* moveComp = OwnerActor->GetComponentByClass<UCharacterMovementComponent>();
    if (moveComp == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("DefaultState: UCharacterMovementComponent is Nullptr"));
        return false;
    }

     moveComp->AddImpulse(FVector(0, 0, 1200.0f), true);

    --JumpCount;
    return true;
}


// ============================================
// Movement Helpers
// ============================================

/**
 * @brief 入力ベクトルとカメラ方向から実際の移動方向を算出する
 */
FVector UDefaultState::CalculateMoveDirection(ACharacter* Character, const FVector2D& MoveInput) const
{
    FRotator CamRot = Character->GetControlRotation();
    FVector CamForward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
    FVector CamRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);

    // 逆さ状態なら左右反転
    if (FVector::DotProduct(OwnerActor->GetActorUpVector(), FVector::UpVector) < 0.f)
        CamRight *= -1.f;

    FVector WorldMoveDir = (CamRight * MoveInput.X + CamForward * MoveInput.Y).GetSafeNormal();
    FVector LocalMoveDir = OwnerActor->GetActorTransform().InverseTransformVectorNoScale(WorldMoveDir);

    return OwnerActor->GetActorRightVector() * LocalMoveDir.Y +
        OwnerActor->GetActorForwardVector() * LocalMoveDir.X;
}

bool UDefaultState::BoostAction(const FInputActionValue& ActionValue)
{
    if(OwnerInterface == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("DefaultState: Owner is Nullptr"));
        return false;
    }

    OwnerInterface->PlayBoost();
    return true;
}

void UDefaultState::SkillActionStop()
{
    IsRecording = false;
    UPostProcessEffectHandle::DeactivateEffect(this, EPostProcessEffectTag::Recording, true);
}

// ============================================
// Landing Lag Calculation
// ============================================
float UDefaultState::CalculateLandingLagDuration(float FallDistance) const
{
    // 最小落下距離未満なら硬直なし
    if (FallDistance < DefaultStateConstants::MIN_FALL_DISTANCE_FOR_LAG)
    {
        return 0.0f;
    }

    // 最大落下距離以上なら最大硬直時間
    if (FallDistance >= DefaultStateConstants::MAX_FALL_DISTANCE_FOR_LAG)
    {
        return DefaultStateConstants::MAX_LANDING_LAG;
    }

    // 線形補間で硬直時間を計算
    float NormalizedDistance = (FallDistance - DefaultStateConstants::MIN_FALL_DISTANCE_FOR_LAG)
        / (DefaultStateConstants::MAX_FALL_DISTANCE_FOR_LAG - DefaultStateConstants::MIN_FALL_DISTANCE_FOR_LAG);

    return FMath::Lerp(DefaultStateConstants::MIN_LANDING_LAG,
        DefaultStateConstants::MAX_LANDING_LAG,
        NormalizedDistance);
}
