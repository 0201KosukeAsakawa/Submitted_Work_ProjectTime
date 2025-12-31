#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsCalculatorComponent.generated.h"

class UBoxComponent;

/**
 * @class UPhysicsCalculatorComponent
 * @brief オブジェクトに対する簡易物理演算を行うコンポーネント。
 *
 * 重力・力の適用・地面との接地判定などを内部で処理し、
 * 物理挙動をコードベースで制御できる汎用物理計算コンポーネントです。
 *
 * @details
 * Unreal Engine の物理シミュレーション（RigidBody）を使わずに、
 * 任意のロジックで重力・落下・衝突を制御したい場合に使用します。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UPhysicsCalculatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** コンストラクタ（デフォルト値の設定） */
	UPhysicsCalculatorComponent();

protected:
	/** ゲーム開始時に呼ばれる初期化処理 */
	virtual void BeginPlay() override;

public:
	/** 毎フレーム呼ばれる物理更新処理 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 現在の力をリセットする
	 *
	 * @note 加速度や速度を初期化する際に使用します。
	 */
	UFUNCTION(BlueprintCallable)
	void ResetForce();

	/**
	 * オブジェクトに力を加える
	 *
	 * @param Direction 力を加える方向
	 * @param Force 力の大きさ
	 * @param bSweep 衝突を考慮するか
	 * @param useLocalOffset ローカル座標で力を加えるか（true: ローカル / false: ワールド）
	 */
	UFUNCTION(BlueprintCallable)
	void AddForce(FVector Direction, float Force, const bool bSweep = true, bool useLocalOffset = true);

	/**
	 * オブジェクトが地面に設置しているかを判定
	 *
	 * @return 設置している場合 true
	 */
	UFUNCTION(BlueprintCallable)
	bool OnGround() const;

	/**
	 * 重力を適用する設定
	 *
	 * @param applyGravity 重力を適用するか
	 * @param scale 重力のスケール値（9.8f など）
	 * @param Modifier 追加の補正値
	 */
	void SetGravityScale(const bool applyGravity = true, float scale = 9.8f, float Modifier = 1.0f);

	/**
	 * 障害物と接触した場合に移動ベクトルを補正
	 *
	 * @param MoveVector 元の移動ベクトル
	 * @return 補正後の移動ベクトル
	 */
	FVector GetBlockedAdjustedVector(const FVector& MoveVector);

	/**
	 * 地面に着地した瞬間であるかを判定
	 *
	 * @return 今フレームで着地した場合 true
	 */
	UFUNCTION(BlueprintCallable)
	const bool HasLanded();

	/** 現在物理計算が有効かどうか */
	bool IsPhysicsEnabled() const { return bIsPhysicsEnabled; }

private:
	/** 接地状態を更新 */
	void UpdateGroundState();

	/** 重力を加える内部処理 */
	void AddGravity();

	/** 設置面の法線を取得 */
	FVector GetGroundNormal() const;

private:
	/** 重力スケール（地球上なら約9.8） */
	float GravityScale;

	/** 力のスケール（外力の強さ） */
	float ForceScale;

	/** 最大落下速度 */
	UPROPERTY(EditAnywhere, Category = "Physics")
	float MaxFallingSpeed;

	/** 現在加えられている力の方向 */
	FVector ForceDirection;

	/** 前回の位置（移動差分の比較に使用） */
	FVector PreviousPosition;

	/** 現在の速度 */
	FVector Velocity;

	/** タイマー（物理演算用の時間管理に使用） */
	float Timer;

	/** 力の補正係数 */
	float GravityDivider;

	/** 重力を適用するかどうか */
	UPROPERTY(EditAnywhere)
	bool bShouldApplyGravity;

	/** スイープ（衝突判定）を有効にするか */
	UPROPERTY(EditAnywhere)
	bool bIsSweep;

	/** 物理計算が有効か */
	UPROPERTY(EditAnywhere)
	bool bIsPhysicsEnabled;

	/** ローカル座標でのオフセットを使用するか */
	bool bUseLocalOffset;

	/** 前フレームで地面にいたか */
	bool bWasOnGround;

	/** 現在落下中か */
	bool bFalling;

	/** 今フレームで着地したか */
	bool bHasJustLanded;

private:
	/** デフォルト重力値 */
	static constexpr float DEFAULT_GRAVITYSCALE = 9.8f;

	/** デフォルト最大落下速度 */
	static constexpr float DEFAULT_MAX_FALLSPEED = 200.0f;
};
