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

	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);

	void PerformLineTrace(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHitResult);

	void PlayBeamTrailEffect(const FVector& BeamStart, const FVector& BeamEnd);

	void PlaySurfaceEffects(FHitResult& FireHitResult);

	void PlayCharacterEffects(FHitResult& FireHitResult);

	UPROPERTY(EditAnywhere)
	float Damage = 10.f;

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

	/*
	* Trace end with scatter
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;
};
