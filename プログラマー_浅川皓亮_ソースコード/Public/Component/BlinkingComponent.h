// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlinkingComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CARRY_API UBlinkingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBlinkingComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 表示時間(秒)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blinking")
    float VisibleDuration = 2.0f;

    // 非表示時間(秒)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blinking")
    float InvisibleDuration = 2.0f;

    // 点滅を有効にするか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blinking")
    bool bEnableBlinking = true;

    // 開始時に表示状態にするか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blinking")
    bool bStartVisible = true;

private:
    // 現在表示中か
    bool bIsVisible;

    // 現在の状態の経過時間
    float CurrentStateTime;

    // 可視性を設定
    void SetVisibility(bool bVisible);
};