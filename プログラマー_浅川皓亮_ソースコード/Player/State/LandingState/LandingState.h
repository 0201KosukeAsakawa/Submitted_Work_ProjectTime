// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interface/PlayerCharacterState.h"
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LandingState.generated.h"

class IPlayerInfoProvider;

/**
 * 
 */
UCLASS()
class CARRY_API ULandingState : public UObject,public IPlayerCharacterState
{
    GENERATED_BODY()

public:
    ULandingState();

    virtual bool OnEnter(AActor* Owner) override;
    virtual bool OnUpdate(float DeltaTime) override;
    virtual bool OnExit() override;

    /** íÖínçdíºéûä‘Çê›íËÇ∑ÇÈ */
    void SetLagDuration(float Duration);
private:
    float LagTime;
    float LagDuration;

    UPROPERTY()
    AActor* OwnerActor;
};

