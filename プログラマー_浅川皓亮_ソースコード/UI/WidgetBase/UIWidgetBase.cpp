// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UIWidgetBase.h"
#include "Component/MyLibraryComponent.h"
#include "Kismet/GameplayStatics.h"

// マクロで簡潔に実装
#define IMPL_WIDGET_SET_PROPERTY(Type, PropertyClass) \
template<> \
bool UUIWidgetBase::SetPropertyInternal<Type>(FProperty* Property, const Type& Value) \
{ \
    if (PropertyClass* Prop = CastField<PropertyClass>(Property)) \
    { \
        void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(this); \
        Prop->SetPropertyValue(ValuePtr, Value); \
        return true; \
    } \
    UE_LOG(LogTemp, Warning, TEXT("UIWidgetBase: Property type mismatch")); \
    return false; \
}

// 基本型
IMPL_WIDGET_SET_PROPERTY(int32, FIntProperty)
IMPL_WIDGET_SET_PROPERTY(FText, FTextProperty)
IMPL_WIDGET_SET_PROPERTY(FString, FStrProperty)
IMPL_WIDGET_SET_PROPERTY(FName, FNameProperty)
IMPL_WIDGET_SET_PROPERTY(bool, FBoolProperty)

// Float（Double対応）
template<>
bool UUIWidgetBase::SetPropertyInternal<float>(FProperty* Property, const float& Value)
{
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        void* ValuePtr = FloatProp->ContainerPtrToValuePtr<void>(this);
        FloatProp->SetPropertyValue(ValuePtr, Value);
        return true;
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        void* ValuePtr = DoubleProp->ContainerPtrToValuePtr<void>(this);
        DoubleProp->SetPropertyValue(ValuePtr, static_cast<double>(Value));
        return true;
    }
    return false;
}

// 構造体：FLinearColor
template<>
bool UUIWidgetBase::SetPropertyInternal<FLinearColor>(FProperty* Property, const FLinearColor& Value)
{
    if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
        {
            void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(this);
            *static_cast<FLinearColor*>(ValuePtr) = Value;
            return true;
        }
    }
    return false;
}

// 構造体：FVector
template<>
bool UUIWidgetBase::SetPropertyInternal<FVector>(FProperty* Property, const FVector& Value)
{
    if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (StructProp->Struct == TBaseStructure<FVector>::Get())
        {
            void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(this);
            *static_cast<FVector*>(ValuePtr) = Value;
            return true;
        }
    }
    return false;
}
