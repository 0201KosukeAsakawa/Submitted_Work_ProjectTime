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
    constexpr float MIN_BOOST_SPEED = 300.0f;
    constexpr float BOOST_FORCE = 2500.0f;
    constexpr float BOOST_DURATION = 0.5f;
    constexpr float AIR_COOLDOWN_MULTIPLIER = 0.5f;
    constexpr float BOOST_FRICTION = 0.2f;
    constexpr float BOOST_DECELERATION = 64.0f;
    constexpr float DEFAULT_FRICTION = 8.0f;
    constexpr float DEFAULT_DECELERATION = 2048.0f;
    constexpr float POST_BOOST_SPEED = 3000.0f;
}

UBoostComponent::UBoostComponent()
    : bIsBoosting(false)
    , CurrentCooldownTime(0.0f)
    , OriginalMaxWalkSpeed(2400.0f)
{
    PrimaryComponentTick.bCanEverTick = true;
}

// 処理の流れ:
// 1. コンポーネントの初期化
// 2. コンポーネントのキャッシュ
// 3. CameraControlコンポーネントのキャッシュ
void UBoostComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeComponent();
    CacheComponents();

    if (CachedOwner.IsValid())
    {
        CachedCameraControl = CachedOwner->FindComponentByClass<UPlayerCameraControlComponent>();

        if (!CachedCameraControl.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("BoostComponent: PlayerCameraControlComponent not found"));
        }
    }
}

// 処理の流れ:
// 1. OwnerをACharacterにキャスト
// 2. キャスト失敗時はエラーログを出力してTickを無効化
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

// 処理の流れ:
// 1. CharacterMovementComponentを取得
// 2. 元の歩行速度を保存
// 3. CameraComponentを取得（現在は使用していない）
void UBoostComponent::CacheComponents()
{
    if (!CachedOwner.IsValid())
    {
        return;
    }

    CachedMovement = CachedOwner->GetCharacterMovement();
    if (!CachedMovement.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BoostComponent: CharacterMovementComponent not found on Owner '%s'"),
            *CachedOwner->GetName());
        return;
    }

    OriginalMaxWalkSpeed = CachedMovement->MaxWalkSpeed;

    TArray<UCameraComponent*> Cameras;
    CachedOwner->GetComponents<UCameraComponent>(Cameras);
}

// 処理の流れ:
// 1. Ownerの有効性を確認
// 2. クールダウンを更新
// 3. ブースト後の速度制御を更新
void UBoostComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CachedOwner.IsValid())
    {
        return;
    }

    UpdateCooldown(DeltaTime);
    UpdatePostBoostSpeed();
}

// 処理の流れ:
// 1. クールダウンが完了している場合は処理をスキップ
// 2. 空中時はクールダウン進行を半分の速度にする
// 3. 地上時は通常速度でクールダウン
void UBoostComponent::UpdateCooldown(float DeltaTime)
{
    if (CurrentCooldownTime >= BoostCooldownDuration)
    {
        return;
    }

    if (CachedMovement.IsValid() && CachedMovement->IsFalling())
    {
        CurrentCooldownTime += DeltaTime * BoostConstants::AIR_COOLDOWN_MULTIPLIER;
    }
    else
    {
        CurrentCooldownTime += DeltaTime;
    }
}

// 処理の流れ:
// 1. Ownerとコンポーネントの有効性確認
// 2. すでにブースト中の場合はfalse
// 3. クールダウン中の場合はfalse
// 4. 最低速度に達していない場合はfalse
// 5. 全ての条件を満たせばtrue
bool UBoostComponent::CanBoost() const
{
    if (!CachedOwner.IsValid() || !CachedMovement.IsValid())
    {
        return false;
    }

    if (bIsBoosting)
    {
        return false;
    }

    if (CurrentCooldownTime < BoostCooldownDuration)
    {
        return false;
    }

    const float CurrentSpeed = CachedOwner->GetVelocity().Size2D();
    if (CurrentSpeed < BoostConstants::MIN_BOOST_SPEED)
    {
        return false;
    }

    return true;
}

