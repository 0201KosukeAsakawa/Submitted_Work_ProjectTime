// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WallDetectionComponent.generated.h"

// ============================================================
// デリゲート宣言
// ============================================================

/**
 * @brief 壁が検出された際に呼び出されるイベント
 * @param AActor* 検出された壁のアクター
 * @param FHitResult& 壁とのヒット情報
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnWallDetected, AActor*, const FHitResult&);

/**
 * @brief 壁を見失った際に呼び出されるイベント
 * @param AActor* 以前検出されていた壁のアクター
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWallLost, AActor*);


// ============================================================
// 壁検出設定構造体
// ============================================================

/**
 * @brief 壁検出の挙動を制御するパラメータ群
 */
USTRUCT(BlueprintType)
struct FWallDetectionSettings
{
    GENERATED_BODY()

    /** 検出距離（レイの長さ） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "10.0", ClampMax = "200.0"))
    float DetectionDistance = 100.0f;

    /** 検出の更新間隔（秒単位） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float DetectionInterval = 0.1f;

    /** 壁として認識する最小角度（法線の角度）。例: 60°以上であれば壁扱い */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MinWallAngle = 10.0f;

    /** デバッグ用にレイキャストの可視化を行うか */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDrawDebug = false;

    /** 壁走り可能な最小角度（度）。この値より小さい角度は正面すぎて無効 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Detection|Approach Angle",
        meta = (ClampMin = "0.0", ClampMax = "90.0",
            ToolTip = "この角度より小さい場合、正面すぎて壁走り不可（デフォルト30度）"))
    float MinValidAngle = 30.0f;

    /** 壁走り可能な最大角度（度）。この値より大きい角度は正面すぎて無効 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Detection|Approach Angle",
        meta = (ClampMin = "90.0", ClampMax = "180.0",
            ToolTip = "この角度より大きい場合、正面すぎて壁走り不可（デフォルト150度）"))
    float MaxValidAngle = 150.0f;
};


// ============================================================
// 壁検出コンポーネント
// ============================================================

/**
 * @class UWallDetectionComponent
 * @brief 壁をレイキャストで検出し、壁発見・喪失イベントを発行するコンポーネント
 * @details キャラクターやアクターに付与し、Tickごとに壁の有無をチェック。
 *          角度判定やデバッグ描画など、壁走り処理などと連携可能。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UWallDetectionComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    // ============================================================
    // コンストラクタ・基本関数
    // ============================================================

    /** デフォルトコンストラクタ */
    UWallDetectionComponent();

    /** コンポーネント初期化処理 */
    virtual void BeginPlay() override;

    /** フレーム更新処理（壁検出の更新） */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * @brief 壁走り中に壁がまだ存在するかチェック
     * @param CharacterLocation キャラクターの位置
     * @param TraceDistance トレース距離
     * @return 壁が検出されればtrue
     */
    bool CheckWallStillExists(const FVector& CharacterLocation, float TraceDistance = 150.0f) const;

    /**
     * @brief 壁走り方向（壁に向かって垂直な方向）を取得
     * @return 壁方向のベクトル
     */
    FVector GetWallDirection() const;

    // ============================================================
    // Detection Interface（外部から利用される関数群）
    // ============================================================

    /**
     * @brief 現在壁が検出されているか
     * @return 壁が検出されていれば true
     */
    UFUNCTION(BlueprintPure, Category = "Wall Detection")
    bool IsWallDetected() const { return bIsWallDetected; }

    /**
     * @brief 現在検出されている壁アクターを取得
     * @return 検出されている壁アクター、存在しない場合は nullptr
     */
    UFUNCTION(BlueprintPure, Category = "Wall Detection")
    AActor* GetDetectedWall() const { return CurrentWall.Get(); }

    /**
     * @brief 最後に検出したヒット情報を取得
     * @return 壁との最後のヒット結果
     */
    UFUNCTION(BlueprintPure, Category = "Wall Detection")
    FHitResult GetLastHitResult() const { return LastHitResult; }

    /**
     * @brief 壁検出の有効/無効を切り替える
     * @param bEnabled 有効化する場合は true
     */
    UFUNCTION(BlueprintCallable, Category = "Wall Detection")
    void SetDetectionEnabled(bool bEnabled);

    /**
     * @brief 壁検出が現在有効かどうか
     * @return 有効なら true
     */
    UFUNCTION(BlueprintPure, Category = "Wall Detection")
    bool IsDetectionEnabled() const { return bDetectionEnabled; }

    void SetWallDirection(const FVector& wallDirection) { CurrentWallNormal = wallDirection;}
    // ============================================================
    // イベント
    // ============================================================

    /** 壁を検出した際に発火するイベント */
    FOnWallDetected OnWallDetected;

    /** 壁を見失った際に発火するイベント */
    FOnWallLost OnWallLost;


private:
    // ============================================================
    // 内部処理関数群
    // ============================================================

    /**
     * @brief 壁検出のメイン処理
     * @details レイキャストを使用し、前方に壁があるか判定。
     *          一定間隔ごとに呼び出され、ヒット情報を更新。
     */
    void DetectWall();

    /**
     * @brief ヒット結果が「壁」として有効か判定
     * @param Hit チェック対象のヒット情報
     * @return 壁と認識できる場合 true
     */
    bool IsValidWall(const FHitResult& Hit) const;

    /**
     * @brief 実際にレイキャストを実行
     * @param Direction レイの発射方向
     * @param OutHit 結果として返すヒット情報
     * @return 壁にヒットした場合 true
     */
    bool PerformRaycast(const FVector& Direction, FHitResult& OutHit) const;

    /**
     * @brief デバッグ用にレイキャスト可視化を行う
     * @param Direction 発射方向
     * @param bHit 壁にヒットしたかどうか
     * @param Hit ヒット情報
     */
    void DrawDebug(const FVector& Direction, bool bHit, const FHitResult& Hit) const;


private:
    // ============================================================
    // 内部状態変数
    // ============================================================

    /** 現在壁を検出しているか */
    bool bIsWallDetected;

    /** 検出が有効化されているか */
    bool bDetectionEnabled;

    /** 現在検出している壁アクター（破棄検知対応） */
    TWeakObjectPtr<AActor> CurrentWall;

    /** 最後にヒットした壁との情報 */
    FHitResult LastHitResult;

    FVector CurrentWallNormal = FVector::ZeroVector;  // リセット

    /** 検出タイマー（DetectionInterval の制御用） */
    float DetectionTimer;

    // ============================================================
    // 設定
    // ============================================================

    /** 検出パラメータ設定構造体 */
    UPROPERTY(EditAnywhere, Category = "Wall Detection")
    FWallDetectionSettings Settings;
};