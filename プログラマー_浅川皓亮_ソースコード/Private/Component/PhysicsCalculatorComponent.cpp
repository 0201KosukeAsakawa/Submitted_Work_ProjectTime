#include "Component/PhysicsCalculatorComponent.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"

// =======================
// コンストラクタ
// =======================

// コンストラクタでデフォルト値を設定
UPhysicsCalculatorComponent::UPhysicsCalculatorComponent()
	: ForceScale(0)                          // 移動ベクトルのスケール
	, GravityScale(DEFAULT_GRAVITYSCALE)	 //重力のスケール
	, ForceDirection(FVector::ZeroVector)    // 移動方向
	, PreviousPosition(FVector::ZeroVector)  // 前フレーム位置
	, Velocity(FVector::ZeroVector)			 //現在かかっている力の向き
	, Timer(0)                               // 経過時間（重力計算用）
	, GravityDivider(1)                       //
	, bShouldApplyGravity(true)              // 重力適用フラグ
	, bIsSweep(false)                        // 移動時に Sweep を行うか
	, bIsPhysicsEnabled(false)               // 物理挙動が有効かどうか
	, bUseLocalOffset(true)
	, bWasOnGround(false)
	, bFalling(false)
	, bHasJustLanded(false)
	, MaxFallingSpeed(DEFAULT_MAX_FALLSPEED)
{
	// Tick を有効化
	PrimaryComponentTick.bCanEverTick = true;
}

// =======================
// 初期化処理
// =======================

void UPhysicsCalculatorComponent::BeginPlay()
{
	Super::BeginPlay();
}

// =======================
// 毎フレーム更新処理
// =======================

void UPhysicsCalculatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();

	// --- OffMesh 状態を判定（IsActive が false なら停止） ---
	if (!IsActive())
	{
		// オーナーが非表示 or Tick 無効なら処理を止める
		return;
	}

	// 重力を適用
	if (bShouldApplyGravity)
	{
		AddGravity();
		UpdateGroundState();
	}

	// 物理が無効なときのみ手動移動を行う
	if (!bIsPhysicsEnabled)
	{
		// 時間経過で力を減衰
		ForceScale = FMath::Max(ForceScale - DeltaTime * 10.0f, 0.0f);

		// 移動ベクトル計算
		FVector MoveVector = ForceDirection * ForceScale;

		// 障害物との衝突調整
		FVector Adjusted = GetBlockedAdjustedVector(MoveVector);

		// ローカル座標 or ワールド座標で移動
		if (bUseLocalOffset)
		{
			GetOwner()->AddActorLocalOffset(Adjusted, bIsSweep);
		}
		else
		{
			GetOwner()->AddActorWorldOffset(Adjusted, bIsSweep);
		}

		// 高さの変化で「落下開始」を検出
		FVector currentPosition = GetOwner()->GetActorLocation();
		float distanceZ = currentPosition.Z - PreviousPosition.Z;

		if (distanceZ < 0)
		{
			ForceDirection.Z = 0;
			ForceScale = 0;
			Timer = 0;
			bIsPhysicsEnabled = true;
			bFalling = true;
		}

		PreviousPosition = currentPosition;
	}
}

// =======================
// 接地状態更新
// =======================

void UPhysicsCalculatorComponent::UpdateGroundState()
{
	// 現在の接地状態を判定
	bool bIsCurrentlyOnGround = OnGround();

	// 前フレームは接地してなかったのに、今回接地していたら「着地」
	bHasJustLanded = (!bWasOnGround && bIsCurrentlyOnGround);

	if (bHasJustLanded)
		UE_LOG(LogTemp, Log, TEXT("着地"));

	bWasOnGround = bIsCurrentlyOnGround;
}

// =======================
// 外部からの力を加える
// =======================

void UPhysicsCalculatorComponent::AddForce(FVector Direction, float Force, const bool bSweep, const bool useLocalOffset)
{
	ForceDirection = Direction;  // 力の方向
	ForceScale = Force;          // 力の大きさ
	Timer = 0;                   // 重力タイマーリセット
	bIsSweep = bSweep;           // Sweep 使用有無
	bIsPhysicsEnabled = false;   // 手動移動モードへ
	bUseLocalOffset = useLocalOffset;
}

// =======================
// 外部からの力をリセット
// =======================

void UPhysicsCalculatorComponent::ResetForce()
{
	ForceDirection = FVector::ZeroVector;
	ForceScale = 0;
	Timer = 0;
	bIsPhysicsEnabled = true;
}

// =======================
// 重力計算
// =======================