// 処理の流れ:
// 1. ブースト後の高速状態でない場合は処理をスキップ
// 2. 移動入力の有無を確認
// 3. 入力がなくなったら通常速度に戻してFOVをリセット
void UBoostComponent::UpdatePostBoostSpeed()
{
    if (!CachedMovement.IsValid())
    {
        return;
    }

    if (bIsPostBoostSpeed)
    {
        const FVector InputVector = CachedMovement->GetLastInputVector();
        const bool bHasMovementInput = !InputVector.IsNearlyZero(0.01f);

        if (!bHasMovementInput)
        {
            CachedMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
            bIsPostBoostSpeed = false;

            if (CachedCameraControl.IsValid())
            {
                CachedCameraControl->ResetFOV(false);
            }

            UE_LOG(LogTemp, Log, TEXT("BoostComponent: Post-boost speed ended (no input)"));
        }
    }
}

// 処理の流れ:
// 1. ブースト可能か確認
// 2. 可能ならブーストを開始
void UBoostComponent::Boost()
{
    if (!CanBoost())
    {
        return;
    }

    StartBoost();
}

// 処理の流れ:
// 1. OwnerとMovementComponentの有効性確認
// 2. ブースト方向を計算（キャラクターの前方向）
// 3. インパルスを加える
// 4. 移動パラメータを変更（摩擦と減速を低下）
// 5. ブースト状態フラグを設定
// 6. クールダウンをリセット
// 7. FOVを変更
// 8. デリゲートを発火
// 9. ブースト終了タイマーを設定
// 10. SEを再生
void UBoostComponent::StartBoost()
{
    if (!CachedOwner.IsValid() || !CachedMovement.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BoostComponent: Cannot start boost - cached components are invalid"));
        return;
    }

    FVector BoostDirection = CachedOwner->GetActorForwardVector();
    BoostDirection.Z = 0.0f;
    BoostDirection.Normalize();

    CachedMovement->AddImpulse(BoostDirection * BoostConstants::BOOST_FORCE, true);

    CachedMovement->GroundFriction = BoostConstants::BOOST_FRICTION;
    CachedMovement->BrakingDecelerationWalking = BoostConstants::BOOST_DECELERATION;

    bIsBoosting = true;
    CurrentCooldownTime = 0.0f;

    if (CachedCameraControl.IsValid())
    {
        CachedCameraControl->SetFOV(BoostFOV, false);
    }

    OnBoostStarted.Broadcast();

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

// 処理の流れ:
// 1. MovementComponentの有効性確認
// 2. 移動パラメータをリセット（摩擦と減速を元に戻す）
// 3. 移動入力の有無を確認
// 4. 入力がある場合:
//    - 高速状態を維持（POST_BOOST_SPEED）
//    - FOVを維持
// 5. 入力がない場合:
//    - 通常速度に戻す
//    - FOVをリセット
// 6. ブースト状態フラグをfalseに設定
// 7. デリゲートを発火
void UBoostComponent::EndBoost()
{
    if (!CachedMovement.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("BoostComponent: Cannot end boost - MovementComponent is invalid"));
        return;
    }

    CachedMovement->GroundFriction = BoostConstants::DEFAULT_FRICTION;
    CachedMovement->BrakingDecelerationWalking = BoostConstants::DEFAULT_DECELERATION;

    const FVector InputVector = CachedMovement->GetLastInputVector();
    const bool bHasMovementInput = !InputVector.IsNearlyZero(0.01f);

    if (bHasMovementInput)
    {
        CachedMovement->MaxWalkSpeed = BoostConstants::POST_BOOST_SPEED;
        bIsPostBoostSpeed = true;
        UE_LOG(LogTemp, Log, TEXT("BoostComponent: Post-boost speed active (FOV maintained)"));
    }
    else
    {
        CachedMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
        bIsPostBoostSpeed = false;

        if (CachedCameraControl.IsValid())
        {
            CachedCameraControl->ResetFOV(false);
        }
    }

    bIsBoosting = false;
    OnBoostEnded.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("BoostComponent: Boost ended"));
}