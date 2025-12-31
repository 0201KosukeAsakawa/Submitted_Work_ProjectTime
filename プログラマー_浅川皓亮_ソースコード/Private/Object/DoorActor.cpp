// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/DoorActor.h"
#include "Components/StaticMeshComponent.h"

ADoorActor::ADoorActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(RootComponent);
}

void ADoorActor::BeginPlay()
{
    Super::BeginPlay();

    ClosedPosition = GetActorLocation();
}

void ADoorActor::OnSwitchStateChanged(bool bIsOn)
{
    if (bIsOn)
    {
        SetActorLocation(OpenPosition);
    }
    else
    {
        SetActorLocation(ClosedPosition);

    }
}