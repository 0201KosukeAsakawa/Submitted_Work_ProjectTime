// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/State/RewindingState.h"
#include "Player/PlayerCharacter.h"
#include "Player/PlayerStateManager.h"
#include "Interface/PlayerInfoProvider.h"
#include "LevelManager.h"
#include "Sound/SoundManager.h"
#include "Component/TimeManipulatorComponent.h"


#include "SoundHandle.h"

namespace RewindingStateConstants
{
    /** デフォルトの巻き戻し時間（秒） */
    static constexpr float DefaultRewindDuration = 3.0f;
}

// ============================================
// Constructor
// ============================================
URewindingState::URewindingState()
{
    
}

// ============================================
// State Lifecycle
// ============================================
bool URewindingState::OnEnter(AActor* Owner)
{
    // オーナーが IPlayerInfoProvider を実装しているか確認し、保持
    if (Owner->GetClass()->ImplementsInterface(UPlayerInfoProvider::StaticClass()))
    {
        OwnerInfoProvider.SetInterface(Cast<IPlayerInfoProvider>(Owner));
        OwnerInfoProvider.SetObject(Owner);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RewindingState: Owner does not implement IPlayerInfoProvider"));
        return false;
    }

    // ★ PlayerCharacter経由で巻き戻しを開始（ポストプロセスも自動適用）
    if (ITimeControllable* TimeControllable = Cast<ITimeControllable>(Owner))
    {
        TimeControllable->StartTimeRewind(RewindingStateConstants::DefaultRewindDuration);
        UE_LOG(LogTemp, Log, TEXT("RewindingState: Rewind started via PlayerCharacter"));
    }
    else
    {
        // フォールバック: 直接TimeManipulatorを使用
        if (UTimeManipulatorComponent* TimeComp = Owner->GetComponentByClass<UTimeManipulatorComponent>())
        {
            TimeComp->StartRewind(RewindingStateConstants::DefaultRewindDuration);
            UE_LOG(LogTemp, Log, TEXT("RewindingState: Rewind started directly"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RewindingState: Failed to find TimeManipulatorComponent"));
            return false;
        }
    }

    return true;
}

bool URewindingState::OnUpdate(float DeltaTime)
{
    if (!OwnerInfoProvider)
        return false;

    // ★ 巻き戻しが終了しているかチェック
    // PlayerCharacter::OnRewindStopped()で自動的にDefaultStateに戻るため
    // ここでは IsRewinding() の確認のみ
    if (!OwnerInfoProvider->IsRewinding())
    {
        // 念のため明示的にDefaultへ遷移
        // （通常は OnRewindStopped コールバックで処理済み）
        UE_LOG(LogTemp, Log, TEXT("RewindingState: Rewind finished, transitioning to Default"));
        OwnerInfoProvider->ChangeState(EPlayerStateType::Default);
    }

    return true;
}

bool URewindingState::OnExit()
{

    USoundHandle::StopSE(this, "Replay");
    return true;
}

// ============================================
// Input Handling
// ============================================
bool URewindingState::RePlayAction(const FInputActionValue& Value)
{
    // 巻き戻し中は追加アクション不可
    UE_LOG(LogTemp, Warning, TEXT("RewindingState: Action input ignored during rewind"));
    return false;
}

bool URewindingState::RecordStop(const FInputActionValue& Value)
{
    // リリース入力も無視
    return false;
}

bool URewindingState::Movement(const FInputActionValue& Value)
{
    // 巻き戻し中は移動入力を無視
    return false;
}

bool URewindingState::Jump(const FInputActionValue& ActionValue)
{
    return false;
}
