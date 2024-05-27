// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallOnGameThread.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CallOnGameThread : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	//~ Begin UK2Node Interface.
	virtual FText GetMenuCategory() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
	//~ End UK2Node Interface.
};
