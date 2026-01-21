// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE5Coro.h"
#include "LevelEffectComponent.generated.h"

class UPostProcessComponent;

/**
 * @brief ポストプロセスエフェクトのタグ
 */
UENUM(BlueprintType)
enum class EPostProcessEffectTag : uint8
{
    None            UMETA(DisplayName = "None"),
    Recording       UMETA(DisplayName = "Recording"),
    Rewinding       UMETA(DisplayName = "Rewinding"),
    SlowMotion      UMETA(DisplayName = "Slow Motion"),
    Damage          UMETA(DisplayName = "Damage"),
    LowHealth       UMETA(DisplayName = "Low Health"),
    Boost           UMETA(DisplayName = "Boost"),
    WallRun         UMETA(DisplayName = "Wall Run"),
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    UMaterialInterface* Material = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BlendWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Priority = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0.0"))
    float FadeInDuration = 0.2f;

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

    EPostProcessEffectTag Tag = EPostProcessEffectTag::None;

    UPROPERTY()
    UMaterialInstanceDynamic* MaterialInstance = nullptr;

    float CurrentWeight = 0.0f;
    float TargetWeight = 1.0f;
    int32 Priority = 50;
};

/**
 * @brief ポストプロセスエフェクト管理クラス（UE5Coro版）
 *
 * プレイヤーのポストプロセスエフェクトを一元管理。
 * コルーチンベースでフェード処理を実装し、Tick依存を排除。
 *
 * **主な責務**
 * - タグベースでエフェクトを制御
 * - フェードイン/アウト対応
 * - 優先度システム
 * - 重複防止
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UPostProcessEffectManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UPostProcessEffectManager();

protected:
    // ============================================
    // Unreal Overrides
    // ============================================

    virtual void BeginPlay() override;

public:
    // ============================================
    // Public API
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
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void SetEffectScalarParameter(EPostProcessEffectTag Tag, FName ParameterName, float Value);

    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void SetEffectVectorParameter(EPostProcessEffectTag Tag, FName ParameterName, FLinearColor Value);

    UFUNCTION(BlueprintCallable, Category = "Post Process")
    void SetEffectTextureParameter(EPostProcessEffectTag Tag, FName ParameterName, UTexture* Value);

private:
    // ============================================
    // Internal Logic
    // ============================================

    /** @brief ポストプロセスコンポーネントを初期化 */
    void InitializePostProcess();

    /** @brief アクティブエフェクトを取得 */
    FActivePostProcessEffect* FindActiveEffect(EPostProcessEffectTag Tag);

    /** @brief アクティブエフェクトを優先度順にソート */
    void SortActiveEffects();

    /** @brief PostProcessComponentを更新 */
    void RefreshPostProcessSettings();

    /** @brief フェードインコルーチン */
    UE5Coro::TCoroutine<> FadeInEffect(EPostProcessEffectTag Tag, float Duration);

    /** @brief フェードアウトコルーチン */
    UE5Coro::TCoroutine<> FadeOutEffect(EPostProcessEffectTag Tag, float Duration);

    /** @brief ウェイト変更コルーチン */
    UE5Coro::TCoroutine<> ChangeEffectWeight(EPostProcessEffectTag Tag, float TargetWeight, float Duration);

private:
    // ============================================
    // Settings
    // ============================================

    /** @brief エフェクト設定マップ（エディタで設定） */
    UPROPERTY(EditAnywhere, Category = "Post Process|Settings")
    TMap<EPostProcessEffectTag, FPostProcessEffectConfig> EffectConfigs;

    /** @brief ポストプロセス設定を継承するか */
    UPROPERTY(EditAnywhere, Category = "Post Process|Settings")
    bool bInheritPostProcessSettings = true;

    /** @brief ポストプロセスの優先度 */
    UPROPERTY(EditAnywhere, Category = "Post Process|Settings")
    float PostProcessPriority = 1.0f;

private:
    // ============================================
    // Cached References
    // ============================================

    /** @brief ポストプロセスコンポーネント */
    UPROPERTY()
    UPostProcessComponent* PostProcessComponent;

private:
    // ============================================
    // Runtime State
    // ============================================

    /** @brief アクティブなエフェクト一覧 */
    UPROPERTY()
    TArray<FActivePostProcessEffect> ActiveEffects;
};