#include "UI/UIManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Component/MyLibraryComponent.h"

void UUIManager::Init()
{
    // 全カテゴリのウィジェットを初期化
    InitAllWidgets();
}

void UUIManager::InitAllWidgets()
{
    // 各カテゴリに属するウィジェットをまとめて初期化
    for (auto& Pair : WidgetDataMap)
    {
        InitWidgetGroup(Pair.Value);
    }
}

void UUIManager::InitWidgetGroup(FWidgetData& WidgetGroup)
{
    // 既存のインスタンスをクリア
    WidgetGroup.WidgetMap.Reset();

    // クラス定義に基づき、ウィジェットを生成して登録
    for (auto& ClassPair : WidgetGroup.WidgetClassMap)
    {
        if (!ClassPair.Value)
            continue;

        UUserWidget* NewWidget = CreateWidget<UUserWidget>(GetWorld(), ClassPair.Value);
        if (NewWidget)
        {
            NewWidget->RemoveFromParent(); // 念のため親から外してから登録
            WidgetGroup.WidgetMap.Add(ClassPair.Key, NewWidget);
        }
    }
}

void UUIManager::CreateWidgetArray(const TArray<TSubclassOf<UUserWidget>>& Classes, TArray<UUserWidget*>& Widgets)
{
    // 複数クラスからウィジェットを生成する汎用関数
    Widgets.Reset();

    for (auto& WidgetClass : Classes)
    {
        if (!WidgetClass)
            continue;

        UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
        if (Widget)
        {
            Widget->RemoveFromParent();
            Widgets.Add(Widget);
        }
    }
}

UUserWidget* UUIManager::ShowWidget(EWidgetCategory CategoryName, FName WidgetName)
{
    UUserWidget* widget = nullptr;

    // 該当カテゴリが存在しなければ無視
    if (!WidgetDataMap.Contains(CategoryName))
        return widget;

    FWidgetData& Group = WidgetDataMap[CategoryName];
    UUserWidget** FoundWidget = Group.WidgetMap.Find(WidgetName);
    if (FoundWidget == nullptr)
        return widget;

    // すでに表示中のウィジェットなら再利用
    if (!Group.CurrentWidget.IsEmpty())
    {
        if (Group.CurrentWidget.Contains(WidgetName))
        {
            Group.CurrentWidget[WidgetName]->AddToViewport();
            return Group.CurrentWidget[WidgetName];
        }
    }

    // 新しく表示リストに追加
    Group.CurrentWidget.Add(WidgetName, *FoundWidget);
    Group.CurrentWidget[WidgetName]->AddToViewport();

    widget = Group.CurrentWidget[WidgetName];
    return widget;
}

const void UUIManager::HideCurrentWidget(EWidgetCategory CategoryName, FName WidgetName)
{
    // カテゴリが存在しなければ無視
    if (!WidgetDataMap.Contains(CategoryName))
        return;

    FWidgetData& Group = WidgetDataMap[CategoryName];

    if (!Group.CurrentWidget.Contains(WidgetName))
        return;

    // ビューポートから削除
    RemoveWidgetFromViewport(Group.CurrentWidget[WidgetName]);
}

bool UUIManager::IsWidgetVisible(EWidgetCategory CategoryName, FName WidgetName) const
{
    // 指定カテゴリが存在するかチェック
    const FWidgetData* Group = WidgetDataMap.Find(CategoryName);
    if (!Group)
        return false;
    if (!Group->CurrentWidget[WidgetName])
        return false;

    return true;
}

UUserWidget* UUIManager::GetWidget(EWidgetCategory CategoryName, FName WidgetName)
{
    // 該当カテゴリが存在しなければ無視
    if (!WidgetDataMap.Contains(CategoryName))
        return nullptr;

    FWidgetData& Group = WidgetDataMap[CategoryName];
    UUserWidget** FoundWidget = Group.WidgetMap.Find(WidgetName);
    if (!FoundWidget)
        return nullptr;

    return *FoundWidget;
}

bool UUIManager::PlayWidgetAnimation(EWidgetCategory CategoryName, FName WidgetName, FName AnimationName)
{
    if (!WidgetDataMap.Contains(CategoryName))
        return false;

    FWidgetData& Group = WidgetDataMap[CategoryName];
    UUserWidget** FoundWidgetPtr = Group.CurrentWidget.Find(WidgetName);
    if (!FoundWidgetPtr || !*FoundWidgetPtr)
        return false;

    return true;
}

void UUIManager::RemoveWidgetFromViewport(UUserWidget*& Widget)
{
    // ウィジェットをビューポートから削除
    if (Widget)
    {
        Widget->RemoveFromViewport();
    }
}

UUIWidgetBase* UUIManager::GetWidgetAsBase(EWidgetCategory CategoryName, FName WidgetName)
{
    return Cast<UUIWidgetBase>(GetWidget(CategoryName, WidgetName));
}