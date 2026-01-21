// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SwitchObject/SwitchBaseObject.h"
#include "Interface/SwitchTargetInterface.h"
#include "Interface/PlayerInfoProvider.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"

#include "UIHandle.h"
#include "SoundHandle.h"

ABaseSwitchObject::ABaseSwitchObject()
    : bIsOn(false)
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    Collision->SetupAttachment(RootComponent);
    Collision->OnComponentBeginOverlap.AddDynamic(this, &ABaseSwitchObject::OnOverlapBegin);
    Collision->OnComponentEndOverlap.AddDynamic(this, &ABaseSwitchObject::OnOverlapEnd);

    SwitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwitchMesh"));
    SwitchMesh->SetupAttachment(RootComponent);

    PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
    PointLight->SetupAttachment(RootComponent);
    PointLight->SetIntensity(3000.f);
    PointLight->SetLightColor(FColor::Red);
}

void ABaseSwitchObject::BeginPlay()
{
    Super::BeginPlay();

    // ターゲットをキャッシュ
    CachedTargets.Empty();
    for (AActor* Target : Targets)
    {
        if (Target == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("Switch has null target!"));
            continue;
        }

        if (Target->Implements<USwitchTargetInterface>())
        {
            TScriptInterface<ISwitchTargetInterface> Interface;
            Interface.SetObject(Target);
            Interface.SetInterface(Cast<ISwitchTargetInterface>(Target));
            CachedTargets.Add(Interface);

            UE_LOG(LogTemp, Log, TEXT("Switch connected to target: %s"), *Target->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Target %s does not implement ISwitchTargetInterface!"), *Target->GetName());
        }
    }

    if (CachedTargets.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Switch has no valid targets!"));
    }

    UpdateLightColor(bIsOn);
}

void ABaseSwitchObject::SetState(bool bNewState)
{
    if (bIsOn != bNewState)
    {
        bIsOn = bNewState;
        NotifyTargets(bNewState);
        UpdateLightColor(bNewState);
        USoundHandle::PlaySE(this, "Switch", false);

        UE_LOG(LogTemp, Log, TEXT("Switch set to: %s"), bIsOn ? TEXT("ON") : TEXT("OFF"));
    }
}

void ABaseSwitchObject::UpdateLightColor(bool bState)
{
    if (!PointLight) return;

    if (bState)
    {
        PointLight->SetLightColor(FColor::Green);
    }
    else
    {
        PointLight->SetLightColor(FColor::Red);
    }
}

void ABaseSwitchObject::NotifyTargets(bool bNewState)
{
    for (const TScriptInterface<ISwitchTargetInterface>& Target : CachedTargets)
    {
        if (Target.GetObject())
        {
            Target->OnSwitchStateChanged(bNewState);
        }
    }
}

void ABaseSwitchObject::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (APawn* Pawn = Cast<APawn>(OtherActor))
    {
        if (OtherActor->Implements<UPlayerInfoProvider>())
        {
            PlayerInRange = Pawn;
            PlayerInfoProvider.SetObject(OtherActor);
            PlayerInfoProvider.SetInterface(Cast<IPlayerInfoProvider>(OtherActor));

            // OnInteractをバインド
            PlayerInfoProvider->SubscribeToInteract(this, FName("OnInteract"));

            UUIHandle::ShowWidget(GetWorld(), EWidgetCategory::Interactive, TEXT("Switch"));
        }
    }
}

void ABaseSwitchObject::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (APawn* Pawn = Cast<APawn>(OtherActor))
    {
        if (Pawn == PlayerInRange)
        {
            if (PlayerInfoProvider.GetInterface())
            {
                PlayerInfoProvider->UnsubscribeFromInteract(this, FName("OnInteract"));
            }

            PlayerInRange = nullptr;
            PlayerInfoProvider.SetObject(nullptr);
            PlayerInfoProvider.SetInterface(nullptr);
            UUIHandle::HideWidget(GetWorld(), EWidgetCategory::Interactive, TEXT("Switch"));
        }
    }
}

void ABaseSwitchObject::OnSwitchStateChanged(bool isOn)
{
    bIsOn = isOn;
    UpdateLightColor(bIsOn);
}