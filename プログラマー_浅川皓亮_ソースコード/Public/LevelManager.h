// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelManager.generated.h"

class USoundManager;
class IUIManagerProvider;
class ISoundManagerProvider;
class UPostProcessEffectManager;
class UUIManager;

/**
 * @brief マネージャーアクセス用のキークラス
 *
 * このクラスのインスタンスはHandleクラスのみが生成可能
 * キーを持っているクラスのみがマネージャーを取得できる
 */
class FManagerAccessKey
{
private:
    // コンストラクタをprivateに
    FManagerAccessKey() = default;

    // コピー・ムーブを禁止
    FManagerAccessKey(const FManagerAccessKey&) = delete;
    FManagerAccessKey& operator=(const FManagerAccessKey&) = delete;
    FManagerAccessKey(FManagerAccessKey&&) = delete;
    FManagerAccessKey& operator=(FManagerAccessKey&&) = delete;

    // Handleクラスのみがキーを生成可能
    friend class UPostProcessEffectHandle;
};

// ============================================
// LevelManager.h
// ============================================

UCLASS()
class CARRY_API ALevelManager : public AActor
{
    GENERATED_BODY()
    static TWeakObjectPtr<ALevelManager> instance;

public:
    ALevelManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    void LoadScene(FName SceneName);

    UFUNCTION(BlueprintCallable)
    static ALevelManager* GetInstance(UObject* WorldContext);

    // ============================================
    // キーを使った安全なアクセス
    // ============================================

    /**
     * @brief サウンドマネージャーを取得
     * @note これはインターフェイスで返す為、キーは必要なし
     * @return サウンドマネージャー
     */
    TScriptInterface<ISoundManagerProvider> GetSoundManager() const
    {
        return mSoundManager;
    }

    /**
     * @brief UIマネージャーを取得
     * @note これはインターフェイスで返す為、キーは必要なし
     * @return UIマネージャー
     */
    TScriptInterface<IUIManagerProvider> GetUIManager() const
    {
        return mUIManager;
    }

    /**
     * @brief ポストプロセスマネージャーを取得（キー必須）
     * @param Key アクセスキー（Handleクラスのみが所持）
     * @return ポストプロセスマネージャー
     */
    UPostProcessEffectManager* GetPostProcessEffectManager(const FManagerAccessKey& Key) const
    {
        return PostProcessEffectManager;
    }

private:
    UPROPERTY(EditAnywhere)
    TSubclassOf<USoundManager> SoundClass;

    UPROPERTY()
    TScriptInterface<ISoundManagerProvider> mSoundManager;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UUIManager> UIManagerClass;

    UPROPERTY()
    TScriptInterface<IUIManagerProvider> mUIManager;

    UPROPERTY(EditAnywhere)
    UPostProcessEffectManager* PostProcessEffectManager;
};