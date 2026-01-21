#include "Component/PhysicsCalculatorComponent.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"

// 処理の流れ:
// 1. 全てのメンバ変数をデフォルト値で初期化
// 2. Tickを有効化
UPhysicsCalculatorComponent::UPhysicsCalculatorComponent()
	: ForceScale(0)
	, GravityScale(DEFAULT_GRAVITYSCALE)
	, ForceDirection(FVector::ZeroVector)
	, PreviousPosition(FVector::ZeroVector)
	, Velocity(FVector::ZeroVector)
	, Timer(0)
	, GravityDivider(1)
	, bShouldApplyGravity(true)
	, bIsSweep(false)
	, bIsPhysicsEnabled(false)
	, bUseLocalOffset(true)
	, bWasOnGround(false)
	, bFalling(false)
	, bHasJustLanded(false)
	, MaxFallingSpeed(DEFAULT_MAX_FALLSPEED)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPhysicsCalculatorComponent::BeginPlay()
{
	Super::BeginPlay();
}

// 処理の流れ:
// 1. Ownerを取得
// 2. アクティブ状態を確認
// 3. 重力を適用
// 4. 接地状態を更新
// 5. 物理が無効な場合、手動移動を実行:
//    - 力を減衰
//    - 移動ベクトルを計算
//    - 障害物との衝突を調整
//    - ローカルまたはワールド座標で移動
//    - 高さの変化で落下開始を検出
void UPhysicsCalculatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();

	if (!IsActive())
	{
		return;
	}

	if (bShouldApplyGravity)
	{
		AddGravity();
		UpdateGroundState();
	}

	if (!bIsPhysicsEnabled)
	{
		ForceScale = FMath::Max(ForceScale - DeltaTime * 10.0f, 0.0f);

		FVector MoveVector = ForceDirection * ForceScale;
		FVector Adjusted = GetBlockedAdjustedVector(MoveVector);

		if (bUseLocalOffset)
		{
			GetOwner()->AddActorLocalOffset(Adjusted, bIsSweep);
		}
		else
		{
			GetOwner()->AddActorWorldOffset(Adjusted, bIsSweep);
		}

		FVector CurrentPosition = GetOwner()->GetActorLocation();
		float DistanceZ = CurrentPosition.Z - PreviousPosition.Z;

		if (DistanceZ < 0)
		{
			ForceDirection.Z = 0;
			ForceScale = 0;
			Timer = 0;
			bIsPhysicsEnabled = true;
			bFalling = true;
		}

		PreviousPosition = CurrentPosition;
	}
}

// 処理の流れ:
// 1. 現在の接地状態を判定
// 2. 前フレームは非接地で今回接地していたら着地と判定
// 3. 接地状態を保存
void UPhysicsCalculatorComponent::UpdateGroundState()
{
	bool bIsCurrentlyOnGround = OnGround();

	bHasJustLanded = (!bWasOnGround && bIsCurrentlyOnGround);

	if (bHasJustLanded)
		UE_LOG(LogTemp, Log, TEXT("着地"));

	bWasOnGround = bIsCurrentlyOnGround;
}

// 処理の流れ:
// 1. 力の方向を設定
// 2. 力の大きさを設定
// 3. 重力タイマーをリセット
// 4. Sweep使用フラグを設定
// 5. 手動移動モードに切り替え
// 6. ローカル/ワールド座標フラグを設定
void UPhysicsCalculatorComponent::AddForce(FVector Direction, float Force, const bool bSweep, const bool UseLocalOffset)
{
	ForceDirection = Direction;
	ForceScale = Force;
	Timer = 0;
	bIsSweep = bSweep;
	bIsPhysicsEnabled = false;
	bUseLocalOffset = UseLocalOffset;
}

// 処理の流れ:
// 1. 全ての力をゼロにリセット
// 2. タイマーをリセット
// 3. 物理モードを有効化
void UPhysicsCalculatorComponent::ResetForce()
{
	ForceDirection = FVector::ZeroVector;
	ForceScale = 0;
	Timer = 0;
	bIsPhysicsEnabled = true;
}

// 処理の流れ:
// 1. 接地中なら落下処理を停止してリセット
// 2. 経過時間を加算
// 3. 落下速度を計算（重力スケール × 経過時間 / 修正係数）
// 4. 最大落下速度で制限
// 5. Z方向に移動（ローカル座標、Sweep有効）
void UPhysicsCalculatorComponent::AddGravity()
{
	if (OnGround())
	{
		bIsPhysicsEnabled = false;
		bFalling = false;
		Timer = 0;
		return;
	}

	Timer += GetWorld()->DeltaTimeSeconds;

	float FallSpeed = (GravityScale * Timer) / GravityDivider;
	FallSpeed = FMath::Min(FallSpeed, MaxFallingSpeed);

	GetOwner()->AddActorLocalOffset(FVector(0, 0, -FallSpeed), true);
}

