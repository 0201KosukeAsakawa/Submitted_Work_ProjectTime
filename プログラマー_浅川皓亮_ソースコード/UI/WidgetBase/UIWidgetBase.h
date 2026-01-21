// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class CARRY_API UUIWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:
public:
    // 汎用プロパティ設定インターフェース
    template<typename T>
    bool SetProperty(FName PropertyName, const T& Value)
    {
        FProperty* Property = GetClass()->FindPropertyByName(PropertyName);
        if (!Property)
        {
            UE_LOG(LogTemp, Warning, TEXT("UIWidgetBase: Property not found - %s in %s"),
                *PropertyName.ToString(), *GetClass()->GetName());
            return false;
        }

        return SetPropertyInternal<T>(Property, Value);
    }

    // よく使う型用の便利関数（オプション）
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool SetFloatProperty(FName PropertyName, float Value)
    {
        return SetProperty<float>(PropertyName, Value);
    }

    UFUNCTION(BlueprintCallable, Category = "UI")
    bool SetIntProperty(FName PropertyName, int32 Value)
    {
        return SetProperty<int32>(PropertyName, Value);
    }

    UFUNCTION(BlueprintCallable, Category = "UI")
    bool SetTextProperty(FName PropertyName, const FText& Value)
    {
        return SetProperty<FText>(PropertyName, Value);
    }

    UFUNCTION(BlueprintCallable, Category = "UI")
    bool SetBoolProperty(FName PropertyName, bool Value)
    {
        return SetProperty<bool>(PropertyName, Value);
    }

    UFUNCTION(BlueprintCallable, Category = "UI")
    bool SetColorProperty(FName PropertyName, FLinearColor Value)
    {
        return SetProperty<FLinearColor>(PropertyName, Value);
    }

protected:
    // 内部実装
    template<typename T>
    bool SetPropertyInternal(FProperty* Property, const T& Value);
};
