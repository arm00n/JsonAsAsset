// Copyright JAA Contributors 2024-2025

#include "JsonAsAsset.h"
#include "JsonAsAssetCommands.h"

#include "./Importers/Constructor/Importer.h"

// ------------------------------------------------------------------------------------------------------------>
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "Interfaces/IMainFrameModule.h"
#else
#include "MainFrame/Public/Interfaces/IMainFrameModule.h"
#endif

#if ENGINE_MAJOR_VERSION == 4
#include "ToolMenus.h"
#include "Logging/MessageLog.h"
#include "LevelEditor.h"
#endif

#include "Windows/WindowsHWrapper.h"

#include "Interfaces/IPluginManager.h"
#include "Settings/JsonAsAssetSettings.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IMessageLogListing.h"
#include "ISettingsModule.h"
#include "MessageLogModule.h"
#include "Styling/SlateIconFinder.h"
#include <TlHelp32.h>

// Settings
#include "EditorStyleSet.h"
#include "./Settings/Details/JsonAsAssetSettingsDetails.h"

#include "Modules/AboutJsonAsAsset.h"
#include "Utilities/AppStyleCompatibility.h"
#include "Utilities/AssetUtilities.h"
// <------------------------------------------------------------------------------------------------------------

#ifdef _MSC_VER
#undef GetObject
#endif

#define LOCTEXT_NAMESPACE "FJsonAsAssetModule"

#if PLATFORM_WINDOWS
    static TWeakPtr<SNotificationItem> ImportantNotificationPtr;
    static TWeakPtr<SNotificationItem> LocalFetchNotificationPtr;
#endif

