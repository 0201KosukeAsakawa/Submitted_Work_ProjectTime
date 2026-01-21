// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/Teleport/TeleportAreaBase.h"
#include "CircleArea.generated.h"

/**
 * 
 */
UCLASS()
class CARRY_API UCircleArea : public UTeleportAreaBase
{
    GENERATED_BODY()

private:
    UPROPERTY(EditAnywhere, Category = "Teleporter")
    float Radius = 200.f;

    virtual FVector GetRandomOffset_Implementation() const override;

    virtual void DrawDebugArea_Implementation(UWorld* World, const FVector& Center) const override;
};