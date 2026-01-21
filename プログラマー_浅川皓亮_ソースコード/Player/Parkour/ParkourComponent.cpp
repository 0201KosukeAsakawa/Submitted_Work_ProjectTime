// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ParkourComponent.h"
#include "Components/CapsuleComponent.h"

#include "Animation/AnimMontage.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/World.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

using namespace UE5Coro;
using namespace UE5Coro::Latent;

// ============================================
// Constructor
// ============================================
UParkourComponent::UParkourComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// ============================================
// Lifecycle
// ============================================

// 処理の流れ:
// 1. キャラクター参照をキャッシュ
// 2. 各コンポーネントをキャッシュ
// 3. ライントレース用の設定を準備
void UParkourComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeComponent();
}

// ============================================
// Initialization
// ============================================

// 処理の流れ:
// 1. Characterを取得してキャッシュ
// 2. Capsule、Movement、AnimInstanceをキャッシュ
// 3. ライントレース用ObjectTypesを準備
void UParkourComponent::InitializeComponent()
{
    CachedCharacter = Cast<ACharacter>(GetOwner());

    if (!CachedCharacter.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("ParkourComponent: Owner is not ACharacter"));
        return;
    }

    CachedCapsule = CachedCharacter->GetCapsuleComponent();
    CachedMovement = CachedCharacter->GetCharacterMovement();

    if (USkeletalMeshComponent* Mesh = CachedCharacter->GetMesh())
    {
        CachedAnimInstance = Mesh->GetAnimInstance();
    }

    // ライントレース用ObjectTypesをキャッシュ
    TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
}

// ============================================
// Public API
// ============================================

// 処理の流れ:
// 1. すでに実行中なら何もしない
// 2. パルクールシーケンスを開始
bool UParkourComponent::Parkour()
{


    // ===== 壁検出 =====
    if (bIsPerformingParkour ||
        !DetectWallImpact(CurrentWallInfo) ||
        !DetectWallTop(CurrentWallInfo))
    {
        return false;
    }
    ParkourSequence();
    return true;
}

// ============================================
// Coroutines
// ============================================

// 処理の流れ:
// 1. 初期化と入力無効化
// 2. 壁を検出
// 3. 適切なアクション（Climb/Vault）を実行
// 4. 終了処理
TCoroutine<bool> UParkourComponent::ParkourSequence()
{
    if (!CachedCharacter.IsValid())
    {
        co_return false;
    }

    bIsPerformingParkour = true;
    OnParkourStarted.Broadcast();
    DisableCharacterInput();
    UE_LOG(LogTemp, Log, TEXT("Parkour: Started"));

    CalculateWallProperties(CurrentWallInfo);
    CurrentWallInfo.bHasInnerSurface = DetectWallThickness(CurrentWallInfo);

    // ===== アクション実行 =====
    if (CurrentWallInfo.bHasInnerSurface)
    {
        if (CanPerformClimb())
        {
            co_await ExecuteClimbSequence();
        }
        else
        {
            co_await ExecuteVaultSequence();
        }
    }
    else
    {
        CurrentWallInfo.bIsThickWall = false;
        co_await ExecuteVaultSequence();
    }

    co_await CleanupParkour();
    co_return true;
}

// 処理の流れ:
// 1. 物理・コリジョンを無効化
// 2. キャラクターを壁の上に移動
// 3. Climbモンタージュを再生
// 4. 厚い壁の場合、GettingUpモンタージュも再生
TCoroutine<> UParkourComponent::ExecuteClimbSequence()
{
    if (!CachedCharacter.IsValid() || CurrentWallInfo.bRequiresClimbing)
    {
        co_return;
    }

    DisablePhysicsAndCollision();

    const FVector OwnerLocation = CachedCharacter->GetActorLocation();
    const FVector TargetLocation(OwnerLocation.X, OwnerLocation.Y, CurrentWallInfo.TopLocation.Z + 20.0f);
    CachedCharacter->SetActorLocation(TargetLocation);

    co_await PlayMontageAsync(EParkourMontageType::Climb);

    if (CurrentWallInfo.bIsThickWall)
    {
        co_await PlayMontageAsync(EParkourMontageType::GettingUp);
    }
}

