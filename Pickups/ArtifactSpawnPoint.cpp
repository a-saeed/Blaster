// Fill out your copyright notice in the Description page of Project Settings.


#include "ArtifactSpawnPoint.h"
#include "Blaster/Pickups/Artifact.h"

AArtifactSpawnPoint::AArtifactSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AArtifactSpawnPoint::SpawnArtifact()
{
	if (ArtifactClass) GetWorld()->SpawnActor<AArtifact>(ArtifactClass, GetActorLocation(), GetActorRotation());
}