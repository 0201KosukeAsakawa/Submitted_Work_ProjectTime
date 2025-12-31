// Fill out your copyright notice in the Description page of Project Settings.


#include "UIHandle.h"
#include "LevelManager.h"

TScriptInterface<IUIManagerProvider> UUIHandle::GetUIManager(UObject* WorldContext)
{
    if (!WorldContext)
    {
        UE_LOG(LogTemp, Warning, TEXT("UIHandle: WorldContext is null"));
        return nullptr;
    }

    ALevelManager* LevelManager = ALevelManager::GetInstance(WorldContext);
    if (!LevelManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("UIHandle: LevelManager not found"));
        return nullptr;
    }

    return LevelManager->GetUIManager();
}

UUserWidget* UUIHandle::ShowWidget(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName)
{
    if (TScriptInterface<IUIManagerProvider> UIManager = GetUIManager(WorldContext))
    {
        return UIManager->ShowWidget(CategoryName, WidgetName);
    }
    return nullptr;
}

void UUIHandle::HideWidget(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName)
{
    if (TScriptInterface<IUIManagerProvider> UIManager = GetUIManager(WorldContext))
    {
        UIManager->HideCurrentWidget(CategoryName, WidgetName);
    }
}

bool UUIHandle::IsWidgetVisible(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName)
{
    if (TScriptInterface<IUIManagerProvider> UIManager = GetUIManager(WorldContext))
    {
        return UIManager->IsWidgetVisible(CategoryName, WidgetName);
    }
    return false;
}

UUserWidget* UUIHandle::GetWidget(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName)
{
    if (TScriptInterface<IUIManagerProvider> UIManager = GetUIManager(WorldContext))
    {
        return UIManager->GetWidget(CategoryName, WidgetName);
    }
    return nullptr;
}

bool UUIHandle::PlayWidgetAnimation(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName, FName AnimationName)
{
    if (TScriptInterface<IUIManagerProvider> UIManager = GetUIManager(WorldContext))
    {
        return UIManager->PlayWidgetAnimation(CategoryName, WidgetName, AnimationName);
    }
    return false;
}

bool UUIHandle::SetWidgetFloatProperty(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName, FName PropertyName, float Value)
{
    return SetWidgetProperty<float>(WorldContext, CategoryName, WidgetName, PropertyName, Value);
}

bool UUIHandle::SetWidgetTextProperty(UObject* WorldContext, EWidgetCategory CategoryName, FName WidgetName, FName PropertyName, const FText& Value)
{
    return SetWidgetProperty<FText>(WorldContext, CategoryName, WidgetName, PropertyName, Value);
}