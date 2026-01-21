// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/WallRun/WallRunComponent.h"
#include "Component/WallRun/WallDetectionComponent.h"
#include "Component/WallRun/WallRunLogicComponent.h"
#include "Component/PlayerCameraControlComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"

#include "Camera/CameraComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Interface/PlayerInfoProvider.h"

namespace WallRun
{
    // 壁走り関連定数
    constexpr float WALL_RUN_GRAVITY_SCALE = 0.0f;
    constexpr float DEFAULT_GRAVITY_SCALE = 5.0f;
    constexpr float WALL_RUN_SPEED = 5000.0f;
    constexpr float CAMERA_ROLL_RIGHT = -25.0f;
    constexpr float CAMERA_ROLL_LEFT = 25.0f;

    // 壁ジャンプ関連定数
    constexpr float WALL_JUMP_FORWARD_MULTIPLIER = 1.2f;
    constexpr float WALL_JUMP_UP_MULTIPLIER = 1.0f;
    constexpr float WALL_JUMP_NORMAL_MULTIPLIER = 0.6f;
    constexpr float WALL_JUMP_POWER = 4000.0f;
    constexpr float WALL_JUMP_RESET_DELAY = 0.5f;
}

UWallRunComponent::UWallRunComponent()
    : SavedMaxWalkSpeed(600.0f)
    ,SavedAirControl(0.05f)
    ,SavedGravityScale(1.0f)
    , bAutoBindInput(true)
    , JumpActionName(TEXT("Jump"))
{
    PrimaryComponentTick.bCanEverTick = true;

    // サブコンポーネント生成
    WallDetector = CreateDefaultSubobject<UWallDetectionComponent>(TEXT("WallDetector"));
    WallDetector->SetupAttachment(this);

    Logic = CreateDefaultSubobject<UWallRunLogicComponent>(TEXT("Logic"));
}

void UWallRunComponent::BeginPlay()
{
    Super::BeginPlay();

    // 初期化
    InitializeComponent();

    // イベントバインド
    WallDetector->OnWallDetected.AddUObject(this, &UWallRunComponent::HandleWallDetected);
    WallDetector->OnWallLost.AddUObject(this, &UWallRunComponent::HandleWallLost);

   
}

void UWallRunComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (Logic->IsWallRunning())
    {
        ACharacter* Character = Cast<ACharacter>(GetOwner());
        if (!Character)
            return;

        UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
        if (!MoveComp)
            return;

        // 移動入力があるかチェック
        bool bHasMovementInput = !MoveComp->GetLastInputVector().IsNearlyZero();

        // ★ 壁走り開始時の方向を使って壁の存在をチェック
        FVector CharacterLocation = Character->GetActorLocation();

        // カメラの向きを取得（視線チェック用）
        UCameraComponent* Camera = PlayerInfoProvider ? PlayerInfoProvider->GetCamera() : nullptr;
        FVector CameraForward = Camera ? Camera->GetForwardVector() : FVector::ZeroVector;

        // 継続できるか判定
        if (!Logic->CanContinueWallRun(MoveComp, bHasMovementInput, true))
        {
            ExitWallRun();
            return;
        }

        // 壁走り中の速度維持
        UpdateWallRunMovement();

        // デバッグ描画
        //DrawDebugWallRun();
    }
}
// ======================
// 初期化
// ======================

void UWallRunComponent::InitializeComponent()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("WallRunComponent: Owner is not ACharacter"));
        return;
    }

    if (!GetOwner()->GetClass()->ImplementsInterface(UPlayerInfoProvider::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("WallRunComponent: Owner doesn't implement IPlayerInfoProvider"));
        return;
    }

    PlayerInfoProvider.SetInterface(Cast<IPlayerInfoProvider>(GetOwner()));
    PlayerInfoProvider.SetObject(GetOwner());

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->SetPlaneConstraintEnabled(true);
        MoveComp->GravityScale = 5.0f;
    }
}

void UWallRunComponent::ExitWallRun()
{
    // 壁走り終了
    if (Logic != nullptr)
    {
        Logic->ExitWallRun();
    }
    if (WallDetector != nullptr)
    {
        WallDetector->SetWallDirection(FVector::Zero());
    }
    ResetMovement();
    ResetCamera();
}

// ======================
// イベントハンドラ
// ======================

