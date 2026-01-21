// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Teleport/Teleporter.h"
#include "Components/BoxComponent.h"
#include "Object/Teleport/TeleportAreaBase.h"

ATeleporter::ATeleporter()
    :ToggleIntervalMax(5.f)
    , ToggleIntervalMin(2.f)
    , delayTime(0.5f)
    , OutputRotation(FRotator::ZeroRotator)
    , TeleporterMode(ETeleporterMode::AlwaysActive)
    , TargetActorTag("Teleportable")
    , bIsActive(true)
    , bDrawDebugArea(true)
    ,bUseDelay(true)
    , bApplyOutputRotation(false)
    , bIsTeleporting(false)
{
    PrimaryActorTick.bCanEverTick = false;
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    RootComponent = Trigger;
    Trigger->InitBoxExtent(FVector(100.f));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ATeleporter::OnOverlapBegin);
}

void ATeleporter::BeginPlay()
{
    Super::BeginPlay();

    // ランダムモードの場合、トグルスケジュールを開始
    if (TeleporterMode == ETeleporterMode::RandomToggle)
    {
        ScheduleNextToggle();
    }
}

void ATeleporter::ScheduleNextToggle()
{
    if (TeleporterMode != ETeleporterMode::RandomToggle)
    {
        return;
    }

    float RandomInterval = FMath::FRandRange(ToggleIntervalMin, ToggleIntervalMax);

    GetWorld()->GetTimerManager().SetTimer(
        ToggleTimerHandle,
        this,
        &ATeleporter::ToggleActive,
        RandomInterval,
        false
    );
}

void ATeleporter::ToggleActive()
{
    bIsActive = !bIsActive;

    UE_LOG(LogTemp, Log, TEXT("Teleporter %s: Active = %s"),
        *GetName(), bIsActive ? TEXT("true") : TEXT("false"));

    // 次のトグルをスケジュール
    ScheduleNextToggle();
}

void ATeleporter::OnOverlapBegin(
    UPrimitiveComponent* OverComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& Sweep)
{
    // アクティブでない場合は何もしない
    if (!bIsActive)
    {
        return;
    }

    if (bIsTeleporting || !TargetTeleporter || !OtherActor) return;
    if (!OtherActor->ActorHasTag(TargetActorTag)) return;

    bIsTeleporting = true;
    TargetTeleporter->bIsTeleporting = true;

    FVector Offset = Area ? Area->GetRandomOffset() : FVector::ZeroVector;
    FVector NewLoc = TargetTeleporter->GetActorLocation() + Offset;
    OtherActor->SetActorLocation(NewLoc);

    // 出力方向の適用
    if (bApplyOutputRotation)
    {
        FRotator CurrentRotation = OtherActor->GetActorRotation();
        FRotator NewRotation = TargetTeleporter->OutputRotation;

        // Yaw（水平回転）のみ適用する場合
        NewRotation.Pitch = CurrentRotation.Pitch;
        NewRotation.Roll = CurrentRotation.Roll;

        OtherActor->SetActorRotation(NewRotation);
    }
    if (bUseDelay)
    {
        FTimerHandle SelfResetTimer;
        GetWorld()->GetTimerManager().SetTimer(SelfResetTimer, [this]()
            {
                bIsTeleporting = false;
            }, delayTime, false);

        FTimerHandle TargetResetTimer;
        GetWorld()->GetTimerManager().SetTimer(TargetResetTimer, [Target = TargetTeleporter]()
            {
                if (Target)
                {
                    Target->bIsTeleporting = false;
                }
            }, delayTime, false);
    }
    else
    {
        bIsTeleporting = false;
        TargetTeleporter->bIsTeleporting = false;
    }
}

void ATeleporter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (bDrawDebugArea && Area)
    {
        Area->DrawDebugArea_Implementation(GetWorld(), GetActorLocation());
    }

    // 出力方向のデバッグ表示
    if (bApplyOutputRotation)
    {
        FVector Start = GetActorLocation();
        FVector ForwardDir = OutputRotation.RotateVector(FVector::ForwardVector);
        FVector End = Start + ForwardDir * 200.0f;

        DrawDebugDirectionalArrow(
            GetWorld(),
            Start,
            End,
            50.0f,
            FColor::Blue,
            false,
            -1.f,
            0,
            3.0f
        );
    }
}