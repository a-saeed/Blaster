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

protected:
	
	virtual void BeginPlay() override;

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

public:	
	
	
};
