// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UIManagerProvider.generated.h"

class UUIWidgetBase;

UENUM(BlueprintType)
enum class EWidgetCategory : uint8
{
    SkillIcon     UMETA(DisplayName = "SkillIcon"),
    Menu UMETA(DisplayName = "Menu"),
    Interactive UMETA(DisplayName = "Interactive")
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUIManagerProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CARRY_API IUIManagerProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    // ============================
    // ==== ウィジェット表示系 ===
    // ============================

    /**
     * @brief 指定したカテゴリ・名前のウィジェットを表示
     * @param CategoryName ウィジェットカテゴリ（State, Combat, Inventory など）
     * @param WidgetName ウィジェット名
     * @return 表示された UUserWidget* ポインタ
     */
    virtual UUserWidget* ShowWidget(EWidgetCategory CategoryName, FName WidgetName) { return nullptr;}

    /**
     * @brief 指定カテゴリ・ウィジェットを非表示にする
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     */
    virtual const void HideCurrentWidget(EWidgetCategory CategoryName, FName WidgetName) {}

    /**
     * @brief 指定カテゴリ・ウィジェットが表示中かを取得
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @return bool 表示中かどうか
     */
    virtual bool IsWidgetVisible(EWidgetCategory CategoryName, FName WidgetName) const { return false; }

    /**
     * @brief 指定カテゴリ・ウィジェットの取得
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @return UUserWidget* ポインタ
     */
    virtual UUserWidget* GetWidget(EWidgetCategory CategoryName, FName WidgetName) { return nullptr; }

    /**
     * @brief ウィジェット内アニメーション再生
     * @param CategoryName ウィジェットカテゴリ
     * @param WidgetName ウィジェット名
     * @param AnimationName 再生するアニメーション名
     * @return bool 成功したか
     */
    virtual bool PlayWidgetAnimation(EWidgetCategory CategoryName, FName WidgetName, FName AnimationName) { return false; }

    /**
    * @brief ウィジェットの値を変更可能にしたクラスを返す
    * @param WidgetName ウィジェット名
    * @return UUIWidgetBase* ポインタ
    */
    virtual UUIWidgetBase* GetWidgetAsBase(EWidgetCategory CategoryName, FName WidgetName) { return nullptr; }

};
