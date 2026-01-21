// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TeleportAreaBase.generated.h"

/**
 * @brief テレポート範囲の抽象クラス
 * 新しい図形範囲はこのクラスを継承して実装する
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class CARRY_API UTeleportAreaBase : public UObject
{
    GENERATED_BODY()

public:
    /**
     * @brief テレポート領域内のランダムなオフセット位置を返す
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Teleporter")
    FVector GetRandomOffset() const;
    virtual FVector GetRandomOffset_Implementation() const { return FVector::ZeroVector; }

    /** Debug表示 */
    UFUNCTION(BlueprintNativeEvent, Category = "Teleporter|Debug")
    void DrawDebugArea(UWorld* World, const FVector& Center) const;
    virtual void DrawDebugArea_Implementation(UWorld* World, const FVector& Center) const {}
};