// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerInputBinder.h"
#include "Components/ActorComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Interface/PlayerInputReceiver.h"

UPlayerInputBinder::UPlayerInputBinder()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerInputBinder::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * @brief Enhanced Input のバインディング処理
 *
 * 指定された EnhancedInputComponent にアクションをバインドし、
 * 入力を受け取った際に Receiver（IPlayerInputReceiver 実装クラス）へ通知する。
 * また、PlayerController 経由で Input Mapping Context を登録する。
 *
 * @param InputComponent EnhancedInputComponent（通常は PlayerCharacter が持つ）
 * @param Receiver 入力を受け取る対象（IPlayerInputReceiver 実装オブジェクト）
 */
void UPlayerInputBinder::BindInputs(UEnhancedInputComponent* InputComponent, TScriptInterface<IPlayerInputReceiver> Receiver)
{
    if (!InputComponent || !Receiver)
    {
        UE_LOG(LogTemp, Warning, TEXT("BindInputs failed: invalid InputComponent or Receiver"));
        return;
    }

    // Receiver から PlayerController を取得
    AActor* ReceiverActor = Cast<AActor>(Receiver.GetObject());
    APlayerController* PlayerController = nullptr;

    if (ReceiverActor)
    {
        // Pawn や Character の場合は GetController() が有効
        if (APawn* Pawn = Cast<APawn>(ReceiverActor))
        {
            PlayerController = Cast<APlayerController>(Pawn->GetController());
        }
        else
        {
            // その他のアクターならオーナーを辿る
            PlayerController = Cast<APlayerController>(ReceiverActor->GetOwner());
        }
    }

    // EnhancedInput の Subsystem 登録
    if (PlayerController)
    {
        if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                if (DefaultMappingContext)
                {
                    Subsystem->AddMappingContext(DefaultMappingContext, 0);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext is null in PlayerInputBinder"));
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get PlayerController from Receiver %s"), *ReceiverActor->GetName());
    }

    // 受信先登録
    InputReceiver = Receiver;

    // --- アクションバインド ---
    InputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UPlayerInputBinder::HandleMove);
    InputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &UPlayerInputBinder::HandleJump);
    InputComponent->BindAction(Action, ETriggerEvent::Started, this, &UPlayerInputBinder::HandleReplay);
    InputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &UPlayerInputBinder::HandleLook);
    InputComponent->BindAction(SpecialAction, ETriggerEvent::Started, this, &UPlayerInputBinder::HandleSlowSkill);
    InputComponent->BindAction(BoostAction, ETriggerEvent::Triggered, this, &UPlayerInputBinder::HandleBoostSkill);
    InputComponent->BindAction(ReplayToWorldAction, ETriggerEvent::Triggered, this, &UPlayerInputBinder::HandleReplayToWorld);
    InputComponent->BindAction(OnInteractAction, ETriggerEvent::Triggered, this, &UPlayerInputBinder::HandleOnInteractAction);
    InputComponent->BindAction(OpenMenuAction, ETriggerEvent::Triggered, this, &UPlayerInputBinder::HandleOpenMenu);
}

void UPlayerInputBinder::UnbindInputs()
{
	InputReceiver = nullptr;
}

void UPlayerInputBinder::HandleMove(const FInputActionValue& Value)
{
	if (InputReceiver)
		InputReceiver->OnMove(Value);
}

void UPlayerInputBinder::HandleJump(const FInputActionValue& Value)
{
    if (InputReceiver)
        InputReceiver->OnJump(Value);
}

void UPlayerInputBinder::HandleReplay(const FInputActionValue& Value)
{
	if (InputReceiver)
		InputReceiver->OnReplayAction(Value);
}

void UPlayerInputBinder::HandleReplayToWorld(const FInputActionValue& Value)
{
    if (InputReceiver)
        InputReceiver->OnReplayToWorldAction(Value);
}

void UPlayerInputBinder::HandleLook(const FInputActionValue& Value)
{
	if (InputReceiver)
		InputReceiver->OnLook(Value);
}

void UPlayerInputBinder::HandleSlowSkill(const FInputActionValue& Value)
{
	if (InputReceiver)
		InputReceiver->OnSlowAction(Value);
}

void UPlayerInputBinder::HandleBoostSkill(const FInputActionValue& Value)
{
    if (InputReceiver)
        InputReceiver->OnBoost(Value);
}

void UPlayerInputBinder::HandleOnInteractAction(const FInputActionValue& Value)
{
    if (InputReceiver)
        InputReceiver->OnInteractAction(Value);
}

void UPlayerInputBinder::HandleOpenMenu(const FInputActionValue& Value)
{
    if (InputReceiver)
        InputReceiver->OpenMenu(Value);
}