void UWallRunComponent::HandleWallDetected(AActor* Wall, const FHitResult& Hit)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
        return;

    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp)
        return;

    // カメラの向きを取得
    UCameraComponent* Camera = PlayerInfoProvider ? PlayerInfoProvider->GetCamera() : nullptr;
    if (!Camera)
    {
        UE_LOG(LogTemp, Warning, TEXT("WallRun: Camera not found"));
        return;
    }

    FVector CameraForward = Camera->GetForwardVector();

    // ✅ ヒット結果の法線を使用
    FVector WallNormal = Hit.ImpactNormal;

    UE_LOG(LogTemp, Log, TEXT("WallRun: Wall detected - Normal=%s, Impact=%s"),
        *WallNormal.ToString(), *Hit.ImpactPoint.ToString());

    // 移動入力があるかチェック
    bool bHasMovementInput = !MoveComp->GetLastInputVector().IsNearlyZero();

    // 壁走り開始条件をチェック
    if (!Logic->CanStartWallRun(MoveComp, bHasMovementInput))
    {
        UE_LOG(LogTemp, Log, TEXT("WallRun: Conditions not met"));
        return;
    }

    // ✅ 壁の法線とヒット結果を渡す
    BeginWallRun(Hit);
}

void UWallRunComponent::HandleWallLost(AActor* Wall)
{
    if (Logic->IsWallRunning())
    {
        UE_LOG(LogTemp, Warning, TEXT("Wall Lost - Ending WallRun"));

        Logic->ExitWallRun();
        ResetMovement();
        ResetCamera();
    }
}

bool UWallRunComponent::HandleJumpPressed()
{
    // 壁ジャンプ可能なら実行
    if (Logic->CanWallJump())
    {
        ExecuteWallJump();
        ExitWallRun();
        return true;
    }

    return false;
}

void UWallRunComponent::SetDetectionEnabled(bool bEnabled)
{
    if (WallDetector != nullptr)
        WallDetector->SetDetectionEnabled(bEnabled);
}

// ======================
// 壁走り処理
// ======================

void UWallRunComponent::BeginWallRun(const FHitResult& WallHit)
{
    if (!PlayerInfoProvider)
        return;

    UCameraComponent* Camera = PlayerInfoProvider->GetCamera();
    ACharacter* Character = Cast<ACharacter>(GetOwner());

    if (!Camera || !Character)
        return;

    // ✅ ヒット結果の法線を使用して計算
    FWallRunData Data = Logic->CalculateWallRunData(
        WallHit.ImpactNormal,  // ← 壁の法線
        Camera->GetForwardVector(),
        Character->GetActorUpVector()
    );

    // 状態更新
    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    float CurrentGravity = MoveComp ? MoveComp->GravityScale : 1.0f;

    Logic->EnterWallRun(Data, CurrentGravity);


    FVector WallDirection;
    // ★ 壁方向を保存（壁の法線の逆方向 = 壁に向かう方向）
    WallDirection = -Data.WallNormal;
    WallDirection.Z = 0.0f;  // 水平方向のみ
    WallDirection.Normalize();

    WallDetector->SetWallDirection(WallDirection);
    // 適用
    ApplyWallRunMovement(Data);
    ApplyWallRunCamera(Data);

    UE_LOG(LogTemp, Warning, TEXT("WallRun Started: Normal=%s, MoveDir=%s"),
        *WallHit.ImpactNormal.ToString(), *Data.MoveDirection.ToString());
}

void UWallRunComponent::UpdateWallRunMovement()
{
    if (!Logic->IsWallRunning())
        return;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
        return;

    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp)
        return;

    // ✅ 壁走り方向を取得
    FVector MoveDir = Logic->GetMoveDirection();

    // 正規化して確認
    if (MoveDir.IsNearlyZero())
    {
        UE_LOG(LogTemp, Error, TEXT("WallRun: Move direction is zero!"));
        return;
    }

    MoveDir.Normalize();

    // Gravityを0に固定
    if (MoveComp->GravityScale != 0.0f)
    {
        MoveComp->GravityScale = 0.0f;
    }

    // 垂直方向の速度を0に保つ
    FVector CurrentVel = MoveComp->Velocity;
    CurrentVel.Z = 0.0f;

    // ✅ 直接速度を設定
    const FWallRunSettings& Settings = Logic->GetSettings();
    FVector NewVelocity = MoveDir * Settings.Speed;
    NewVelocity.Z = 0.0f;  // 念のため

    MoveComp->Velocity = NewVelocity;

    UE_LOG(LogTemp, Verbose, TEXT("WallRun: Dir=%s, Vel=%s, Speed=%.2f"),
        *MoveDir.ToString(), *MoveComp->Velocity.ToString(), MoveComp->Velocity.Size2D());
}


void UWallRunComponent::ExecuteWallJump()
{
    if (!PlayerInfoProvider)
        return;

    UCameraComponent* Camera = PlayerInfoProvider->GetCamera();
    ACharacter* Character = Cast<ACharacter>(GetOwner());

    if (!Camera || !Character)
        return;

    // 計算
    FVector JumpDirection = Logic->CalculateWallJumpDirection(
        Camera->GetForwardVector(),
        Character->GetActorUpVector()
    );
    FVector LaunchVelocity = Logic->CalculateWallJumpVelocity(JumpDirection);

    // 状態変更
    Logic->ConsumeWallJump();

    // 実行
    Character->LaunchCharacter(LaunchVelocity, true, true);
    ResetMovement();
}

// ======================
// 適用処理
// ======================

