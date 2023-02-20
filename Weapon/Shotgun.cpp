// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());			//Using ACharacter should also work
	if (!OwnerPawn) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> HitMap;

		for (uint32 i = 0; i < NumberOfShards; i++)
		{
			FHitResult FireHitResult;
			PerformLineTrace(Start, HitTarget, FireHitResult);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHitResult.GetActor());
			if (BlasterCharacter)
			{
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					HitMap.Emplace(BlasterCharacter, 1);
				}
				PlayCharacterEffects(FireHitResult);
			}
			else if(FireHitResult.bBlockingHit)
			{
				PlaySurfaceEffects(FireHitResult);
			}
		}

		for (TPair<ABlasterCharacter*, uint32> HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,					//damage * how many times this character got hit
					InstigatorController,
					this,
					UDamageType::StaticClass());

			}
		}
	}
}
