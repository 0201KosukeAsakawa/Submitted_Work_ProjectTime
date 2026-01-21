// Fill out your copyright notice in the Description page of Project Settings.


#include "SubSystem/TimeManagerSubsystem.h"
#include "Component/TimeManipulatorComponent.h"
#include "LevelManager.h"
#include "Interface/UIManagerProvider.h"

#include "UIHandle.h"
#include "SoundHandle.h"
#include "SaveManager.h"

FOnSlowStopped UTimeManagerSubsystem::OnSlowStopped;

UTimeManagerSubsystem::UTimeManagerSubsystem()
    : Duration(10.f)
    , CurrentUpdateIndex(0)
    , TimeSinceLastSort(0.0f)
    , LastCameraLocation(FVector::ZeroVector)
{
}

void UTimeManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (bEnableFrameDistribution)
    {
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this](float DeltaTime) {
                return this->Tick(DeltaTime);
                }), 0.0f);

        UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Frame distribution initialized (MaxPerFrame: %d, CameraPriority: %s)"),
            MaxUpdatesPerFrame, bUseCameraPriority ? TEXT("ON") : TEXT("OFF"));
    }

    ApplyRewindQualityPreset(USaveManager::GetRewindQuality());
}

void UTimeManagerSubsystem::Deinitialize()
{
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
    }

    Super::Deinitialize();
}

bool UTimeManagerSubsystem::Tick(float DeltaTime)
{
    ProcessRewindUpdates(DeltaTime);
    return true;
}

void UTimeManagerSubsystem::SortComponentsByPriority()
{
    if (!bUseCameraPriority) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
    LastCameraLocation = CameraLocation;

    // カメラからの距離でソート
    WorldComponents.Sort([this, CameraLocation](const TWeakObjectPtr<UTimeManipulatorComponent>& A,
        const TWeakObjectPtr<UTimeManipulatorComponent>& B) {
            UTimeManipulatorComponent* CompA = A.Get();
            UTimeManipulatorComponent* CompB = B.Get();

            if (!CompA) return false;
            if (!CompB) return true;

            float PriorityA = CalculateComponentPriority(CompA);
            float PriorityB = CalculateComponentPriority(CompB);

            return PriorityA > PriorityB;
        });

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, VeryVerbose, TEXT("TimeManager: Components sorted by camera priority"));
#endif
}

float UTimeManagerSubsystem::CalculateComponentPriority(UTimeManipulatorComponent* Component) const
{
    if (!Component) return 0.0f;

    AActor* Owner = Component->GetOwner();
    if (!Owner) return 0.0f;

    float Distance = FVector::Dist(LastCameraLocation, Owner->GetActorLocation());
    float Priority = 1.0f / FMath::Max(Distance, 100.0f);

    if (Component->IsRewinding())
    {
        Priority *= 10.0f;
    }

    return Priority;
}

void UTimeManagerSubsystem::ProcessRewindUpdates(float DeltaTime)
{
    if (!bEnableFrameDistribution)
    {
        return;
    }

    WorldComponents.RemoveAll([](const TWeakObjectPtr<UTimeManipulatorComponent>& Comp) {
        return !Comp.IsValid();
        });

    int32 TotalComponents = WorldComponents.Num();
    if (TotalComponents == 0)
    {
        return;
    }

    // 定期的にカメラ距離でソート
    if (bUseCameraPriority)
    {
        TimeSinceLastSort += DeltaTime;
        if (TimeSinceLastSort >= PrioritySortInterval)
        {
            SortComponentsByPriority();
            TimeSinceLastSort = 0.0f;
            CurrentUpdateIndex = 0;
        }
    }

    int32 UpdatesThisFrame = FMath::Min(MaxUpdatesPerFrame, TotalComponents);
    int32 UpdatesExecuted = 0;

    for (int32 i = 0; i < UpdatesThisFrame; ++i)
    {
        if (CurrentUpdateIndex >= TotalComponents)
        {
            CurrentUpdateIndex = 0;
        }

        if (UTimeManipulatorComponent* Comp = WorldComponents[CurrentUpdateIndex].Get())
        {
            if (Comp->IsRewinding() && Comp->UseFrameDistribution())
            {
                Comp->ExecuteFrameDistributedUpdate(DeltaTime);
                UpdatesExecuted++;
            }
        }

        CurrentUpdateIndex++;
    }

#if !UE_BUILD_SHIPPING
    static int32 FrameCounter = 0;
    if (++FrameCounter % 120 == 0 && UpdatesExecuted > 0)
    {
        float CycleTimeSeconds = (float)TotalComponents / (float)MaxUpdatesPerFrame / 60.0f;
        UE_LOG(LogTemp, Verbose, TEXT("TimeManager: Processed %d/%d components (Cycle: %.2fs, Priority: %s)"),
            UpdatesExecuted, TotalComponents, CycleTimeSeconds,
            bUseCameraPriority ? TEXT("ON") : TEXT("OFF"));
    }
#endif
}

