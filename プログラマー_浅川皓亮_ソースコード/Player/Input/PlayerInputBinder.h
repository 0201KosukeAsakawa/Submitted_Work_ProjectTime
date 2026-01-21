// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "PlayerInputBinder.generated.h"

struct FInputActionValue;
class UEnhancedInputComponent;
class UInputAction;
class IPlayerInputReceiver;
class UInputMappingContext;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UPlayerInputBinder : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInputBinder();

protected:
	virtual void BeginPlay() override;

public:
	/**
	 * 入力アクションをバインドする
	 *
	 * @param InputComponent 入力コンポーネント
	 * @param Receiver 入力を受け取るオブジェクト
	 */
	void BindInputs(UEnhancedInputComponent* InputComponent, TScriptInterface<IPlayerInputReceiver> Receiver);

	/**
	 * バインド済みの入力を全て解除する
	 */
	void UnbindInputs();

	void HandleMove(const FInputActionValue& Value);

	void HandleJump(const FInputActionValue& Value);

	void HandleReplay(const FInputActionValue& Value);

	void HandleReplayToWorld(const FInputActionValue& Value);

	void HandleLook(const FInputActionValue& Value);

	void HandleSlowSkill(const FInputActionValue& Value);

	void HandleBoostSkill(const FInputActionValue& Value);

	void HandleOnInteractAction(const FInputActionValue& Value);

	void HandleOpenMenu(const FInputActionValue& Value);
private:
	/** 入力受信対象 */
	TScriptInterface<IPlayerInputReceiver> InputReceiver;

	/** 入力アクションの定義 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ReplayToWorldAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* SpecialAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* BoostAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* OnInteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* OpenMenuAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
};