// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityChanger.generated.h"

class UBoxComponent;

/**
 * @brief 重力変更トリガー
 *
 * プレイヤーがエリアに入ると、重力方向を変更し、キャラクターの姿勢やカメラを補正。
 */
UCLASS()
class CARRY_API AGravityChanger : public AActor
{
	GENERATED_BODY()

public:
	AGravityChanger();

protected:
	virtual void BeginPlay() override;

public:
	/**
	 * @brief 重力変更エリアに入ったときの処理
	 * @param OverlappedComponent オーバーラップしたコンポーネント
	 * @param OtherActor 当たったアクター
	 * @param OtherComp 当たったコンポーネント
	 * @param OtherBodyIndex ボディインデックス
	 * @param bFromSweep Sweep判定か
	 * @param SweepResult Sweep結果
	 */
	UFUNCTION()
	void OnEnterGravityArea(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/**
	 * @brief 重力変更エリアから出たときの処理
	 * @param OverlappedComponent オーバーラップしていたコンポーネント
	 * @param OtherActor 当たったアクター
	 * @param OtherComp 当たったコンポーネント
	 * @param OtherBodyIndex ボディインデックス
	 */
	UFUNCTION()
	void OnExitGravityArea(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/** 回転角度（将来的に利用可能、現在は未使用） */
	UPROPERTY(EditAnywhere, Category = "Gravity")
	float GravityRotationAmount;

	/** 重力方向ベクトル */
	UPROPERTY(EditAnywhere, Category = "Gravity")
	FVector GravityDirection;

	/** 重力トリガーボリューム */
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* GravityTriggerVolume;

	/** 重力変更が既に実行されたか */
	bool bHasActivatedGravityChange = false;
};