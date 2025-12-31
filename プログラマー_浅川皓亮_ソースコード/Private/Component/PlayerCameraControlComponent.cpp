// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerCameraControlComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

#include "SaveManager.h"

namespace CameraControlConstants
{
    /** 入力を無視する閾値 */
    constexpr float InputDeadZone = 0.2f;

    /** デフォルトのカメラロール */
    constexpr float DefaultRoll = 0.0f;
}

// ============================================
// Constructor
// ============================================
UPlayerCameraControlComponent::UPlayerCameraControlComponent()
    :/* CameraLocationOffset(FVector(0.0f, 0.0f, 60.0f))
    , */bUsePawnControlRotation(true)
    , Camera(nullptr)
    , CurrentRoll(0.0f)
    , TargetRoll(0.0f)
    , bPitchInverted(false)
    , HeadBobTime(0.0f)
    , HeadBobBaseLocation(FVector::ZeroVector)
    , CurrentFOV(90.0f)
    , TargetFOV(90.0f)
{
    PrimaryComponentTick.bCanEverTick = true;

    // カメラコンポーネントを作成
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
    if (Camera)
    {
        Camera->SetupAttachment(this);
        Camera->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
        Camera->bUsePawnControlRotation = bUsePawnControlRotation;

        UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: Camera created at offset: %s"), *CameraLocationOffset.ToString());
    }
}

// ============================================
// Component Lifecycle
// ============================================
void UPlayerCameraControlComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Camera && !Camera->IsAttachedTo(this))
    {
        Camera->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
        UE_LOG(LogTemp, Warning, TEXT("Camera was reattached to PlayerCameraControlComponent"));
    }
   InitializeComponent();
   RotationSettings.Sensitivity = USaveManager::GetCameraSensitivity();
}

void UPlayerCameraControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateCameraRoll(DeltaTime);
    UpdateFOV(DeltaTime);
}

// ============================================
// Initialization
// ============================================
void UPlayerCameraControlComponent::InitializeComponent()
{
    if (!Camera)
    {
        UE_LOG(LogTemp, Error, TEXT("PlayerCameraControl: Camera component is null"));
        return;
    }

    // ✅ ソケット基準の相対位置を使用
    CameraLocationOffset = GetRelativeLocation();

    Camera->bUsePawnControlRotation = bUsePawnControlRotation;

    // FOVの初期化
    CurrentFOV = FOVSettings.DefaultFOV;
    TargetFOV = CurrentFOV;
    Camera->SetFieldOfView(CurrentFOV);

    // MovementComponentをキャッシュ
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        CachedMovement = Character->GetCharacterMovement();
    }

    // ヘッドボブのベース位置を設定（ソケットからの相対位置）
    HeadBobBaseLocation = CameraLocationOffset;

    UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: Initialized on socket (Offset: %s, FOV: %.1f)"),
        *CameraLocationOffset.ToString(), CurrentFOV);
}
// ============================================
// Camera Rotation
// ============================================

/**
 * プレイヤーの視点入力を処理し、カメラ回転を制御する
 *
 * @param Value 入力デバイス（スティックやマウス）からの回転入力値
 */
void UPlayerCameraControlComponent::ProcessLookInput(const FInputActionValue& Value)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessLookInput: Owner is null."));
        return;
    }

    // 型チェック（想定外の入力型を防ぐ）
    if (Value.GetValueType() != EInputActionValueType::Axis2D)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessLookInput: Invalid input value type."));
        return;
    }

    FVector2D LookInput = Value.Get<FVector2D>();

    // デッドゾーン処理（一定以下の入力を無視）
    const float InputMagnitude = LookInput.Size();
    if (InputMagnitude < CameraControlConstants::InputDeadZone)
        return;

    // 感度適用
    LookInput *= RotationSettings.Sensitivity;

    // ピッチ反転（ユーザー設定対応）
    if (bPitchInverted)
    {
        LookInput.Y *= -1.0f;
    }

    // コントローラに回転入力を適用
    if (APlayerController* PlayerController = Cast<APlayerController>(Owner->GetInstigatorController()))
    {
        PlayerController->AddYawInput(LookInput.X);
        PlayerController->AddPitchInput(LookInput.Y);
    }
}

void UPlayerCameraControlComponent::SetCameraRoll(float Roll)
{
    TargetRoll = Roll;
}

