// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AProjectile();

	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;

	/*
	* Used with sevrer side rewind, accessed by projectile weapon class.. 
	*/
	bool bUseServerSideRewind = false;		// if true projectile will send a server score request once it hits a target
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000;

	/*
	* DAMAGE
	*/

	// only set this for rockets and grenades
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	// Doesn't matter for rockets and grenades
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 20.f;

protected:
	
	virtual void BeginPlay() override;

	/*
	* HIT EVENT
	*/

	UFUNCTION() //it has to be a ufunction to work
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/*
	* BODY
	*/

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	/*
	* Fx
	*/

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* CharacterImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* CharacterImpactSound;

	void SetImpactSurfaceEffects(AActor* OtherActor);

	/*
	* MOVEMENT
	*/

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	/*
	* 
	* Common Functionalities for Rokects and Grenade Launchers
	* 
	*/

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	void SpawnTrailSystem();
	void SpawnBulletTracer();
	void StartDestroyTimer();
	void DestroyTimerFinished();
	void ExplodeDamageWithFX();

	/*
	*
	* Radial damage parameters
	*
	*/

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius;

private:

	/*
	* FX
	*/

	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;

	class UParticleSystemComponent* TracerComponent;

	UPROPERTY()
	UParticleSystem* ParticlesToPlay;

	UPROPERTY()
	USoundCue* SoundToPlay;

	/*
	* Trail System Timer
	*/

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

public:	
	
	
};
