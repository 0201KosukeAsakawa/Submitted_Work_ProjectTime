// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ParkourComponent.h"
#include "Components/CapsuleComponent.h"

#include "Animation/AnimMontage.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/World.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

UParkourComponent::UParkourComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UParkourComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UParkourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// === メインパルクールロジック ===

bool UParkourComponent::Parkour()
{
    if (!bCanParkour)
    {
        return false;
    }

    bCanParkour = false;
    bIsPerformingParkour = true;

    // パルクール開始イベントを発行
    OnParkourStarted.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("Parkour: Started"));

    // 入力を無効化
    DisableCharacterInput();

    // 壁への接触を検出
    if (!DetectWallImpact(CurrentWallInfo))
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    // 壁の上端を検出
    if (!DetectWallTop(CurrentWallInfo))
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    // 壁のプロパティを計算
    CalculateWallProperties(CurrentWallInfo);

    // 壁の厚さを検出
    CurrentWallInfo.bHasInnerSurface = DetectWallThickness(CurrentWallInfo);
    bool b = true;

    // 内側表面が検出された場合、両方のアクションを実行
    if (CurrentWallInfo.bHasInnerSurface)
    {
        if (!ExecuteClimb() && !ExecuteVault())
        {
            b = false;
            bIsPerformingParkour = false;
            EnableCharacterInput();
            OnParkourEnded.Broadcast();
        }
    }
    else
    {
        // 内側表面がない場合は薄い壁として扱う
        CurrentWallInfo.bIsThickWall = false;
        b = ExecuteVault();

        if (!b)
        {
            bIsPerformingParkour = false;
            EnableCharacterInput();
            OnParkourEnded.Broadcast();
        }
    }

    return b;
}

// === 壁検出メソッド ===

bool UParkourComponent::DetectWallImpact(FWallDetectionInfo& OutWallInfo)
{
    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        return false;
    }

    const FVector OwnerLocation = Character->GetActorLocation();
    const FVector OwnerForward = Character->GetActorForwardVector();

    const FVector TraceStart = OwnerLocation - FVector(0, 0, CharacterCenterOffset);
    const FVector TraceEnd = TraceStart + (OwnerForward * WallDetectionDistance);

    FHitResult HitResult;
    if (!PerformLineTrace(TraceStart, TraceEnd, HitResult))
    {
        return false;
    }

    if (AActor* HitActor = HitResult.GetActor())
    {
        if (HitActor->ActorHasTag(FName("NoParkour")))
        {
            UE_LOG(LogTemp, Warning, TEXT("Parkour: Hit actor has 'NoParkour' tag, cannot perform parkour"));
            return false;
        }
    }

    OutWallInfo.ImpactLocation = HitResult.Location;
    OutWallInfo.SurfaceNormal = HitResult.Normal;

    return true;
}

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

    // 壁の厚さを計算
    const float Thickness = InOutWallInfo.TopLocation.Z - InOutWallInfo.InnerTopLocation.Z;
    InOutWallInfo.bIsThickWall = Thickness < ThicknessThreshold;

    return true;
}

void UParkourComponent::CalculateWallProperties(FWallDetectionInfo& InOutWallInfo)
{
    InOutWallInfo.Height = InOutWallInfo.TopLocation.Z - InOutWallInfo.ImpactLocation.Z;
    InOutWallInfo.bRequiresClimbing = InOutWallInfo.Height > ClimbHeightThreshold;
}

// === アクション実行メソッド ===

bool UParkourComponent::ExecuteClimb()
{
     if (!CanPerformClimb())
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    UCapsuleComponent* CapsuleComp = Character->GetCapsuleComponent();
    UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();

    DisablePhysicsAndCollision(CapsuleComp, MovementComp);

    const FVector OwnerLocation = Character->GetActorLocation();
    const FVector TargetLocation(OwnerLocation.X, OwnerLocation.Y, CurrentWallInfo.TopLocation.Z + 20.0f);
    Character->SetActorLocation(TargetLocation);

    PlayMontageWithCallback(EParkourMontageType::Climb, [this]()
        {
            OnClimbComplete();
        });

    return true;
}

bool UParkourComponent::ExecuteVault()
{
    if (CurrentWallInfo.bRequiresClimbing)
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    UCapsuleComponent* CapsuleComp = Character->GetCapsuleComponent();
    UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();

    DisablePhysicsAndCollision(CapsuleComp, MovementComp);

    FVector TargetLocation;
    EParkourMontageType MontageType;

    if (CurrentWallInfo.bIsThickWall)
    {
        const FRotator NormalRotation = UKismetMathLibrary::MakeRotFromX(CurrentWallInfo.SurfaceNormal);
        const FVector ForwardVector = UKismetMathLibrary::GetForwardVector(NormalRotation);

        TargetLocation = Character->GetActorLocation() + (ForwardVector * 50.0f);
        MontageType = EParkourMontageType::GettingUp;
    }
    else
    {
        const FVector CurrentLocation = Character->GetActorLocation();
        TargetLocation = FVector(CurrentLocation.X, CurrentLocation.Y, CurrentWallInfo.TopLocation.Z + 20.0f);
        MontageType = EParkourMontageType::Vault;
    }

    Character->SetActorLocation(TargetLocation);

    PlayMontageWithCallback(MontageType, [this]()
        {
            OnParkourComplete();
        });

    return true;
}

