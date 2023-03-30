// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());

	APawn* OwnerPawn = Cast<APawn>(GetOwner());			//Using ACharacter should also work
	if (!OwnerPawn) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector Start = SocketTransform.GetLocation();
 
		TMap<ABlasterCharacter*, uint32> HitMap;
		TMap<ABlasterCharacter*, uint32> HeadShotsMap;

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHitResult;
			PerformLineTrace(Start, HitTarget, FireHitResult);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHitResult.GetActor());
			if (BlasterCharacter)
			{
				const bool bHeadShot = FireHitResult.BoneName.ToString() == FString("head") ? true : false;

				if (bHeadShot)
				{
					if (HeadShotsMap.Contains(BlasterCharacter)) HeadShotsMap[BlasterCharacter]++;
					else HeadShotsMap.Emplace(BlasterCharacter, 1);
				}
				else    // Not a headshot
				{
					if (HitMap.Contains(BlasterCharacter)) HitMap[BlasterCharacter]++;
					else HitMap.Emplace(BlasterCharacter, 1);
				}

				PlayCharacterEffects(FireHitResult);
			}
			else if (FireHitResult.bBlockingHit)
			{
				PlaySurfaceEffects(FireHitResult);
			}
		}

		
		TArray<ABlasterCharacter*> HitCharacters;	// Array to send hitcharacters for SSR
		TMap<ABlasterCharacter*, float> DamageMap;	// Maps hitCharacter to Total damage

		// Calculate body shot damage by multiplying times hit x Damage - store in damage map
		for (auto& HitPair : HitMap)
		{
			if (HitPair.Key)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);

				HitCharacters.AddUnique(HitPair.Key);
			}
		}

		// Calculate head shot damage by multiplying times hit x HeadShotDamage - store in damage map
		for (auto& HeadShotPair : HeadShotsMap)
		{
			if (HeadShotPair.Key)
			{
				//chratacer sustained body shots; accumulate damage
				if (DamageMap.Contains(HeadShotPair.Key)) DamageMap[HeadShotPair.Key] += HeadShotPair.Value * HeadShotDamage;
				//charcater only sustained head shot damage
				else DamageMap.Emplace(HeadShotPair.Key, HeadShotPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotPair.Key);
			}
		}

		//if on the server.. loop through damage map to apply total damage for each character
		bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
		if (HasAuthority() && bCauseAuthDamage)
		{
			for (auto& DamagePair : DamageMap)
			{
				if (DamagePair.Key && InstigatorController)
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key,						//Character that was hit
						DamagePair.Value,					//damage calculated in the two for loops above
						InstigatorController,
						this,
						UDamageType::StaticClass());
				}
			}
		}
		
		/*this is a client machine..Call server-side rewind */
		if (!HasAuthority() && bUseServerSideRewind)
		{
			BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
			BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;

			if (BlasterOwnerController &&
				BlasterOwnerCharacter &&
				BlasterOwnerCharacter->GetLagCompensation() &&
				BlasterOwnerCharacter->IsLocallyControlled()	//since Fire() is called on all machines.. we only need the locally controlled character to send a server score request
				)	
			{
				BlasterOwnerCharacter->GetLagCompensation()->ServerShotgunScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime  //the time corresponding to when the victim character was where we thought it was when we fired..
				);
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutHitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (!MuzzleFlashSocket) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfShards; i++)
	{
		const FVector RandomPointInSphere = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0, SphereRadius);
		const FVector EndLocation = SphereCenter + RandomPointInSphere;
		const FVector ToEnd = EndLocation - TraceStart;

		OutHitTargets.Add(TraceStart + ToEnd);
	}
}
