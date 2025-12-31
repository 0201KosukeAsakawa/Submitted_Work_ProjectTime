// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/Teleport/TeleportAreaBase.h"
#include "RectangleArea.generated.h"

/**
 * 
 */
UCLASS()
class CARRY_API URectangleArea : public UTeleportAreaBase
{
    GENERATED_BODY()

private:
    /** X:幅 / Y:奥行 */
    UPROPERTY(EditAnywhere, Category = "Teleporter")
    FVector2D Size = FVector2D(200.f, 200.f);

    virtual FVector GetRandomOffset_Implementation() const override;

    virtual void DrawDebugArea_Implementation(UWorld* World, const FVector& Center) const override;
};