bool UParkourComponent::CanPerformClimb()
{
    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        return false;
    }

    const FVector OwnerLocation = Character->GetActorLocation();
    const FVector OwnerForward = Character->GetActorForwardVector();

    const FVector TraceStart = OwnerLocation + FVector(0, 0, 200.0f);
    const FVector TraceEnd = TraceStart + (OwnerForward * WallDetectionDistance);

    FHitResult HitResult;
    // 前方に障害物がない場合のみ登れる
    return !PerformLineTrace(TraceStart, TraceEnd, HitResult);
}

// === 物理・コリジョン制御 ===

void UParkourComponent::DisablePhysicsAndCollision(UCapsuleComponent* CapsuleComp, UCharacterMovementComponent* MovementComp)
{
    if (CapsuleComp)
    {
        CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (MovementComp)
    {
        // 既存の速度と力を全てリセット
        MovementComp->StopMovementImmediately();
        MovementComp->Velocity = FVector::ZeroVector;

        // 落下モードに設定し、重力を無効化
        MovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
        MovementComp->GravityScale = 0.0f;

        UE_LOG(LogTemp, Log, TEXT("Parkour: All forces and velocity reset"));
    }
}

void UParkourComponent::EnablePhysicsAndCollision()
{
    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        return;
    }

    UCapsuleComponent* CapsuleComp = Character->GetCapsuleComponent();
    UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();

    if (CapsuleComp)
    {
        CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    if (MovementComp)
    {
        MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);
        MovementComp->GravityScale = NormalGravityScale;
    }

    bCanParkour = true;
}


// === アニメーション制御 ===

void UParkourComponent::PlayMontageWithCallback(EParkourMontageType MontageType, TFunction<void()> OnComplete)
{
    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        if (OnComplete)
        {
            OnComplete();
        }
        return;
    }

    UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
    if (!AnimInstance || !AnimMontageMap.Contains(MontageType))
    {
        if (OnComplete)
        {
            OnComplete();
        }
        return;
    }

    const float MontageDuration = AnimInstance->Montage_Play(AnimMontageMap[MontageType], 1.0f);

    FTimerHandle TimerHandle;
    GetOwner()->GetWorldTimerManager().SetTimer(
        TimerHandle,
        FTimerDelegate::CreateLambda(OnComplete),
        MontageDuration,
        false
    );
}

// === 終了処理 ===

void UParkourComponent::OnClimbComplete()
{
    if (CurrentWallInfo.bIsThickWall)
    {
        // 厚い壁の場合：追加のモンタージュを再生
        PlayMontageWithCallback(EParkourMontageType::None, [this]()
            {
                OnParkourComplete();
            });
    }
    else
    {
        // 薄い壁の場合：すぐに終了
        OnParkourComplete();
    }
}


void UParkourComponent::OnParkourComplete()
{
    EnablePhysicsAndCollision();

    bIsPerformingParkour = false;
    EnableCharacterInput();

    // パルクール終了イベントを発行
    OnParkourEnded.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("Parkour: Ended"));
}

// === ヘルパーメソッド ===

bool UParkourComponent::PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHitResult)
{
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

    TArray<AActor*> ActorsToIgnore;
    constexpr bool bIgnoreSelf = true;
    constexpr bool bTraceComplex = false;
    constexpr EDrawDebugTrace::Type DrawDebugType = EDrawDebugTrace::None;
    const FLinearColor TraceColor(1.0f, 0.0f, 0.0f);
    const FLinearColor TraceHitColor(0.0f, 1.0f, 0.0f);
    constexpr float DrawTime = 5.0f;

    return UKismetSystemLibrary::LineTraceSingleForObjects(
        GetWorld(),
        Start,
        End,
        ObjectTypes,
        bTraceComplex,
        ActorsToIgnore,
        DrawDebugType,
        OutHitResult,
        bIgnoreSelf,
        TraceColor,
        TraceHitColor,
        DrawTime
    );
}

ACharacter* UParkourComponent::GetOwnerCharacter() const
{
    return Cast<ACharacter>(GetOwner());
}

// === 入力制御メソッド ===

void UParkourComponent::DisableCharacterInput()
{
    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (PC)
    {
        Character->DisableInput(PC);
        UE_LOG(LogTemp, Log, TEXT("Parkour: Input disabled"));
    }
}

void UParkourComponent::EnableCharacterInput()
{
    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (PC)
    {
        Character->EnableInput(PC);
        UE_LOG(LogTemp, Log, TEXT("Parkour: Input enabled"));
    }
}