void FJsonAsAssetModule::PluginButtonClicked() {
	Settings = GetDefault<UJsonAsAssetSettings>();
	
	if (Settings->ExportDirectory.Path.IsEmpty())
		return;

	// Invalid Export Directory
	if (Settings->ExportDirectory.Path.Contains("\\")) {
		FNotificationInfo Info(LOCTEXT("JsonAsAssetNotificationTitle", "Export Directory Invalid"));
#if ENGINE_MAJOR_VERSION >= 5
		Info.SubText = LOCTEXT("JsonAsAssetNotificationText",
			"Please fix your export directory in the plugin settings, as it is invalid and contains the character \"\\\"."
		);
#endif

		Info.HyperlinkText = LOCTEXT("UnrealSoftwareRequirements", "JsonAsAsset Plugin Settings");
		Info.Hyperlink = FSimpleDelegate::CreateStatic([]() {
			// Send user to plugin settings
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
				.ShowViewer("Editor", "Plugins", "JsonAsAsset");
		});

		Info.bFireAndForget = true;
		Info.FadeOutDuration = 2.0f;
		Info.ExpireDuration = 3.0f;
		Info.bUseLargeFont = false;
		Info.bUseThrobber = false;
		Info.Image = FJsonAsAssetStyle::Get().GetBrush("JsonAsAsset.Logo");

		LocalFetchNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
		LocalFetchNotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Fail);

		return;
	}

	if (Settings->bEnableLocalFetch) {
		TSharedPtr<SNotificationItem> NotificationItem = LocalFetchNotificationPtr.Pin();

		if (NotificationItem.IsValid()) {
			NotificationItem->SetFadeOutDuration(0.001);
			NotificationItem->Fadeout();
			LocalFetchNotificationPtr.Reset();
		}

		bool bIsLocalHost = Settings->Url.StartsWith("http://localhost");

		if (!IsProcessRunning("LocalFetch.exe") && bIsLocalHost) {
			FNotificationInfo Info(LOCTEXT("JsonAsAssetNotificationTitle", "Local Fetch API"));
#if ENGINE_MAJOR_VERSION >= 5
			Info.SubText = LOCTEXT("JsonAsAssetNotificationText",
				"Please start the Local Fetch API to use JsonAsAsset with no issues, if you need any assistance figuring out Local Fetch and the settings, please take a look at the documentation:"
			);
#endif

			Info.HyperlinkText = LOCTEXT("UnrealSoftwareRequirements", "JsonAsAsset Docs");
			Info.Hyperlink = FSimpleDelegate::CreateStatic([]() {
				FString TheURL = "https://github.com/JsonAsAsset/JsonAsAsset";
				FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);
			});

			Info.bFireAndForget = false;
			Info.FadeOutDuration = 3.0f;
			Info.ExpireDuration = 3.0f;
			Info.bUseLargeFont = false;
			Info.bUseThrobber = false;
			Info.Image = FJsonAsAssetStyle::Get().GetBrush("JsonAsAsset.Logo");

			Info.ButtonDetails.Add(
				FNotificationButtonInfo(LOCTEXT("StartLocalFetch", "Execute LocalFetch API (.EXE)"), FText::GetEmpty(),
					FSimpleDelegate::CreateStatic([]() {
						TSharedPtr<SNotificationItem> NotificationItem = LocalFetchNotificationPtr.Pin();

						if (NotificationItem.IsValid()) {
							NotificationItem->SetFadeOutDuration(0.001);
							NotificationItem->Fadeout();
							LocalFetchNotificationPtr.Reset();
						}

						FString PluginBinariesFolder;

						const TSharedPtr<IPlugin> PluginInfo = IPluginManager::Get().FindPlugin("JsonAsAsset");
						if (PluginInfo.IsValid()) {
							const FString PluginBaseDir = PluginInfo->GetBaseDir();
							PluginBinariesFolder = FPaths::Combine(PluginBaseDir, TEXT("Binaries"));

							if (!FPaths::DirectoryExists(PluginBinariesFolder)) {
								PluginBinariesFolder = FPaths::Combine(PluginBaseDir, TEXT("JsonAsAsset/Binaries"));
							}
						}

						FString FullPath = FPaths::ConvertRelativePathToFull(PluginBinariesFolder + "/Win64/LocalFetch/LocalFetch.exe");
						FPlatformProcess::LaunchFileInDefaultExternalApplication(*FullPath, TEXT("--urls=http://localhost:1500/"), ELaunchVerb::Open);
					})
				)
			);

			LocalFetchNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
			LocalFetchNotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);

			return;
		}
	}

	// Dialog for a JSON File
	TArray<FString> OutFileNames = OpenFileDialog("Open JSON file", "JSON Files|*.json");
	if (OutFileNames.Num() == 0)
		return;

	for (FString& File : OutFileNames) {
		// Clear Message Log
		FMessageLogModule& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		TSharedRef<IMessageLogListing> LogListing = (MessageLogModule.GetLogListing("JsonAsAsset"));
		LogListing->ClearMessages();

		// Import asset by IImporter
		IImporter* Importer = new IImporter();
		Importer->ImportReference(File);
	}
}

