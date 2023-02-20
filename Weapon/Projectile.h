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

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_OnHit(AActor* OtherActor);

protected:
	
	virtual void BeginPlay() override;

	/*
	* HIT EVENT
	*/
	UFUNCTION() //it has to be a ufunction to work
		virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	/*
	* DAMAGE
	*/
	UPROPERTY(EditAnywhere)
		float Damage = 20.f;
	/*
	* BODY
	*/
	UPROPERTY(EditAnywhere)
		class UBoxComponent* CollisionBox;
	/*
	* Fx
	*/
	UPROPERTY(EditAnywhere)
		class UParticleSystem* ParticlesToPlay;

	UPROPERTY(EditAnywhere)
		class USoundCue* SoundToPlay;
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
	void StartDestroyTimer();
	void DestroyTimerFinished();

private:
	/*
	* FX
	*/
	UPROPERTY(EditAnywhere)
		 UParticleSystem* Tracer;

	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
		UParticleSystem* ImpactParticles;
	
	UPROPERTY(EditAnywhere)
		USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
		UParticleSystem* CharacterImpactParticles;
	
	UPROPERTY(EditAnywhere)
		USoundCue* CharacterImpactSound;
	/*
	* Trail System Timer
	*/
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

public:	
	
	
};
