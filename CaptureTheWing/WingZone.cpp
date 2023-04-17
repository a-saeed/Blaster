// Fill out your copyright notice in the Description page of Project Settings.


#include "WingZone.h"
#include "Components/SphereComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Pickups/Artifact.h"
#include "Blaster/GameMode/CaptureTheWingGameMode.h"
#include "Blaster/GameMode/CaptureTheWingGameMode.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

AWingZone::AWingZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Zone Mesh"));
	ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SetRootComponent(ZoneMesh);

	ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Zone Sphere"));
	ZoneSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ZoneSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ZoneSphere->SetupAttachment(ZoneMesh);
}

void AWingZone::BeginPlay()
{
	Super::BeginPlay();

	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AWingZone::OnSphereOverlap);
}

void AWingZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACaptureTheWingGameMode* WingGameMode = Cast<ACaptureTheWingGameMode>(GetWorld()->GetAuthGameMode());

	ABlasterCharacter* OverlappingCharacter = Cast<ABlasterCharacter>(OtherActor);
	
	bool bShouldScore = OverlappingCharacter &&
		OverlappingCharacter->GetPlayerTeam() == ZoneTeam &&
		OverlappingCharacter->IsHoldingArtifact();

	if (bShouldScore && WingGameMode)
	{
		WingGameMode->ArtifactCaptured(OverlappingCharacter);

		if (CaptureParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CaptureParticles, GetActorLocation(), GetActorRotation());
		}
		if (CasptureSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), CasptureSound, GetActorLocation());
		}
	}
}