void UPlayerCameraControlComponent::ResetCameraRoll()
{
    TargetRoll = CameraControlConstants::DefaultRoll;
}

void UPlayerCameraControlComponent::SetPitchInverted(bool bInverted)
{
    bPitchInverted = bInverted;
}

void UPlayerCameraControlComponent::UpdateCameraRoll(float DeltaTime)
{
    if (!Camera)
        return;

    CurrentRoll = FMath::FInterpTo(CurrentRoll, TargetRoll, DeltaTime, RotationSettings.RollInterpSpeed);

    // ✅ PlayerController の ControlRotation に直接 Roll を設定
    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (PC/* && !FMath::IsNearlyZero(TargetRoll)*/)
    {
        FRotator ControlRotation = PC->GetControlRotation();
        ControlRotation.Roll = CurrentRoll;
        PC->SetControlRotation(ControlRotation);
    }
}

void UPlayerCameraControlComponent::SetSensitivity(float NewSensitivity)
{
    RotationSettings.Sensitivity = FMath::Clamp(NewSensitivity, 0.01f, 3.f);
    USaveManager::SetCameraSettings(RotationSettings.Sensitivity, bIsCameraAttachedToHead);
}

void UPlayerCameraControlComponent::AddSensitivity(float NewSensitivity)
{
    float newSensitivity = RotationSettings.Sensitivity + NewSensitivity;
    RotationSettings.Sensitivity = FMath::Clamp(newSensitivity, 0.01f, 3.f);
    USaveManager::SetCameraSettings(RotationSettings.Sensitivity, bIsCameraAttachedToHead);
}

// ============================================
// Camera Offset
// ============================================
void UPlayerCameraControlComponent::SetCameraOffset(const FVector& Offset)
{
    // ソケットからの相対オフセットを更新
    CameraLocationOffset = Offset;

    if (Camera)
    {
        Camera->SetRelativeLocation(Offset);
    }

    // ヘッドボブのベース位置も更新
    HeadBobBaseLocation = Offset;
}


FVector UPlayerCameraControlComponent::GetCameraOffset() const
{
    return CameraLocationOffset;
}

// ============================================
// Head Bob
// ============================================
void UPlayerCameraControlComponent::UpdateHeadBob(const FVector2D& MoveInput, bool IsMoving, bool IsFalling)
{
    if (!HeadBobSettings.bEnabled || !Camera)
        return;

    // 停止中またはジャンプ中はヘッドボブをリセット
    if (!IsMoving || IsFalling || MoveInput.IsNearlyZero(CameraControlConstants::InputDeadZone))
    {
        FVector CurrentRelativeLocation = Camera->GetRelativeLocation();
        FVector TargetRelativeLocation = CameraLocationOffset; // ベース位置に戻す
        FVector NewRelativeLocation = FMath::VInterpTo(
            CurrentRelativeLocation,
            TargetRelativeLocation,
            GetWorld()->GetDeltaSeconds(),
            HeadBobSettings.InterpSpeed
        );
        Camera->SetRelativeLocation(NewRelativeLocation);
        HeadBobTime = 0.0f;
        return;
    }

    // 移動速度に応じたボブ強度の計算
    float SpeedFactor = 1.0f;
    if (GetOwner())
    {
        float CurrentSpeed = GetOwner()->GetVelocity().Size();
        SpeedFactor = FMath::Clamp(CurrentSpeed / HeadBobSettings.SpeedReference, 0.0f, 1.0f);
    }

    // ボブオフセットを計算
    float BobOffset = CalculateHeadBobOffset(GetWorld()->GetDeltaSeconds(), SpeedFactor);

    // ソケット基準の相対位置にボブを適用
    FVector NewRelativeLocation = CameraLocationOffset;
    NewRelativeLocation.Z += BobOffset;

    Camera->SetRelativeLocation(NewRelativeLocation);
}

void UPlayerCameraControlComponent::SetHeadBobEnabled(bool bEnabled)
{
    HeadBobSettings.bEnabled = bEnabled;

    if (!bEnabled)
    {
        ResetHeadBob();
    }
}

void UPlayerCameraControlComponent::ResetHeadBob()
{
    if (!Camera)
        return;

    HeadBobTime = 0.0f;
    Camera->SetRelativeLocation(CameraLocationOffset); // ソケット基準の位置に戻す
}

