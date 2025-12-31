// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/SwitchTargetInterface.h"
#include "SwitchBaseObject.generated.h"

class IPlayerInfoProvider;
class UBoxComponent;
class UPointLightComponent;

UCLASS(Abstract)
class CARRY_API ABaseSwitchObject : public AActor, public ISwitchTargetInterface
{
    GENERATED_BODY()

public:
    ABaseSwitchObject();

    // スイッチの状態を取得
    UFUNCTION(BlueprintPure, Category = "Switch")
    bool GetSwitchState() const { return bIsOn; }

protected:
    virtual void BeginPlay() override;

    // 派生クラスで実装する純粋仮想関数
    UFUNCTION()
    virtual void OnInteract() PURE_VIRTUAL(ABaseSwitchObject::OnInteract, );

    // 状態を変更してターゲットに通知
    void SetState(bool bNewState);

    // ライトの色を更新
    void UpdateLightColor(bool bState);

    // 接続されたターゲットに通知
    void NotifyTargets(bool bNewState);

    // オーバーラップイベント
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    // ISwitchTargetInterface実装
    UFUNCTION()
    virtual void OnSwitchStateChanged(bool isOn) override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "Switch")
    bool bIsOn;

    // エディタで設定するターゲットのリスト
    UPROPERTY(EditAnywhere, Category = "Switch Connection", meta = (AllowedClasses = "/Script/Engine.Actor"))
    TArray<AActor*> Targets;

    // キャッシュされたインターフェースポインタ
    UPROPERTY()
    TArray<TScriptInterface<ISwitchTargetInterface>> CachedTargets;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* SwitchMesh;

    UPROPERTY()
    APawn* PlayerInRange = nullptr;

    UPROPERTY()
    TScriptInterface<IPlayerInfoProvider> PlayerInfoProvider;

    UPROPERTY(EditAnywhere)
    UBoxComponent* Collision;

    UPROPERTY(EditAnywhere, Category = "Light")
    UPointLightComponent* PointLight;
};