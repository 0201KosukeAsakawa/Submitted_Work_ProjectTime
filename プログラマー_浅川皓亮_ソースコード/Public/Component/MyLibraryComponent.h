// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyLibraryComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API UMyLibraryComponent : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * @brief 指定した名前のコンポーネントを検索して返す
     * @tparam T 指定するコンポーネント型
     * @param TargetActor 対象のアクター
     * @param ComponentName 検索するコンポーネントの名前
     * @return 指定型にキャスト可能なコンポーネント、見つからなければ nullptr
     */
    template <typename T>
    static T* FindComponentByName(AActor* TargetActor, FName ComponentName);

    /**
     * @brief TMap内で指定した値に対応するキーを取得
     * @tparam A キー型
     * @tparam B 値型
     * @param Map 対象のTMap
     * @param Value 検索する値
     * @return 該当キー（存在しない場合は TOptional 空）
     */
    template <typename A, typename B>
    static TOptional<TPair<A, B>> GetTMapKey(const TMap<A, B>& Map, const B& Value);

    /**
     * @brief TMapから指定インデックスの要素を取得
     * @tparam A キー型
     * @tparam B 値型
     * @param Map 対象のTMap
     * @param idx 取得するインデックス
     * @return 該当要素（存在しない場合は TOptional 空）
     */
    template <typename A, typename B>
    static TOptional<TPair<A, B>> TakeElementBeIndex(const TMap<A, B>& Map, const int idx);

    /**
     * @brief TMapから指定インデックスのキーを取得
     * @tparam A キー型
     * @tparam B 値型
     * @param Map 対象のTMap
     * @param idx 取得するインデックス
     * @return 該当キー（存在しない場合は TOptional 空）
     */
    template <typename A, typename B>
    static TOptional<A> GetTMapKeyByIndex(const TMap<A, B>& Map, const int idx);

    /**
     * @brief 指定座標に近づいたか判定する
     * @param StartLocation 判定対象の現在位置
     * @param TargetLocation 目標座標
     * @param Threshold 許容距離
     * @return 目標に近ければ true、そうでなければ false
     */
    static bool IsNearTarget(const FVector StartLocation, const FVector TargetLocation, const float Threshold);
};

template <typename T>
static T* UMyLibraryComponent::FindComponentByName(AActor* TargetActor, FName ComponentName)
{
    if (!TargetActor)
        return nullptr;

    //全てのComponentを格納する配列
    TArray<UActorComponent*> Components;
    TargetActor->GetComponents(Components);

    //Componentの数分
    for (UActorComponent* Comp : Components)
    {
        // Componentがnull出ない　かつ　名前が同じである
        if (Comp && Comp->GetFName() == ComponentName)
        {
            // 指定された型でキャストできる
            T* targetComp = Cast<T>(Comp);
            if (targetComp)
            {
                //キャストしたものを返す
                return targetComp;
            }
        }
    }
    //見つからなかった
    return nullptr;
}

template <typename A, typename B>
static TOptional<TPair<A, B>> UMyLibraryComponent::GetTMapKey(const TMap<A, B>& Map, const B& Value)
{  
    // Bがポインタ型の場合
    if constexpr (std::is_pointer_v<B>)
    {
        // マップ内のすべての要素を走査して、値に対応するキーを返す
        for (const TPair<A, B>& Elem : Map)
        {
            if (*Elem.Value == *Value)  // ポインタ型の場合、デリファレンスして比較
            {
                return Elem.Key;  // 値に対応するキーを返す
            }
        }
    }
    else
    {
        // Bがポインタ型でない場合
        for (const TPair<A, B>& Elem : Map)
        {
            if (Elem.Value == Value)  // 通常の比較
            {
                return Elem.Key;  // 値に対応するキーを返す
            }
        }
    }

    // A型のデフォルト値を返す
    return TOptional<A>();
}

template <typename A, typename B>
static TOptional<TPair<A, B>> UMyLibraryComponent::TakeElementBeIndex(const TMap<A, B>& Map, const int idx)
{
    // TMapをTArrayに変換（要素を順番に格納）
    TArray<TPair<A, B>> Array = Map.Array();

    // インデックスが無効な場合、TOptionalを使って返す
    if (Array.Num() <= idx)
    {
        // 無効なインデックスの場合はTOptionalでemptyを返す
        return TOptional<TPair<A, B>>();
    }

    // 有効なインデックスの場合は、要素を返す
    return TOptional<TPair<A, B>>(Array[idx]);
}

template <typename A, typename B>
static TOptional<A> UMyLibraryComponent::GetTMapKeyByIndex(const TMap<A, B>& Map, const int idx)
{
    // TMapをTArrayに変換（キーとバリューのペアを順番に格納）
    TArray<TTuple<A, B>> ArrayTuple = Map.Array();

    // TArray<TTuple<A, B>> から TArray<TKeyValuePair<A, B>> に変換
    TArray<TKeyValuePair<A, B>> Array;
    for (const TTuple<A, B>& Tuple : ArrayTuple)
    {
        Array.Add(TKeyValuePair<A, B>(Tuple.Get<0>(), Tuple.Get<1>()));  // Tupleのキーと値をTKeyValuePairに変換
    }

    // インデックスが有効か確認
    if (Array.IsValidIndex(idx))
    {
        return Array[idx].Key; // N番目のキーを返す
    }

    // 無効な場合は空のTOptionalを返す
    return TOptional<A>();
}