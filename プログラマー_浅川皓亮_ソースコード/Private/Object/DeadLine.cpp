
// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/DeadLine.h"
#include "Components/BoxComponent.h"
#include "Component/RespawnComponent.h"

// Sets default values
ADeadLine::ADeadLine()
{
	PrimaryActorTick.bCanEverTick = false;
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ADeadLine::OnOverlapBegin);
}

// Called when the game starts or when spawned
void ADeadLine::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ADeadLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADeadLine::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr)
		return;
	URespawnComponent* respawnComp = OtherActor->GetComponentByClass<URespawnComponent>();
	if (!respawnComp)
		return;

	respawnComp->Respawn();
	return;
}
