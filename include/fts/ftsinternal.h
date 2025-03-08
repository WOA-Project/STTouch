/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		ftsinternal.h

	Abstract:

		Contains common types and defintions used internally
		by the multi touch screen driver.

	Environment:

		Kernel mode

	Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>
#include <controller.h>
#include <resolutions.h>
#include <Cross Platform Shim/bitops.h>
#include <Cross Platform Shim/hweight.h>
#include <report.h>

// Ignore warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning (disable : 4152)

// Ignore warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)

// Ignore warning C4201: nonstandard extension used : bit field types other than in
#pragma warning (disable : 4214)

// Ignore warning C4324: 'xxx' : structure was padded due to __declspec(align())
#pragma warning (disable : 4324)

//
// Logical structure for getting registry config settings
//
typedef struct _RM4_F01_CTRL_REGISTERS_LOGICAL
{
	UINT32 SleepMode;
	UINT32 NoSleep;
	UINT32 ReportRate;
	UINT32 Configured;
	UINT32 InterruptEnable;
	UINT32 DozeInterval;
	UINT32 DozeThreshold;
	UINT32 DozeHoldoff;
} FTS_F01_CTRL_REGISTERS_LOGICAL;

#define FTS_MILLISECONDS_TO_TENTH_MILLISECONDS(n) n/10
#define FTS_SECONDS_TO_HALF_SECONDS(n) 2*n

//
// Function $11 - 2-D Touch Sensor
//

//
// Logical structure for getting registry config settings
//
typedef struct _FTS_F11_CTRL_REGISTERS_LOGICAL
{
	UINT32 ReportingMode;
	UINT32 AbsPosFilt;
	UINT32 RelPosFilt;
	UINT32 RelBallistics;
	UINT32 Dribble;
	UINT32 PalmDetectThreshold;
	UINT32 MotionSensitivity;
	UINT32 ManTrackEn;
	UINT32 ManTrackedFinger;
	UINT32 DeltaXPosThreshold;
	UINT32 DeltaYPosThreshold;
	UINT32 Velocity;
	UINT32 Acceleration;
	UINT32 SensorMaxXPos;
	UINT32 SensorMaxYPos;
	UINT32 ZTouchThreshold;
	UINT32 ZHysteresis;
	UINT32 SmallZThreshold;
	UINT32 SmallZScaleFactor;
	UINT32 LargeZScaleFactor;
	UINT32 AlgorithmSelection;
	UINT32 WxScaleFactor;
	UINT32 WxOffset;
	UINT32 WyScaleFactor;
	UINT32 WyOffset;
	UINT32 XPitch;
	UINT32 YPitch;
	UINT32 FingerWidthX;
	UINT32 FingerWidthY;
	UINT32 ReportMeasuredSize;
	UINT32 SegmentationSensitivity;
	UINT32 XClipLo;
	UINT32 XClipHi;
	UINT32 YClipLo;
	UINT32 YClipHi;
	UINT32 MinFingerSeparation;
	UINT32 MaxFingerMovement;
} FTS_F11_CTRL_REGISTERS_LOGICAL;

//
// Driver structures
//

typedef struct _FTS_CONFIGURATION
{
	FTS_F01_CTRL_REGISTERS_LOGICAL DeviceSettings;
	FTS_F11_CTRL_REGISTERS_LOGICAL TouchSettings;
	UINT32 PepRemovesVoltageInD3;
} FTS_CONFIGURATION;

typedef struct _FTS_CONTROLLER_CONTEXT
{
	WDFDEVICE FxDevice;
	WDFWAITLOCK ControllerLock;

	//
	// Power state
	//
	DEVICE_POWER_STATE DevicePowerState;

	//
	// Register configuration programmed to chip
	//
	TOUCH_SCREEN_SETTINGS TouchSettings;
	FTS_CONFIGURATION Config;

	BYTE MaxFingers;

	DETECTED_OBJECTS DetectedObjects;
} FTS_CONTROLLER_CONTEXT;

#define DEVICE_CONTROL_SLEEP_MODE_OPERATING  0
#define DEVICE_CONTROL_SLEEP_MODE_SLEEPING   1

#pragma pack(push)
#pragma pack(1)
typedef enum _REPORTING_FLAGS
{
	REPORTING_CONTINUOUS_MODE = 0,
	REPORTING_REDUCED_MODE = 1,
	REPORTING_WAKEUP_GESTURE_MODE = 2,
} REPORTING_FLAGS;
#pragma pack(pop)

NTSTATUS
FtsBuildFunctionsTable(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
);

NTSTATUS
FtsChangePage(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN int DesiredPage
);

NTSTATUS
FtsConfigureFunctions(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
);

NTSTATUS
FtsServiceInterrupts(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
);

NTSTATUS
FtsConfigureInterruptEnable(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
);

NTSTATUS
FtsGetFirmwareVersion(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
);

NTSTATUS
FtsCheckInterrupts(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN ULONG* InterruptStatus
);

NTSTATUS
FtsChangeChargerConnectedState(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR ChargerConnectedState
);

NTSTATUS
FtsChangeSleepState(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR SleepState
);

NTSTATUS
FtsSetReportingFlags(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR NewMode,
	OUT UCHAR* OldMode
);

NTSTATUS
FtsEnableInterrupts(IN SPB_CONTEXT* SpbContext);