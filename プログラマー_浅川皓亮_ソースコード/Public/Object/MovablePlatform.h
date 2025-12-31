// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/SwitchTargetInterface.h"
#include "MovablePlatform.generated.h"

UCLASS()
class CARRY_API AMovablePlatform : public AActor , public ISwitchTargetInterface
{
	GENERATED_BODY()

public:
	AMovablePlatform();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FVector TargetLocationA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FVector TargetLocationB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	float MoveSpeed = 200.0f;

private:
	virtual void Tick(float DeltaTime) override;
	virtual void OnSwitchStateChanged(bool bIsOn) override;

	FVector StartLocation;
	FVector CurrentTargetLocation;
	bool bIsMoving;
	float CurrentMoveProgress;
};