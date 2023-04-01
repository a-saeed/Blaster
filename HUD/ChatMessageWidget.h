// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatMessageWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UChatMessageWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* ChatBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MessageText;

	void SetMessageText(FString PlayerName, FString Message);
};
