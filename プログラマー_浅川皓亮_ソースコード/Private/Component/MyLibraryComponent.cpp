// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/MyLibraryComponent.h"
#include "Math/Vector.h"


bool UMyLibraryComponent::IsNearTarget(const FVector StartLocation,const FVector TargetLocation,const float Threshold)
{
        FVector CurrentLocation = StartLocation;  // 現在のプレイヤーの位置
        float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);  // 現在位置と目標位置の距離

        // 距離が許容範囲（Threshold）以内であればtrueを返す
        return DistanceToTarget <= Threshold;
}