void UPlayerCameraControlComponent::SetCameraAttachedToHead(bool bAttached)
{
    bIsCameraAttachedToHead = bAttached;

    // ベース位置を更新
    UpdateBaseCameraLocation();

    if (bAttached)
    {
        UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: FPS mode - Head attached (with motion)"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: FPS mode - Root attached (stable, no shake)"));
    }
}

float UPlayerCameraControlComponent::CalculateHeadBobOffset(float DeltaTime, float SpeedFactor)
{
    HeadBobTime += DeltaTime;

    float BobAmount = SpeedFactor * HeadBobSettings.Amplitude;
    float BobOffset = FMath::Sin(HeadBobTime * HeadBobSettings.Frequency) * BobAmount;

    return BobOffset;
}

FVector UPlayerCameraControlComponent::GetHeadBobBaseLocation() const
{
    // ソケット基準のベース位置を返す
    return CameraLocationOffset;
}

// ============================================
// Camera Shake
// ============================================
void UPlayerCameraControlComponent::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale,
    ECameraShakePlaySpace PlaySpace, FRotator UserPlaySpaceRot)
{
    if (!ShakeClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCameraControl: ShakeClass is null"));
        return;
    }

    // PlayerControllerを取得
    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCameraControl: PlayerController not found"));
        return;
    }

    // カメラシェイクを再生
    PC->ClientStartCameraShake(ShakeClass, Scale, PlaySpace, UserPlaySpaceRot);
}

void UPlayerCameraControlComponent::StopAllCameraShakes(bool bImmediately)
{
    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (!PC)
    {
        return;
    }

    PC->ClientStopCameraShake(nullptr, bImmediately);
}

void UPlayerCameraControlComponent::StopCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, bool bImmediately)
{
    if (!ShakeClass)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (!PC)
    {
        return;
    }

    PC->ClientStopCameraShake(ShakeClass, bImmediately);
}

void UPlayerCameraControlComponent::PlayLightShake()
{
    if (LightShakeClass)
    {
        PlayCameraShake(LightShakeClass);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCameraControl: LightShakeClass not set"));
    }
}

void UPlayerCameraControlComponent::PlayMediumShake()
{
    if (MediumShakeClass)
    {
        PlayCameraShake(MediumShakeClass);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCameraControl: MediumShakeClass not set"));
    }
}

void UPlayerCameraControlComponent::PlayHeavyShake()
{
    if (HeavyShakeClass)
    {
        PlayCameraShake(HeavyShakeClass);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCameraControl: HeavyShakeClass not set"));
    }
}

void UPlayerCameraControlComponent::PlayCustomShake(float Duration, float Amplitude, float Frequency)
{
    // 簡易的なカメラシェイク（C++で動的生成）
    // 注: 実際の実装では、カスタムシェイククラスを動的に生成する必要があります
    UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: Custom shake (Duration: %.2f, Amplitude: %.2f, Frequency: %.2f)"),
        Duration, Amplitude, Frequency);

    // フォールバック: MediumShakeを使用
    if (MediumShakeClass)
    {
        float Scale = Amplitude / 5.0f;  // スケールに変換
        PlayCameraShake(MediumShakeClass, Scale);
    }
}

// ============================================
// FOV Control
// ============================================
void UPlayerCameraControlComponent::SetFOV(float NewFOV, bool bInstant)
{
    TargetFOV = FMath::Clamp(NewFOV, 60.0f, 120.0f);

    if (bInstant && Camera)
    {
        CurrentFOV = TargetFOV;
        Camera->SetFieldOfView(CurrentFOV);
    }
}

void UPlayerCameraControlComponent::ResetFOV(bool bInstant)
{
    SetFOV(FOVSettings.DefaultFOV, bInstant);
}

void UPlayerCameraControlComponent::UpdateFOV(float DeltaTime)
{
    if (!Camera)
        return;

    // FOVを補間
    CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, FOVSettings.InterpSpeed);
    Camera->SetFieldOfView(CurrentFOV);
}

void UPlayerCameraControlComponent::UpdateBaseCameraLocation()
{
    if (!GetOwner()) return;

    // 基準位置を記録（ヘッドボブ計算用）
    HeadBobBaseLocation = GetRelativeLocation();
}