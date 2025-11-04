// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "SaveGamePlugin.h"

#include "Modules/ModuleManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE UE_MODULE_NAME

class FSaveGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
};

void FSaveGameModule::StartupModule()
{
#if WITH_EDITOR
	FNotificationInfo Info(
		LOCTEXT("ProductionWarning", "This save game plugin is not production ready!\n"
			"Additionally, World Partition isn't fully supported!"));
	Info.ExpireDuration = 10.f;
	Info.Image = FAppStyle::Get().GetBrush(TEXT("MessageLog.Warning"));
	FSlateNotificationManager::Get().AddNotification(Info);
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSaveGameModule, SaveGamePlugin)
