// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/State/LandingState.h"
#include "Interface/PlayerInfoProvider.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/PlayerStateManager.h"

namespace LandingStateConst
{
    static constexpr float LANDING_LAG_TIME = 0.20f;    // デフォルト硬直時間
}

ULandingState::ULandingState()
    : LagTime(0.f)
    , LagDuration(LandingStateConst::LANDING_LAG_TIME)
{
}

void ULandingState::SetLagDuration(float Duration)
{
    if (Duration < LandingStateConst::LANDING_LAG_TIME)
        Duration = LandingStateConst::LANDING_LAG_TIME;

    // 0秒～2秒の範囲にクランプ
    LagDuration = FMath::Clamp(Duration, 0.f, Duration);
}


bool ULandingState::OnEnter(AActor* Owner)
{
    if (!Owner)
        return false;
    OwnerActor = Owner;
    ACharacter* Character = Cast<ACharacter>(OwnerActor);
    if (!Character)
        return false;
    if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
    {
        MoveComp->DisableMovement();   // 移動完全停止
    }
    LagTime = 0.f;

    UE_LOG(LogTemp, Log, TEXT("LandingState: Entered with lag duration: %f seconds"), LagDuration);

    // 必要なら着地アニメ
    // Character->PlayAnimMontage(LandingMontage);
    return true;
}

bool ULandingState::OnUpdate(float DeltaTime)
{
    LagTime += DeltaTime;
    if (LagTime >= LagDuration)
    {
        // DefaultState へ戻す
        if (IPlayerInfoProvider* OwnerInterface = Cast<IPlayerInfoProvider>(OwnerActor))
        {
            OwnerInterface->ChangeState(EPlayerStateType::Default);
        }
    }
    return true;
}

bool ULandingState::OnExit()
{
    if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            // 移動再開
            MoveComp->SetMovementMode(EMovementMode::MOVE_Walking);
        }
    }
    return true;
}