// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelEffectComponent.generated.h"


/**
 * @brief ポストプロセスエフェクトのタグ
 * @details エフェクトを識別するための列挙型
 */
UENUM(BlueprintType)
enum class EPostProcessEffectTag : uint8
{
    None            UMETA(DisplayName = "None"),
    Recording       UMETA(DisplayName = "Recording"),      // 記録中
    Rewinding       UMETA(DisplayName = "Rewinding"),      // 巻き戻し中
    SlowMotion      UMETA(DisplayName = "Slow Motion"),    // スロー
    Damage          UMETA(DisplayName = "Damage"),         // ダメージ
    LowHealth       UMETA(DisplayName = "Low Health"),     // 体力低下
    Boost           UMETA(DisplayName = "Boost"),          // ブースト
    WallRun         UMETA(DisplayName = "Wall Run"),       // 壁走り
    Custom1         UMETA(DisplayName = "Custom 1"),
    Custom2         UMETA(DisplayName = "Custom 2"),
    Custom3         UMETA(DisplayName = "Custom 3")
};

/**
 * @brief エフェクト設定
 */
USTRUCT(BlueprintType)
struct FPostProcessEffectConfig
{
    GENERATED_BODY()

    /** エフェクトのマテリアル */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    UMaterialInterface* Material = nullptr;

    /** ブレンドウェイト（0.0 ~ 1.0） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BlendWeight = 1.0f;

    /** 優先度（高いほど優先） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Priority = 50;

    /** フェードイン時間（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0.0"))
    float FadeInDuration = 0.2f;

    /** フェードアウト時間（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0.0"))
    float FadeOutDuration = 0.2f;
};

/**
 * @brief アクティブなエフェクト情報
 */
USTRUCT()
struct FActivePostProcessEffect
{
    GENERATED_BODY()

    /** エフェクトタグ */
    EPostProcessEffectTag Tag = EPostProcessEffectTag::None;

    /** 動的マテリアルインスタンス */
    UPROPERTY()
    UMaterialInstanceDynamic* MaterialInstance = nullptr;

    /** 現在のウェイト */
    float CurrentWeight = 0.0f;

    /** 目標ウェイト */
    float TargetWeight = 1.0f;

    /** フェード速度 */
    float FadeSpeed = 5.0f;

    /** 優先度 */
    int32 Priority = 50;

    /** フェード中か */
    bool bIsFading = false;

    /** フェードアウト中か */
    bool bIsFadingOut = false;
};

/**
 * @brief ポストプロセスエフェクト管理クラス
 * @details プレイヤーのポストプロセスエフェクトを一元管理
 *          - タグベースでエフェクトを制御
 *          - フェードイン/アウト対応
 *          - 優先度システム
 *          - 重複防止
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UPostProcessEffectManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UPostProcessEffectManager();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
    // ============================================
    // Effect Control
    // ============================================

    /**
     * @brief エフェクトを有効化
     * @param Tag エフェクトタグ
     * @param bInstant true: 即座に表示、false: フェードイン
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void ActivateEffect(EPostProcessEffectTag Tag, bool bInstant = false);

    /**
     * @brief エフェクトを無効化
     * @param Tag エフェクトタグ
     * @param bInstant true: 即座に非表示、false: フェードアウト
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void DeactivateEffect(EPostProcessEffectTag Tag, bool bInstant = false);

    /**
     * @brief すべてのエフェクトをクリア
     * @param bInstant true: 即座にクリア、false: フェードアウト
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void ClearAllEffects(bool bInstant = false);

    /**
     * @brief エフェクトがアクティブか確認
     */
    UFUNCTION(BlueprintPure, Category = "Post Process")
    bool IsEffectActive(EPostProcessEffectTag Tag) const;

    /**
     * @brief エフェクトのウェイトを設定
     * @param Tag エフェクトタグ
     * @param Weight ウェイト（0.0 ~ 1.0）
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void SetEffectWeight(EPostProcessEffectTag Tag, float Weight);

    /**
     * @brief マテリアルパラメータを設定
     * @param Tag エフェクトタグ
     * @param ParameterName パラメータ名
     * @param Value 値
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void SetEffectScalarParameter(EPostProcessEffectTag Tag, FName ParameterName, float Value);

    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void SetEffectVectorParameter(EPostProcessEffectTag Tag, FName ParameterName, FLinearColor Value);

    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void SetEffectTextureParameter(EPostProcessEffectTag Tag, FName ParameterName, UTexture* Value);

private:
    /**
     * @brief ポストプロセスコンポーネントを初期化
     */
    void InitializePostProcess();

    /**
     * @brief エフェクトを更新（フェード処理）
     */
    void UpdateEffects(float DeltaTime);

    /**
     * @brief PostProcessComponentを更新
     */
    void RefreshPostProcessSettings();

    /**
     * @brief アクティブエフェクトを取得
     */
    FActivePostProcessEffect* FindActiveEffect(EPostProcessEffectTag Tag);

    /**
     * @brief アクティブエフェクトを優先度順にソート
     */
    void SortActiveEffects();

private:
    /** ポストプロセスコンポーネント */
    UPROPERTY()
    class UPostProcessComponent* PostProcessComponent;

    /** アクティブなエフェクト一覧 */
    UPROPERTY()
    TArray<FActivePostProcessEffect> ActiveEffects;

    /** PostProcess設定の更新が必要か */
    bool bNeedsRefresh;


    // ============================================
    // Settings
    // ============================================

    /** エフェクト設定マップ（エディタで設定） */
    UPROPERTY(EditAnywhere, Category = "Post Process|Settings")
    TMap<EPostProcessEffectTag, FPostProcessEffectConfig> EffectConfigs;

    /** ポストプロセス設定を継承するか */
    UPROPERTY(EditAnywhere, Category = "Post Process|Settings")
    bool bInheritPostProcessSettings = true;

    /** ポストプロセスの優先度 */
    UPROPERTY(EditAnywhere, Category = "Post Process|Settings")
    float PostProcessPriority = 1.0f;


};