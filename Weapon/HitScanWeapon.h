// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:
	
	virtual void Fire(const FVector& HitTarget) override;


protected:

	void PerformLineTrace(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHitResult);

	void PlayBeamTrailEffect(const FVector& BeamStart, const FVector& BeamEnd);

	void PlaySurfaceEffects(FHitResult& FireHitResult);

	void PlayCharacterEffects(FHitResult& FireHitResult);

private:

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* CharacterImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* CharacterImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* SniperGlow;
};