void UWallRunComponent::ApplyWallRunMovement(const FWallRunData& Data)
{
    if (!PlayerInfoProvider)
        return;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
        return;

    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp)
        return;

    const FWallRunSettings& Settings = Logic->GetSettings();

    // 元の設定を保存
    SavedMaxWalkSpeed = MoveComp->MaxWalkSpeed;
    SavedAirControl = MoveComp->AirControl;
    SavedGravityScale = MoveComp->GravityScale;

    // 壁走り用設定を適用
    MoveComp->MaxWalkSpeed = Settings.Speed;
    MoveComp->AirControl = 1.0f;
    MoveComp->GravityScale = Settings.GravityScale;

    FVector CurrentVelocity = MoveComp->Velocity;

    // 壁走り方向の速度のみを設定（垂直方向はゼロに）
    FVector NewVelocity = Data.MoveDirection * WallRun::WALL_RUN_SPEED;
    NewVelocity.Z = 0.0f;  // 垂直方向の速度をゼロに

    UE_LOG(LogTemp, Warning, TEXT("WallRun Applied: Speed=%f, AirControl=%f, Gravity=%f"),
        MoveComp->MaxWalkSpeed, MoveComp->AirControl, MoveComp->GravityScale);
}

void UWallRunComponent::ApplyWallRunCamera(const FWallRunData& Data)
{
    if (!PlayerInfoProvider)
        return;

    // ✅ カメラコントロールにロール値を渡すだけ
    //    実際の適用はコントロール側の責任
    UPlayerCameraControlComponent* CameraControl = PlayerInfoProvider->GetCameraControl();
    if (CameraControl)
    {
        CameraControl->SetCameraRoll(Data.CameraRoll);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WallRun: CameraControl not found"));
    }
}

void UWallRunComponent::ResetMovement()
{
    if (!PlayerInfoProvider)
        return;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
        return;

    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp)
        return;

    // 保存していた設定を復元
    MoveComp->MaxWalkSpeed = SavedMaxWalkSpeed;
    MoveComp->AirControl = SavedAirControl;
    MoveComp->GravityScale = SavedGravityScale;

    // 平面拘束をクリア
    MoveComp->SetPlaneConstraintNormal(FVector::ZeroVector);

    UE_LOG(LogTemp, Warning, TEXT("WallRun Reset: Speed=%f, AirControl=%f, Gravity=%f"),
        MoveComp->MaxWalkSpeed, MoveComp->AirControl, MoveComp->GravityScale);
}

void UWallRunComponent::ResetCamera()
{
    if (!PlayerInfoProvider)
        return;

    // ✅ カメラコントロールにリセットを依頼するだけ
    UPlayerCameraControlComponent* CameraControl = PlayerInfoProvider->GetCameraControl();
    if (CameraControl)
    {
        CameraControl->ResetCameraRoll();
    }
}




// ======================
// 公開API
// ======================

bool UWallRunComponent::IsWallRunning() const
{
    return Logic ? Logic->IsWallRunning() : false;
}

bool UWallRunComponent::GetCanWallJump() const
{
    return Logic ? Logic->CanWallJump() : false;
}

FVector UWallRunComponent::GetWallNormal() const
{
    return Logic ? Logic->GetWallNormal() : FVector::ZeroVector;
}
// ============================================
// デバッグ用のビジュアル化
// ============================================


void UWallRunComponent::DrawDebugWallRun()
{
    if (!GetWorld())
        return;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
        return;

    FVector CharacterLoc = Character->GetActorLocation();
    FVector WallNormal = Logic->GetWallNormal();
    FVector MoveDir = Logic->GetMoveDirection();

    // 壁の法線（赤）
    DrawDebugLine(GetWorld(), CharacterLoc, CharacterLoc + WallNormal * 200.0f,
        FColor::Red, false, -1.0f, 0, 5.0f);

    // 進行方向（緑）
    DrawDebugLine(GetWorld(), CharacterLoc, CharacterLoc + MoveDir * 200.0f,
        FColor::Green, false, -1.0f, 0, 5.0f);

    // 現在の速度（青）
    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (MoveComp)
    {
        FVector Velocity = MoveComp->Velocity;
        Velocity.Z = 0.0f;
        DrawDebugLine(GetWorld(), CharacterLoc, CharacterLoc + Velocity,
            FColor::Blue, false, -1.0f, 0, 3.0f);
    }

    // カメラ方向（黄色）
    if (PlayerInfoProvider)
    {
        UCameraComponent* Camera = PlayerInfoProvider->GetCamera();
        if (Camera)
        {
            FVector CameraForward = Camera->GetForwardVector();
            CameraForward.Z = 0.0f;
            CameraForward.Normalize();
            DrawDebugLine(GetWorld(), CharacterLoc, CharacterLoc + CameraForward * 200.0f,
                FColor::Yellow, false, -1.0f, 0, 3.0f);
        }
    }
}