void FJsonAsAssetModule::StartupModule() {
    // Initialize plugin style, reload textures, and register commands
    FJsonAsAssetStyle::Initialize();
    FJsonAsAssetStyle::ReloadTextures();
    FJsonAsAssetCommands::Register();

    // Set up plugin command list and map actions
    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FJsonAsAssetCommands::Get().PluginAction,
        FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked),
        FCanExecuteAction()
    );

    // Register menus on startup
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJsonAsAssetModule::RegisterMenus));

    // Check for export directory in settings
    Settings = GetDefault<UJsonAsAssetSettings>();
	
    if (Settings->ExportDirectory.Path.IsEmpty()) {
        const FText TitleText = LOCTEXT("JsonAsAssetNotificationTitle", "Missing export directory for JsonAsAsset");
        const FText MessageText = LOCTEXT("JsonAsAssetNotificationText",
            "JsonAsAsset requires an export directory to handle references and to locally check for files to import. "
            "The plugin may not function properly without this set.\n\nFor more information, please see the documentation for JsonAsAsset."
        );

        FNotificationInfo Info(TitleText);
#if ENGINE_MAJOR_VERSION >= 5
        Info.SubText = MessageText;
#endif

        // Set up hyperlink for documentation
        Info.HyperlinkText = LOCTEXT("UnrealSoftwareRequirements", "JsonAsAsset Docs");
        Info.Hyperlink = FSimpleDelegate::CreateStatic([]() { 
            FString TheURL = "https://github.com/JsonAsAsset/JsonAsAsset";
            FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr); 
        });

        // Notification settings
        Info.bFireAndForget = false;
        Info.FadeOutDuration = 3.0f;
        Info.ExpireDuration = 0.0f;
        Info.bUseLargeFont = false;
        Info.bUseThrobber = false;

        // Add button to open plugin settings
        Info.ButtonDetails.Add(
            FNotificationButtonInfo(
                LOCTEXT("OpenPluginSettings", "Open Settings"),
                FText::GetEmpty(),
                FSimpleDelegate::CreateStatic([]() {
                    TSharedPtr<SNotificationItem> NotificationItem = ImportantNotificationPtr.Pin();
                    if (NotificationItem.IsValid()) {
                        NotificationItem->Fadeout();
                        ImportantNotificationPtr.Reset();
                    }

                    // Send user to plugin settings
                    FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
                        .ShowViewer("Editor", "Plugins", "JsonAsAsset");
                })
            )
        );

        ImportantNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
        ImportantNotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
    }

    // Set up message log for JsonAsAsset
    {
        FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
        FMessageLogInitializationOptions InitOptions;
        InitOptions.bShowPages = true;
        InitOptions.bAllowClear = true;
        InitOptions.bShowFilters = true;
        MessageLogModule.RegisterLogListing("JsonAsAsset", NSLOCTEXT("JsonAsAsset", "JsonAsAssetLogLabel", "JsonAsAsset"), InitOptions);
    }
	

#if ENGINE_MAJOR_VERSION == 4
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
    	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
    	ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FJsonAsAssetModule::AddToolbarExtension));

    	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
#endif

    // Register custom class layout for settings
    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout(UJsonAsAssetSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FJsonAsAssetSettingsDetails::MakeInstance));
}

void FJsonAsAssetModule::ShutdownModule() {
	// Unregister startup callback and tool menus
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	// Shutdown the plugin style and unregister commands
	FJsonAsAssetStyle::Shutdown();
	FJsonAsAssetCommands::Unregister();

	// Unregister message log listing if the module is loaded
	if (FModuleManager::Get().IsModuleLoaded("MessageLog")) {
		FMessageLogModule& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing("JsonAsAsset");
	}
}

void FJsonAsAssetModule::RegisterMenus() {
	FToolMenuOwnerScoped OwnerScoped(this);

	// Extend the Level Editor toolbar
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("JsonAsAsset");

	// Retrieve plugin info
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("JsonAsAsset");

	// Add combo button to toolbar
	FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitComboButton(
		"JsonAsAsset",
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction(),
			FGetActionCheckState()
		),
		FOnGetContent::CreateRaw(this, &FJsonAsAssetModule::CreateToolbarDropdown),
		FText::FromString(Plugin->GetDescriptor().VersionName),
		LOCTEXT("JsonAsAsset", "List of actions for JsonAsAsset"),
		FSlateIcon(FJsonAsAssetStyle::Get().GetStyleSetName(), FName("JsonAsAsset.Logo")),
		false,
		"JsonAsAsset"
	));

#if ENGINE_MAJOR_VERSION >= 5
	Entry.StyleNameOverride = "CalloutToolbar";
#endif
	Entry.SetCommandList(PluginCommands);
}

TArray<FString> FJsonAsAssetModule::OpenFileDialog(FString Title, FString Type) {
	TArray<FString> ReturnValue;

	// Window Handler for Windows
	void* ParentWindowHandle = nullptr;

	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();

	// Define the window handle, if it's valid
	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid()) ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform) {
		uint32 SelectionFlag = 1;

		// Open File Dialog
		DesktopPlatform->OpenFileDialog(ParentWindowHandle, Title, FString(""), FString(""), Type, SelectionFlag, ReturnValue);
	}

	return ReturnValue;
}

