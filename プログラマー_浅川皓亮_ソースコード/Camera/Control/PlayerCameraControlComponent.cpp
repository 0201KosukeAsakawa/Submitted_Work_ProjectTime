// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerCameraControlComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

#include "SaveManager.h"

namespace CameraControlConstants
{
    constexpr float InputDeadZone = 0.2f;
    constexpr float DefaultRoll = 0.0f;
}

// 処理の流れ:
// 1. メンバ変数を初期化
// 2. Tickを有効化
// 3. カメラコンポーネントを作成
// 4. カメラをこのコンポーネントにアタッチ
// 5. PawnControlRotationを設定
UPlayerCameraControlComponent::UPlayerCameraControlComponent()
    : bUsePawnControlRotation(true)
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

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
    if (Camera)
    {
        Camera->SetupAttachment(this);
        Camera->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
        Camera->bUsePawnControlRotation = bUsePawnControlRotation;

        UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: Camera created at offset: %s"), *CameraLocationOffset.ToString());
    }
}

// 処理の流れ:
// 1. カメラが正しくアタッチされているか確認
// 2. 必要なら再アタッチ
// 3. コンポーネントを初期化
// 4. セーブマネージャーから感度を読み込み
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

// 処理の流れ:
// 1. カメラのロールを更新
// 2. FOVを更新
void UPlayerCameraControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateCameraRoll(DeltaTime);
    UpdateFOV(DeltaTime);
}

// 処理の流れ:
// 1. カメラの有効性確認
// 2. ソケット基準の相対位置を取得
// 3. PawnControlRotationを設定
// 4. FOVを初期化
// 5. CharacterMovementComponentをキャッシュ
// 6. ヘッドボブのベース位置を設定
void UPlayerCameraControlComponent::InitializeComponent()
{
    if (!Camera)
    {
        UE_LOG(LogTemp, Error, TEXT("PlayerCameraControl: Camera component is null"));
        return;
    }

    CameraLocationOffset = GetRelativeLocation();

    Camera->bUsePawnControlRotation = bUsePawnControlRotation;

    CurrentFOV = FOVSettings.DefaultFOV;
    TargetFOV = CurrentFOV;
    Camera->SetFieldOfView(CurrentFOV);

    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        CachedMovement = Character->GetCharacterMovement();
    }

    HeadBobBaseLocation = CameraLocationOffset;

    UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: Initialized on socket (Offset: %s, FOV: %.1f)"),
        *CameraLocationOffset.ToString(), CurrentFOV);
}

// 処理の流れ:
// 1. Ownerの有効性確認
// 2. 入力値の型チェック
// 3. 2D入力値を取得
// 4. デッドゾーン処理
// 5. 感度を適用
// 6. ピッチ反転処理
// 7. PlayerControllerに回転入力を適用
void UPlayerCameraControlComponent::ProcessLookInput(const FInputActionValue& Value)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessLookInput: Owner is null."));
        return;
    }

    if (Value.GetValueType() != EInputActionValueType::Axis2D)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessLookInput: Invalid input value type."));
        return;
    }

    FVector2D LookInput = Value.Get<FVector2D>();

    const float InputMagnitude = LookInput.Size();
    if (InputMagnitude < CameraControlConstants::InputDeadZone)
        return;

    LookInput *= RotationSettings.Sensitivity;

    if (bPitchInverted)
    {
        LookInput.Y *= -1.0f;
    }

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

// 処理の流れ:
// 1. カメラの有効性確認
// 2. 現在のロールをターゲットに向けて補間
// 3. PlayerControllerのControlRotationに直接Rollを設定
void UPlayerCameraControlComponent::UpdateCameraRoll(float DeltaTime)
{
    if (!Camera)
        return;

    CurrentRoll = FMath::FInterpTo(CurrentRoll, TargetRoll, DeltaTime, RotationSettings.RollInterpSpeed);

    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (PC)
    {
        FRotator ControlRotation = PC->GetControlRotation();
        ControlRotation.Roll = CurrentRoll;
        PC->SetControlRotation(ControlRotation);
    }
}

// 処理の流れ:
// 1. 感度を0.01〜3.0の範囲にクランプ
// 2. SaveManagerに設定を保存
void UPlayerCameraControlComponent::SetSensitivity(float NewSensitivity)
{
    RotationSettings.Sensitivity = FMath::Clamp(NewSensitivity, 0.01f, 3.f);
    USaveManager::SetCameraSettings(RotationSettings.Sensitivity, bIsCameraAttachedToHead);
}

// 処理の流れ:
// 1. 現在の感度に値を加算
// 2. 0.01〜3.0の範囲にクランプ
// 3. SaveManagerに設定を保存
void UPlayerCameraControlComponent::AddSensitivity(float NewSensitivity)
{
    float NewValue = RotationSettings.Sensitivity + NewSensitivity;
    RotationSettings.Sensitivity = FMath::Clamp(NewValue, 0.01f, 3.f);
    USaveManager::SetCameraSettings(RotationSettings.Sensitivity, bIsCameraAttachedToHead);
}

// 処理の流れ:
// 1. カメラオフセットを更新
// 2. カメラの相対位置を設定
// 3. ヘッドボブのベース位置を更新
void UPlayerCameraControlComponent::SetCameraOffset(const FVector& Offset)
{
    CameraLocationOffset = Offset;

    if (Camera)
    {
        Camera->SetRelativeLocation(Offset);
    }

    HeadBobBaseLocation = Offset;
}

FVector UPlayerCameraControlComponent::GetCameraOffset() const
{
    return CameraLocationOffset;
}

