// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TimeControllable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTimeControllable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CARRY_API ITimeControllable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
    /** 時間記録を開始 */
    virtual void StartTimeRecording() {};

    /** 時間記録を停止 */
    virtual void StopTimeRecording() {};

    /** 巻き戻しを開始 */
    virtual void StartTimeRewind(float Duration = 3.0f) {};

    /** スロー効果を開始 */
    virtual void StartTimeSlow(float SlowScale = 0.3f, float Duration = 3.0f) {};

    /** スロー効果を停止 */
    virtual void StopTimeSlow() {};

    /** 記録中かどうか */
    virtual  bool IsRecording() const { return false; };

    /** スロー中かどうか */
    virtual  bool IsSlowMotion() const { return false; };
};
