// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	/*
	* -Get the instigator controller from the owner character
	* -Get muzzle flash socket from the weapon mesh
	* -Get socket transform which will be used to determine the start point for the line trace
	* -Perform a line trace that depends on if a wepaon applies scatter + play beam effect.
	* -only apply damage on the server.
	*/
	APawn* OwnerPawn = Cast<APawn>(GetOwner());			//Using ACharacter should also work
	if (!OwnerPawn) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector_NetQuantize Start = SocketTransform.GetLocation();

		FHitResult FireHitResult;
		PerformLineTrace(Start, HitTarget, FireHitResult);

		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHitResult.GetActor());
		if (BlasterCharacter && InstigatorController)
		{
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			if (HasAuthority() && bCauseAuthDamage)
			{
				UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass());
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
					BlasterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						BlasterCharacter,
						Start,
						FireHitResult.ImpactPoint,
						BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime,  //the time corresponding to when the victim character was where we thought it was when we fired..
						this);
				}
			}

			PlayCharacterEffects(FireHitResult);
		}

		else if (FireHitResult.bBlockingHit)
		{
			PlaySurfaceEffects(FireHitResult);
		}

		if (GetWeaponType() == EWeaponType::EWT_SniperRifle && SniperGlow)			//play sniper glow effect
		{
			UGameplayStatics::SpawnEmitterAttached(
				SniperGlow,
				GetRootComponent(),
				FName(""),
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator(),
				EAttachLocation::KeepWorldPosition);
		}
	}
}
/*
*
* Line Trace
*/
void AHitScanWeapon::PerformLineTrace(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHitResult)
{
	FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;

	GetWorld()->LineTraceSingleByChannel(OutHitResult,
		TraceStart,
		End,
		ECollisionChannel::ECC_Visibility
	);

	FVector BeamEnd = End;

	if (OutHitResult.bBlockingHit)
	{
		BeamEnd = OutHitResult.ImpactPoint;
	}
	PlayBeamTrailEffect(TraceStart, BeamEnd);

	//DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), BeamEnd, 10.f, 12, FColor::Orange, false, 5.f);
	//DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEnd, FColor::Blue, true);
}

/*
* 
* Effects
*/
void AHitScanWeapon::PlayBeamTrailEffect(const FVector& BeamStart, const FVector& BeamEnd)
{
	if (BeamParticles)
	{
		UParticleSystemComponent* BeamComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, BeamStart, FRotator::ZeroRotator, true);
		if (BeamComponent)
		{
			BeamComponent->SetVectorParameter(FName("Target"), BeamEnd);			//add the endpoint parameter to the particle system
		}
	}
}

void AHitScanWeapon::PlaySurfaceEffects(FHitResult& FireHitResult)
{
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHitResult.ImpactPoint, FireHitResult.ImpactNormal.Rotation());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, FireHitResult.ImpactPoint, 0.5f, FMath::FRandRange(-0.5f, 0.5f));
	}
}

void AHitScanWeapon::PlayCharacterEffects(FHitResult& FireHitResult)
{
	if (CharacterImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CharacterImpactParticles, FireHitResult.ImpactPoint, FireHitResult.ImpactNormal.Rotation());
	}
	if (CharacterImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CharacterImpactSound, FireHitResult.ImpactPoint, 0.5f, FMath::FRandRange(-0.5f, 0.5f));
	}
}