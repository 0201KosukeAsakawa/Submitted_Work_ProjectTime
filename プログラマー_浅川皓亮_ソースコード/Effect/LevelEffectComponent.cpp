// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LevelEffectComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UE5Coro.h"

using namespace UE5Coro;
using namespace UE5Coro::Latent;

// ============================================
// Constructor
// ============================================
UPostProcessEffectManager::UPostProcessEffectManager()
{
    PrimaryComponentTick.bCanEverTick = false;

    PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
    if (PostProcessComponent)
    {
        PostProcessComponent->bUnbound = true;
        PostProcessComponent->Priority = PostProcessPriority;
    }
}

// ============================================
// Lifecycle
// ============================================

// 処理の流れ:
// 1. ポストプロセスコンポーネントの初期化
void UPostProcessEffectManager::BeginPlay()
{
    Super::BeginPlay();
    InitializePostProcess();
}

// ============================================
// Initialization
// ============================================

// 処理の流れ:
// 1. PostProcessComponentの有効性確認
// 2. 初期化ログ出力
void UPostProcessEffectManager::InitializePostProcess()
{
    if (!PostProcessComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("PostProcessEffectManager: PostProcessComponent is null"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Initialized with %d effect configs"), EffectConfigs.Num());
}

// ============================================
// Effect Control
// ============================================

// 処理の流れ:
// 1. タグとConfigの有効性確認
// 2. すでにアクティブなら何もしない
// 3. 新しいエフェクトを作成
// 4. 即座に表示 or フェードイン
void UPostProcessEffectManager::ActivateEffect(EPostProcessEffectTag Tag, bool bInstant)
{
    if (Tag == EPostProcessEffectTag::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectManager: Cannot activate None tag"));
        return;
    }

    const FPostProcessEffectConfig* Config = EffectConfigs.Find(Tag);
    if (!Config || !Config->Material)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectManager: No config or material for tag %d"), static_cast<int32>(Tag));
        return;
    }

    if (FindActiveEffect(Tag))
    {
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Effect %d already active"), static_cast<int32>(Tag));
        return;
    }

    FActivePostProcessEffect NewEffect;
    NewEffect.Tag = Tag;
    NewEffect.MaterialInstance = UMaterialInstanceDynamic::Create(Config->Material, this);
    NewEffect.TargetWeight = Config->BlendWeight;
    NewEffect.Priority = Config->Priority;
    NewEffect.CurrentWeight = bInstant ? Config->BlendWeight : 0.0f;

    ActiveEffects.Add(NewEffect);
    SortActiveEffects();
    RefreshPostProcessSettings();

    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Activated effect %d (Instant: %d)"),
        static_cast<int32>(Tag), bInstant);

    if (!bInstant)
    {
        FadeInEffect(Tag, Config->FadeInDuration);
    }
}

// 処理の流れ:
// 1. エフェクトの存在確認
// 2. 即座に削除 or フェードアウト
void UPostProcessEffectManager::DeactivateEffect(EPostProcessEffectTag Tag, bool bInstant)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (!Effect)
    {
        return;
    }

    if (bInstant)
    {
        ActiveEffects.RemoveAll([Tag](const FActivePostProcessEffect& E) { return E.Tag == Tag; });
        RefreshPostProcessSettings();
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Instantly deactivated effect %d"), static_cast<int32>(Tag));
    }
    else
    {
        const FPostProcessEffectConfig* Config = EffectConfigs.Find(Tag);
        const float Duration = Config ? Config->FadeOutDuration : 0.2f;
        FadeOutEffect(Tag, Duration);
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Fading out effect %d"), static_cast<int32>(Tag));
    }
}

// 処理の流れ:
// 1. 即座にクリア or 全エフェクトをフェードアウト
void UPostProcessEffectManager::ClearAllEffects(bool bInstant)
{
    if (bInstant)
    {
        ActiveEffects.Empty();
        RefreshPostProcessSettings();
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Cleared all effects instantly"));
    }
    else
    {
        TArray<EPostProcessEffectTag> Tags;
        for (const auto& Effect : ActiveEffects)
        {
            Tags.Add(Effect.Tag);
        }

        for (const auto& Tag : Tags)
        {
            DeactivateEffect(Tag, false);
        }
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Fading out all effects"));
    }
}

bool UPostProcessEffectManager::IsEffectActive(EPostProcessEffectTag Tag) const
{
    return ActiveEffects.ContainsByPredicate([Tag](const FActivePostProcessEffect& E)
        {
            return E.Tag == Tag;
        });
}

// 処理の流れ:
// 1. エフェクトの存在確認
// 2. ウェイト変更コルーチンを開始
void UPostProcessEffectManager::SetEffectWeight(EPostProcessEffectTag Tag, float Weight)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (!Effect)
    {
        return;
    }

    const float ClampedWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
    ChangeEffectWeight(Tag, ClampedWeight, 0.2f);
}

void UPostProcessEffectManager::SetEffectScalarParameter(EPostProcessEffectTag Tag, FName ParameterName, float Value)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (Effect && Effect->MaterialInstance)
    {
        Effect->MaterialInstance->SetScalarParameterValue(ParameterName, Value);
    }
}

