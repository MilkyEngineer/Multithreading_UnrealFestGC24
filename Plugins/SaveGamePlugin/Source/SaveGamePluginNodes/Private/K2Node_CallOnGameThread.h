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
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ End UK2Node Interface.
};
