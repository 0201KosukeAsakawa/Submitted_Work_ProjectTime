// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PropellerObject.generated.h"

class UBoxComponent;

/*
 * @brief プロペラオブジェクト
 *
 * イベント発行に応じて回転するオブジェクト。
 */
UCLASS()
class CARRY_API APropellerObject : public AActor
{
	GENERATED_BODY()

public:
	// コンストラクタ：デフォルト値設定
	APropellerObject();

protected:
	// ゲーム開始時またはスポーン時に呼ばれる
	virtual void BeginPlay() override;

public:
	// 毎フレーム呼び出される処理
	virtual void Tick(float DeltaTime) override;

	// 現在イベント状態が有効かを返す
	UFUNCTION(BlueprintCallable, Category = "Event")
	bool IsPlayEvent() const;

	// イベントを実行（回転）する
	bool PlayEvent(float DeltaTime);

	// イベント状態を切り替える（On/Off）
	void SwitchPlayEvent() ;

	// イベントIDを取得する
	FName GetEventID() const ;

protected:
	// 初期化処理（イベントシステムへの登録）
	void Init();

	// 回転処理
	FRotator Rotation();

protected:
	// 初期化済みかを示す内部フラグ（実行時制御用）
	bool bIsInitialized = false;

private:
	// プロペラの回転速度（度/秒）
	UPROPERTY(EditAnywhere, Category = "Propeller")
	float RotationSpeed = 180.0f; // デフォルト180度/秒

	// このオブジェクトの識別用イベントID
	UPROPERTY(EditAnywhere, Category = "Event")
	FName EventID;

	// 回転軸ベクトル（例：Z軸なら FVector(0, 0, 1)）
	UPROPERTY(EditAnywhere, Category = "Propeller")
	FVector RotationAxis = FVector(0, 0, 1);

	// 回転の有効・無効を示すフラグ
	UPROPERTY(EditAnywhere, Category = "Event")
	bool bIsActive = true;

	// 回転を繰り返すか（ループ回転）
	UPROPERTY(EditAnywhere, Category = "Event")
	bool bIsLooping = false;

	// 回転時に使用する定数
	static constexpr float DegreesToRadians = PI / 180.0f;
};
