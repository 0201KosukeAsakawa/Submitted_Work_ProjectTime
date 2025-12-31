// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerInputReceiver.generated.h"

struct FInputActionValue;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPlayerInputReceiver : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CARRY_API IPlayerInputReceiver
{
	GENERATED_BODY()

public:
	/** 移動入力 */
	virtual void OnMove(const FInputActionValue& Value) {}

	/** ジャンプ */
	virtual void OnJump(const FInputActionValue& Value) {}

	/** メインアクション（攻撃・ジャンプなど） */
	virtual void OnReplayAction(const FInputActionValue& Value) {}

	virtual void OnReplayToWorldAction(const FInputActionValue& Value) {}

	/** カメラ回転入力 */
	virtual void OnLook(const FInputActionValue& Value) {}

	/** その他アクション（ダッシュ・リワインドなど） */
	virtual void OnSlowAction(const FInputActionValue& Value) {}

	/** ブースト */
	virtual void OnBoost(const FInputActionValue& Value) {}

	/** ボタンを押す/扉を開けるなどのアクション*/
	virtual void OnInteractAction(const FInputActionValue& Value){}

	/** メニューを開く*/
	virtual void OpenMenu(const FInputActionValue& Value) {}
};
