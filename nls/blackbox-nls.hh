#define BaseDisplaySet 0x1
#define BaseDisplayXError 0x1
#define BaseDisplaySignalCaught 0x2
#define BaseDisplayShuttingDown 0x3
#define BaseDisplayAborting 0x4
#define BaseDisplayXConnectFail 0x5
#define BaseDisplayCloseOnExecFail 0x6
#define BaseDisplayBadWindowRemove 0x7

#define BasemenuSet 0x2
#define BasemenuBlackboxMenu 0x1

#define ConfigmenuSet 0x3
#define ConfigmenuConfigOptions 0x1
#define ConfigmenuFocusModel 0x2
#define ConfigmenuWindowPlacement 0x3
#define ConfigmenuImageDithering 0x4
#define ConfigmenuOpaqueMove 0x5
#define ConfigmenuFullMax 0x6
#define ConfigmenuFocusNew 0x7
#define ConfigmenuFocusLast 0x8
#define ConfigmenuClickToFocus 0x9
#define ConfigmenuSloppyFocus 0xa
#define ConfigmenuAutoRaise 0xb
#define ConfigmenuSmartRows 0xc
#define ConfigmenuSmartCols 0xd
#define ConfigmenuCascade 0xe
#define ConfigmenuLeftRight 0xf
#define ConfigmenuRightLeft 0x10
#define ConfigmenuTopBottom 0x11
#define ConfigmenuBottomTop 0x12

#define IconSet 0x4
#define IconIcons 0x1
#define IconUnnamed 0x2

#define ImageSet 0x5
#define ImageErrorCreatingSolidPixmap 0x1
#define ImageErrorCreatingXImage 0x2
#define ImageUnsupVisual 0x3
#define ImageErrorCreatingPixmap 0x4
#define ImageInvalidColormapSize 0x5
#define ImageErrorAllocatingColormap 0x6
#define ImageColorAllocFail 0x7
#define ImagePixmapRelease 0x8
#define ImagePixmapCacheLarge 0x9
#define ImageColorParseError 0xa
#define ImageColorAllocError 0xb

#define ScreenSet 0x6
#define ScreenAnotherWMRunning 0x1
#define ScreenManagingScreen 0x2
#define ScreenFontLoadFail 0x3
#define ScreenDefaultFontLoadFail 0x4
#define ScreenEmptyMenuFile 0x5
#define Screenxterm 0x6
#define ScreenRestart 0x7
#define ScreenExit 0x8
#define ScreenEXECError 0x9
#define ScreenEXITError 0xa
#define ScreenSTYLEError 0xb
#define ScreenCONFIGError 0xc
#define ScreenINCLUDEError 0xd
#define ScreenINCLUDEErrorReg 0xe
#define ScreenSUBMENUError 0xf
#define ScreenRESTARTError 0x10
#define ScreenRECONFIGError 0x11
#define ScreenSTYLESDIRError 0x12
#define ScreenSTYLESDIRErrorNotDir 0x13
#define ScreenSTYLESDIRErrorNoExist 0x14
#define ScreenWORKSPACESError 0x15
#define ScreenPositionLength 0x16
#define ScreenPositionFormat 0x17
#define ScreenGeometryFormat 0x18

#define SlitSet 0x7
#define SlitSlitTitle 0x1
#define SlitSlitDirection 0x2
#define SlitSlitPlacement 0x3

#define ToolbarSet 0x8
#define ToolbarNoStrftimeLength 0x1
#define ToolbarNoStrftimeDateFormat 0x2
#define ToolbarNoStrftimeDateFormatEu 0x3
#define ToolbarNoStrftimeTimeFormat24 0x4
#define ToolbarNoStrftimeTimeFormat12 0x5
#define ToolbarNoStrftimeTimeFormatP 0x6
#define ToolbarNoStrftimeTimeFormatA 0x7
#define ToolbarToolbarTitle 0x8
#define ToolbarEditWkspcName 0x9
#define ToolbarToolbarPlacement 0xa

#define WindowSet 0x9
#define WindowCreating 0x1
#define WindowXGetWindowAttributesFail 0x2
#define WindowCannotFindScreen 0x3
#define WindowUnnamed 0x4
#define WindowMapRequest 0x5
#define WindowUnmapNotify 0x6
#define WindowUnmapNotifyReparent 0x7

#define WindowmenuSet 0xa
#define WindowmenuSendTo 0x1
#define WindowmenuShade 0x2
#define WindowmenuIconify 0x3
#define WindowmenuMaximize 0x4
#define WindowmenuRaise 0x5
#define WindowmenuLower 0x6
#define WindowmenuStick 0x7
#define WindowmenuKillClient 0x8
#define WindowmenuClose 0x9

#define WorkspaceSet 0xb
#define WorkspaceDefaultNameFormat 0x1

#define WorkspacemenuSet 0xc
#define WorkspacemenuWorkspacesTitle 0x1
#define WorkspacemenuNewWorkspace 0x2
#define WorkspacemenuRemoveLast 0x3

#define blackboxSet 0xd
#define blackboxNoManagableScreens 0x1
#define blackboxMapRequest 0x2

#define CommonSet 0xe
#define CommonYes 0x1
#define CommonNo 0x2
#define CommonDirectionTitle 0x3
#define CommonDirectionHoriz 0x4
#define CommonDirectionVert 0x5
#define CommonAlwaysOnTop 0x6
#define CommonPlacementTitle 0x7
#define CommonPlacementTopLeft 0x8
#define CommonPlacementCenterLeft 0x9
#define CommonPlacementBottomLeft 0xa
#define CommonPlacementTopCenter 0xb
#define CommonPlacementBottomCenter 0xc
#define CommonPlacementTopRight 0xd
#define CommonPlacementCenterRight 0xe
#define CommonPlacementBottomRight 0xf

#define mainSet 0xf
#define mainRCRequiresArg 0x1
#define mainDISPLAYRequiresArg 0x2
#define mainWarnDisplaySet 0x3
#define mainUsage 0x4
#define mainCompileOptions 0x5

#define bsetrootSet 0x10
#define bsetrootMustSpecify 0x1
#define bsetrootUsage 0x2
