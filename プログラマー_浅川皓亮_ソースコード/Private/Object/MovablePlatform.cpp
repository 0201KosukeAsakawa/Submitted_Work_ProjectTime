// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/MovablePlatform.h"

AMovablePlatform::AMovablePlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsMoving = false;
	CurrentMoveProgress = 0.0f;
}

void AMovablePlatform::BeginPlay()
{
	Super::BeginPlay();

	// 初期位置を保存
	StartLocation = GetActorLocation();
	CurrentTargetLocation = StartLocation;
}

void AMovablePlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMoving)
	{
		FVector CurrentLocation = GetActorLocation();
		float DistanceToTarget = FVector::Dist(CurrentLocation, CurrentTargetLocation);

		// 目標地点に到達したか確認
		if (DistanceToTarget < 1.0f)
		{
			SetActorLocation(CurrentTargetLocation);
			bIsMoving = false;
			return;
		}

		// シームレスに移動
		FVector NewLocation = FMath::VInterpConstantTo(
			CurrentLocation,
			CurrentTargetLocation,
			DeltaTime,
			MoveSpeed
		);

		SetActorLocation(NewLocation);
	}
}

void AMovablePlatform::OnSwitchStateChanged(bool bIsOn)
{
	// 現在地を取得
	StartLocation = GetActorLocation();

	// スイッチの状態に応じて目標地点を設定
	if (bIsOn)
	{
		CurrentTargetLocation = TargetLocationA;
	}
	else
	{
		CurrentTargetLocation = TargetLocationB;
	}

	bIsMoving = true;
}
