// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"
#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	StartSpawnTimer(nullptr);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::SpawnPickup()
{
	if (!PickupClasses.IsEmpty())
	{
		int32 RandomPickup = FMath::RandRange(0, PickupClasses.Num() - 1);

		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[RandomPickup], GetActorTransform());

		if (SpawnedPickup)
		{
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnTimer);	//bind to AActor OnDestroyed delegate
		}
	}
}

void APickupSpawnPoint::StartSpawnTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::RandRange(MinSpawnTime, MaxSpawnTime);

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &APickupSpawnPoint::SpawnTimerFinished, SpawnTime);
}

void APickupSpawnPoint::SpawnTimerFinished()
{
	if (HasAuthority())
	{
		SpawnPickup();
	}
}