bool FJsonAsAssetModule::IsProcessRunning(const FString& ProcessName) {
	bool bIsRunning = false;

	// Convert FString to WCHAR
	const TCHAR* ProcessNameChar = *ProcessName;
	const WCHAR* ProcessNameWChar = (const WCHAR*)ProcessNameChar;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 ProcessEntry;
		ProcessEntry.dwSize = sizeof(ProcessEntry);

		if (Process32First(hSnapshot, &ProcessEntry)) {
			do {
				if (_wcsicmp(ProcessEntry.szExeFile, ProcessNameWChar) == 0) {
					bIsRunning = true;
					break;
				}
			} while (Process32Next(hSnapshot, &ProcessEntry));
		}

		CloseHandle(hSnapshot);
	}

	return bIsRunning;
}

#if ENGINE_MAJOR_VERSION == 4
void FJsonAsAssetModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("JsonAsAsset");

	Builder.AddComboButton(
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction(),
			FGetActionCheckState()
		),
		FOnGetContent::CreateRaw(this, &FJsonAsAssetModule::CreateToolbarDropdown),
		FText::FromString(Plugin->GetDescriptor().VersionName),
		LOCTEXT("JsonAsAsset", "List of actions for JsonAsAsset"),
		FSlateIcon(FJsonAsAssetStyle::Get().GetStyleSetName(), FName("JsonAsAsset.Logo"))
	);
}
#endif

