// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Teleport/CircleArea.h"

FVector UCircleArea::GetRandomOffset_Implementation() const
{
    float R = FMath::FRandRange(0.f, Radius);
    float A = FMath::FRandRange(0.f, UE_TWO_PI);
    return FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.f);
}

void UCircleArea::DrawDebugArea_Implementation(UWorld* World, const FVector& Center) const
{
    DrawDebugCircle(
        World,
        Center,
        Radius,
        32,
        FColor::Blue,
        false,
        -1.f,
        0,
        2.f,
        FVector(1, 0, 0),
        FVector(0, 1, 0),
        false
    );
}
