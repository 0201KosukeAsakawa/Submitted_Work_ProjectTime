// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/PropellerObject.h"
//#include "InGameInstance.h"
#include "Components/BoxComponent.h"

// コンストラクタ
APropellerObject::APropellerObject()
{
	PrimaryActorTick.bCanEverTick = true; // Tick有効化

	// デフォルト値
	RotationSpeed = FMath::Max(RotationSpeed, 1.0f); // 1度/秒未満は1度/秒に設定
	RotationAxis = RotationAxis.GetSafeNormal();
}

// ゲーム開始時
void APropellerObject::BeginPlay()
{
	Super::BeginPlay();
	// 初回のみイベントシステムに登録
	Init();
}

// 毎フレーム処理
void APropellerObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 回転処理
	Rotation();
}

// イベント状態が有効かを返す
bool APropellerObject::IsPlayEvent() const
{
	return bIsActive;
}

// Tickによる回転処理はここで呼ばれるため、単純にtrueを返す
bool APropellerObject::PlayEvent(float DeltaTime)
{
	return true;
}

// イベント状態切替
void APropellerObject::SwitchPlayEvent()
{
	bIsActive = !bIsActive;
}

// イベントID取得
FName APropellerObject::GetEventID() const
{
	return EventID;
}

// 初期化処理：イベントシステムへの登録
void APropellerObject::Init()
{
	if (bIsInitialized)
		return;

	//UInGameInstance* GameInstance = Cast<UInGameInstance>(GetWorld()->GetGameInstance());
	//if (!GameInstance)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("APropellerObject: GameInstance is null"));
	//	return;
	//}

	//UEventSystem* EventSystem = GameInstance->GetEventSystem();
	//if (!EventSystem)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("APropellerObject: EventSystem is null"));
	//	return;
	//}

	//EventSystem->SetEventObject(this);
	bIsInitialized = true;
}

// 回転処理
FRotator APropellerObject::Rotation()
{
	if (!bIsActive)
		return GetActorRotation();

	// 現在の時間拡張係数（スローや加速演出用）
	float TimeDilation = GetActorTimeDilation();

	// ローカル回転の適用
	FQuat RotationQuat = FQuat(RotationAxis, DegreesToRadians * RotationSpeed * TimeDilation);
	AddActorLocalRotation(RotationQuat);

	return GetActorRotation();
}