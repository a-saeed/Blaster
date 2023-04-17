// Fill out your copyright notice in the Description page of Project Settings.


#include "ArtifactStatus.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"

void UArtifactStatus::SetArtifactStatusText(FString ArtifactStatus, FSlateColor Color)
{
	FString ArtifactStatusString = FString::Printf(TEXT("%s"), *ArtifactStatus);

	if (ArtifactStatusText)
	{
		ArtifactStatusText->SetText(FText::FromString(ArtifactStatusString));
		ArtifactStatusText->SetColorAndOpacity(Color);
	}
}
