// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/BoostComponent.h"
#include "Component/PlayerCameraControlComponent.h"
#include "Camera/CameraComponent.h"

#include "LevelManager.h"
#include "SoundHandle.h"

#include "Interface/Soundable.h"
#include "Interface/PlayerInfoProvider.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace BoostConstants
{
    // ブースト定数
    constexpr float MIN_BOOST_SPEED = 300.0f;
    constexpr float BOOST_FORCE = 2500.0f;
    constexpr float BOOST_DURATION = 0.5f;
    constexpr float AIR_COOLDOWN_MULTIPLIER = 0.5f;
    constexpr float BOOST_FRICTION = 0.2f;
    constexpr float BOOST_DECELERATION = 64.0f;
    constexpr float DEFAULT_FRICTION = 8.0f;
    constexpr float DEFAULT_DECELERATION = 2048.0f;

    // ブースト後の高速移動
    constexpr float POST_BOOST_SPEED = 3000.0f;  // ブースト後の速度
}

UBoostComponent::UBoostComponent()
    : bIsBoosting(false)
    , CurrentCooldownTime(0.0f)
    , OriginalMaxWalkSpeed(2400.0f)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBoostComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeComponent();
    CacheComponents();

    // カメラコントロールをキャッシュ
    if (CachedOwner.IsValid())
    {
        CachedCameraControl = CachedOwner->FindComponentByClass<UPlayerCameraControlComponent>();

        if (!CachedCameraControl.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("BoostComponent: PlayerCameraControlComponent not found"));
        }
    }
}


void UBoostComponent::InitializeComponent()
{
    CachedOwner = Cast<ACharacter>(GetOwner());

    if (!CachedOwner.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BoostComponent: Owner is not ACharacter. Component will not function."));
        SetComponentTickEnabled(false);
        return;
    }
}

void UBoostComponent::CacheComponents()
{
    if (!CachedOwner.IsValid())
    {
        return;
    }

    // MovementComponent取得
    CachedMovement = CachedOwner->GetCharacterMovement();
    if (!CachedMovement.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BoostComponent: CharacterMovementComponent not found on Owner '%s'"),
            *CachedOwner->GetName());
        return;
    }

    // 元の速度を保存
    OriginalMaxWalkSpeed = CachedMovement->MaxWalkSpeed;

    // CameraComponent取得（キャラクターから検索）
    TArray<UCameraComponent*> Cameras;
    CachedOwner->GetComponents<UCameraComponent>(Cameras);

}

void UBoostComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CachedOwner.IsValid())
    {
        return;
    }

    // クールダウン更新
    UpdateCooldown(DeltaTime);

    // ブースト後の速度制御
    UpdatePostBoostSpeed();
}

void UBoostComponent::UpdateCooldown(float DeltaTime)
{
    if (CurrentCooldownTime >= BoostCooldownDuration)
    {
        return;
    }

    // 空中時はクールダウン進行を遅くする
    if (CachedMovement.IsValid() && CachedMovement->IsFalling())
    {
        CurrentCooldownTime += DeltaTime * BoostConstants::AIR_COOLDOWN_MULTIPLIER;
    }
    else
    {
        CurrentCooldownTime += DeltaTime;
    }
}

bool UBoostComponent::CanBoost() const
{
    if (!CachedOwner.IsValid() || !CachedMovement.IsValid())
    {
        return false;
    }

    // すでにブースト中
    if (bIsBoosting)
    {
        return false;
    }

    // クールダウン中
    if (CurrentCooldownTime < BoostCooldownDuration)
    {
        return false;
    }

    // 速度が足りない
    const float CurrentSpeed = CachedOwner->GetVelocity().Size2D();
    if (CurrentSpeed < BoostConstants::MIN_BOOST_SPEED)
    {
        return false;
    }

    return true;
}

void UBoostComponent::UpdatePostBoostSpeed()
{
    if (!CachedMovement.IsValid())
    {
        return;
    }

    // ブースト後の高速状態の場合
    if (bIsPostBoostSpeed)
    {
        // 移動入力があるかチェック
        const FVector InputVector = CachedMovement->GetLastInputVector();
        const bool bHasMovementInput = !InputVector.IsNearlyZero(0.01f);

        if (!bHasMovementInput)
        {
            // 入力がなくなったら通常速度に戻す
            CachedMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
            bIsPostBoostSpeed = false;

            // FOVをリセット
            if (CachedCameraControl.IsValid())
            {
                CachedCameraControl->ResetFOV(false);
            }

            UE_LOG(LogTemp, Log, TEXT("BoostComponent: Post-boost speed ended (no input)"));
        }
    }
}
void UBoostComponent::Boost()
{
    if (!CanBoost())
    {
        return;
    }

    StartBoost();
}

void UBoostComponent::StartBoost()
{
    if (!CachedOwner.IsValid() || !CachedMovement.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BoostComponent: Cannot start boost - cached components are invalid"));
        return;
    }

    // カメラ方向を取得
    FVector BoostDirection = FVector::ForwardVector;

    // カメラがない場合はキャラクターの向き
    BoostDirection = CachedOwner->GetActorForwardVector();
    BoostDirection.Z = 0.0f;
    BoostDirection.Normalize();


    // インパルス加速
    CachedMovement->AddImpulse(BoostDirection * BoostConstants::BOOST_FORCE, true);

    // 移動パラメータ変更
    CachedMovement->GroundFriction = BoostConstants::BOOST_FRICTION;
    CachedMovement->BrakingDecelerationWalking = BoostConstants::BOOST_DECELERATION;

    // 状態変更
    bIsBoosting = true;
    CurrentCooldownTime = 0.0f;

    // FOV変更
    if (CachedCameraControl.IsValid())
    {
        CachedCameraControl->SetFOV(BoostFOV, false);
    }

    // デリゲート発火
    OnBoostStarted.Broadcast();

    // ブースト終了タイマー設定
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().SetTimer(
            BoostEndTimerHandle,
            this,
            &UBoostComponent::EndBoost,
            BoostConstants::BOOST_DURATION,
            false
        );
    }
    USoundHandle::PlaySE(this, "Boost");
    UE_LOG(LogTemp, Log, TEXT("BoostComponent: Boost started"));
}

void UBoostComponent::EndBoost()
{
    if (!CachedMovement.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("BoostComponent: Cannot end boost - MovementComponent is invalid"));
        return;
    }

    // 移動パラメータをリセット（慣性系）
    CachedMovement->GroundFriction = BoostConstants::DEFAULT_FRICTION;
    CachedMovement->BrakingDecelerationWalking = BoostConstants::DEFAULT_DECELERATION;

    // 移動入力があるかチェック
    const FVector InputVector = CachedMovement->GetLastInputVector();
    const bool bHasMovementInput = !InputVector.IsNearlyZero(0.01f);

    if (bHasMovementInput)
    {
        // 入力があれば高速状態を維持
        CachedMovement->MaxWalkSpeed = BoostConstants::POST_BOOST_SPEED;
        bIsPostBoostSpeed = true;
        // FOVは維持（ResetFOVを呼ばない）
        UE_LOG(LogTemp, Log, TEXT("BoostComponent: Post-boost speed active (FOV maintained)"));
    }
    else
    {
        // 入力がなければ通常速度に戻す
        CachedMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
        bIsPostBoostSpeed = false;

        // FOVをリセット
        if (CachedCameraControl.IsValid())
        {
            CachedCameraControl->ResetFOV(false);
        }
    }

    // 状態変更
    bIsBoosting = false;

    // デリゲート発火
    OnBoostEnded.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("BoostComponent: Boost ended"));
}