TSharedRef<SWidget> FJsonAsAssetModule::CreateToolbarDropdown() {
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("JsonAsAsset");
	Settings = GetDefault<UJsonAsAssetSettings>();

	FMenuBuilder MenuBuilder(false, nullptr);
	MenuBuilder.BeginSection("JsonAsAssetSection", FText::FromString("JSON Tools v" + Plugin->GetDescriptor().VersionName));
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetAssetTypesMenu", "Asset Types"),
			LOCTEXT("JsonAsAssetAssetTypesMenuToolTip", "List of supported assets for JsonAsAsset"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetSection", LOCTEXT("JsonAsAssetSection", "Asset Classes"));
				{
					for (FString& Asset : ImporterAcceptedTypes)
					{
						if (Asset == "") { // Separator
							InnerMenuBuilder.AddSeparator();
						}
						
						else {
							UClass* Class = FindObject<UClass>(nullptr, *("/Script/Engine." + Asset));
							FText Description = Class ? Class->GetToolTipText() : FText::FromString(Asset);

							InnerMenuBuilder.AddMenuEntry(
								FText::FromString(Asset),
								Description,
								FSlateIconFinder::FindCustomIconForClass(Class, TEXT("ClassThumbnail")),
								FUIAction()
							);
						}
					}
				}
				InnerMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "LevelEditor.Tabs.Viewports")
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetDocumentationButton", "Documentation"),
			LOCTEXT("JsonAsAssetDocumentationButtonTooltip", "Documentation for JsonAsAsset"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Documentation"),
			FUIAction(
				FExecuteAction::CreateLambda([this]() {
					FString TheURL = "https://github.com/JsonAsAsset/JsonAsAsset";
					FPlatformProcess::LaunchURL(*TheURL, nullptr, nullptr);
				})
			),
			NAME_None
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("JsonAsAssetActionButton", "JsonAsAsset"),
			LOCTEXT("JsonAsAssetActionButtonTooltip", "Execute JsonAsAsset"),
			FSlateIcon(FJsonAsAssetStyle::Get().GetStyleSetName(), "JsonAsAsset.Logo"),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::PluginButtonClicked),
				FCanExecuteAction::CreateLambda([this]() {
					const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();

					return !Settings->ExportDirectory.Path.IsEmpty();
				})
			),
			NAME_None
		);
	}

	MenuBuilder.EndSection();

	bActionRequired =
		Settings->ExportDirectory.Path.IsEmpty() //||..
		;

	if (bActionRequired) {
		MenuBuilder.BeginSection("JsonAsAssetActionRequired", FText::FromString("Action Required"));
		{
			// Export Directory Missing
			if (Settings->ExportDirectory.Path.IsEmpty())
				MenuBuilder.AddMenuEntry(
					LOCTEXT("JsonAsAssetActionRequiredButton", "Export Directory Missing"),
					LOCTEXT("JsonAsAssetActionRequiredButtonTooltip", "Change your exports directory in JsonAsAsset's Plugin Settings"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.WarningWithColor"),
					FUIAction(
						FExecuteAction::CreateLambda([this]() {
							// Send user to plugin
							FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
								.ShowViewer("Editor", "Plugins", "JsonAsAsset");
						})
					),
					NAME_None
				);
		}
		MenuBuilder.EndSection();
	}

	CreateLocalFetchDropdown(MenuBuilder);

	MenuBuilder.AddSeparator();

	if (Settings->AssetSettings.bEnableAssetTools) {
		MenuBuilder.AddSubMenu(
			LOCTEXT("JsonAsAssetAssetTypesMenu", "Open Asset Tools"),
			LOCTEXT("JsonAsAssetAssetTypesMenuToolTip", "Extra functionality / tools to do very specific actions with assets."),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
				InnerMenuBuilder.BeginSection("JsonAsAssetSection", LOCTEXT("JsonAsAssetSection", "Asset Tools"));
				{
					InnerMenuBuilder.AddMenuEntry(
						LOCTEXT("ConvexCollisionExButton", "Import Folder Collision Convex"),
						LOCTEXT("ConvexCollisionExButtonTooltip", "Imports convex collision data from a folder of JSON files and applies it to the corresponding assets."),
						FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BspMode"),

						FUIAction(
							FExecuteAction::CreateRaw(this, &FJsonAsAssetModule::ImportConvexCollision),
							FCanExecuteAction::CreateLambda([this]() {
								return true;
							})
						),
						NAME_None
					);
				}
				InnerMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "ProjectSettings.TabIcon")
		);	
	}

	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("JsonAsAssetPluginSettingsButton", "Open Plugin Settings"),
		LOCTEXT("JsonAsAssetPluginSettingsButtonTooltip", "Brings you to the JsonAsAsset Settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"),
		FUIAction(
			FExecuteAction::CreateLambda([this]() {
				// Send user to plugin settings
				FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
					.ShowViewer("Editor", "Plugins", "JsonAsAsset");
			})
		),
		NAME_None
	);
	MenuBuilder.AddMenuEntry(
		LOCTEXT("JsonAsAssetMessageLogButton", "Open Message Log"),
		LOCTEXT("JsonAsAssetMessageLogButtonTooltip", "Message Log for JsonAsAsset"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "MessageLog.TabIcon"),
		FUIAction(
			FExecuteAction::CreateLambda([this]() {
				FMessageLog MessageLogger = FMessageLog(FName("JsonAsAsset"));
				MessageLogger.Open(EMessageSeverity::Info, true);
			})
		),
		NAME_None
	);

	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(
		LOCTEXT("JsonAsAssetAboutButton", "About JsonAsAsset"),
		LOCTEXT("JsonAsAssetAboutButtonTooltip", "More information about JsonAsAsset"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "MessageLog.Action"),
		FUIAction(
			FExecuteAction::CreateLambda([this]() {
				TSharedPtr<SWindow> AboutWindow =
					SNew(SWindow)
					.Title(LOCTEXT("AboutJsonAsAsset", "About JsonAsAsset"))
					.ClientSize(FVector2D(720.f, 175.f))
					.SupportsMaximize(false).SupportsMinimize(false)
					.SizingRule(ESizingRule::FixedSize)
					[
						SNew(SAboutJsonAsAsset)
					];

				IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
				TSharedPtr<SWindow> ParentWindow = MainFrame.GetParentWindow();

				if (ParentWindow.IsValid())
					FSlateApplication::Get().AddModalWindow(AboutWindow.ToSharedRef(), ParentWindow.ToSharedRef());
				else FSlateApplication::Get().AddWindow(AboutWindow.ToSharedRef());
			})
		),
		NAME_None
	);

	return MenuBuilder.MakeWidget();
}

