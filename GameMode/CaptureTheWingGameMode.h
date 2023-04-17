// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CaptureTheWingGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ACaptureTheWingGameMode : public ATeamsGameMode
{
	GENERATED_BODY()

public:

	void ArtifactCaptured(class ABlasterCharacter* BlasterCharacter);
};
