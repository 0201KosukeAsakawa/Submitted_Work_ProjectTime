// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/GravityChanger.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "LevelManager.h"
#include "Sound/SoundManager.h"
#include "Player/PlayerCharacter.h"


AGravityChanger::AGravityChanger()
{
	PrimaryActorTick.bCanEverTick = false; // Tick不要

	// トリガーボリューム作成
	GravityTriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GravityTrigger"));
	RootComponent = GravityTriggerVolume;
}

void AGravityChanger::BeginPlay()
{
	Super::BeginPlay();

	if (GravityTriggerVolume)
	{
		GravityTriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AGravityChanger::OnEnterGravityArea);
		GravityTriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AGravityChanger::OnExitGravityArea);
	}
}

void AGravityChanger::OnEnterGravityArea(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasActivatedGravityChange || !OtherActor)
		return;

	// IPlayerInfoProvider インターフェースを持つか
	IPlayerInfoProvider* Player = Cast<IPlayerInfoProvider>(OtherActor);
	if (!Player ||  Player->IsRewinding())
		return;

	//Player->AdjustCharacterOrientation(GravityDirection);

	bHasActivatedGravityChange = true;

	// サウンド再生
	if (ALevelManager* LevelManager = ALevelManager::GetInstance(GetWorld()))
	{
		//if (LevelManager->GetSoundManager())
		//{
		//	LevelManager->GetSoundManager()->PlaySound("SE", "ChangeGravity");
		//}
	}
}

void AGravityChanger::OnExitGravityArea(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// エリアから出たらフラグリセット
	bHasActivatedGravityChange = false;
}