// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WallRunComponent.generated.h"

class UCameraComponent;
class IPlayerInfoProvider;
class UWallDetectionComponent;
class UWallRunLogicComponent;
struct FWallRunData;
struct  FTimeline;



/**
 * @brief 壁走り（WallRun）を管理するコンポーネント
 *
 * キャラクターが壁を検出してから壁走りを開始・維持・終了するまでの全体制御を行う。
 * 壁走り中は、速度・重力・カメラの挙動を補正し、滑らかな移動体験を提供する。
 *
 * 依存関係:
 * - UWallDetectionComponent : 壁の検出
 * - UWallRunLogicComponent   : 壁走り中の移動・姿勢ロジック
 * - IPlayerInfoProvider      : キャラクター情報アクセス（MovementComponentなど）
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UWallRunComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    /** コンストラクタ：デフォルト値を設定 */
    UWallRunComponent();

    // ============================================================
    // 公開インターフェース（外部から状態を参照可能）
    // ============================================================

    /**
     * @brief 壁走り中かどうかを返す
     * @return 壁走り状態なら true
     */
    bool IsWallRunning() const;

    /**
     * @brief 壁ジャンプ可能かを返す
     * @return 壁走り中にジャンプ可能であれば true
     */
    bool GetCanWallJump() const;

    /**
     * @brief 現在走っている壁の法線ベクトルを取得
     * @return 壁法線ベクトル（未検出時はゼロベクトル）
     */
    FVector GetWallNormal() const;

    /**
     * @brief ジャンプ入力が押されたときの処理を行う
     * @return 壁ジャンプを処理した場合 true（通常ジャンプは無効化可能）
     */
    bool HandleJumpPressed();

    void SetDetectionEnabled(bool bEnabled);

    /**
     * @brief 壁走りを終了する
     * @details 状態をリセットし、重力・速度・カメラを通常状態へ戻す
     */
    void ExitWallRun();
protected:
    // ============================================================
    // UEライフサイクル関数
    // ============================================================

    /** ゲーム開始時に初期化処理を行う */
    virtual void BeginPlay() override;

    /** フレームごとに更新処理を行う */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    // ============================================================
    // 内部処理関数群
    // ============================================================

    /** デバッグ用に壁走り状態を可視化 */
    void DrawDebugWallRun();
    /**
     * @brief コンポーネント初期化処理
     * @details 壁検出イベントの購読や PlayerInfoProvider のキャッシュを行う
     */
    void InitializeComponent();




    // ============================================================
    // 壁検出イベントハンドラ
    // ============================================================

    /**
     * @brief 壁が検出されたときの処理
     * @param Wall 検出された壁アクター
     * @param Hit 壁とのヒット情報
     */
    void HandleWallDetected(AActor* Wall, const FHitResult& Hit);

    /**
     * @brief 壁を見失ったときの処理
     * @param Wall 以前検出されていた壁アクター
     */
    void HandleWallLost(AActor* Wall);


    // ============================================================
    // 壁走り挙動制御
    // ============================================================

    /**
     * @brief 壁走りを開始する
     * @param WallHit 壁とのヒット情報（位置・法線など）
     */
    void BeginWallRun(const FHitResult& WallHit);

    /**
     * @brief 壁走り中の移動更新処理
     * @details 毎フレーム呼ばれ、キャラクターの速度・重力・方向を補正
     */
    void UpdateWallRunMovement();

    /**
     * @brief 壁走り中のジャンプを実行
     * @details 壁の法線方向へ跳ねる処理。通常のジャンプと差別化される。
     */
    void ExecuteWallJump();


    // ============================================================
    // 状態適用系処理
    // ============================================================

    /**
     * @brief 壁走り用の移動補正を適用
     * @param Data 壁走りロジックから提供される補正データ
     */
    void ApplyWallRunMovement(const FWallRunData& Data);

    /**
     * @brief 壁走り時のカメラ補正を適用
     * @param Data 壁走りロジックから提供される補正データ
     */
    void ApplyWallRunCamera(const FWallRunData& Data);

    /**
     * @brief 通常の移動設定に戻す
     */
    void ResetMovement();

    /**
     * @brief カメラ補正をリセット
     */
    void ResetCamera();


private:
    // ============================================================
    // サブコンポーネント参照
    // ============================================================

    /** 壁検出用コンポーネント（壁との距離・角度を判定） */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UWallDetectionComponent* WallDetector;

    /** 壁走り用ロジックコンポーネント（速度・方向計算を担当） */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UWallRunLogicComponent* Logic;

    /** プレイヤー情報アクセス用（移動コンポーネントや姿勢など） */
    UPROPERTY()
    TScriptInterface<IPlayerInfoProvider> PlayerInfoProvider;


    // ============================================================
    // 入力設定
    // ============================================================

    /** 壁ジャンプに使用する入力アクション名 */
    UPROPERTY(EditAnywhere, Category = "Input")
    FName JumpActionName;

    /** 入力を自動バインドするか（falseなら外部で手動バインド） */
    UPROPERTY(EditAnywhere, Category = "Input")
    bool bAutoBindInput;


    // ============================================================
    // 内部状態キャッシュ
    // ============================================================

    /** 壁走り前に保存する歩行速度（復元用） */
    float SavedMaxWalkSpeed;

    /** 壁走り前に保存する空中制御値 */
    float SavedAirControl;

    /** 壁走り前に保存する重力スケール */
    float SavedGravityScale;
};