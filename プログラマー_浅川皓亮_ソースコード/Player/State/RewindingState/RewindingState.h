// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/PlayerCharacterState.h"
#include "RewindingState.generated.h"

class IPlayerInfoProvider;
class APlayerCharacter;
class ISoundManagerProvider;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CARRY_API URewindingState : public UObject, public IPlayerCharacterState
{

	GENERATED_BODY()

public:
	// デフォルトコンストラクタ
	URewindingState();

public:
    /**
    * @brief State開始時に呼ばれる処理
    * @param Owner 所有している親クラス
    * @return true: 正常に開始処理完了 / false: 開始処理失敗
    */
    virtual bool OnEnter(AActor* Owner);

    /**
     * @brief State毎フレーム更新処理
     * @param DeltaTime 前フレームからの経過時間
     * @return true: 更新成功 / false: 更新不可
     */
    virtual bool OnUpdate(float DeltaTime);

    /**
     * @brief State終了時に呼ばれる処理
     * @return true: 正常に終了処理完了 / false: 終了処理失敗
     */
    virtual bool OnExit();

    /**
     * @brief State固有のアクション入力処理
     * @param ActionValue 入力値（移動量やボタン押下など）
     * @return true: アクション実行成功 / false: 実行不可
     */
    virtual bool RePlayAction(const FInputActionValue& ActionValue);

    /**
     * @brief State固有の移動処理
     * @param ActionValue 入力値（方向ベクトルや速度）
     * @return true: 移動処理成功 / false: 移動不可
     */
    virtual bool Movement(const FInputActionValue& ActionValue);

    /**
     * @brief State固有のジャンプ処理
     * @param ActionValue 入力値（方向ベクトルや速度）
     * @return true: 処理成功 / false: 不可
     */
    virtual bool Jump(const FInputActionValue& ActionValue);

    /**
     * @brief アクションボタンが離された時の処理
     * @param ActionValue 入力値
     * @return true: 処理成功 / false: 処理不可
     */
    virtual bool RecordStop(const FInputActionValue& ActionValue);

private:
	// プレイヤー情報取得用インターフェース
	UPROPERTY()
	TScriptInterface<IPlayerInfoProvider> OwnerInfoProvider;
};