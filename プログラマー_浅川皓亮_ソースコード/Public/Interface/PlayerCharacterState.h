// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerCharacterState.generated.h"

class APlayerCharacter;
struct FInputActionValue;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPlayerCharacterState : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CARRY_API IPlayerCharacterState
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
    /**
     * @brief State開始時に呼ばれる処理
     * @param Owner 所有している親クラス
     * @return true: 正常に開始処理完了 / false: 開始処理失敗
     */
    virtual bool OnEnter(AActor* Owner) { return false; }

    /**
     * @brief State毎フレーム更新処理
     * @param DeltaTime 前フレームからの経過時間
     * @return true: 更新成功 / false: 更新不可
     */
    virtual bool OnUpdate(float DeltaTime) { return false; }

    /**
     * @brief State終了時に呼ばれる処理
     * @return true: 正常に終了処理完了 / false: 終了処理失敗
     */
    virtual bool OnExit() { return false; }

    /**
     * @brief State固有のアクション入力処理
     * @param ActionValue 入力値（移動量やボタン押下など）
     * @return true: アクション実行成功 / false: 実行不可
     */
    virtual bool RePlayAction(const FInputActionValue& ActionValue) { return false; }

    /**
     * @brief State固有の移動処理
     * @param ActionValue 入力値（方向ベクトルや速度）
     * @return true: 移動処理成功 / false: 移動不可
     */
    virtual bool Movement(const FInputActionValue& ActionValue) { return false; }

    /**
     * @brief State固有のジャンプ処理
     * @param ActionValue 入力値（方向ベクトルや速度）
     * @return true: 処理成功 / false: 不可
     */
    virtual bool Jump(const FInputActionValue& ActionValue) { return false; }

    /**
     * @brief スキルが途中でCancelされた場合の処理
     */
    virtual void SkillActionStop() {}

    /**
     * @brief 前にBoostする処理
     * @param ActionValue 入力値
     * @return true: 処理成功 / false: 処理不可
     */
    virtual bool BoostAction(const FInputActionValue& ActionValue) { return false; }
};
