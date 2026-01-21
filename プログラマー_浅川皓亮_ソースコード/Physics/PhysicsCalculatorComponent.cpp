#include "Component/PhysicsCalculatorComponent.h"
#include "UE5Coro.h"

using namespace UE5Coro;
using namespace UE5Coro::Latent;

UPhysicsCalculatorComponent::UPhysicsCalculatorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// 処理の流れ:
// 1. Ownerをキャッシュ
// 2. 自動で物理計算を開始
void UPhysicsCalculatorComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeComponent();
    StartPhysics();
}

void UPhysicsCalculatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopPhysics();
    Super::EndPlay(EndPlayReason);
}

// 処理の流れ:
// 1. Ownerを取得してキャッシュ
void UPhysicsCalculatorComponent::InitializeComponent()
{
    CachedOwner = GetOwner();

    if (!CachedOwner.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("PhysicsCalculator: Owner is invalid"));
    }
}

// 処理の流れ:
// 1. 物理ループを開始
void UPhysicsCalculatorComponent::StartPhysics()
{
    if (bIsPhysicsActive)
    {
        return;
    }

    bIsPhysicsActive = true;
    bShouldStopPhysics = false;
    PhysicsUpdateLoop();
}

// 処理の流れ:
// 1. 物理ループを停止
void UPhysicsCalculatorComponent::StopPhysics()
{
    bShouldStopPhysics = true;
    bIsPhysicsActive = false;
}

// 処理の流れ:
// 1. 力のパラメータを設定
// 2. タイマーをリセット
void UPhysicsCalculatorComponent::AddForce(FVector Direction, float Force, bool bSweep, bool bLocalOffset)
{
    ForceDirection = Direction;
    ForceScale = Force;
    GravityTimer = 0.0f;
    bUseSweep = bSweep;
    bUseLocalOffset = bLocalOffset;
}

// 処理の流れ:
// 1. 全ての力をリセット
void UPhysicsCalculatorComponent::ResetForce()
{
    ForceDirection = FVector::ZeroVector;
    ForceScale = 0.0f;
    GravityTimer = 0.0f;
}

// 処理の流れ:
// 1. 重力パラメータを設定
void UPhysicsCalculatorComponent::SetGravityScale(bool bApplyGravity, float Scale, float Modifier)
{
    bShouldApplyGravity = bApplyGravity;
    GravityScale = Scale;
    GravityDivider = Modifier;
}

// ============================================
// Coroutines
// ============================================

// 処理の流れ:
// 1. 毎フレーム非同期で接地判定
// 2. 接地状態に応じて重力または力を適用
// 3. 停止フラグが立つまで継続
TCoroutine<> UPhysicsCalculatorComponent::PhysicsUpdateLoop()
{
    while (!bShouldStopPhysics && CachedOwner.IsValid())
    {
        co_await NextTick();

        const float DeltaTime = GetWorld()->GetDeltaSeconds();

        // 非同期で接地判定
        bool bNewGroundState = co_await AsyncCheckGroundState();
        UpdateGroundState(bNewGroundState);

        // 重力適用
        if (bShouldApplyGravity && !bIsOnGround)
        {
            ApplyGravity(DeltaTime);
        }

        // 力適用
        if (ForceScale > 0.0f)
        {
            ApplyForce(DeltaTime);
        }
    }

    bIsPhysicsActive = false;
}

// 処理の流れ:
// 1. 非同期Sweepで接地判定
// 2. 結果を返す
TCoroutine<bool> UPhysicsCalculatorComponent::AsyncCheckGroundState()
{
    if (!CachedOwner.IsValid())
    {
        co_return false;
    }

    const FVector ActorLocation = CachedOwner->GetActorLocation();
    const FVector ActorScale = CachedOwner->GetActorScale();
    const float HalfHeight = CachedOwner->GetSimpleCollisionHalfHeight();
    const FQuat ActorRotation = CachedOwner->GetActorQuat();

    const FVector DownVector = ActorRotation.GetUpVector() * -1.0f;
    const FVector FootLocation = ActorLocation + DownVector * HalfHeight;

    const FVector BoxExtent(
        GroundCheckBoxExtent.X * ActorScale.X,
        GroundCheckBoxExtent.Y * ActorScale.Y,
        GroundCheckBoxExtent.Z
    );

    const FVector StartTrace = FootLocation;
    const FVector EndTrace = FootLocation + DownVector * GroundCheckDistance;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(CachedOwner.Get());

    FHitResult Hit;

    // 同期Sweepだが、コルーチン内なので他の処理をブロックしない
    bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        StartTrace,
        EndTrace,
        ActorRotation,
        ECC_Visibility,
        FCollisionShape::MakeBox(BoxExtent),
        Params
    );

    co_return bHit;
}

// 処理の流れ:
// 1. 重力タイマーを更新
// 2. 落下速度を計算
// 3. 最大速度で制限
// 4. 下方向に移動
void UPhysicsCalculatorComponent::ApplyGravity(float DeltaTime)
{
    if (!CachedOwner.IsValid())
    {
        return;
    }

    GravityTimer += DeltaTime;

    float FallSpeed = (GravityScale * GravityTimer) / GravityDivider;
    FallSpeed = FMath::Min(FallSpeed, MaxFallingSpeed);

    CachedOwner->AddActorLocalOffset(FVector(0, 0, -FallSpeed), true);
}

// 処理の流れ:
// 1. 力を減衰
// 2. 移動ベクトルを計算
// 3. 移動適用
void UPhysicsCalculatorComponent::ApplyForce(float DeltaTime)
{
    if (!CachedOwner.IsValid())
    {
        return;
    }

    ForceScale = FMath::Max(ForceScale - DeltaTime * 10.0f, 0.0f);

    const FVector MoveVector = ForceDirection * ForceScale;

    if (bUseLocalOffset)
    {
        CachedOwner->AddActorLocalOffset(MoveVector, bUseSweep);
    }
    else
    {
        CachedOwner->AddActorWorldOffset(MoveVector, bUseSweep);
    }
}

// 処理の流れ:
// 1. 着地判定
// 2. 接地時は重力タイマーをリセット
// 3. 状態を保存
void UPhysicsCalculatorComponent::UpdateGroundState(bool bNewGroundState)
{
    bHasJustLanded = (!bWasOnGround && bNewGroundState);

    if (bHasJustLanded)
    {
        GravityTimer = 0.0f;
        UE_LOG(LogTemp, Log, TEXT("PhysicsCalculator: Landed"));
    }

    bIsOnGround = bNewGroundState;
    bWasOnGround = bNewGroundState;
}