void UTimeManagerSubsystem::RegisterTimeComponent(UTimeManipulatorComponent* Component, bool bIsPlayer)
{
    if (!Component)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Attempted to register null component"));
        return;
    }

    if (bIsPlayer)
    {
        if (PlayerComponent.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Player component already registered, replacing"));
        }
        PlayerComponent = Component;
        UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Registered player time component"));
    }
    else
    {
        if (!WorldComponents.Contains(Component))
        {
            WorldComponents.Add(Component);

            if (bUseCameraPriority)
            {
                TimeSinceLastSort = PrioritySortInterval;
            }

            UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Registered world component (Total: %d)"),
                WorldComponents.Num());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Component already registered"));
        }
    }
}

void UTimeManagerSubsystem::UnregisterTimeComponent(UTimeManipulatorComponent* Component)
{
    if (!Component)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: Attempted to unregister null component"));
        return;
    }

    if (PlayerComponent == Component)
    {
        PlayerComponent.Reset();
        UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Unregistered player component"));
    }

    const int32 RemovedCount = WorldComponents.Remove(Component);
    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Unregistered world component (Remaining: %d)"),
            WorldComponents.Num());
    }
}

void UTimeManagerSubsystem::RewindToWorld(float duration)
{
    int32 ComponentsRewound = 0;
    int32 ComponentsFailed = 0;

    if (bUseCameraPriority)
    {
        SortComponentsByPriority();
        CurrentUpdateIndex = 0;
    }

    for (const TWeakObjectPtr<UTimeManipulatorComponent>& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->StartRewind(duration);
            ComponentsRewound++;
        }
        else
        {
            ComponentsFailed++;
        }
    }

    if (ComponentsFailed > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManagerSubsystem: %d components became invalid during rewind"),
            ComponentsFailed);
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Rewound %d world components (Priority sorted: %s)"),
        ComponentsRewound, bUseCameraPriority ? TEXT("Yes") : TEXT("No"));
}

void UTimeManagerSubsystem::StartSlowMotion(float SlowScale)
{
    SetCustomTimeDilationWorld(SlowScale);
    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Slow motion started (Scale=%.2f)"), SlowScale);

    if (GetWorld()->GetTimerManager().IsTimerActive(SlowMotionTimerHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(SlowMotionTimerHandle);
    }

    UUIHandle::ShowWidget(this, EWidgetCategory::Interactive, "Timer");
    UUIHandle::SetWidgetProperty<float>(this, EWidgetCategory::Interactive, "Timer", "Time", Duration);

    GetWorld()->GetTimerManager().SetTimer(
        SlowMotionTimerHandle,
        this,
        &UTimeManagerSubsystem::ResetTimeDilation,
        Duration,
        false
    );
}

void UTimeManagerSubsystem::ResetTimeDilation()
{
    SetCustomTimeDilationWorld(1.0f);
    UUIHandle::HideWidget(this, EWidgetCategory::Interactive, "Timer");
    USoundHandle::StopSE(this, "SlowTime");
    OnSlowStopped.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Time dilation reset to normal"));
}

void UTimeManagerSubsystem::ApplyRewindQualityPreset(ERewindQuality RewindQuality)
{
    switch (RewindQuality)
    {
    case ERewindQuality::Low:
        MaxUpdatesPerFrame = 50;
        PrioritySortInterval = 0.2f;
        bUseCameraPriority = true;
        bEnableFrameDistribution = true; 
        break;

    case ERewindQuality::Medium:
        MaxUpdatesPerFrame = 100;
        PrioritySortInterval = 0.15f;
        bUseCameraPriority = true;
        bEnableFrameDistribution = true;  
        break;


    case ERewindQuality::High:
        MaxUpdatesPerFrame = 150;
        PrioritySortInterval = 0.1f;
        bUseCameraPriority = true;
        bEnableFrameDistribution = true;  
        break;

    case ERewindQuality::Ultra:
        MaxUpdatesPerFrame = 500;
        PrioritySortInterval = 0.05f;
        bEnableFrameDistribution = false;  // Ultraのみ無効化
        bUseCameraPriority = false;
        break;
    }

    // コンポーネント側の設定も更新
    for (const TWeakObjectPtr<UTimeManipulatorComponent>& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->ApplyRewindQualityPreset(RewindQuality);
        }
    }

    USaveManager::SetRewindQuality(RewindQuality);
}

void UTimeManagerSubsystem::SetCustomTimeDilationWorld(float Scale)
{
    int32 ComponentsAffected = 0;

    for (const TWeakObjectPtr<UTimeManipulatorComponent>& WeakComp : WorldComponents)
    {
        if (UTimeManipulatorComponent* Comp = WeakComp.Get())
        {
            Comp->SetCustomTimeDilation(Scale);
            ComponentsAffected++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TimeManagerSubsystem: Set time dilation %.2f for %d components"),
        Scale, ComponentsAffected);
}