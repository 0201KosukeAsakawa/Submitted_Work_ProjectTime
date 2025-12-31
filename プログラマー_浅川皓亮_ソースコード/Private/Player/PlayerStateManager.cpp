#include "Player/PlayerStateManager.h"
#include "Interface/PlayerCharacterState.h"



// ====================================================================
// UPlayerStateManager - プレイヤーの状態を管理するクラス
// 各状態は IPlayerCharacterState インターフェイスを実装した UObject クラスとして扱う
// ====================================================================

// コンストラクタ
UPlayerStateManager::UPlayerStateManager()
    : CurrentState(nullptr)
{
}

// 初期化処理
void UPlayerStateManager::Init()
{
    // 初期ステートを Default に設定
    ChangeState(EPlayerStateType::Default);
}

// 毎フレーム更新
void UPlayerStateManager::Update(float DeltaTime)
{
    if (CurrentState)
    {
        CurrentState->OnUpdate(DeltaTime);
    }
}

// ステート切り替え処理
TScriptInterface<IPlayerCharacterState> UPlayerStateManager::ChangeState(EPlayerStateType NextStateTag)
{
    if (!StateClassMap.Contains(NextStateTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("UPlayerStateManager::ChangeState - Invalid state tag"));
        return nullptr;
    }

    // 現在のステートを終了
    if (CurrentState)
    {
        CurrentState->OnExit();
    }

    // ステートクラスを取得
    const TSubclassOf<UObject> StateClass = StateClassMap[NextStateTag];
    if (!StateClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UPlayerStateManager::ChangeState - StateClass is null"));
        return nullptr;
    }

    // ステート生成
    UObject* NewStateObject = NewObject<UObject>(this, StateClass);
    if (!NewStateObject)
    {
        UE_LOG(LogTemp, Error, TEXT("UPlayerStateManager::ChangeState - Failed to create state instance"));
        return nullptr;
    }

    // インターフェイスとしてキャスト
    IPlayerCharacterState* NewStateInterface = Cast<IPlayerCharacterState>(NewStateObject);
    if (!NewStateInterface)
    {
        UE_LOG(LogTemp, Error, TEXT("UPlayerStateManager::ChangeState - State does not implement IPlayerCharacterState"));
        return nullptr;
    }

    // ステート開始処理
    NewStateInterface->OnEnter(GetOwner());

    // ScriptInterfaceにセット
    CurrentState.SetObject(NewStateObject);
    CurrentState.SetInterface(NewStateInterface);

    return CurrentState;
}

// 現在のステートが一致しているか確認
bool UPlayerStateManager::IsStateMatch(EPlayerStateType StateTag)
{
    if (!StateClassMap.Contains(StateTag) || !CurrentState.GetObject())
    {
        return false;
    }

    const TSubclassOf<UObject> TargetStateClass = StateClassMap[StateTag];
    return CurrentState.GetObject()->GetClass() == TargetStateClass;
}