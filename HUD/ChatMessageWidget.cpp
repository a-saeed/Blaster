// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatMessageWidget.h"
#include "Components/TextBlock.h"

void UChatMessageWidget::SetMessageText(FString PlayerName, FString Message)
{
	FString Text = FString::Printf(TEXT("%s: %s"), *PlayerName, *Message);

	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Text));
	}
}
