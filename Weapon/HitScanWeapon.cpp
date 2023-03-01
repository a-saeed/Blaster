// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

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
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHitResult;
		PerformLineTrace(Start, HitTarget, FireHitResult);

		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHitResult.GetActor());
		if (BlasterCharacter)
		{
			if (HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass());
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
	FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

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
}
/*
* 
* Scatter
*/
FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	FVector RandomPointInSphere = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0, SphereRadius);
	FVector EndLocation = SphereCenter + RandomPointInSphere;
	FVector ToEnd = EndLocation - TraceStart;

	/*DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLocation, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEnd, FColor::Blue, true);
	*/
	return FVector(TraceStart + ToEnd);
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