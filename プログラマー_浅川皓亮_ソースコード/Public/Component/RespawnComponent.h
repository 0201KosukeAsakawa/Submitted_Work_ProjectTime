// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RespawnComponent.generated.h"

/**
 * @brief キャラクターのリスポーン位置を管理するコンポーネント
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CARRY_API URespawnComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /**
     * @brief コンストラクタ：デフォルト値を設定
     */
    URespawnComponent();

protected:
    /**
     * @brief ゲーム開始時に呼ばれる初期化処理
     */
    virtual void BeginPlay() override;

private:
    /**
     * @brief 毎フレーム呼ばれる処理（必要に応じて使用）
     * @param DeltaTime 前フレームからの経過時間
     * @param TickType レベルのティック種類
     * @param ThisTickFunction 現在のティック関数
     */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
    /**
     * @brief リスポーン位置を更新する
     * @param nextPosition 新しいリスポーン座標
     */
    void SetRespawnPosition(FVector nextPosition);

    /**
     * @brief 設定されたリスポーン位置にキャラクターを復活させる
     */
    void Respawn();

private:
    /** 現在設定されているリスポーン位置 */
    FVector mRespawnPosition;
};
