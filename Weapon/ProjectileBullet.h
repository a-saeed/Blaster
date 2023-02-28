// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * Heirarchy: Projectile Bullet ->Projectile
 */
UCLASS()
class BLASTER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()
	
public:
	AProjectileBullet();

	UFUNCTION()																			//must be a ufunction to be bound to a dynamic delegate
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

protected:

	virtual void BeginPlay() override;

	//inherting from Projectile, no need for a ufunction macro
	//virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	void ApplyPointDamage(AActor* Target);
};
