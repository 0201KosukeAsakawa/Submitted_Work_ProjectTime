// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Component/LevelEffectComponent.h"
#include "PostProcessEffectHandle.generated.h"



/**
 * @brief ポストプロセスエフェクトへの簡易アクセスを提供するハンドルクラス
 *
 * LevelManager経由でアクセスする必要がなく、グローバルに利用可能
 *
 * 使用例:
 *   UPostProcessEffectHandle::ActivateEffect(EPostProcessEffectTag::Damage);
 *   UPostProcessEffectHandle::DeactivateEffect(EPostProcessEffectTag::Damage);
 */
UCLASS()
class CARRY_API UPostProcessEffectHandle : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ============================================
    // エフェクト制御（簡易アクセス）
    // ============================================

    /**
     * @brief エフェクトを有効化
     * @param WorldContext ワールドコンテキスト（通常はthis）
     * @param Tag エフェクトタグ
     * @param bInstant 即座に表示するか
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static void ActivateEffect(UObject* WorldContext, EPostProcessEffectTag Tag, bool bInstant = false);

    /**
     * @brief エフェクトを無効化
     * @param WorldContext ワールドコンテキスト
     * @param Tag エフェクトタグ
     * @param bInstant 即座に非表示にするか
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static void DeactivateEffect(UObject* WorldContext, EPostProcessEffectTag Tag, bool bInstant = false);

    /**
     * @brief すべてのエフェクトをクリア
     * @param WorldContext ワールドコンテキスト
     * @param bInstant 即座にクリアするか
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static void ClearAllEffects(UObject* WorldContext, bool bInstant = false);

    /**
     * @brief エフェクトがアクティブか確認
     * @param WorldContext ワールドコンテキスト
     * @param Tag エフェクトタグ
     * @return アクティブならtrue
     */
    UFUNCTION(BlueprintPure, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static bool IsEffectActive(UObject* WorldContext, EPostProcessEffectTag Tag);

    /**
     * @brief エフェクトのウェイトを設定
     * @param WorldContext ワールドコンテキスト
     * @param Tag エフェクトタグ
     * @param Weight ウェイト（0.0 ~ 1.0）
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static void SetEffectWeight(UObject* WorldContext, EPostProcessEffectTag Tag, float Weight);

    /**
     * @brief マテリアルのスカラーパラメータを設定
     * @param WorldContext ワールドコンテキスト
     * @param Tag エフェクトタグ
     * @param ParameterName パラメータ名
     * @param Value 値
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static void SetEffectScalarParameter(UObject* WorldContext, EPostProcessEffectTag Tag, FName ParameterName, float Value);

    /**
     * @brief マテリアルのベクトルパラメータを設定
     * @param WorldContext ワールドコンテキスト
     * @param Tag エフェクトタグ
     * @param ParameterName パラメータ名
     * @param Value 値
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static void SetEffectVectorParameter(UObject* WorldContext, EPostProcessEffectTag Tag, FName ParameterName, FLinearColor Value);

    /**
     * @brief マテリアルのテクスチャパラメータを設定
     * @param WorldContext ワールドコンテキスト
     * @param Tag エフェクトタグ
     * @param ParameterName パラメータ名
     * @param Value テクスチャ
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process", meta = (WorldContext = "WorldContext"))
    static void SetEffectTextureParameter(UObject* WorldContext, EPostProcessEffectTag Tag, FName ParameterName, UTexture* Value);

    // ============================================
    // 便利なプリセット関数
    // ============================================

    /**
     * @brief スローモーションエフェクトを開始
     * @param WorldContext ワールドコンテキスト
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process|Presets", meta = (WorldContext = "WorldContext"))
    static void StartSlowMotionEffect(UObject* WorldContext);

    /**
     * @brief スローモーションエフェクトを終了
     * @param WorldContext ワールドコンテキスト
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process|Presets", meta = (WorldContext = "WorldContext"))
    static void StopSlowMotionEffect(UObject* WorldContext);

    /**
    * @brief 中央から広がるトランジションエフェクトを開始
    * @param WorldContext ワールドコンテキスト
    * @param Duration トランジション時間（秒）
    * @param OnComplete 完了時のコールバック（オプション）
    */
    UFUNCTION(BlueprintCallable, Category = "Post Process|Transition", meta = (WorldContext = "WorldContext"))
    static void StartRadialTransition(UObject* WorldContext, float Duration = 1.0f);

    /**
     * @brief トランジションを即座に完了
     * @param WorldContext ワールドコンテキスト
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process|Transition", meta = (WorldContext = "WorldContext"))
    static void CompleteRadialTransition(UObject* WorldContext);

    /**
     * @brief トランジションエフェクトをリセット（開く）
     * @param WorldContext ワールドコンテキスト
     * @param Duration トランジション時間（秒）
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process|Transition", meta = (WorldContext = "WorldContext"))
    static void ReverseRadialTransition(UObject* WorldContext, float Duration = 1.0f);

    /**
     * @brief カスタムトランジション（進行度を手動制御）
     * @param WorldContext ワールドコンテキスト
     * @param Progress 進行度（0.0 = 完全に開いている, 1.0 = 完全に閉じている）
     */
    UFUNCTION(BlueprintCallable, Category = "Post Process|Transition", meta = (WorldContext = "WorldContext"))
    static void SetTransitionProgress(UObject* WorldContext, float Progress);


private:
    /**
     * @brief PostProcessEffectManagerを取得
     * @param WorldContext ワールドコンテキスト
     * @return マネージャーのポインタ（nullptrの可能性あり）
     */
    static UPostProcessEffectManager* GetManager(UObject* WorldContext);

    private:
        /** トランジション用の内部構造体 */
        struct FRadialTransitionData
        {
            float CurrentProgress = 0.0f;
            float TargetProgress = 0.0f;
            float Duration = 1.0f;
            float ElapsedTime = 0.0f;
            bool bIsActive = false;
            FTimerHandle TimerHandle;
        };

        /** ワールドごとのトランジションデータを保持 */
        static TMap<TWeakObjectPtr<UWorld>, FRadialTransitionData> TransitionDataMap;

        /** トランジション更新処理 */
        static void UpdateRadialTransition(UObject* WorldContext, float DeltaTime);
};
