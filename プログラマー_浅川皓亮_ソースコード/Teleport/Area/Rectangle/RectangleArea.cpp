// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Teleport/RectangleArea.h"

FVector URectangleArea::GetRandomOffset_Implementation() const
{
    return FVector(
        FMath::FRandRange(-Size.X / 2, Size.X / 2),
        FMath::FRandRange(-Size.Y / 2, Size.Y / 2),
        0.f
    );
}

void URectangleArea::DrawDebugArea_Implementation(UWorld* World, const FVector& Center) const
{
    FVector Extent(Size.X / 2.f, Size.Y / 2.f, 5.f);
    DrawDebugBox(World, Center, Extent, FColor::Green, false, -1.f, 0, 2.f);
}