// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/TimelineComponent.h"
#include "Interface/PlayerInfoProvider.h"
#include "Interface/PlayerInputReceiver.h"
#include "Interface/TimeControllable.h"
#include "PlayerCharacter.generated.h"

// Forward declarations
class UPlayerStateManager;
class UPlayerCameraControlComponent;
class UPlayerInputBinder;

class UWallRunComponent;
class UTimeManipulatorComponent;
class UCameraComponent;
class UBoostComponent;
class UMaterialInterface;
class UParkourComponent;


/**
 * @brief プレイヤーキャラクタークラス
 * @details 時間操作機能を持つプレイヤーキャラクター
 *          - 任意記録型（削除型）の時間記録
 *          - 巻き戻し機能
 *          - スロー機能
 *          - 各機能に対応したポストプロセスエフェクト
 */
UCLASS()
class CARRY_API APlayerCharacter : public ACharacter,
    public IPlayerInfoProvider,
    public IPlayerInputReceiver,
    public ITimeControllable
{
    GENERATED_BODY()

public:
    // ============================================
    // Lifecycle
    // ============================================
    APlayerCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    /** スロー中のポストプロセスエフェクトを適用 */
    UFUNCTION(BlueprintNativeEvent, Category = "Effects")
    void ApplySlowMotionPostProcess();

    /** スロー中のポストプロセスエフェクトを削除 */
    UFUNCTION(BlueprintNativeEvent, Category = "Effects")
    void RemoveSlowMotionPostProcess();
private:
    // ============================================
    // Time Manipulation Interface
    // ============================================

    /** 時間記録を開始 */
    UFUNCTION(BlueprintCallable, Category = "Time Control")
    void StartTimeRecording()override;

    /** 時間記録を停止 */
    UFUNCTION(BlueprintCallable, Category = "Time Control")
    void StopTimeRecording()override;

    /** 巻き戻しを開始 */
    UFUNCTION(BlueprintCallable, Category = "Time Control")
    void StartTimeRewind(float Duration = 3.0f)override;

    /** 記録中かどうか */
    UFUNCTION(BlueprintPure, Category = "Time Control")
    bool IsRecording() const override;

    // ============================================
    // Input Callbacks (IPlayerInputReceiver)
    // ============================================

    /** 移動入力 */
    virtual void OnMove(const FInputActionValue& Value) override;

    /** ジャンプ入力 */
    virtual void OnJump(const FInputActionValue& Value) override;

    /** メインアクション（攻撃・ジャンプなど） */
    virtual void OnReplayAction(const FInputActionValue& Value) override;

    /** カメラ回転入力 */
    virtual void OnLook(const FInputActionValue& Value) override;

    /** その他アクション（ダッシュ・リワインドなど） */
    virtual void OnSlowAction(const FInputActionValue& Value) override;

    virtual void OnReplayToWorldAction(const FInputActionValue& Value) override;

    /** ブースト */
    virtual void OnBoost(const FInputActionValue& Value) override;

    virtual void OnInteractAction(const FInputActionValue& Value)override;

    /** メニューを開く*/
    virtual void OpenMenu(const FInputActionValue& Value)override;
    // ============================================
    // IPlayerInfoProvider Interface
    // ============================================

    virtual bool ChangeState(EPlayerStateType) override;
    virtual bool IsRewinding() const override;
    virtual UCameraComponent* GetCamera() const override;

    void SetNewCameraRotation(float Roll)override;
    virtual void PlayBoost()override;
    virtual bool PlayParkour()override;

    virtual UPlayerCameraControlComponent* GetCameraControl() const override;

    UFUNCTION()
    void OnParkourStarted();
    UFUNCTION()
    void OnParkourEnded();

    // ============================================
    // Post Process Effects
    // ============================================

    /** 記録中のポストプロセスエフェクトを適用 */
    UFUNCTION()
    void ApplyRecordingPostProcess();

    /** 記録中のポストプロセスエフェクトを削除 */
    UFUNCTION()
    void RemoveRecordingPostProcess();

    /** 巻き戻し中のポストプロセスエフェクトを適用 */
    UFUNCTION()
    void ApplyRewindPostProcess();

    /** 巻き戻し中のポストプロセスエフェクトを削除 */
    UFUNCTION()
    void RemoveRewindPostProcess();

    // ============================================
    // Time Manipulator Callbacks
    // ============================================

    /** 巻き戻し開始時のコールバック */
    UFUNCTION()
    void OnRewindStarted();

    /** 巻き戻し停止時のコールバック */
    UFUNCTION()
    void OnRewindStopped();

    UFUNCTION()
    void OnSlowStopped();

    /*
    *デリゲートの登録とか
    *
    */
    virtual void SubscribeToInteract(UObject* Object, FName FunctionName) override;
    virtual void UnsubscribeFromInteract(UObject* Object, FName FunctionName) override;


    /**
     * @befie カメラを頭のソケットにアタッチするかどうかを動的に切り替える
     * @param bAttachToHead true: 頭のソケットにアタッチ(頭の動きに追従), false: ルートコンポーネントにアタッチ(揺れなし)
     * @note どちらもFPSモード（メッシュは非表示）
     */
    void SetCameraAttachToHead_Implementation(bool bAttachToHead)override;

    /**
     * 現在カメラが頭にアタッチされているかを取得
     */
    bool IsCameraAttachedToHead_Implementation() const override { return bIsCameraAttachedToHead; }


    void Landed(const FHitResult& Hit)override;
public:
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractPressed OnInteractPressed;
private:
    // ============================================
    // Components
    // ============================================

    /** ステート管理コンポーネント */
    UPROPERTY(EditAnywhere, Category = "Components")
    UPlayerStateManager* StateManager;

    /** 入力バインディングコンポーネント */
    UPROPERTY(EditAnywhere, Category = "Components")
    UPlayerInputBinder* InputBinder;

    /** 壁走りコンポーネント */
    UPROPERTY(EditAnywhere, Category = "Components")
    UWallRunComponent* WallRunComponent;

    /** 時間操作コンポーネント */
    UPROPERTY(EditAnywhere, Category = "Components")
    UTimeManipulatorComponent* TimeManipulator;

    /** カメラ制御コンポーネント */
    UPROPERTY(EditAnywhere, Category = "Components")
    UPlayerCameraControlComponent* CameraControl;

    /** ブーストコンポーネント */
    UPROPERTY(EditAnywhere, Category = "Components")
    UBoostComponent* BoostComponent;

    UPROPERTY(EditAnywhere, Category = "Components")
    UParkourComponent* ParkourComponent;

    // ============================================
    // Slow Motion Settings
    // ============================================

    /** スロー効果のタイマー */
    float SlowMotionTimer;

    /** スロー効果の持続時間 */
    float SlowMotionDuration;

    /** スロー時の時間スケール */
    float SlowMotionScale;

    bool bPlaySlowMotion = false;

    bool bPlayReplayWorld = false;

    // ============================================
    // Camera Settings
    // ============================================
        /** カメラが頭にアタッチされているか */
    bool bIsCameraAttachedToHead = true;

    /** カメラがアタッチされるソケット名 */
    FName HeadSocketName = FName("head");

    FVector SavedLocalLocation;
};