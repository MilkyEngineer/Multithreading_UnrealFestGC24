// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "K2Node_CallOnGameThread.h"

#include "SaveGameFunctionLibrary.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CreateDelegate.h"
#include "KismetCompiler.h"
#include "ToolMenu.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"

#define LOCTEXT_NAMESPACE "SaveGamePluginNodes"

FText CreateTitle(const FText& FunctionName)
{
	return FText::Format(LOCTEXT("CallOnGameThread_MenuTitle", "Game Thread: {0}"), FunctionName);
}

FText CreateCategory(const UFunction* Function)
{
	if (Function)
	{
		return UK2Node_CallFunction::GetDefaultCategoryForFunction(
			Function, LOCTEXT("CallOnGameThread_CategoryTitle", "Call on Game Thread"));
	}
	return FText::GetEmpty();
}

FText UK2Node_CallOnGameThread::GetMenuCategory() const
{
	return CreateCategory(GetTargetFunction());
}

FText UK2Node_CallOnGameThread::GetTooltipText() const
{
	return FText::Format(
		LOCTEXT("CallOnGameThread_TooltipText",
			"Will ensure that this function is run from the Game Thread.\n"
			"{0}"), Super::GetTooltipText());
}

FSlateIcon UK2Node_CallOnGameThread::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = GetNodeTitleColor();
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "Graph.AnimationFastPathIndicator");
}

FLinearColor UK2Node_CallOnGameThread::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.8f, 0.2f);
}

void UK2Node_CallOnGameThread::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	const FName PN_Delegate(TEXT("Delegate"));

	bool bSuccess = true;
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	const UFunction* TargetFunction = GetTargetFunction();

	UK2Node_CreateDelegate* CreateDelegate_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CreateDelegate>(this, SourceGraph);
	CreateDelegate_Node->SetFunction(GetFunctionName());
	CreateDelegate_Node->SelectedFunctionGuid = FunctionReference.GetMemberGuid();
	CreateDelegate_Node->HandleAnyChangeWithoutNotifying();
	CreateDelegate_Node->AllocateDefaultPins();

	check(CreateDelegate_Node->GetFunctionName().IsValid());

	const UFunction* CallOnGameThread = USaveGameFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USaveGameFunctionLibrary, CallOnGameThread));
	UK2Node_CallFunction* CallFunction_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunction_Node->SetFromFunction(CallOnGameThread);
	CallFunction_Node->AllocateDefaultPins();

	// Connect self pins
	bSuccess &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Self), *CreateDelegate_Node->GetObjectInPin()).CanSafeConnect();

	// Connect exec pins
	bSuccess &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Execute), *CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute)).CanSafeConnect();
	bSuccess &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then), *CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Then)).CanSafeConnect();

	UEdGraphPin* CreateDelegate_DelegatePin = CreateDelegate_Node->GetDelegateOutPin();
	UEdGraphPin* CallFunction_DelegatePin = CallFunction_Node->FindPinChecked(PN_Delegate);

	// Connect delegate pins
	bSuccess &= Schema->TryCreateConnection(CallFunction_DelegatePin, CreateDelegate_DelegatePin);

	for (UEdGraphPin* Pin : Pins)
	{
		if (!Schema->IsSelfPin(*Pin) && !Schema->IsExecPin(*Pin))
		{
			UEdGraphPin* OtherPin = CallFunction_Node->FindPin(Pin->GetFName(), Pin->Direction);
			if (OtherPin == nullptr)
			{
				OtherPin = CallFunction_Node->CreatePin(Pin->Direction, Pin->PinType, Pin->GetFName());
			}
			bSuccess &= CompilerContext.MovePinLinksToIntermediate(*Pin, *OtherPin).CanSafeConnect();
		}
	}

	// We need to do this here... as it gets overridden by TryCreateConnection
	FMemberReference::FillSimpleMemberReference<UFunction>(TargetFunction, CallFunction_DelegatePin->PinType.PinSubCategoryMemberReference);
}

void UK2Node_CallOnGameThread::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	struct GetMenuActions_Utils
	{
		static void SetNodeFunc(UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UFunction> FunctionPtr)
		{
			UK2Node_CallOnGameThread* Node = CastChecked<UK2Node_CallOnGameThread>(NewNode);
			if (FunctionPtr.IsValid())
			{
				const UFunction* Func = FunctionPtr.Get();
				Node->SetFromFunction(Func);
			}
		}

		static void SetUISpec(const FBlueprintActionContext& /*Context*/, const IBlueprintNodeBinder::FBindingSet& Bindings, FBlueprintActionUiSpec* UiSpecOut, TWeakObjectPtr<UFunction> FunctionPtr)
		{
			const UFunction* Function = FunctionPtr.Get();
			UiSpecOut->MenuName = GetUserFacingFunctionName(Function);
			UiSpecOut->Category = CreateCategory(Function);
		}
	};

	UClass* NodeClass = GetClass();
	auto CreateNodeSpawner = [NodeClass](const UFunction* Function) -> UBlueprintNodeSpawner*
	{
		if (!Function->GetName().EndsWith(TEXT("_GameThread")) ||
			Function->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction))
		{
			return nullptr;
		}

		UBlueprintNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(Function);
		check(NodeSpawner != nullptr);
		NodeSpawner->NodeClass = NodeClass;

		TWeakObjectPtr<UFunction> FunctionPtr = MakeWeakObjectPtr(const_cast<UFunction*>(Function));
		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(GetMenuActions_Utils::SetNodeFunc, FunctionPtr);
		NodeSpawner->DynamicUiSignatureGetter = UBlueprintNodeSpawner::FUiSpecOverrideDelegate::CreateStatic(GetMenuActions_Utils::SetUISpec, FunctionPtr);

		return NodeSpawner;
	};

	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) ||
			FKismetEditorUtilities::IsClassABlueprintSkeleton(Class))
		{
			continue;
		}

		for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;

			if (UBlueprintNodeSpawner* NodeSpawner = CreateNodeSpawner(Function))
			{
				ActionRegistrar.AddBlueprintAction(Function, NodeSpawner);
			}
		}
	}
}

bool UK2Node_CallOnGameThread::CanPasteHere(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::CanPasteHere(TargetGraph)
		&& TargetGraph && TargetGraph->GetFName() == GET_FUNCTION_NAME_CHECKED(ISaveGameObject, OnSerialize)
		&& Blueprint && FBlueprintEditorUtils::ImplementsInterface(Blueprint, true, USaveGameObject::StaticClass())
		&& FBlueprintEditorUtils::GetInterfaceFunction(Blueprint, TargetGraph->GetFName());
}

#undef LOCTEXT_NAMESPACE
