// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shakes/LegacyCameraShake.h"
#include "LandingCameraShake.generated.h"

/**
 * 
 */
UCLASS()
class CARRY_API ULandingCameraShake : public ULegacyCameraShake
{
	GENERATED_BODY()
	
public:
	ULandingCameraShake()
	{
		OscillationDuration = 0.2f;         // 短く重く
		OscillationBlendInTime = 0.02f;
		OscillationBlendOutTime = 0.1f;

		// 縦にグッと沈む
		LocOscillation.Z.Amplitude = 12.f;
		LocOscillation.Z.Frequency = 5.f;

		// 前に一瞬倒れる（視点が下に向く）
		RotOscillation.Pitch.Amplitude = 8.f;
		RotOscillation.Pitch.Frequency = 6.f;

		// 横揺れはごくわずかに
		RotOscillation.Roll.Amplitude = 0.5f;
		RotOscillation.Roll.Frequency = 8.f;
	}

};
