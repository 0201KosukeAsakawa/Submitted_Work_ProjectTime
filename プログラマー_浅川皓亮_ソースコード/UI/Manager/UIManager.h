#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"  // UUserWidget をインクルード
#include "UI/UIWidgetBase.h"
#include "Interface/UIManagerProvider.h"
#include "Containers/Map.h"
#include "UIManager.generated.h"

/**
 * 複数ウィジェットカテゴリ（例: State, Menu, HUD）に対応するデータ構造
 */
USTRUCT(BlueprintType)
struct FWidgetData : public FTableRowBase
{
    GENERATED_USTRUCT_BODY()

    // ウィジェット名に対応するウィジェットクラス
    UPROPERTY(EditAnywhere, Category = "UI")
    TMap<FName, TSubclassOf<UUserWidget>> WidgetClassMap;

    // 実行時に生成されたウィジェットのインスタンスを保持
    UPROPERTY(Transient)
    TMap<FName, UUserWidget*> WidgetMap;

    // 現在表示中のウィジェット
    UPROPERTY(Transient)
    TMap<FName, UUserWidget*> CurrentWidget;
};


/**
 * @brief ゲーム全体で UI を一元管理するクラス
 * HUD に依存せず、UObjectベースでウィジェット管理を行う
 */
UCLASS(Blueprintable)
class CARRY_API UUIManager : public UObject, public IUIManagerProvider
{
    GENERATED_BODY()

public:
    /**
     * @brief UIManagerの初期化
     */
    virtual void Init();

    // 汎用プロパティ設定（シンプルなラッパー）
    template<typename T>
    bool SetWidgetProperty(EWidgetCategory CategoryName, FName WidgetName, FName PropertyName, const T& Value);

private:
    // ============================
    // ==== ウィジェット表示系 ===
    // ============================

    /**
     * @brief 指定したカテゴリ・名前のウィジェットを表示
     * @param CategoryName ウィジェットカテゴリ（State, Combat, Inventory など）
     * @param WidgetName ウィジェット名
     * @return 表示された UUserWidget* ポインタ
     */
    UFUNCTION(BlueprintCallable)
    UUserWidget* ShowWidget(EWidgetCategory CategoryName, FName WidgetName)override;

    /**
     * @brief 指定カテゴリ・ウィジェットを非表示にする
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     */
    UFUNCTION(BlueprintCallable)
    const void HideCurrentWidget(EWidgetCategory CategoryName, FName WidgetName)override;

    /**
     * @brief 指定カテゴリ・ウィジェットが表示中かを取得
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @return bool 表示中かどうか
     */
    UFUNCTION(BlueprintCallable)
    bool IsWidgetVisible(EWidgetCategory CategoryName, FName WidgetName) const override;

    /**
     * @brief 指定カテゴリ・ウィジェットの取得
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @return UUserWidget* ポインタ
     */
    UFUNCTION(BlueprintCallable)
    UUserWidget* GetWidget(EWidgetCategory CategoryName, FName WidgetName)override;

    /**
     * @brief ウィジェット内アニメーション再生
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @param AnimationName 再生するアニメーション名
     * @return bool 成功したか
     */
    UFUNCTION()
    bool PlayWidgetAnimation(EWidgetCategory CategoryName, FName WidgetName, FName AnimationName)override;

    // ============================
    // ==== ウィジェット初期化系 ==
    // ============================

    /** @brief 全カテゴリのウィジェット初期化 */
    void InitAllWidgets();

    /**
     * @brief 指定カテゴリ内のウィジェットを生成しマップに登録
     * @param WidgetGroup 初期化対象のウィジェットデータ
     */
    void InitWidgetGroup(FWidgetData& WidgetGroup);

    /**
     * @brief 配列形式ウィジェットの生成（CrossHair等旧仕様対応）
     * @param WidgetClasses 生成するウィジェットクラス配列
     * @param OutWidgets 生成結果を格納する配列
     */
    void CreateWidgetArray(const TArray<TSubclassOf<UUserWidget>>& WidgetClasses, TArray<UUserWidget*>& OutWidgets);

    /**
     * @brief ウィジェットをビューポートから削除し nullptr にする
     * @param Widget 削除対象のウィジェット
     */
    void RemoveWidgetFromViewport(UUserWidget*& Widget);

    virtual UUIWidgetBase* GetWidgetAsBase(EWidgetCategory CategoryName, FName WidgetName) override;


private:
    // ============================
    // ==== 変数 ==================
    // ============================

    /** @brief ウィジェットカテゴリごとのデータマップ */
    UPROPERTY(EditAnywhere, Category = "UI")
    TMap<EWidgetCategory, FWidgetData> WidgetDataMap;

};

template<typename T>
bool UUIManager::SetWidgetProperty(EWidgetCategory CategoryName, FName WidgetName, FName PropertyName, const T& Value)
{
    if (UUIWidgetBase* Widget = GetWidgetAsBase(CategoryName, WidgetName))
    {
        return Widget->template SetProperty<T>(PropertyName, Value);
    }
    return false;
}