void UPostProcessEffectManager::SetEffectVectorParameter(EPostProcessEffectTag Tag, FName ParameterName, FLinearColor Value)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (Effect && Effect->MaterialInstance)
    {
        Effect->MaterialInstance->SetVectorParameterValue(ParameterName, Value);
    }
}

void UPostProcessEffectManager::SetEffectTextureParameter(EPostProcessEffectTag Tag, FName ParameterName, UTexture* Value)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (Effect && Effect->MaterialInstance)
    {
        Effect->MaterialInstance->SetTextureParameterValue(ParameterName, Value);
    }
}

// ============================================
// Internal Methods
// ============================================

// 処理の流れ:
// 1. Blendableをクリア
// 2. アクティブエフェクトをPostProcessComponentに追加
void UPostProcessEffectManager::RefreshPostProcessSettings()
{
    if (!PostProcessComponent)
    {
        return;
    }

    PostProcessComponent->Settings.WeightedBlendables.Array.Empty();

    for (const auto& Effect : ActiveEffects)
    {
        if (Effect.MaterialInstance && Effect.CurrentWeight > 0.0f)
        {
            FWeightedBlendable Blendable;
            Blendable.Object = Effect.MaterialInstance;
            Blendable.Weight = Effect.CurrentWeight;
            PostProcessComponent->Settings.WeightedBlendables.Array.Add(Blendable);
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("PostProcessEffectManager: Refreshed with %d active effects"), ActiveEffects.Num());
}

FActivePostProcessEffect* UPostProcessEffectManager::FindActiveEffect(EPostProcessEffectTag Tag)
{
    return ActiveEffects.FindByPredicate([Tag](const FActivePostProcessEffect& E)
        {
            return E.Tag == Tag;
        });
}

// 処理の流れ:
// 1. 優先度でソート（高い順）
void UPostProcessEffectManager::SortActiveEffects()
{
    ActiveEffects.Sort([](const FActivePostProcessEffect& A, const FActivePostProcessEffect& B)
        {
            return A.Priority > B.Priority;
        });
}

// ============================================
// Coroutines
// ============================================

// 処理の流れ:
// 1. エフェクトの存在確認
// 2. ウェイトを0から目標値まで補間
// 3. 毎フレームPostProcessを更新
TCoroutine<> UPostProcessEffectManager::FadeInEffect(EPostProcessEffectTag Tag, float Duration)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (!Effect)
    {
        co_return;
    }

    const float TargetWeight = Effect->TargetWeight;
    const float FadeSpeed = Duration > 0.0f ? (1.0f / Duration) : 10.0f;

    while (Effect->CurrentWeight < TargetWeight - 0.01f)
    {
        co_await NextTick();

        Effect = FindActiveEffect(Tag);
        if (!Effect)
        {
            co_return;
        }

        Effect->CurrentWeight = FMath::FInterpTo(
            Effect->CurrentWeight,
            TargetWeight,
            GetWorld()->GetDeltaSeconds(),
            FadeSpeed
        );

        RefreshPostProcessSettings();
    }

    if (Effect)
    {
        Effect->CurrentWeight = TargetWeight;
        RefreshPostProcessSettings();
    }
}

// 処理の流れ:
// 1. エフェクトの存在確認
// 2. ウェイトを目標値から0まで補間
// 3. 完了したらエフェクトを削除
TCoroutine<> UPostProcessEffectManager::FadeOutEffect(EPostProcessEffectTag Tag, float Duration)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (!Effect)
    {
        co_return;
    }

    const float FadeSpeed = Duration > 0.0f ? (1.0f / Duration) : 10.0f;

    while (Effect && Effect->CurrentWeight > 0.01f)
    {
        co_await NextTick();

        Effect = FindActiveEffect(Tag);
        if (!Effect)
        {
            co_return;
        }

        Effect->CurrentWeight = FMath::FInterpTo(
            Effect->CurrentWeight,
            0.0f,
            GetWorld()->GetDeltaSeconds(),
            FadeSpeed
        );

        RefreshPostProcessSettings();
    }

    ActiveEffects.RemoveAll([Tag](const FActivePostProcessEffect& E) { return E.Tag == Tag; });
    RefreshPostProcessSettings();
    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Removed effect %d"), static_cast<int32>(Tag));
}

// 処理の流れ:
// 1. エフェクトの存在確認
// 2. 現在のウェイトから目標値まで補間
TCoroutine<> UPostProcessEffectManager::ChangeEffectWeight(EPostProcessEffectTag Tag, float TargetWeight, float Duration)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (!Effect)
    {
        co_return;
    }

    const float FadeSpeed = Duration > 0.0f ? (1.0f / Duration) : 10.0f;

    while (FMath::Abs(Effect->CurrentWeight - TargetWeight) > 0.01f)
    {
        co_await NextTick();

        Effect = FindActiveEffect(Tag);
        if (!Effect)
        {
            co_return;
        }

        Effect->CurrentWeight = FMath::FInterpTo(
            Effect->CurrentWeight,
            TargetWeight,
            GetWorld()->GetDeltaSeconds(),
            FadeSpeed
        );

        RefreshPostProcessSettings();
    }

    if (Effect)
    {
        Effect->CurrentWeight = TargetWeight;
        Effect->TargetWeight = TargetWeight;
        RefreshPostProcessSettings();
    }
}