// 処理の流れ:
// 1. 登りが必要な高さかチェック
// 2. 物理・コリジョンを無効化
// 3. 厚い壁 or 薄い壁でターゲット位置とモンタージュを決定
// 4. キャラクターを移動してモンタージュを再生
TCoroutine<> UParkourComponent::ExecuteVaultSequence()
{
    if (!CachedCharacter.IsValid() || CurrentWallInfo.bRequiresClimbing)
    {
        co_return;
    }

    DisablePhysicsAndCollision();

    FVector TargetLocation;
    EParkourMontageType MontageType;

    if (CurrentWallInfo.bIsThickWall)
    {
        const FRotator NormalRotation = UKismetMathLibrary::MakeRotFromX(CurrentWallInfo.SurfaceNormal);
        const FVector ForwardVector = UKismetMathLibrary::GetForwardVector(NormalRotation);

        TargetLocation = CachedCharacter->GetActorLocation() + (ForwardVector * 50.0f);
        MontageType = EParkourMontageType::GettingUp;
    }
    else
    {
        const FVector CurrentLocation = CachedCharacter->GetActorLocation();
        TargetLocation = FVector(CurrentLocation.X, CurrentLocation.Y, CurrentWallInfo.TopLocation.Z + 20.0f);
        MontageType = EParkourMontageType::Vault;
    }

    CachedCharacter->SetActorLocation(TargetLocation);
    co_await PlayMontageAsync(MontageType);
}

// 処理の流れ:
// 1. AnimInstanceとモンタージュの有効性確認
// 2. モンタージュを再生
// 3. モンタージュ終了まで待機
TCoroutine<> UParkourComponent::PlayMontageAsync(EParkourMontageType MontageType)
{
    if (!CachedAnimInstance.IsValid() || !AnimMontageMap.Contains(MontageType))
    {
        UE_LOG(LogTemp, Warning, TEXT("Parkour: Invalid montage type or AnimInstance"));
        co_return;
    }

    UAnimMontage* Montage = AnimMontageMap[MontageType];
    if (!Montage)
    {
        co_return;
    }

    const float Duration = CachedAnimInstance->Montage_Play(Montage, 1.0f);

    if (Duration > 0.0f)
    {
        co_await Seconds(Duration);
    }
}

// 処理の流れ:
// 1. 物理・コリジョンを有効化
// 2. 入力を有効化
// 3. フラグとイベントをリセット
TCoroutine<> UParkourComponent::CleanupParkour()
{
    EnablePhysicsAndCollision();
    EnableCharacterInput();

    bIsPerformingParkour = false;
    OnParkourEnded.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Parkour: Ended"));
    co_return;
}

// ============================================
// Detection Methods
// ============================================

// 処理の流れ:
// 1. トレース位置を計算
// 2. ライントレースを実行
// 3. ヒットしたアクターが「NoParkour」タグを持つか確認
// 4. 壁の接触位置と法線を記録
bool UParkourComponent::DetectWallImpact(FWallDetectionInfo& OutWallInfo)
{
    if (!CachedCharacter.IsValid())
    {
        return false;
    }

    const FVector OwnerLocation = CachedCharacter->GetActorLocation();
    const FVector OwnerForward = CachedCharacter->GetActorForwardVector();

    const FVector TraceStart = OwnerLocation - FVector(0, 0, CharacterCenterOffset);
    const FVector TraceEnd = TraceStart + (OwnerForward * WallDetectionDistance);

    FHitResult HitResult;
    if (!PerformLineTrace(TraceStart, TraceEnd, HitResult))
    {
        return false;
    }

    if (const AActor* HitActor = HitResult.GetActor())
    {
        if (HitActor->ActorHasTag(FName("NoParkour")))
        {
            UE_LOG(LogTemp, Warning, TEXT("Parkour: Hit actor has 'NoParkour' tag"));
            return false;
        }
    }

    OutWallInfo.ImpactLocation = HitResult.Location;
    OutWallInfo.SurfaceNormal = HitResult.Normal;

    return true;
}

// 処理の流れ:
// 1. 法線から回転を計算
// 2. トレース位置を計算（上方向に検出）
// 3. ライントレースを実行
// 4. 壁の上端位置を記録
bool UParkourComponent::DetectWallTop(FWallDetectionInfo& InOutWallInfo)
{
    const FRotator NormalRotation = UKismetMathLibrary::MakeRotFromX(InOutWallInfo.SurfaceNormal);
    const FVector NormalForward = UKismetMathLibrary::GetForwardVector(NormalRotation);

    const FVector TraceStart = InOutWallInfo.ImpactLocation + (NormalForward * -10.0f);
    const FVector TraceEnd = TraceStart + FVector(0, 0, MaxDetectionHeight);

    FHitResult HitResult;
    if (!PerformLineTrace(TraceEnd, TraceStart, HitResult))
    {
        return false;
    }

    InOutWallInfo.TopLocation = HitResult.Location;
    return true;
}

