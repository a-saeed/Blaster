// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blaster/BlasterTypes/Team.h"
#include "WingZone.generated.h"

UCLASS()
class BLASTER_API AWingZone : public AActor
{
	GENERATED_BODY()
	
public:	

	AWingZone();



protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:

	UPROPERTY(EditAnywhere)
	ETeam ZoneTeam;

	UPROPERTY(EditAnywhere)
	class USphereComponent* ZoneSphere;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* ZoneMesh;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* CaptureParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* CasptureSound;

public:	
};
