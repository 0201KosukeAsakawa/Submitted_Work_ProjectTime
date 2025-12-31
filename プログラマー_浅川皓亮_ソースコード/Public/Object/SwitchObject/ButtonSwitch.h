// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SwitchObject/SwitchBaseObject.h"
#include "ButtonSwitch.generated.h"

/**
 * 一時的なスイッチ (マイクラのボタン)
 * 押すと一定時間ONになり、自動的にOFFに戻る
 */
UCLASS()
class CARRY_API AButtonSwitch : public ABaseSwitchObject
{
    GENERATED_BODY()

public:
    AButtonSwitch();

    // ボタンを押す
    UFUNCTION(BlueprintCallable, Category = "Switch")
    void PressButton();

protected:
    virtual void BeginPlay() override;
    virtual void OnInteract() override;

private:
    // タイマーでOFFに戻す
    void ResetButton();

    UPROPERTY(EditAnywhere, Category = "Button", meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float ButtonDuration = 2.0f;  // ボタンが押されている時間(秒)

    FTimerHandle ResetTimerHandle;
};