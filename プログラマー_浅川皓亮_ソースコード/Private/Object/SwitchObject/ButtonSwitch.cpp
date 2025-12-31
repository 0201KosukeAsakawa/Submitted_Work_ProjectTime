// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SwitchObject/ButtonSwitch.h"
#include "TimerManager.h"

AButtonSwitch::AButtonSwitch()
{
    ButtonDuration = 2.0f;
}

void AButtonSwitch::BeginPlay()
{
    Super::BeginPlay();

    // ボタンは初期状態でOFF
    SetState(false);
}

void AButtonSwitch::OnInteract()
{
    PressButton();
}

void AButtonSwitch::PressButton()
{
    // 既に押されている場合は何もしない
    if (bIsOn)
    {
        return;
    }

    // ONにする
    SetState(true);

    // タイマーをセット
    GetWorldTimerManager().SetTimer(
        ResetTimerHandle,
        this,
        &AButtonSwitch::ResetButton,
        ButtonDuration,
        false
    );

    UE_LOG(LogTemp, Log, TEXT("Button pressed! Will reset in %.1f seconds"), ButtonDuration);
}

void AButtonSwitch::ResetButton()
{
    // OFFに戻す
    SetState(false);
    UE_LOG(LogTemp, Log, TEXT("Button reset to OFF"));
}