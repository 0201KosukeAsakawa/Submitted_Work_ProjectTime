// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/WallRun/WallDetectionComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

UWallDetectionComponent::UWallDetectionComponent()
    : bIsWallDetected(false)
    , bDetectionEnabled(true)
    , DetectionTimer(0.0f)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWallDetectionComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UWallDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bDetectionEnabled)
    {
        return;
    }

    // 検出間隔の制御
    DetectionTimer += DeltaTime;
    if (DetectionTimer >= Settings.DetectionInterval)
    {
        DetectWall();
        DetectionTimer = 0.0f;
    }
}

// ============================================
// Detection
// ============================================

void UWallDetectionComponent::DetectWall()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    if (!bIsWallDetected)
    {
        // キャラクターの前方向を取得
        FVector ForwardDirection = Owner->GetActorForwardVector();

        // 左右にもレイキャストを飛ばす
        TArray<FVector> Directions = {
            ForwardDirection,                                           // 前
            ForwardDirection.RotateAngleAxis(45.0f, FVector::UpVector),  // 右前
            ForwardDirection.RotateAngleAxis(-45.0f, FVector::UpVector)  // 左前
        };

        FHitResult BestHit;
        bool bFoundWall = false;

        // 各方向にレイキャスト
        for (const FVector& Direction : Directions)
        {
            FHitResult Hit;
            if (PerformRaycast(Direction, Hit))
            {
                if (IsValidWall(Hit))
                {
                    BestHit = Hit;
                    bFoundWall = true;
                    break;  // 最初に見つかった壁を使用
                }
            }

            // デバッグ描画
            //if (Settings.bDrawDebug)
            //{
            //    DrawDebug(Direction, Hit.bBlockingHit, Hit);
            //}
        }

        // 壁の検出状態が変化したかチェック
        if (bFoundWall)
        {
            AActor* HitActor = BestHit.GetActor();

            if (!bIsWallDetected || CurrentWall != HitActor)
            {
                // 新しい壁を検出
                bIsWallDetected = true;
                CurrentWall = HitActor;
                LastHitResult = BestHit;

                OnWallDetected.Broadcast(HitActor, BestHit);
                UE_LOG(LogTemp, Log, TEXT("WallDetection: Wall detected - %s"), *HitActor->GetName());
            }
            else
            {
                // 既存の壁を更新
                LastHitResult = BestHit;
            }
        }
    }
    else
    {
        if (!CheckWallStillExists(GetOwner()->GetActorLocation()))
        {
            // 壁を失った
            AActor* LostWall = CurrentWall.Get();
            bIsWallDetected = false;
            CurrentWall = nullptr;

            OnWallLost.Broadcast(LostWall);
            UE_LOG(LogTemp, Log, TEXT("WallDetection: Wall lost"));
        }
    }
   
}

bool UWallDetectionComponent::IsValidWall(const FHitResult& Hit) const
{
    if (!Hit.bBlockingHit)
    {
        return false;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    FVector Normal = Hit.ImpactNormal;

    // 壁の角度をチェック（垂直に近い面のみ）
    float VerticalAngle = FMath::Acos(FVector::DotProduct(Normal, FVector::UpVector)) * 180.0f / PI;
    if (VerticalAngle < Settings.MinWallAngle)
    {
        return false;
    }

    // キャラクターの前方向と壁の法線の角度をチェック
    // 壁の法線は壁から外側を向いているので、キャラクターが壁に向かっている場合は
    // 前方向と法線が逆向き（内積が負）になる
    FVector ForwardDirection = Owner->GetActorForwardVector();
    float DotProduct = FVector::DotProduct(ForwardDirection, Normal);

    // 内積から角度を計算（-1.0 ～ 1.0 の範囲）
    // -1.0 = 180度（真逆）、0.0 = 90度（直角）、1.0 = 0度（同じ向き）
    float AngleDegrees = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f)) * 180.0f / PI;

    // 壁に対して正面から接触している場合は無効（壁走り不可）
    // 斜めから接触している場合のみ有効（壁走り可能）
    // 例：30度以下または150度以上の角度は正面すぎるので無効
    float MinValidAngle = Settings.MinValidAngle; // デフォルト: 30度
    float MaxValidAngle = Settings.MaxValidAngle; // デフォルト: 150度

    // 正面に近すぎる（30度未満または150度超）場合は無効
    if (AngleDegrees < MinValidAngle || AngleDegrees > MaxValidAngle)
    {
        //UE_LOG(LogTemp, Verbose, TEXT("WallDetection: Too frontal approach, invalid (Angle: %.2f degrees)"), AngleDegrees);
        return false;
    }

    //UE_LOG(LogTemp, VeryVerbose, TEXT("WallDetection: Valid wall detected at angle: %.2f degrees"), AngleDegrees);
    return true;
}
bool UWallDetectionComponent::PerformRaycast(const FVector& Direction, FHitResult& OutHit) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    FVector Start = GetOwner()->GetActorLocation();
    FVector End = Start + Direction * Settings.DetectionDistance;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    return GetWorld()->LineTraceSingleByChannel(
        OutHit,
        Start,
        End,
        ECC_Visibility,
        Params
    );
}

void UWallDetectionComponent::DrawDebug(const FVector& Direction, bool bHit, const FHitResult& Hit) const
{
    if (!GetWorld())
    {
        return;
    }

    FVector Start = GetOwner()->GetActorLocation();
    FVector End = Start + Direction * Settings.DetectionDistance;
    FColor Color = bHit ? FColor::Green : FColor::Red;

    DrawDebugLine(GetWorld(), Start, End, Color, false, Settings.DetectionInterval);

    if (bHit)
    {
        DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.0f, FColor::Yellow, false, Settings.DetectionInterval);
        DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 50.0f, FColor::Blue, false, Settings.DetectionInterval);
    }
}

void UWallDetectionComponent::SetDetectionEnabled(bool bEnabled)
{
    bDetectionEnabled = bEnabled;

    if (!bEnabled && bIsWallDetected)
    {
        // 検出を無効化したら現在の壁を失う
        AActor* LostWall = CurrentWall.Get();
        bIsWallDetected = false;
        CurrentWall = nullptr;
        OnWallLost.Broadcast(LostWall);
    }
}

bool UWallDetectionComponent::CheckWallStillExists(const FVector& CharacterLocation, float TraceDistance) const
{
    if (CurrentWallNormal.IsNearlyZero())
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // 壁方向にレイキャストを実行
    FVector StartLocation = CharacterLocation;
    FVector EndLocation = StartLocation + (CurrentWallNormal * TraceDistance);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());

    // レイキャスト実行
    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        QueryParams
    );

    // デバッグ描画
#if ENABLE_DRAW_DEBUG
    DrawDebugLine(
        World,
        StartLocation,
        EndLocation,
        bHit ? FColor::Green : FColor::Red,
        false,
        0.1f,
        0,
        2.0f
    );
#endif

    if (!bHit)
    {
        UE_LOG(LogTemp, Verbose, TEXT("WallRun: Wall lost (no hit)"));
        return false;
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("WallRun: Wall still detected (Distance: %.2f)"),
        HitResult.Distance);
    return true;
}

FVector UWallDetectionComponent::GetWallDirection() const
{
    return CurrentWallNormal;
}