void FJsonAsAssetModule::CreateLocalFetchDropdown(FMenuBuilder MenuBuilder) {
	// Local Fetch must be enabled, and if there is an action required, don't create Local Fetch's dropdown
	if (!Settings->bEnableLocalFetch || bActionRequired) {
		return;
	}
	
	MenuBuilder.BeginSection("JsonAsAssetSection", FText::FromString("Local Fetch"));
	MenuBuilder.AddSubMenu(
		LOCTEXT("JsonAsAssetLocalFetchTypesMenu", "Asset Types"),
		LOCTEXT("JsonAsAssetLocalFetchTypesMenuToolTip", "List of supported classes that can be locally fetched using the API"),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
			InnerMenuBuilder.BeginSection("JsonAsAssetSection", LOCTEXT("JsonAsAssetSection", "Asset Classes"));
			{
				TArray<FString> AcceptedTypes = LocalFetchAcceptedTypes;

				for (FString& Asset : AcceptedTypes) {
					if (Asset == "") { // Separator
						InnerMenuBuilder.AddSeparator();
					}
					
					else {
						UClass* Class = FindObject<UClass>(nullptr, *("/Script/Engine." + Asset));
						FText Description = Class ? Class->GetToolTipText() : FText::FromString(Asset);
						
						InnerMenuBuilder.AddMenuEntry(
							FText::FromString(Asset),
							Description,
							FSlateIconFinder::FindCustomIconForClass(Class, TEXT("ClassThumbnail")),
							FUIAction()
						);
					}
				}
			}
			InnerMenuBuilder.EndSection();
		}),
		false,
		FSlateIcon()
	);

	MenuBuilder.AddSubMenu(
		LOCTEXT("JsonAsAssetLocalFetchCMDMenu", "Command-line Application"),
		LOCTEXT("", ""),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InnerMenuBuilder) {
			InnerMenuBuilder.BeginSection("JsonAsAssetSection", LOCTEXT("JsonAsAssetSection", "Console"));
			{
				InnerMenuBuilder.AddMenuEntry(
					FText::FromString("Execute Local Fetch (.EXE)"),
					FText::FromString(""),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this]() {
							TSharedPtr<SNotificationItem> NotificationItem = LocalFetchNotificationPtr.Pin();

							if (NotificationItem.IsValid()) {
								NotificationItem->SetFadeOutDuration(0.001);
								NotificationItem->Fadeout();
								LocalFetchNotificationPtr.Reset();
							}

							FString PluginBinariesFolder;

							const TSharedPtr<IPlugin> PluginInfo = IPluginManager::Get().FindPlugin("JsonAsAsset");
							if (PluginInfo.IsValid()) {
								const FString PluginBaseDir = PluginInfo->GetBaseDir();
								PluginBinariesFolder = FPaths::Combine(PluginBaseDir, TEXT("Binaries"));

								if (!FPaths::DirectoryExists(PluginBinariesFolder)) {
									PluginBinariesFolder = FPaths::Combine(PluginBaseDir, TEXT("JsonAsAsset/Binaries"));
								}
							}

							FString FullPath = FPaths::ConvertRelativePathToFull(PluginBinariesFolder + "/Win64/LocalFetch/LocalFetch.exe");
							FPlatformProcess::LaunchFileInDefaultExternalApplication(*FullPath, TEXT("--urls=http://localhost:1500/"), ELaunchVerb::Open);
						}),
						FCanExecuteAction::CreateLambda([this]() {
							return !IsProcessRunning("LocalFetch.exe");
						})
					)
				);

				InnerMenuBuilder.AddMenuEntry(
					FText::FromString("Shutdown Local Fetch (.EXE)"),
					FText::FromString(""),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this]() {
							FString ProcessName = TEXT("LocalFetch.exe");
							TCHAR* ProcessNameChar = ProcessName.GetCharArray().GetData();

							DWORD ProcessID = 0;
							HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
							if (hSnapshot != INVALID_HANDLE_VALUE) {
								PROCESSENTRY32 ProcessEntry;
								ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

								if (Process32First(hSnapshot, &ProcessEntry)) {
									do {
										if (FCString::Stricmp(ProcessEntry.szExeFile, ProcessNameChar) == 0) {
											ProcessID = ProcessEntry.th32ProcessID;
											break;
										}
									} while (Process32Next(hSnapshot, &ProcessEntry));
								}
								CloseHandle(hSnapshot);
							}

							if (ProcessID != 0) {
								HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, ProcessID);
								if (hProcess != nullptr) {
									TerminateProcess(hProcess, 0);
									CloseHandle(hProcess);
								}
							}
						}),
						FCanExecuteAction::CreateLambda([this]() {
							return IsProcessRunning("LocalFetch.exe");
						})
					)
				);
			}
			InnerMenuBuilder.EndSection();
		}),
		false,
		FSlateIcon()
	);
	MenuBuilder.EndSection();
}

