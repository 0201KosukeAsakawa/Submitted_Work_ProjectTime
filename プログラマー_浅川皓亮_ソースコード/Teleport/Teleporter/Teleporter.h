// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Teleporter.generated.h"

class UBoxComponent;
class UTeleportAreaBase;

UENUM(BlueprintType)
enum class ETeleporterMode : uint8
{
    AlwaysActive    UMETA(DisplayName = "Always Active"),
    RandomToggle    UMETA(DisplayName = "Random Toggle")
};


/**
 * @brief 双方向テレポーター
 */
UCLASS()
class CARRY_API ATeleporter : public AActor
{
    GENERATED_BODY()

public:
    ATeleporter();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& Sweep
    );

    virtual void OnConstruction(const FTransform& Transform) override;

private:
    void ScheduleNextToggle();
    void ToggleActive();


private:
    /** 対象が侵入したら通知されるトリガー */
    UPROPERTY(VisibleAnywhere)
    UBoxComponent* Trigger;

    /** テレポート先ペア */
    UPROPERTY(EditAnywhere, Category = "Teleporter")
    ATeleporter* TargetTeleporter;

    /** テレポート許可タグ */
    UPROPERTY(EditAnywhere, Category = "Teleporter")
    FName TargetActorTag;

    /** テレポート範囲オブジェクト（多様な図形に対応） */
    UPROPERTY(EditAnywhere, Instanced, Category = "Teleporter|Area")
    UTeleportAreaBase* Area;

    // 出力方向
    UPROPERTY(EditAnywhere, Category = "Teleporter")
    FRotator OutputRotation;

    UPROPERTY(EditAnywhere, Category = "Teleporter")
    ETeleporterMode TeleporterMode;

    FTimerHandle ToggleTimerHandle;

    /** 多段テレポート防止用フラグ */
    bool bIsTeleporting;

    UPROPERTY(EditAnywhere, Category = "Teleporter|Debug")
    bool bDrawDebugArea;

    UPROPERTY(EditAnywhere, Category = "Teleporter")
    bool bApplyOutputRotation;

    UPROPERTY()
    bool bIsActive;

    UPROPERTY(EditAnywhere)
    bool bUseDelay;

    UPROPERTY(EditAnywhere)
    float delayTime;

    // ランダムモード用の設定
    UPROPERTY(EditAnywhere, Category = "Teleporter|Random Mode",
        meta = (EditCondition = "TeleporterMode == ETeleporterMode::RandomToggle", EditConditionHides))
    float ToggleIntervalMin;

    UPROPERTY(EditAnywhere, Category = "Teleporter|Random Mode",
        meta = (EditCondition = "TeleporterMode == ETeleporterMode::RandomToggle", EditConditionHides))
    float ToggleIntervalMax;
};