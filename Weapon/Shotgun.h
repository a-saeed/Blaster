// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()
	
public:

	virtual void Fire(const FVector& HitTarget) override;
	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector>& OutHitTargets);

private:

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfShards = 10;			//No. of line traces to perform
};