// 処理の流れ:
// 1. Ownerとワールドの有効性確認
// 2. Actorの位置とスケールを取得
// 3. 足元にBoxExtentを作成
// 4. 回転を考慮した下方向ベクトルを計算
// 5. 足元の位置を算出
// 6. 足元5cm下へSweep
// 7. ヒット結果を返す
bool UPhysicsCalculatorComponent::OnGround() const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr)
		return false;

	FVector ActorLocation = Owner->GetActorLocation();
	FVector ActorScale = Owner->GetActorScale();

	FVector BoxExtent(40.0f * ActorScale.X, 20.0f * ActorScale.Y, 15.0f);

	float HalfHeight = Owner->GetSimpleCollisionHalfHeight();

	const FQuat ActorRotation = Owner->GetActorQuat();
	FVector DownVector = ActorRotation.GetUpVector() * -1.f;

	FVector FootLocation = ActorLocation + DownVector * HalfHeight;

	FVector StartTrace = FootLocation;
	FVector EndTrace = FootLocation + DownVector * 5.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	FHitResult Hit;

	bool bHit = World->SweepSingleByChannel(
		Hit,
		StartTrace,
		EndTrace,
		ActorRotation,
		ECC_Visibility,
		FCollisionShape::MakeBox(BoxExtent),
		Params
	);

	return bHit;
}

// 処理の流れ:
// 1. 重力スケールを設定
// 2. 重力適用フラグを設定
// 3. 重力修正係数を設定
void UPhysicsCalculatorComponent::SetGravityScale(bool ApplyGravity, float Scale, float Modifier)
{
	GravityScale = Scale;
	bShouldApplyGravity = ApplyGravity;
	GravityDivider = Modifier;
}

// 処理の流れ:
// 1. Owner、ワールド、移動ベクトルの有効性確認
// 2. Actorの位置とスケールを取得
// 3. BoxExtentを作成
// 4. 移動方向を正規化
// 5. 開始・終了位置を計算
// 6. Sweepで移動先に障害物があるか判定
// 7. 衝突時は移動距離を補正
// 8. 衝突なしならそのまま返す
FVector UPhysicsCalculatorComponent::GetBlockedAdjustedVector(const FVector& MoveVector)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr || MoveVector.IsNearlyZero())
		return MoveVector;

	FVector ActorLocation = Owner->GetActorLocation();
	FVector ActorScale = Owner->GetActorScale();

	FVector BoxExtent(
		20.0f * ActorScale.X,
		20.0f * ActorScale.Y,
		20.0f * ActorScale.Z
	);

	FVector Direction = MoveVector.GetSafeNormal();
	const float BackstepDistance = 1.0f;

	FVector Start = ActorLocation - Direction * BackstepDistance;
	FVector End = Start + MoveVector;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	bool bHit = World->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeBox(BoxExtent),
		Params
	);

	if (bHit)
	{
		const float AdjustMargin = 0.1f;
		float Distance = FMath::Max(Hit.Distance - AdjustMargin, 0.0f);
		return Direction * Distance;
	}

	return MoveVector;
}

// 処理の流れ:
// 1. Owner、ワールドの有効性確認
// 2. Actorの位置とスケールを取得
// 3. 足元にBoxExtentを作成
// 4. 足元の位置を計算
// 5. 足元5cm下をSweep
// 6. ヒットしていれば法線を返す
// 7. 非接地時は上方向を返す
FVector UPhysicsCalculatorComponent::GetGroundNormal() const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr)
		return FVector::UpVector;

	FVector ActorLocation = Owner->GetActorLocation();
	FVector ActorScale = Owner->GetActorScale();

	FVector BoxExtent(20.0f * ActorScale.X, 20.0f * ActorScale.Y, 2.0f);

	float HalfHeight = Owner->GetSimpleCollisionHalfHeight();
	FVector FootLocation = ActorLocation - FVector(0, 0, HalfHeight);

	FVector StartTrace = FootLocation;
	FVector EndTrace = FootLocation - FVector(0, 0, 5.0f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	FHitResult Hit;

	bool bHit = World->SweepSingleByChannel(
		Hit,
		StartTrace,
		EndTrace,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeBox(BoxExtent),
		Params
	);

	if (bHit)
	{
		return Hit.Normal;
	}

	return FVector::UpVector;
}

// 処理の流れ:
// 直前のフレームで接地状態に変わったかを返す
const bool UPhysicsCalculatorComponent::HasLanded()
{
	return bHasJustLanded;
}