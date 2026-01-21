#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStateManager.generated.h"

class IPlayerCharacterState;

// ============================================================
// プレイヤーの状態タイプを定義する列挙体
// 各状態はプレイヤーの挙動や入力処理の分岐に利用される
// ============================================================

UENUM(BlueprintType)
enum class EPlayerStateType : uint8
{
    /** 通常状態（地上での待機・移動など） */
    Default  UMETA(DisplayName = "Default"),

    /** 逆再生中*/
    Rewinding    UMETA(DisplayName = "Rewinding"),

    /** プレイヤーが死亡した状態 */
    Dead     UMETA(DisplayName = "Dead"),


    /** 着地硬直状態*/
    Landing UMETA(DisplayName = "Landing"),
    // 新しい状態を追加する場合はここに追記
    // 例: Dash, Swim, Attack など
};

// プレイヤーの状態管理クラス
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Blueprintable)
class CARRY_API UPlayerStateManager : public UActorComponent
{
    GENERATED_BODY()

public:
    /**
     * @brief コンストラクタ。GreenStateManagerの初期化（デフォルト値設定）
     */
    UPlayerStateManager();

    /**
     * @brief ゲーム開始時の初期化処理
     */
    void Init();

    /**
     * @brief 毎フレーム呼び出される更新処理（Tick 相当）
     *
     * @param DeltaTime 前フレームからの経過時間
     */
    void Update(float DeltaTime);

    /**
     * @brief 指定ステートタグのステートに切り替える
     *
     * @param NextStateTag 遷移先ステートのタグ
     * @return 遷移したステートインスタンス
     */
    TScriptInterface<IPlayerCharacterState> ChangeState(EPlayerStateType NextStateTag);

    /**
     * @brief 現在のステートが指定タグと一致するか確認
     *
     * @param StateTag チェックするステートタグ
     * @return 一致する場合 true
     */
    bool IsStateMatch(EPlayerStateType StateTag);

    /**
     * @brief 現在のアクティブステートを取得
     *
     * @return 現在のステートインスタンス
     */
    inline TScriptInterface<IPlayerCharacterState> GetCurrentState()const { return CurrentState; }

protected:
    /** @brief ステートタグとステートクラスのマップ（ステート生成用） */
    UPROPERTY(EditAnywhere)
    TMap<EPlayerStateType, TSubclassOf<UObject>> StateClassMap;

    /** @brief 現在アクティブなステート */
    UPROPERTY()
    TScriptInterface<IPlayerCharacterState> CurrentState;
};