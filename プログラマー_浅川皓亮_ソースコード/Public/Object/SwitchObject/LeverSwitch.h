// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SwitchObject/SwitchBaseObject.h"
#include "LeverSwitch.generated.h"

/**
 * トグル式スイッチ (マイクラのレバー)
 * 使用するたびにON/OFFが切り替わる
 */
UCLASS()
class CARRY_API ALeverSwitch : public ABaseSwitchObject
{
    GENERATED_BODY()

public:
    // スイッチを切り替える
    UFUNCTION(BlueprintCallable, Category = "Switch")
    void ToggleSwitch();

    // スイッチの状態を直接設定
    UFUNCTION(BlueprintCallable, Category = "Switch")
    void SetSwitchState(bool bNewState);

protected:
    // 基底クラスの純粋仮想関数を実装
    virtual void OnInteract() override;
};