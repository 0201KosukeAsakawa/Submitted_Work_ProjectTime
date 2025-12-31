// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerInfoProvider.generated.h"

class UPlayerCameraControlComponent;

// インタラクトキーが押されたことを通知するデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractPressed, APawn*, Instigator);


// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UPlayerInfoProvider : public UInterface
{
	GENERATED_BODY()
};

class UCameraComponent;
enum class EPlayerStateType : uint8;

class CARRY_API IPlayerInfoProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool ChangeState(EPlayerStateType) { return false; }

	virtual bool IsRewinding()const { return false; }

	virtual void SetNewCameraRotation(float Roll) {}

	virtual void PlayBoost() {}

	virtual bool PlayParkour() { return false; };

	/** カメラコンポーネントを取得 */
	virtual UCameraComponent* GetCamera() const { return nullptr; }

	/** カメラコントロールコンポーネントを取得 */
	virtual UPlayerCameraControlComponent* GetCameraControl() const { return nullptr; }

	// デリゲート購読（ヘルパー関数）
	virtual void SubscribeToInteract(UObject* Object, FName FunctionName) {};

	// デリゲート購読解除（ヘルパー関数）
	virtual void UnsubscribeFromInteract(UObject* Object, FName FunctionName) {}

	/**
	 * @befie カメラを頭のソケットにアタッチするかどうかを動的に切り替える
	 * @param bAttachToHead true: 頭のソケットにアタッチ(頭の動きに追従), false: ルートコンポーネントにアタッチ(揺れなし)
	 * @note どちらもFPSモード（メッシュは非表示）
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Camera")
	void SetCameraAttachToHead(bool bAttachToHead);
	virtual void SetCameraAttachToHead_Implementation(bool bAttachToHead) {};

	/**
	 * 現在カメラが頭にアタッチされているかを取得
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Camera")
	bool IsCameraAttachedToHead() const;
	virtual bool IsCameraAttachedToHead_Implementation()const { return false; };
};
