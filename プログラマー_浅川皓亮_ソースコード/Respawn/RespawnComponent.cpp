// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/RespawnComponent.h"

// 処理の流れ:
// 1. リスポーン位置をゼロベクトルで初期化
// 2. Tickを有効化
URespawnComponent::URespawnComponent()
    : mRespawnPosition(FVector(0, 0, 0))
{
    PrimaryComponentTick.bCanEverTick = true;
}

// 処理の流れ:
// 1. Ownerの取得
// 2. Ownerの現在位置を初期リスポーン位置として記録
void URespawnComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner())
    {
        mRespawnPosition = GetOwner()->GetActorLocation();
    }
}

void URespawnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// 処理の流れ:
// 1. 新しいリスポーン位置を記録
void URespawnComponent::SetRespawnPosition(FVector nextPosition)
{
    mRespawnPosition = nextPosition;
}

// 処理の流れ:
// 1. Ownerの有効性確認
// 2. 設定されたリスポーン位置にOwnerを移動
void URespawnComponent::Respawn()
{
    if (GetOwner() == nullptr)
        return;

    GetOwner()->SetActorLocation(mRespawnPosition);
}