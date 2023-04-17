// Fill out your copyright notice in the Description page of Project Settings.


#include "Artifact.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
AArtifact::AArtifact()
{
	SetReplicateMovement(true);

	PickupMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	PickupMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	LightBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Light Beam"));
	LightBeam->SetupAttachment(RootComponent);
	LightBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LightBeam->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
}

void AArtifact::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(AArtifact, ArtifactState);
	DOREPLIFETIME(AArtifact, BlasterOwnerCharacter);
}

void AArtifact::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);
}

void AArtifact::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// only called on the server

	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OtherActor) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && !BlasterOwnerCharacter->IsElimmed() && GetOwner() == nullptr)
	{
		SetOwner(BlasterOwnerCharacter);
		// disable overlap events
		GetOverlapSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		SetArtifactState(EArtifactState::EAS_Equipped);
	}
}

void AArtifact::SetArtifactState(EArtifactState NewState)
{
	ArtifactState = NewState;

	OnArtifactStateSet();
}

void AArtifact::OnRep_ArtifactState()
{
	OnArtifactStateSet();
}

