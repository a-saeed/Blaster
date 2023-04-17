// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArtifactStatus.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UArtifactStatus : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ArtifactStatusText;

	void SetArtifactStatusText(FString ArtifactStatus, FSlateColor Color);
};