// 処理の流れ:
// 1. ヘッドボブが有効か、カメラが存在するか確認
// 2. 停止中またはジャンプ中はヘッドボブをリセット
// 3. 移動速度に応じたボブ強度を計算
// 4. ボブオフセットを計算
// 5. ソケット基準の相対位置にボブを適用
void UPlayerCameraControlComponent::UpdateHeadBob(const FVector2D& MoveInput, bool IsMoving, bool IsFalling)
{
    if (!HeadBobSettings.bEnabled || !Camera)
        return;

    if (!IsMoving || IsFalling || MoveInput.IsNearlyZero(CameraControlConstants::InputDeadZone))
    {
        FVector CurrentRelativeLocation = Camera->GetRelativeLocation();
        FVector TargetRelativeLocation = CameraLocationOffset;
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

    float SpeedFactor = 1.0f;
    if (GetOwner())
    {
        float CurrentSpeed = GetOwner()->GetVelocity().Size();
        SpeedFactor = FMath::Clamp(CurrentSpeed / HeadBobSettings.SpeedReference, 0.0f, 1.0f);
    }

    float BobOffset = CalculateHeadBobOffset(GetWorld()->GetDeltaSeconds(), SpeedFactor);

    FVector NewRelativeLocation = CameraLocationOffset;
    NewRelativeLocation.Z += BobOffset;

    Camera->SetRelativeLocation(NewRelativeLocation);
}

// 処理の流れ:
// 1. ヘッドボブの有効/無効を設定
// 2. 無効にした場合はヘッドボブをリセット
void UPlayerCameraControlComponent::SetHeadBobEnabled(bool bEnabled)
{
    HeadBobSettings.bEnabled = bEnabled;

    if (!bEnabled)
    {
        ResetHeadBob();
    }
}

// 処理の流れ:
// 1. カメラの有効性確認
// 2. ヘッドボブタイマーをリセット
// 3. カメラをソケット基準の位置に戻す
void UPlayerCameraControlComponent::ResetHeadBob()
{
    if (!Camera)
        return;

    HeadBobTime = 0.0f;
    Camera->SetRelativeLocation(CameraLocationOffset);
}

// 処理の流れ:
// 1. カメラアタッチフラグを設定
// 2. ベース位置を更新
// 3. ログ出力
void UPlayerCameraControlComponent::SetCameraAttachedToHead(bool bAttached)
{
    bIsCameraAttachedToHead = bAttached;

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

// 処理の流れ:
// 1. 時間を加算
// 2. ボブ量を計算（速度係数 × 振幅）
// 3. サイン波でオフセットを計算
float UPlayerCameraControlComponent::CalculateHeadBobOffset(float DeltaTime, float SpeedFactor)
{
    HeadBobTime += DeltaTime;

    float BobAmount = SpeedFactor * HeadBobSettings.Amplitude;
    float BobOffset = FMath::Sin(HeadBobTime * HeadBobSettings.Frequency) * BobAmount;

    return BobOffset;
}

FVector UPlayerCameraControlComponent::GetHeadBobBaseLocation() const
{
    return CameraLocationOffset;
}

// 処理の流れ:
// 1. ShakeClassの有効性確認
// 2. PlayerControllerを取得
// 3. カメラシェイクを再生
void UPlayerCameraControlComponent::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale,
    ECameraShakePlaySpace PlaySpace, FRotator UserPlaySpaceRot)
{
    if (!ShakeClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCameraControl: ShakeClass is null"));
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerCameraControl: PlayerController not found"));
        return;
    }

    PC->ClientStartCameraShake(ShakeClass, Scale, PlaySpace, UserPlaySpaceRot);
}

// 処理の流れ:
// 1. PlayerControllerを取得
// 2. 全てのカメラシェイクを停止
void UPlayerCameraControlComponent::StopAllCameraShakes(bool bImmediately)
{
    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (!PC)
    {
        return;
    }

    PC->ClientStopCameraShake(nullptr, bImmediately);
}

// 処理の流れ:
// 1. ShakeClassの有効性確認
// 2. PlayerControllerを取得
// 3. 指定したカメラシェイクを停止
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

// 処理の流れ:
// 1. カスタムシェイクのパラメータをログ出力
// 2. フォールバックとしてMediumShakeを使用
// 3. Amplitudeをスケールに変換して再生
void UPlayerCameraControlComponent::PlayCustomShake(float Duration, float Amplitude, float Frequency)
{
    UE_LOG(LogTemp, Log, TEXT("PlayerCameraControl: Custom shake (Duration: %.2f, Amplitude: %.2f, Frequency: %.2f)"),
        Duration, Amplitude, Frequency);

    if (MediumShakeClass)
    {
        float Scale = Amplitude / 5.0f;
        PlayCameraShake(MediumShakeClass, Scale);
    }
}

// 処理の流れ:
// 1. ターゲットFOVを60〜120の範囲にクランプ
// 2. 即座に適用する場合はCurrentFOVも更新してカメラに設定
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

// 処理の流れ:
// 1. カメラの有効性確認
// 2. CurrentFOVをTargetFOVに向けて補間
// 3. カメラにFOVを設定
void UPlayerCameraControlComponent::UpdateFOV(float DeltaTime)
{
    if (!Camera)
        return;

    CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, FOVSettings.InterpSpeed);
    Camera->SetFieldOfView(CurrentFOV);
}

// 処理の流れ:
// 1. Ownerの有効性確認
// 2. 現在の相対位置をヘッドボブのベース位置として記録
void UPlayerCameraControlComponent::UpdateBaseCameraLocation()
{
    if (!GetOwner()) return;

    HeadBobBaseLocation = GetRelativeLocation();
}