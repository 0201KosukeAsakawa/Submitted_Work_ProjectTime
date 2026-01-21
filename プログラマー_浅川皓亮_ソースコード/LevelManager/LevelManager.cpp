// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelManager.h"

#include "Interface/Soundable.h"
#include "Interface/UIManagerProvider.h"

#include "UI/UIManager.h"

#include "Sound/SoundManager.h"

#include "EngineUtils.h"

#include "Kismet/GameplayStatics.h"

#include "Component/LevelEffectComponent.h"

TWeakObjectPtr<ALevelManager> ALevelManager::instance = nullptr;

// Sets default values
ALevelManager::ALevelManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PostProcessEffectManager = CreateDefaultSubobject<UPostProcessEffectManager>(TEXT("PostProcessEffectManager"));
}

// Called when the game starts or when spawned
void ALevelManager::BeginPlay()
{
	Super::BeginPlay();

	instance = this;


	if (SoundClass)
	{
		// SoundManager をインスタンス化
		USoundManager* SoundInstance = NewObject<USoundManager>(this, SoundClass);
		if (SoundInstance)
		{
			SoundInstance->Init(); // 独自の初期化
			mSoundManager.SetObject(SoundInstance);
			mSoundManager.SetInterface(Cast<ISoundManagerProvider>(SoundInstance));
			//mSoundManager->PlaySound(ESoundKinds::BGM, "Default");
		}
	}

	if (UIManagerClass)
	{
		// SoundManager をインスタンス化
		UUIManager* UIInstance = NewObject<UUIManager>(this, UIManagerClass);
		if (UIInstance)
		{
			UIInstance->Init(); // 独自の初期化
			mUIManager.SetObject(UIInstance);
			mUIManager.SetInterface(Cast<IUIManagerProvider>(UIInstance));
		}
	}
}

// Called every frame
void ALevelManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALevelManager::LoadScene(FName nextMap)
{
	UGameplayStatics::OpenLevel(this, nextMap);
}

// GetInstance関数で、インスタンスが未設定の場合は初期化処理を強制する
ALevelManager* ALevelManager::GetInstance(UObject* WorldContext)
{
	if (!instance.IsValid())
	{
		// WorldContextからレベルマネージャーを検索
		UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World)
			return nullptr;

		// インスタンスがまだ設定されていない場合
		for (TActorIterator<ALevelManager> It(World); It; ++It)
		{
			instance = *It;
			instance->InitializeComponents();  // 必要な初期化処理を行う
			return *It;
		}

		// レベルマネージャーが見つからなければ、新しく生成
		ALevelManager* NewInstance = World->SpawnActor<ALevelManager>();
		instance = NewInstance;
		instance->InitializeComponents();  // 必要な初期化処理を行う
	}

	return instance.Get();
}
