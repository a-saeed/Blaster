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
	* -Extend the end vector by 25% to ensure a target at exactly the end of the line trace 
	* -Permorm the line trace and coressponding hit particles on all machines..but only apply damage on the server.
	*/
	APawn* OwnerPawn = Cast<APawn>(GetOwner());			//Using ACharacter should also work
	if (!OwnerPawn) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
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

		FVector BeamEnd = End;
		if (FireHitResult.bBlockingHit)
		{
			BeamEnd = FireHitResult.ImpactPoint;

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
		if (BeamParticles)
		{
			UParticleSystemComponent* BeamComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
			if (BeamComponent)
			{
				BeamComponent->SetVectorParameter(FName("Target"), BeamEnd);			//add the endpoint parameter to the particle system
			}
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	FVector RandomPointInSphere = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0, SphereRadius);
	FVector EndLocation = SphereCenter + RandomPointInSphere;
	FVector ToEnd = EndLocation - TraceStart;

	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLocation, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEnd * 50, FColor::Blue, true);

	return FVector(TraceStart + ToEnd);
}