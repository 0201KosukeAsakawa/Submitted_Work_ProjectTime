// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/RotationGroup.h"
// コンストラクタ
ARotationGroup::ARotationGroup()
{
	PrimaryActorTick.bCanEverTick = true; // 毎フレーム Tick を有効化
}

// ゲーム開始時に初期化
void ARotationGroup::BeginPlay()
{
	Super::BeginPlay();

	// 初期回転状態を保存
	PreviousQuat = GetActorQuat();
}

// 毎フレーム処理
void ARotationGroup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 親オブジェクトの回転処理
	Rotation();

	// 現在の回転を取得
	FQuat CurrentQuat = GetActorQuat();

	// 回転差分を計算（親の回転変化量）
	FQuat DeltaQuat = CurrentQuat * PreviousQuat.Inverse();

	// 差分を子オブジェクトに反映
	RotationObject(DeltaQuat);

	// 次フレーム用に保存
	PreviousQuat = CurrentQuat;
}

// 子オブジェクト群に回転差分を適用
void ARotationGroup::RotationObject(const FQuat& DeltaQuat)
{
	if (TargetArray.Num() == 0)
		return;

	// 親オブジェクトの中心位置
	const FVector Center = GetActorLocation();

	for (AActor* ChildActor : TargetArray)
	{
		if (!ChildActor)
			continue;

		// 親基準での相対位置
		FVector RelativePos = ChildActor->GetActorLocation() - Center;

		// 回転差分を適用
		FVector RotatedPos = DeltaQuat.RotateVector(RelativePos);
		ChildActor->SetActorLocation(Center + RotatedPos);

		// bShouldRotate が true の場合、回転自体も適用
		if (bShouldRotate)
		{
			FQuat NewQuat = DeltaQuat * ChildActor->GetActorQuat();
			ChildActor->SetActorRotation(NewQuat);
		}
	}
}