// 処理の流れ:
// 1. 法線から回転を計算
// 2. 内側表面のトレース位置を計算
// 3. ライントレースを実行
// 4. 内側の上端位置を記録
// 5. 壁の厚さを計算
bool UParkourComponent::DetectWallThickness(FWallDetectionInfo& InOutWallInfo)
{
    const FRotator NormalRotation = UKismetMathLibrary::MakeRotFromX(InOutWallInfo.SurfaceNormal);
    const FVector NormalForward = UKismetMathLibrary::GetForwardVector(NormalRotation);

    const FVector TraceStart = InOutWallInfo.ImpactLocation + (NormalForward * -50.0f);
    const FVector TraceEnd = TraceStart + FVector(0, 0, MaxDetectionHeight);

    FHitResult HitResult;
    if (!PerformLineTrace(TraceEnd, TraceStart, HitResult))
    {
        return false;
    }

    InOutWallInfo.InnerTopLocation = HitResult.Location;

    const float Thickness = InOutWallInfo.TopLocation.Z - InOutWallInfo.InnerTopLocation.Z;
    InOutWallInfo.bIsThickWall = Thickness < ThicknessThreshold;

    return true;
}

// 処理の流れ:
// 1. 壁の高さを計算
// 2. 高さが閾値を超える場合、登りが必要と判定
void UParkourComponent::CalculateWallProperties(FWallDetectionInfo& InOutWallInfo)
{
    InOutWallInfo.Height = InOutWallInfo.TopLocation.Z - InOutWallInfo.ImpactLocation.Z;
    InOutWallInfo.bRequiresClimbing = InOutWallInfo.Height > ClimbHeightThreshold;
}

// 処理の流れ:
// 1. トレース位置を計算（200cm上方から前方）
// 2. ライントレースを実行
// 3. 前方に障害物がない場合のみ登れる
bool UParkourComponent::CanPerformClimb() const
{
    if (!CachedCharacter.IsValid())
    {
        return false;
    }

    const FVector OwnerLocation = CachedCharacter->GetActorLocation();
    const FVector OwnerForward = CachedCharacter->GetActorForwardVector();

    const FVector TraceStart = OwnerLocation + FVector(0, 0, 200.0f);
    const FVector TraceEnd = TraceStart + (OwnerForward * WallDetectionDistance);

    FHitResult HitResult;
    return !PerformLineTrace(TraceStart, TraceEnd, HitResult);
}

// ============================================
// Physics & Input Control
// ============================================

// 処理の流れ:
// 1. Capsuleのコリジョンを無効化
// 2. MovementComponentの速度と力をリセット
// 3. 落下モードに設定
// 4. 重力を無効化
void UParkourComponent::DisablePhysicsAndCollision()
{
    if (CachedCapsule.IsValid())
    {
        CachedCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (CachedMovement.IsValid())
    {
        CachedMovement->StopMovementImmediately();
        CachedMovement->Velocity = FVector::ZeroVector;
        CachedMovement->SetMovementMode(EMovementMode::MOVE_Falling);
        CachedMovement->GravityScale = 0.0f;
    }

    UE_LOG(LogTemp, Log, TEXT("Parkour: Physics disabled"));
}

// 処理の流れ:
// 1. Capsuleのコリジョンを有効化
// 2. 歩行モードに設定
// 3. 重力を通常値に戻す
void UParkourComponent::EnablePhysicsAndCollision()
{
    if (CachedCapsule.IsValid())
    {
        CachedCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    if (CachedMovement.IsValid())
    {
        CachedMovement->SetMovementMode(EMovementMode::MOVE_Walking);
        CachedMovement->GravityScale = NormalGravityScale;
    }
}

// 処理の流れ:
// 1. PlayerControllerを取得
// 2. 入力を無効化
void UParkourComponent::DisableCharacterInput()
{
    if (!CachedCharacter.IsValid())
    {
        return;
    }

    if (APlayerController* PC = Cast<APlayerController>(CachedCharacter->GetController()))
    {
        CachedCharacter->DisableInput(PC);
        UE_LOG(LogTemp, Log, TEXT("Parkour: Input disabled"));
    }
}

// 処理の流れ:
// 1. PlayerControllerを取得
// 2. 入力を有効化
void UParkourComponent::EnableCharacterInput()
{
    if (!CachedCharacter.IsValid())
    {
        return;
    }

    if (APlayerController* PC = Cast<APlayerController>(CachedCharacter->GetController()))
    {
        CachedCharacter->EnableInput(PC);
        UE_LOG(LogTemp, Log, TEXT("Parkour: Input enabled"));
    }
}

// ============================================
// Helper Methods
// ============================================

// 処理の流れ:
// 1. キャッシュされたObjectTypesを使用
// 2. ライントレースを実行
// 3. ヒット結果を返す
bool UParkourComponent::PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHitResult) const
{
    constexpr bool bTraceComplex = false;
    constexpr bool bIgnoreSelf = true;
    constexpr EDrawDebugTrace::Type DrawDebugType = EDrawDebugTrace::None;

    TArray<AActor*> ActorsToIgnore;

    return UKismetSystemLibrary::LineTraceSingleForObjects(
        GetWorld(),
        Start,
        End,
        TraceObjectTypes,
        bTraceComplex,
        ActorsToIgnore,
        DrawDebugType,
        OutHitResult,
        bIgnoreSelf
    );
}