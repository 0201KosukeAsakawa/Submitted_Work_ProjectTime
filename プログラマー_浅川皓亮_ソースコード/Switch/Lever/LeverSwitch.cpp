// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SwitchObject/LeverSwitch.h"

void ALeverSwitch::OnInteract()
{
    ToggleSwitch();
}

void ALeverSwitch::ToggleSwitch()
{
    SetState(!bIsOn);
}

void ALeverSwitch::SetSwitchState(bool bNewState)
{
    SetState(bNewState);
}