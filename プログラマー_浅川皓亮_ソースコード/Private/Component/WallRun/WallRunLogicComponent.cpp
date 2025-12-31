// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/WallRun/WallRunLogicComponent.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

UWallRunLogicComponent::UWallRunLogicComponent()
    : bIsWallRunning(false)
    , bCanWallJump(false)
    , CurrentWallNormal(FVector::ZeroVector)
    , CurrentMoveDirection(FVector::ZeroVector)
    , PreviousGravityScale(1.0f)
{
    PrimaryComponentTick.bCanEverTick = false;
}

// ============================================
// State Queries
// ============================================

bool UWallRunLogicComponent::CanStartWallRun(UCharacterMovementComponent* CharacterMovement, bool bHasMovementInput) const
{
    // すでに壁走り中
    if (bIsWallRunning)
    {
        return false;
    }

    // MovementComponent がない
    if (!CharacterMovement)
    {
        return false;
    }

    // 条件1: 空中にいること
    if (!CharacterMovement->IsFalling())
    {
        UE_LOG(LogTemp, Log, TEXT("WallRun: Cannot start - not falling"));
        return false;
    }

    // 条件2: 移動入力があること
    if (!bHasMovementInput)
    {
        UE_LOG(LogTemp, Log, TEXT("WallRun: Cannot start - no movement input"));
        return false;
    }

    // 条件3: 最低速度を満たしていること
    float CurrentSpeed = CharacterMovement->Velocity.Size2D();
    if (CurrentSpeed < Settings.MinimumSpeed)
    {
        UE_LOG(LogTemp, Log, TEXT("WallRun: Cannot start - speed too low (%.2f < %.2f)"),
            CurrentSpeed, Settings.MinimumSpeed);
        return false;
    }

    //UE_LOG(LogTemp, Log, TEXT("WallRun: Can start (Speed: %.2f, IsFalling: true, HasInput: true)"), CurrentSpeed);
    return true;
}

bool UWallRunLogicComponent::CanContinueWallRun(UCharacterMovementComponent* CharacterMovement, bool bHasMovementInput, bool bWallStillDetected) const
{
    // 壁走り中でない
    if (!bIsWallRunning)
    {
        return false;
    }

    // MovementComponent がない
    if (!CharacterMovement)
    {
        return false;
    }

    // 条件1: 壁がまだ検出されているか
    if (!bWallStillDetected)
    {
        UE_LOG(LogTemp, Log, TEXT("WallRun: Ending - wall lost"));
        return false;
    }

    // 条件3: 速度が閾値以上か
    float CurrentSpeed = CharacterMovement->Velocity.Size2D();
    if (CurrentSpeed < Settings.EndSpeedThreshold)
    {
        UE_LOG(LogTemp, Log, TEXT("WallRun: Ending - speed too low (%.2f < %.2f)"),
            CurrentSpeed, Settings.EndSpeedThreshold);
        return false;
    }

    // 条件4: 地面に着いていないか
    if (CharacterMovement->IsMovingOnGround())
    {
        UE_LOG(LogTemp, Log, TEXT("WallRun: Ending - on ground"));
        return false;
    }

    return true;
}

// ============================================
// Calculations
// ============================================

FWallRunData UWallRunLogicComponent::CalculateWallRunData(
    const FVector& WallNormal,
    const FVector& CameraForward,
    const FVector& ActorUp) const
{
    FWallRunData Data;

    // ✅ ヒット結果の法線を使用（壁アクターの向きではなく）
    Data.WallNormal = WallNormal;

    // 壁の右方向を計算（壁に沿って横に移動する方向）
    FVector WallRight = CalculateWallRightVector(Data.WallNormal);

    // カメラの向き（水平成分のみ）
    FVector CamForwardFlat = CameraForward;
    CamForwardFlat.Z = 0.0f;
    CamForwardFlat.Normalize();

    // カメラの向きと壁の右方向の内積で、左右どちらに走るか判定
    float DotProduct = FVector::DotProduct(WallRight, CamForwardFlat);
    Data.bIsRightSide = DotProduct > 0.0f;

    // 進行方向を決定（壁の右または左）
    Data.MoveDirection = WallRight * FMath::Sign(DotProduct);

    // カメラロール角度を計算
    Data.CameraRoll = CalculateCameraRoll(Data.bIsRightSide, ActorUp);

    return Data;
}

FVector UWallRunLogicComponent::CalculateWallJumpDirection(
    const FVector& CameraForward,
    const FVector& ActorUp) const
{
    FVector CameraDir = CameraForward;
    CameraDir.Z = 0.0f;
    CameraDir.Normalize();

    FVector JumpDirection = (
        CurrentWallNormal * Settings.JumpNormalMultiplier +
        CameraDir * Settings.JumpForwardMultiplier +
        ActorUp * Settings.JumpUpMultiplier
        ).GetSafeNormal();

    return JumpDirection;
}


FVector UWallRunLogicComponent::CalculateWallJumpVelocity(const FVector& JumpDirection) const
{
    return JumpDirection * Settings.JumpPower;
}


FVector UWallRunLogicComponent::CalculateWallRightVector(const FVector& WallNormal) const
{
    return FVector::CrossProduct(FVector::UpVector, WallNormal).GetSafeNormal();
}


float UWallRunLogicComponent::CalculateCameraRoll(bool bIsRightSide, const FVector& ActorUp) const
{
    float TargetRoll = bIsRightSide ? -Settings.CameraRollAngle : Settings.CameraRollAngle;

    if (FVector::DotProduct(ActorUp, FVector::UpVector) < 0.0f)
    {
        TargetRoll *= -1.0f;
    }

    return TargetRoll;
}


// ============================================
// State Management
// ============================================

void UWallRunLogicComponent::EnterWallRun(const FWallRunData& Data, float InitialGravityScale)
{
    bIsWallRunning = true;
    bCanWallJump = true;
    CurrentWallNormal = Data.WallNormal;
    CurrentMoveDirection = Data.MoveDirection;
    PreviousGravityScale = InitialGravityScale;
}


void UWallRunLogicComponent::ExitWallRun()
{
    bIsWallRunning = false;
    StartWallJumpWindow(Settings.JumpWindow);

    UE_LOG(LogTemp, Log, TEXT("WallRun: Exited (Jump window: %.2f)"), Settings.JumpWindow);
}


void UWallRunLogicComponent::StartWallJumpWindow(float JumpWindow)
{
    bCanWallJump = true;

    UWorld* World = GetWorld();
    if (!World)
        return;

    if (WallJumpWindowHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(WallJumpWindowHandle);
    }

    World->GetTimerManager().SetTimer(
        WallJumpWindowHandle,
        FTimerDelegate::CreateLambda([this]()
            {
                bCanWallJump = false;
                UE_LOG(LogTemp, Log, TEXT("WallRun: Jump window expired"));
            }),
        JumpWindow,
        false
    );
}

void UWallRunLogicComponent::ConsumeWallJump()
{
    bIsWallRunning = false;
    bCanWallJump = false;

    UWorld* World = GetWorld();
    if (World && WallJumpWindowHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(WallJumpWindowHandle);
    }

    //UE_LOG(LogTemp, Log, TEXT("WallRun: Wall jump consumed"));
}





