// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interface/PlayerCharacterState.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DefaultState.generated.h"

class APlayerCharacter;
class IPlayerInfoProvider;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UDefaultState : public UObject, public IPlayerCharacterState
{
	GENERATED_BODY()

public:
	/**
	 * コンストラクタ
	 *
	 * Tick を有効化し、初期化フラグなどの基本設定を行う
	 */
	UDefaultState();

	/** 最後に接地していた高度を取得 */
	float GetLastGroundHeight() const { return LastGroundHeight; }

	/** 落下距離から着地硬直時間を計算する */
	float CalculateLandingLagDuration(float FallDistance) const;

private:
	/**
	 * @brief State開始時に呼ばれる処理
	 * @param Owner 所有している親クラス
	 * @return true: 正常に開始処理完了 / false: 開始処理失敗
	 */
	virtual bool OnEnter(AActor* owner) override;

	/**
	 * ステート更新処理
	 *
	 * @param DeltaTime 経過時間
	 * @return 成功した場合 true
	 *
	 * @note ステートが有効な間、フレームごとに呼ばれる処理を定義する
	 */
	virtual bool OnUpdate(float DeltaTime) override;

	/**
	 * ステート終了時の後処理
	 *
	 * @return 成功した場合 true
	 *
	 * @note 終了時のクリーンアップやエフェクト解除などを行う
	 */
	virtual bool OnExit() override;

	/**
	 * ステート中の主要アクション（攻撃・ジャンプなど）を処理する
	 *
	 * @param Value 入力アクション値
	 * @return 入力を処理した場合 true
	 */
	virtual bool RePlayAction(const FInputActionValue& Value) override;

	/**
	 * 移動入力処理
	 *
	 * @param Value 入力アクション値（2D移動ベクトル）
	 * @return 入力を処理した場合 true
	 *
	 * @note カメラ方向に基づいた移動と Head Bob 処理を呼び出す
	 */
	virtual bool Movement(const FInputActionValue& Value) override;

	/**
	 * @brief State固有のジャンプ処理
	 * @param ActionValue 入力値（方向ベクトルや速度）
	 * @return true: 処理成功 / false: 不可
	 */
	virtual bool Jump(const FInputActionValue& Value) override;

	/**
	 * カメラ方向と入力ベクトルから、ワールド上の移動方向を算出する
	 *
	 * @param Character 対象キャラクター
	 * @param MoveInput 移動入力ベクトル（X: 横, Y: 前後）
	 * @return ワールド空間上の正規化された移動方向ベクトル
	 *
	 * @note カメラ回転を考慮して方向を算出し、上下反転時の左右補正も行う
	 */
	FVector CalculateMoveDirection(ACharacter* Character, const FVector2D& MoveInput) const;

	/**
	* @brief 前にBoostする処理
	* @param ActionValue 入力値
	* @return true: 処理成功 / false: 処理不可
	*/
	virtual bool BoostAction(const FInputActionValue& ActionValue)override;

	/**
	* @brief スキルが途中でCancelされた場合の処理
	*/
	virtual void SkillActionStop()override;

private:
	/** オーナーキャラクター情報取得用インターフェース */
	UPROPERTY()
	TScriptInterface<IPlayerInfoProvider> OwnerInterface;

	UPROPERTY()
	AActor* OwnerActor;

	/** デフォルトの移動速度 [cm/s] */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveSpeed;

	/** カメラボブの周波数（1秒あたりの揺れ回数） */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraBobFrequency;

	/** カメラボブの最大振幅（上下揺れ幅） */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraBobAmplitude;

	/**現在ジャンプした回数*/
	float JumpCount;

	/**逆再生中記録中*/
	bool IsRecording;

	/** 最後に接地していた高さ（Z座標） */
	float LastGroundHeight;

};
