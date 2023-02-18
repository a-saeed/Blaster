// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

	AProjectileRocket();

protected:
	virtual void BeginPlay() override;
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

private:

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RocketMesh;

	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;
	/*
	* Trail System Timer
	*/
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	void DestroyTimerFinished();
	/*
	* Looping sound
	*/
	UPROPERTY(EditAnywhere)
	USoundCue* RocketLoopingSound;

	UPROPERTY()
	UAudioComponent* RocketLoopingSoundComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* RocketLoopingSoundAttenuation;			//att needed as default gets unset when created dynamically
};