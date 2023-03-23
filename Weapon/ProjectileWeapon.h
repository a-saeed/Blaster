// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()
	
protected:

	virtual void BeginPlay() override;

	virtual void Fire(const FVector& HitTarget) override;

private:

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<class AProjectile> ServerSideRewindProjectileClass;
};
