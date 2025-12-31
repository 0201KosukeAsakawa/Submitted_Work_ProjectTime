// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/BlinkingComponent.h"

UBlinkingComponent::UBlinkingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bIsVisible = true;
    CurrentStateTime = 0.0f;
}

void UBlinkingComponent::BeginPlay()
{
    Super::BeginPlay();

    bIsVisible = bStartVisible;
    CurrentStateTime = 0.0f;

    // 初期状態を設定
    SetVisibility(bIsVisible);
}

void UBlinkingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnableBlinking)
    {
        return;
    }

    CurrentStateTime += DeltaTime;

    // 現在の状態の時間を超えたら切り替え
    float CurrentDuration = bIsVisible ? VisibleDuration : InvisibleDuration;

    if (CurrentStateTime >= CurrentDuration)
    {
        // 状態を反転
        bIsVisible = !bIsVisible;
        CurrentStateTime = 0.0f;

        // 可視性を更新
        SetVisibility(bIsVisible);
    }
}

void UBlinkingComponent::SetVisibility(bool bVisible)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // オーナーアクターの全ての可視コンポーネントの表示/非表示を切り替え
    TArray<UPrimitiveComponent*> PrimitiveComponents;
    Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* Component : PrimitiveComponents)
    {
        if (Component)
        {
            // 表示/非表示を切り替え
            Component->SetVisibility(bVisible, true);

            // 当たり判定も切り替え
            Component->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        }
    }

    // アクターのルートコンポーネントも設定
    USceneComponent* RootComp = Owner->GetRootComponent();
    if (RootComp)
    {
        RootComp->SetVisibility(bVisible, true);
    }
}