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

using namespace UE5Coro;
using namespace UE5Coro::Latent;

namespace BoostConstants
{
    constexpr float MIN_BOOST_SPEED = 300.0f;
    constexpr float BOOST_FORCE = 2500.0f;
    constexpr float AIR_COOLDOWN_MULTIPLIER = 0.5f;
    constexpr float BOOST_FRICTION = 0.2f;
    constexpr float BOOST_DECELERATION = 64.0f;
    constexpr float DEFAULT_FRICTION = 8.0f;
    constexpr float DEFAULT_DECELERATION = 2048.0f;
    constexpr float POST_BOOST_SPEED = 3000.0f;
}

UBoostComponent::UBoostComponent()
    : bIsBoosting(false)
    , OriginalMaxWalkSpeed(2400.0f)
{
    PrimaryComponentTick.bCanEverTick = false; // Tick不要
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
// 2. キャスト失敗時はエラーログを出力
void UBoostComponent::InitializeComponent()
{
    CachedOwner = Cast<ACharacter>(GetOwner());

    if (!CachedOwner.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BoostComponent: Owner is not ACharacter. Component will not function."));
        return;
    }
}

// 処理の流れ:
// 1. CharacterMovementComponentを取得
// 2. 元の歩行速度を保存
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
}

// 処理の流れ:
// 1. Ownerとコンポーネントの有効性確認
// 2. すでにブースト中の場合はfalse
// 3. 最低速度に達していない場合はfalse
// 4. 全ての条件を満たせばtrue
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

    const float CurrentSpeed = CachedOwner->GetVelocity().Size2D();
    if (CurrentSpeed < BoostConstants::MIN_BOOST_SPEED)
    {
        return false;
    }

    return true;
}

// 処理の流れ:
// 1. ブースト可能か確認
// 2. 可能ならブーストシーケンスを開始
void UBoostComponent::Boost()
{
    if (!CanBoost())
    {
        return;
    }

    BoostSequence();
}

// 処理の流れ:
// 1. ブースト開始処理（力加算、摩擦低下、FOV変更）
// 2. 指定時間待機
// 3. ブースト終了処理（摩擦復帰）
// 4. 入力がある場合は高速状態維持
// 5. 入力がなくなるまで待機
// 6. 通常速度に復帰、FOVリセット
// 7. クールダウン待機（空中なら半分の速度）
TCoroutine<> UBoostComponent::BoostSequence()
{
    if (!CachedOwner.IsValid() || !CachedMovement.IsValid())
    {
        co_return;
    }

    bIsBoosting = true;
    OnBoostStarted.Broadcast();

    // ===== ブースト開始 =====
    FVector BoostDirection = CachedOwner->GetActorForwardVector();
    BoostDirection.Z = 0.0f;
    BoostDirection.Normalize();

    CachedMovement->AddImpulse(BoostDirection * BoostConstants::BOOST_FORCE, true);
    CachedMovement->GroundFriction = BoostConstants::BOOST_FRICTION;
    CachedMovement->BrakingDecelerationWalking = BoostConstants::BOOST_DECELERATION;

    if (CachedCameraControl.IsValid())
    {
        CachedCameraControl->SetFOV(BoostFOV, false);
    }

    USoundHandle::PlaySE(this, "Boost");
    UE_LOG(LogTemp, Log, TEXT("BoostComponent: Boost started"));

    // ===== ブースト持続時間待機 =====
    co_await Seconds(BoostDuration);

    // ===== ブースト終了処理 =====
    if (!CachedMovement.IsValid())
    {
        bIsBoosting = false;
        co_return;
    }

    CachedMovement->GroundFriction = BoostConstants::DEFAULT_FRICTION;
    CachedMovement->BrakingDecelerationWalking = BoostConstants::DEFAULT_DECELERATION;

    const bool bHasInput = !CachedMovement->GetLastInputVector().IsNearlyZero(0.01f);

    if (bHasInput)
    {
        // 入力あり：高速維持
        CachedMovement->MaxWalkSpeed = BoostConstants::POST_BOOST_SPEED;
        UE_LOG(LogTemp, Log, TEXT("BoostComponent: Post-boost speed active (FOV maintained)"));

        // 入力がなくなるまで待機
        co_await Until([this]()
            {
                return !CachedMovement.IsValid() ||
                    CachedMovement->GetLastInputVector().IsNearlyZero(0.01f);
            });

        UE_LOG(LogTemp, Log, TEXT("BoostComponent: Post-boost speed ended (no input)"));
    }

    // ===== 通常速度に戻す =====
    if (CachedMovement.IsValid())
    {
        CachedMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
    }

    if (CachedCameraControl.IsValid())
    {
        CachedCameraControl->ResetFOV(false);
    }

    bIsBoosting = false;
    OnBoostEnded.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("BoostComponent: Boost ended"));

    // ===== クールダウン処理 =====
    float CooldownRemaining = BoostCooldownDuration;

    while (CooldownRemaining > 0.0f)
    {
        co_await NextTick();

        if (!CachedMovement.IsValid())
        {
            break;
        }

        // 空中ならクールダウンが半分の速度で進む
        float Multiplier = CachedMovement->IsFalling()
            ? BoostConstants::AIR_COOLDOWN_MULTIPLIER
            : 1.0f;

        CooldownRemaining -= GetWorld()->GetDeltaSeconds() * Multiplier;
    }

    UE_LOG(LogTemp, Log, TEXT("BoostComponent: Cooldown complete"));
}
