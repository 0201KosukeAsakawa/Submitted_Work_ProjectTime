// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/RespawnComponent.h"

/**
 * @brief コンストラクタ：デフォルト値を設定
 */
URespawnComponent::URespawnComponent()
    : mRespawnPosition(FVector(0,0,0))
{
    // コンポーネントTickを必要に応じて有効化
    PrimaryComponentTick.bCanEverTick = true;
}

/**
 * @brief ゲーム開始時の初期化処理
 */
void URespawnComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner())
    {
        // 初期リスポーン位置は現在のキャラクター位置
        mRespawnPosition = GetOwner()->GetActorLocation();
    }
}

/**
 * @brief 毎フレーム呼ばれる処理
 */
void URespawnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // 現在は未使用。必要に応じて処理追加可
}

/**
 * @brief リスポーン位置を更新
 * @param nextPosition 新しいリスポーン座標
 */
void URespawnComponent::SetRespawnPosition(FVector nextPosition)
{
    mRespawnPosition = nextPosition;
}

/**
 * @brief 設定されたリスポーン位置にキャラクターを復活
 */
void URespawnComponent::Respawn()
{
    if (GetOwner() == nullptr)
        return;

    // 設定されたリスポーン位置に移動
    GetOwner()->SetActorLocation(mRespawnPosition);
}