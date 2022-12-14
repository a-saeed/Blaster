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
private:

	/*
	* BODY
	*/
	UPROPERTY(EditAnywhere)
		class UBoxComponent* CollisionBox;

	/*
	* MOVEMENT
	*/
	UPROPERTY(VisibleAnywhere)
		class UProjectileMovementComponent* ProjectileMovementComponent;
	/*
	* FX
	*/
	UPROPERTY(EditAnywhere)
		class UParticleSystem* Tracer;

	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
		UParticleSystem* ImpactParticles;
	
	UPROPERTY(EditAnywhere)
		class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
		UParticleSystem* CharacterImpactParticles;
	
	UPROPERTY(EditAnywhere)
		class USoundCue* CharacterImpactSound;
	
	UPROPERTY()
	UParticleSystem* ParticlesToPlay;

	UPROPERTY()
	USoundCue* SoundToPlay;
public:	
	
	
};
