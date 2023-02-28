// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystem.h"
#include "Components/SphereComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UCombatComponent* Combat = BlasterCharacter->GetCombatComponent();
		if (Combat)
		{
			bCharacterPickedAmmo = Combat->PickupAmmo(Weapontype, AmmoAmount);
		}
		if (bCharacterPickedAmmo)
		{
			Destroy();
		}
	}
}

void AAmmoPickup::PlayAmmoFullFX()
{
	if (FullSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FullSound, GetActorLocation());
	}
	if (FullParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FullParticle, GetActorTransform());
	}
}
