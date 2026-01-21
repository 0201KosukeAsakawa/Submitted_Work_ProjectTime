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

// 処理の流れ:
// 1. パルクール可能か確認
// 2. パルクールフラグを設定
// 3. イベントをブロードキャスト
// 4. 入力を無効化
// 5. 壁への接触を検出
// 6. 壁の上端を検出
// 7. 壁のプロパティを計算
// 8. 壁の厚さを検出
// 9. 内側表面がある場合、ClimbとVaultを両方実行
// 10. 内側表面がない場合、Vaultのみ実行
bool UParkourComponent::Parkour()
{
    if (!bCanParkour)
    {
        return false;
    }

    bCanParkour = false;
    bIsPerformingParkour = true;

    OnParkourStarted.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("Parkour: Started"));

    DisableCharacterInput();

    if (!DetectWallImpact(CurrentWallInfo))
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    if (!DetectWallTop(CurrentWallInfo))
    {
        bCanParkour = true;
        bIsPerformingParkour = false;
        EnableCharacterInput();
        OnParkourEnded.Broadcast();
        return false;
    }

    CalculateWallProperties(CurrentWallInfo);

    CurrentWallInfo.bHasInnerSurface = DetectWallThickness(CurrentWallInfo);
    bool bSuccess = true;

    if (CurrentWallInfo.bHasInnerSurface)
    {
        if (!ExecuteClimb() && !ExecuteVault())
        {
            bSuccess = false;
            bIsPerformingParkour = false;
            EnableCharacterInput();
            OnParkourEnded.Broadcast();
        }
    }
    else
    {
        CurrentWallInfo.bIsThickWall = false;
        bSuccess = ExecuteVault();

        if (!bSuccess)
        {
            bIsPerformingParkour = false;
            EnableCharacterInput();
            OnParkourEnded.Broadcast();
        }
    }

    return bSuccess;
}

// 処理の流れ:
// 1. Characterを取得
// 2. トレース開始位置と終了位置を計算
// 3. ライントレースを実行
// 4. ヒットしたアクターが「NoParkour」タグを持つか確認
// 5. 壁の接触位置と法線を記録
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

// 処理の流れ:
// 1. 法線から回転を計算
// 2. 前方ベクトルを取得
// 3. トレース開始位置と終了位置を計算（上方向に検出）
// 4. ライントレースを実行
// 5. 壁の上端位置を記録
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
// 2. 前方ベクトルを取得
// 3. 内側表面のトレース開始位置と終了位置を計算
// 4. ライントレースを実行
// 5. 内側の上端位置を記録
// 6. 壁の厚さを計算
// 7. 厚さが閾値未満なら厚い壁と判定
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
// 1. 登りが可能か確認
// 2. Characterを取得
// 3. CapsuleとMovementComponentを取得
// 4. 物理とコリジョンを無効化
// 5. ターゲット位置を計算（壁の上端+20cm）
// 6. Characterを移動
// 7. Climbモンタージュを再生
// 8. 完了コールバックを設定
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

// 処理の流れ:
// 1. 登りが必要な高さの場合は失敗
// 2. Characterを取得
// 3. CapsuleとMovementComponentを取得
// 4. 物理とコリジョンを無効化
// 5. 厚い壁か薄い壁かでターゲット位置とモンタージュを決定
// 6. Characterを移動
// 7. モンタージュを再生
// 8. 完了コールバックを設定
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

// 処理の流れ:
// 1. Characterを取得
// 2. トレース開始位置と終了位置を計算（200cm上方から前方）
// 3. ライントレースを実行
// 4. 前方に障害物がない場合のみ登れる
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
    return !PerformLineTrace(TraceStart, TraceEnd, HitResult);
}

// 処理の流れ:
// 1. Capsuleのコリジョンを無効化
// 2. MovementComponentの全ての速度と力をリセット
// 3. 落下モードに設定
// 4. 重力を無効化
void UParkourComponent::DisablePhysicsAndCollision(UCapsuleComponent* CapsuleComp, UCharacterMovementComponent* MovementComp)
{
    if (CapsuleComp)
    {
        CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (MovementComp)
    {
        MovementComp->StopMovementImmediately();
        MovementComp->Velocity = FVector::ZeroVector;

        MovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
        MovementComp->GravityScale = 0.0f;

        UE_LOG(LogTemp, Log, TEXT("Parkour: All forces and velocity reset"));
    }
}

// 処理の流れ:
// 1. Characterを取得
// 2. CapsuleとMovementComponentを取得
// 3. Capsuleのコリジョンを有効化
// 4. 歩行モードに設定
// 5. 重力を通常値に戻す
// 6. パルクール可能フラグをtrueに設定
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

// 処理の流れ:
// 1. Characterを取得
// 2. AnimInstanceを取得
// 3. モンタージュマップから対応するモンタージュを取得
// 4. モンタージュを再生
// 5. モンタージュ終了時にコールバックを実行するタイマーを設定
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

// 処理の流れ:
// 1. 厚い壁の場合、追加のモンタージュを再生してから完了処理
// 2. 薄い壁の場合、すぐに完了処理
void UParkourComponent::OnClimbComplete()
{
    if (CurrentWallInfo.bIsThickWall)
    {
        PlayMontageWithCallback(EParkourMontageType::None, [this]()
            {
                OnParkourComplete();
            });
    }
    else
    {
        OnParkourComplete();
    }
}

// 処理の流れ:
// 1. 物理とコリジョンを有効化
// 2. パルクール実行フラグをfalseに設定
// 3. 入力を有効化
// 4. パルクール終了イベントをブロードキャスト
void UParkourComponent::OnParkourComplete()
{
    EnablePhysicsAndCollision();

    bIsPerformingParkour = false;
    EnableCharacterInput();

    OnParkourEnded.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("Parkour: Ended"));
}

// 処理の流れ:
// 1. ObjectTypeを設定
// 2. ライントレースを実行
// 3. ヒット結果を返す
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

// 処理の流れ:
// 1. Characterを取得
// 2. PlayerControllerを取得
// 3. 入力を無効化
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

// 処理の流れ:
// 1. Characterを取得
// 2. PlayerControllerを取得
// 3. 入力を有効化
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