void FJsonAsAssetModule::ImportConvexCollision()
{
	TArray<FAssetData> AssetDataList = GetAssetsInSelectedFolder();
	TArray<FString> OutFolderNames = OpenFolderDialog("Select a folder for JSON files");

	if (OutFolderNames.Num() == 0 || AssetDataList.Num() == 0) {
		// Exit if no folder is selected
		return;
	}

	for (const FAssetData& AssetData : AssetDataList) {
		if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset())) {
			StaticMesh->ReleaseResources();
			StaticMesh->Modify(true);

			// Get the name of the static mesh
			FString StaticMeshName = StaticMesh->GetName();

			FString JsonFileName = StaticMeshName + ".json";
			FString JsonFilePath = OutFolderNames[0] / JsonFileName;

			if (FPaths::FileExists(JsonFilePath)) {
				UE_LOG(LogTemp, Log, TEXT("Found JSON file for Static Mesh: %s"), *JsonFilePath);

				FString ContentBefore;
				if (FFileHelper::LoadFileToString(ContentBefore, *JsonFilePath)) {
					FString Content = FString(TEXT("{\"data\": "));
					Content.Append(ContentBefore);
					Content.Append(FString("}"));

					TSharedPtr<FJsonObject> JsonParsed;
					const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);

					if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
						const TArray<TSharedPtr<FJsonValue>> DataObjects = JsonParsed->GetArrayField("data");

						for (const TSharedPtr<FJsonValue>& DataObject : DataObjects) {
							if (!DataObject.IsValid() || !DataObject->AsObject().IsValid()) {
								continue;
							}

							TSharedPtr<FJsonObject> JsonObject = DataObject->AsObject();
							FString TypeValue;

							// Check if the "Type" field exists and matches "BodySetup"
							if (JsonObject->TryGetStringField("Type", TypeValue) && TypeValue == "BodySetup") {
								// Check for "Class" with value "UScriptClass'BodySetup'"
								FString ClassValue;
								if (JsonObject->TryGetStringField("Class", ClassValue) && ClassValue == "UScriptClass'BodySetup'") {
									// Navigate to "Properties"
									TSharedPtr<FJsonObject> PropertiesObject = JsonObject->GetObjectField("Properties");
									if (PropertiesObject.IsValid()) {
										// Navigate to "AggGeom"
										TSharedPtr<FJsonObject> AggGeomObject = PropertiesObject->GetObjectField("AggGeom");
										if (AggGeomObject.IsValid()) {
											FKAggregateGeom AggGeom;

											auto GObjectSerializer = NewObject<UObjectSerializer>();
											GObjectSerializer->DeserializeObjectProperties(PropertiesObject, StaticMesh->GetBodySetup());
											StaticMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseDefault;
											StaticMesh->MarkPackageDirty();
											StaticMesh->GetBodySetup()->PostEditChange();
											StaticMesh->Modify(true);

											// Notification
											AppendNotification(
												FText::FromString("Imported Convex Collision: " + StaticMeshName),
												FText::FromString(StaticMeshName),
												3.5f,
												FEditorStyle::GetBrush("PhysicsAssetEditor.EnableCollision.Small"),
												SNotificationItem::CS_Success,
												false,
												310.0f
											);
										}
									}
								}
							}
						}
					}
				}
			}

			// Notify the editor about the changes
			StaticMesh->GetBodySetup()->InvalidatePhysicsData();
			StaticMesh->GetBodySetup()->CreatePhysicsMeshes();

			StaticMesh->MarkPackageDirty();
			StaticMesh->GetBodySetup()->PostEditChange();
			StaticMesh->PostLoad();
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJsonAsAssetModule, JsonAsAsset)