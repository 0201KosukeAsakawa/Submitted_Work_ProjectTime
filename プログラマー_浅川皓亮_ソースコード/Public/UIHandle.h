// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interface/UIManagerProvider.h"
#include "UI/UIWidgetBase.h"
#include "UI/UIManager.h"
#include "UIHandle.generated.h"

/**
 * @brief UI操作用のハンドルクラス
 *
 * LevelManagerのUIManagerへの便利なアクセスを提供します。
 * Blueprint/C++の両方から使用可能です。
 */
UCLASS()
class CARRY_API UUIHandle : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // ============================
    // ==== ウィジェット表示系 ===
    // ============================

    /**
     * @brief 指定したカテゴリ・名前のウィジェットを表示
     * @param WorldContext ワールドコンテキスト
     * @param CategoryName ウィジェットカテゴリ（State, Combat, Inventory など）
     * @param WidgetName ウィジェット名
     * @return 表示された UUserWidget* ポインタ
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Widget", meta = (WorldContext = "WorldContext"))
    static UUserWidget* ShowWidget(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName);

    /**
     * @brief 指定カテゴリ・ウィジェットを非表示にする
     * @param WorldContext ワールドコンテキスト
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Widget", meta = (WorldContext = "WorldContext"))
    static void HideWidget(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName);

    /**
     * @brief 指定カテゴリ・ウィジェットが表示中かを取得
     * @param WorldContext ワールドコンテキスト
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @return bool 表示中かどうか
     */
    UFUNCTION(BlueprintPure, Category = "UI|Widget", meta = (WorldContext = "WorldContext"))
    static bool IsWidgetVisible(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName);

    /**
     * @brief 指定カテゴリ・ウィジェットの取得
     * @param WorldContext ワールドコンテキスト
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @return UUserWidget* ポインタ
     */
    UFUNCTION(BlueprintPure, Category = "UI|Widget", meta = (WorldContext = "WorldContext"))
    static UUserWidget* GetWidget(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName);

    /**
     * @brief ウィジェット内アニメーション再生
     * @param WorldContext ワールドコンテキスト
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @param AnimationName 再生するアニメーション名
     * @return bool 成功したか
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Animation", meta = (WorldContext = "WorldContext"))
    static bool PlayWidgetAnimation(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName, FName AnimationName);

    /**
    * @brief ウィジェットのプロパティを汎用的に設定（テンプレート版）
    * @tparam T 設定する値の型（float, int32, FText, FLinearColor, AActor* など）
    * @param WorldContext ワールドコンテキスト
    * @param CategoryName ウィジェットカテゴリ
    * @param WidgetName ウィジェット名
    * @param PropertyName 設定するプロパティ名
    * @param Value 設定する値
    * @return bool 成功したか
    */
    template<typename T>
    static bool SetWidgetProperty(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName, FName PropertyName, const T& Value)
    {
        if (UUIManager* UIManager = Cast<UUIManager>(GetUIManager(WorldContext).GetObject()))
        {
            return UIManager->SetWidgetProperty<T>(CategoryName, WidgetName, PropertyName, Value);
        }
        return false;
    }
    /**
     * @brief ウィジェットのFloatプロパティを設定（Blueprint用）
     * @param WorldContext ワールドコンテキスト
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @param PropertyName 設定するプロパティ名
     * @param Value 設定する値
     * @return bool 成功したか
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    static bool SetWidgetFloatProperty(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName, FName PropertyName, float Value);

    /**
     * @brief ウィジェットのTextプロパティを設定（Blueprint用）
     * @param WorldContext ワールドコンテキスト
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @param PropertyName 設定するプロパティ名
     * @param Value 設定する値
     * @return bool 成功したか
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    static bool SetWidgetTextProperty(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName, FName PropertyName, const FText& Value);

private:
    /**
     * @brief UIManagerを取得
     * @param WorldContext ワールドコンテキスト
     * @return UIManagerのインターフェース（取得失敗時はnullptr）
     */
    static TScriptInterface<IUIManagerProvider> GetUIManager(UObject* WorldContext);
};