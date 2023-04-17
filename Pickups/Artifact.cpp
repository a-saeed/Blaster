#include "Artifact.h"
#include "Components/StaticMeshComponent.h"
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

