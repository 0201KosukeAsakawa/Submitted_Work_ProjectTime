// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/PropellerObject.h"
#include "RotationGroup.generated.h"

/**
 * @brief 回転グループ
 *
 * ARotationGroup は APropellerObject を継承し、子オブジェクト群をまとめて回転させるクラス。
 * DeltaQuat を利用して、親の回転差分を子に反映させる。
 */
UCLASS()
class CARRY_API ARotationGroup : public APropellerObject
{
	GENERATED_BODY()

public:
	// コンストラクタ：デフォルト値設定
	ARotationGroup();

protected:
	// ゲーム開始時またはスポーン時に呼ばれる
	virtual void BeginPlay() override;

private:
	// 毎フレーム呼ばれる処理
	virtual void Tick(float DeltaTime) override;

	// 子オブジェクト群に回転を適用
	void RotationObject(const FQuat& DeltaQuat);

private:
	// 回転対象となるオブジェクト群
	UPROPERTY(EditAnywhere, Category = "RotationGroup")
	TArray<AActor*> TargetArray;

	// オブジェクト自身も回転させるか
	UPROPERTY(EditAnywhere, Category = "RotationGroup")
	bool bShouldRotate = false;

	// 前フレームの回転クォータニオン（差分計算用）
	FQuat PreviousQuat;
};