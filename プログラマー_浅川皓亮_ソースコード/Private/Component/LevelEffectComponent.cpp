// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LevelEffectComponent.h"
#include "Components/PostProcessComponent.h"

// ============================================
// Constructor
// ============================================
UPostProcessEffectManager::UPostProcessEffectManager()
    : bNeedsRefresh(false)
{
    PrimaryComponentTick.bCanEverTick = true;

    // ポストプロセスコンポーネントを作成
    PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
    if (PostProcessComponent)
    {
        PostProcessComponent->bUnbound = true;  // 全画面適用
        PostProcessComponent->Priority = PostProcessPriority;
    }
}

// ============================================
// Lifecycle
// ============================================
void UPostProcessEffectManager::BeginPlay()
{
    Super::BeginPlay();
    InitializePostProcess();
}

void UPostProcessEffectManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // エフェクトの更新（フェード処理）
    UpdateEffects(DeltaTime);

    // PostProcess設定の更新が必要な場合
    if (bNeedsRefresh)
    {
        RefreshPostProcessSettings();
        bNeedsRefresh = false;
    }
}

// ============================================
// Initialization
// ============================================
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
void UPostProcessEffectManager::ActivateEffect(EPostProcessEffectTag Tag, bool bInstant)
{
    if (Tag == EPostProcessEffectTag::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectManager: Cannot activate None tag"));
        return;
    }

    // 設定を取得
    FPostProcessEffectConfig* Config = EffectConfigs.Find(Tag);
    if (!Config || !Config->Material)
    {
        UE_LOG(LogTemp, Warning, TEXT("PostProcessEffectManager: No config or material for tag %d"), static_cast<int32>(Tag));
        return;
    }

    // すでにアクティブか確認
    FActivePostProcessEffect* ExistingEffect = FindActiveEffect(Tag);
    if (ExistingEffect)
    {
        // すでにアクティブな場合は、フェードアウト中ならキャンセル
        if (ExistingEffect->bIsFadingOut)
        {
            ExistingEffect->bIsFadingOut = false;
            ExistingEffect->bIsFading = true;
            ExistingEffect->TargetWeight = Config->BlendWeight;
            UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Reactivating effect %d"), static_cast<int32>(Tag));
        }
        return;
    }

    // 新しいエフェクトを作成
    FActivePostProcessEffect NewEffect;
    NewEffect.Tag = Tag;
    NewEffect.MaterialInstance = UMaterialInstanceDynamic::Create(Config->Material, this);
    NewEffect.TargetWeight = Config->BlendWeight;
    NewEffect.Priority = Config->Priority;
    NewEffect.bIsFading = !bInstant;
    NewEffect.bIsFadingOut = false;

    if (bInstant)
    {
        // 即座に表示
        NewEffect.CurrentWeight = Config->BlendWeight;
        NewEffect.FadeSpeed = 0.0f;
    }
    else
    {
        // フェードイン
        NewEffect.CurrentWeight = 0.0f;
        NewEffect.FadeSpeed = Config->FadeInDuration > 0.0f ? (1.0f / Config->FadeInDuration) : 10.0f;
    }

    ActiveEffects.Add(NewEffect);
    SortActiveEffects();
    bNeedsRefresh = true;

    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Activated effect %d (Instant: %d)"),
        static_cast<int32>(Tag), bInstant);
}

void UPostProcessEffectManager::DeactivateEffect(EPostProcessEffectTag Tag, bool bInstant)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (!Effect)
    {
        return;
    }

    if (bInstant)
    {
        // 即座に削除
        ActiveEffects.RemoveAll([Tag](const FActivePostProcessEffect& E) { return E.Tag == Tag; });
        bNeedsRefresh = true;
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Instantly deactivated effect %d"), static_cast<int32>(Tag));
    }
    else
    {
        // フェードアウト
        Effect->bIsFadingOut = true;
        Effect->bIsFading = true;
        Effect->TargetWeight = 0.0f;

        FPostProcessEffectConfig* Config = EffectConfigs.Find(Tag);
        if (Config)
        {
            Effect->FadeSpeed = Config->FadeOutDuration > 0.0f ? (1.0f / Config->FadeOutDuration) : 10.0f;
        }

        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Fading out effect %d"), static_cast<int32>(Tag));
    }
}

void UPostProcessEffectManager::ClearAllEffects(bool bInstant)
{
    if (bInstant)
    {
        ActiveEffects.Empty();
        bNeedsRefresh = true;
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Cleared all effects instantly"));
    }
    else
    {
        // すべてフェードアウト
        for (FActivePostProcessEffect& Effect : ActiveEffects)
        {
            Effect.bIsFadingOut = true;
            Effect.bIsFading = true;
            Effect.TargetWeight = 0.0f;

            FPostProcessEffectConfig* Config = EffectConfigs.Find(Effect.Tag);
            if (Config)
            {
                Effect.FadeSpeed = Config->FadeOutDuration > 0.0f ? (1.0f / Config->FadeOutDuration) : 10.0f;
            }
        }
        UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Fading out all effects"));
    }
}

bool UPostProcessEffectManager::IsEffectActive(EPostProcessEffectTag Tag) const
{
    return ActiveEffects.ContainsByPredicate([Tag](const FActivePostProcessEffect& E)
        {
            return E.Tag == Tag && !E.bIsFadingOut;
        });
}

void UPostProcessEffectManager::SetEffectWeight(EPostProcessEffectTag Tag, float Weight)
{
    FActivePostProcessEffect* Effect = FindActiveEffect(Tag);
    if (!Effect)
    {
        return;
    }

    Effect->TargetWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
    Effect->bIsFading = true;
    bNeedsRefresh = true;
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
void UPostProcessEffectManager::UpdateEffects(float DeltaTime)
{
    bool bAnyFading = false;

    // フェード処理
    for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
    {
        FActivePostProcessEffect& Effect = ActiveEffects[i];

        if (Effect.bIsFading)
        {
            // ウェイトを補間
            Effect.CurrentWeight = FMath::FInterpTo(
                Effect.CurrentWeight,
                Effect.TargetWeight,
                DeltaTime,
                Effect.FadeSpeed
            );

            // フェード完了チェック
            if (FMath::IsNearlyEqual(Effect.CurrentWeight, Effect.TargetWeight, 0.01f))
            {
                Effect.CurrentWeight = Effect.TargetWeight;
                Effect.bIsFading = false;

                // フェードアウト完了なら削除
                if (Effect.bIsFadingOut && Effect.CurrentWeight <= 0.0f)
                {
                    UE_LOG(LogTemp, Log, TEXT("PostProcessEffectManager: Removed effect %d"), static_cast<int32>(Effect.Tag));
                    ActiveEffects.RemoveAt(i);
                    bNeedsRefresh = true;
                    continue;
                }
            }

            bAnyFading = true;
            bNeedsRefresh = true;
        }
    }
}

void UPostProcessEffectManager::RefreshPostProcessSettings()
{
    if (!PostProcessComponent)
    {
        return;
    }

    // Blendableをクリア
    PostProcessComponent->Settings.WeightedBlendables.Array.Empty();

    // 優先度順にエフェクトを追加
    for (const FActivePostProcessEffect& Effect : ActiveEffects)
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

void UPostProcessEffectManager::SortActiveEffects()
{
    // 優先度でソート（高い順）
    ActiveEffects.Sort([](const FActivePostProcessEffect& A, const FActivePostProcessEffect& B)
        {
            return A.Priority > B.Priority;
        });
}