// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	/*
	* -Get the instigator controller from the owner character
	* -Get muzzle flash socket from the weapon mesh
	* -Get socket transform which will be used to determine the start point for the line trace
	* -Extend the end vector by 25% to ensure a target at exactly the end of the line trace 
	* -Permorm the line trace and coressponding hit particles on all machines..but only apply damage on the server.
	*/
	APawn* OwnerPawn = Cast<APawn>(GetOwner());			//Using ACharacter should also work
	if (!OwnerPawn) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket && InstigatorController)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;

		FHitResult FireHitResult;
		GetWorld()->LineTraceSingleByChannel(FireHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (FireHitResult.bBlockingHit)
		{
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHitResult.GetActor());
			if (BlasterCharacter)
			{
				if (HasAuthority())
				{
					UGameplayStatics::ApplyDamage(
						BlasterCharacter,
						Damage,
						InstigatorController,
						this,
						UDamageType::StaticClass());
				}
				if (CharacterImpactParticles && CharacterImpactSound)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CharacterImpactParticles, FireHitResult.ImpactPoint, FireHitResult.ImpactNormal.Rotation());
					UGameplayStatics::PlaySoundAtLocation(GetWorld(), CharacterImpactSound, FireHitResult.ImpactPoint);
				}
			}
			if (ImpactParticles && ImpactSound)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHitResult.ImpactPoint, FireHitResult.ImpactNormal.Rotation());
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, FireHitResult.ImpactPoint);
			}
		}
	}
}
