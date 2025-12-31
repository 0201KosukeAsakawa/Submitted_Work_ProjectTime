// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/SwitchTargetInterface.h"
#include "DoorActor.generated.h"

/**
 * 
 */
UCLASS()
class CARRY_API ADoorActor :public AActor, public ISwitchTargetInterface
{
    GENERATED_BODY()

public:
    ADoorActor();

    // インターフェースの実装
    virtual void OnSwitchStateChanged(bool bIsOn) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* DoorMesh;


    FVector ClosedPosition;
    UPROPERTY(EditAnywhere)
    FVector OpenPosition;
};