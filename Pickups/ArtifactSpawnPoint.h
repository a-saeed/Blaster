// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArtifactSpawnPoint.generated.h"

UCLASS()
class BLASTER_API AArtifactSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	

	AArtifactSpawnPoint();

	void SpawnArtifact();

private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<class APickup> ArtifactClass;

public:	
};