void UPhysicsCalculatorComponent::AddGravity()
{
	// 接地中なら落下処理を停止
	if (OnGround())
	{
		bIsPhysicsEnabled = false;
		bFalling = false;
		Timer = 0;
		return;
	}

	// 経過時間を加算
	Timer += GetWorld()->DeltaTimeSeconds;

	// 落下速度 = 重力スケール × 経過時間 / 修正係数
	float FallSpeed = (GravityScale * Timer) / GravityDivider;

	// 最大落下速度を制限
	FallSpeed = FMath::Min(FallSpeed, MaxFallingSpeed);

	// Z方向に移動（ローカル座標、Sweep有効）
	GetOwner()->AddActorLocalOffset(FVector(0, 0, -FallSpeed), true);
}

// =======================
// 接地判定
// =======================

bool UPhysicsCalculatorComponent::OnGround() const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner ==nullptr || World == nullptr) 
		return false;

	// 位置・スケール取得
	FVector ActorLocation = Owner->GetActorLocation();
	FVector ActorScale = Owner->GetActorScale();

	// 足元に薄い BoxExtent を作成して接地判定
	FVector BoxExtent(40.0f * ActorScale.X, 20.0f * ActorScale.Y, 15.0f);

	float HalfHeight = Owner->GetSimpleCollisionHalfHeight();

	// 回転を考慮した下方向ベクトル
	const FQuat ActorRotation = Owner->GetActorQuat();
	FVector DownVector = ActorRotation.GetUpVector() * -1.f;

	// 足元の位置を算出
	FVector FootLocation = ActorLocation + DownVector * HalfHeight;

	// 足元 5cm 下へトレース
	FVector StartTrace = FootLocation;
	FVector EndTrace = FootLocation + DownVector * 5.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	FHitResult Hit;

	// Sweep で地面と接触しているかを判定
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

// =======================
// 重力スケールの設定
// =======================

void UPhysicsCalculatorComponent::SetGravityScale(bool applyGravity, float scale, float Modifier)
{
	GravityScale = scale;
	bShouldApplyGravity = applyGravity;
	GravityDivider = Modifier;
}

// =======================
// 移動方向の衝突補正
// =======================

FVector UPhysicsCalculatorComponent::GetBlockedAdjustedVector(const FVector& MoveVector)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr || MoveVector.IsNearlyZero()) 
		return MoveVector;

	// 位置・スケール取得
	FVector ActorLocation = Owner->GetActorLocation();
	FVector ActorScale = Owner->GetActorScale();

	// 固定サイズの BoxExtent（20cm 基準）
	FVector BoxExtent(
		20.0f * ActorScale.X,
		20.0f * ActorScale.Y,
		20.0f * ActorScale.Z
	);

	// 移動方向を正規化
	FVector Direction = MoveVector.GetSafeNormal();
	const float BackstepDistance = 1.0f;

	// 開始・終了位置
	FVector Start = ActorLocation - Direction * BackstepDistance;
	FVector End = Start + MoveVector;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	// Sweep で移動先に障害物があるかを判定
	bool bHit = World->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeBox(BoxExtent),
		Params
	);

	// 衝突時は移動距離を補正
	if (bHit)
	{
		const float AdjustMargin = 0.1f;
		float Distance = FMath::Max(Hit.Distance - AdjustMargin, 0.0f);
		return Direction * Distance;
	}

	// 衝突なしならそのまま
	return MoveVector;
}

// =======================
// 接地面の法線取得
// =======================

FVector UPhysicsCalculatorComponent::GetGroundNormal() const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr)
		return FVector::UpVector;

	// 位置・スケール取得
	FVector ActorLocation = Owner->GetActorLocation();
	FVector ActorScale = Owner->GetActorScale();

	// 足元に薄い BoxExtent を作成
	FVector BoxExtent(20.0f * ActorScale.X, 20.0f * ActorScale.Y, 2.0f);

	float HalfHeight = Owner->GetSimpleCollisionHalfHeight();
	FVector FootLocation = ActorLocation - FVector(0, 0, HalfHeight);

	// 足元 5cm 下をトレース
	FVector StartTrace = FootLocation;
	FVector EndTrace = FootLocation - FVector(0, 0, 5.0f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	FHitResult Hit;

	// Sweep で接地判定
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
		// 接地していれば法線を返す
		return Hit.Normal;
	}

	// 非接地時は上方向を返す
	return FVector::UpVector;
}

// =======================
// 着地判定
// =======================

const bool UPhysicsCalculatorComponent::HasLanded()
{
	// 直前のフレームで接地状態に変わったかを返す
	return bHasJustLanded;
}
