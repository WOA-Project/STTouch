/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		ftsinternal.c

	Abstract:

		Contains ST initialization code

	Environment:

		Kernel mode

	Revision History:

--*/

#include <Cross Platform Shim\compat.h>
#include <spb.h>
#include <report.h>
#include <fts\ftsinternal.h>
#include <ftsinternal.tmh>

BYTE cmd_sense_on[3] = { 0x00, 0x00, 0x00 };
BYTE cmd_enable_int[3] = { 0x00, 0x2c, 0x41 };
BYTE cmd_system_reset[3] = { 0x00, 0x28, 0x80 };

BYTE EventDataBuffer[0x256];

#define EVT_ID_CONTROL_READY           0x10
#define EVT_ID_NO_EVENT                0x00
#define EVT_ID_ENTER_POINT             0x03
#define EVT_ID_LEAVE_POINT             0x04
#define EVT_ID_MOTION_POINT            0x05

NTSTATUS
FtsGetObjectStatusFromController(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN DETECTED_OBJECTS* Data
)
/*++

Routine Description:

	This routine reads raw touch messages from hardware. If there is
	no touch data available (if a non-touch interrupt fired), the
	function will not return success and no touch data was transferred.

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current i2c context
	Data - A pointer to any returned F11 touch data

Return Value:

	NTSTATUS, where only success indicates data was returned

--*/
{
	NTSTATUS status;
	FTS_CONTROLLER_CONTEXT* controller;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetObjectStatusFromController - Entry");

	int i, x, y;
	controller = (FTS_CONTROLLER_CONTEXT*)ControllerContext;

	if (controller->MaxFingers == 0)
	{
		status = STATUS_SUCCESS;
		goto exit;
	}

	// Get all event data
	status = SpbReadDataSynchronously(SpbContext, 0x86, EventDataBuffer, 16);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsGetObjectStatusFromController - Error reading event buffer - 0x%08lX",
			status);
		goto exit;
	}

	UINT8 FingerCount = (EventDataBuffer[2] & 0xF0) >> 4;

	if (FingerCount >= 0 && FingerCount <= 8)
	{
		status = SpbReadDataSynchronously(SpbContext, 0x86, EventDataBuffer + 16, 16 * FingerCount);
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"FtsGetObjectStatusFromController - Error reading event buffer - 0x%08lX",
				status);
			goto exit;
		}

		// We only do 8 fingers max here
		if (FingerCount > 8)
		{
			FingerCount = 8;
		}
	}

	BYTE ObjectTypeAndStatus = 0;
	BYTE X_MSB = 0;
	BYTE X_LSB = 0;
	BYTE Y_MSB = 0;
	BYTE Y_LSB = 0;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_REPORTING,
		"FtsGetObjectStatusFromController - FingerCount = %d",
		FingerCount);

	for (i = 0; i < FingerCount; i++)
	{
		if (EventDataBuffer[i * 16 + 1] == 0x28)
		{
			continue;
		}

		int touchId = (EventDataBuffer[i * 16 + 2] & 0x0F);

		if (touchId >= MAX_TOUCHES || touchId < 0)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_REPORTING,
				"FtsGetObjectStatusFromController - Invalid touch id %d",
				touchId);
			continue;
		}

		X_MSB = EventDataBuffer[i * 16 + 3];
		X_LSB = (EventDataBuffer[i * 16 + 5] & 0xF0) >> 4;

		Y_MSB = EventDataBuffer[i * 16 + 4];
		Y_LSB = EventDataBuffer[i * 16 + 5] & 0x0F;

		ObjectTypeAndStatus = EventDataBuffer[i * 16 + 1];

		x = (X_MSB << 4) | X_LSB;
		y = (Y_MSB << 4) | Y_LSB;

		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_REPORTING,
			"FtsGetObjectStatusFromController - Finger %d: X = %d, Y = %d, Type = %d",
			touchId,
			x,
			y,
			ObjectTypeAndStatus);

		switch (ObjectTypeAndStatus)
		{
		case EVT_ID_ENTER_POINT:
		case EVT_ID_MOTION_POINT:
			Data->States[touchId] = OBJECT_STATE_FINGER_PRESENT_WITH_ACCURATE_POS;
			break;
		case EVT_ID_LEAVE_POINT:
			Data->States[touchId] = OBJECT_STATE_NOT_PRESENT;
			break;
		default:
			Data->States[touchId] = OBJECT_STATE_NOT_PRESENT;
			break;
		}

		Data->Positions[touchId].X = x;
		Data->Positions[touchId].Y = y;
	}

exit:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetObjectStatusFromController - Exit");

	return status;
}

NTSTATUS
TchServiceObjectInterrupts(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	DETECTED_OBJECTS data;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"TchServiceObjectInterrupts - Entry");

	RtlZeroMemory(&data, sizeof(data));

	//
	// See if new touch data is available
	//
	status = FtsGetObjectStatusFromController(
		ControllerContext,
		SpbContext,
		&data
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"TchServiceObjectInterrupts - No object data to report - 0x%08lX",
			status);

		goto exit;
	}

	status = ReportObjects(
		ReportContext,
		data);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"TchServiceObjectInterrupts - Error while reporting objects - 0x%08lX",
			status);

		goto exit;
	}

exit:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"TchServiceObjectInterrupts - Exit");

	return status;
}

NTSTATUS
FtsServiceInterrupts(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;
	FTS_CONTROLLER_CONTEXT* controller;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsServiceInterrupts - Entry");

	controller = (FTS_CONTROLLER_CONTEXT*)ControllerContext;

	//
	// Grab a waitlock to ensure the ISR executes serially and is 
	// protected against power state transitions
	//
	WdfWaitLockAcquire(controller->ControllerLock, NULL);

	status = TchServiceObjectInterrupts(ControllerContext, SpbContext, ReportContext);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsServiceInterrupts - Error servicing F12 interrupt - 0x%08lX",
			status);

		goto exit;
	}

exit:

	WdfWaitLockRelease(controller->ControllerLock);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsServiceInterrupts - Exit");

	return status;
}

NTSTATUS
FtsChangePage(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN int DesiredPage
)
/*++

  Routine Description:

	This utility function changes the current register address page.

  Arguments:

	ControllerContext - A pointer to the current touch controller context
	SpbContext - A pointer to the current i2c context
	DesiredPage - The page the caller expects to be mapped in

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(ControllerContext);
	UNREFERENCED_PARAMETER(SpbContext);
	UNREFERENCED_PARAMETER(DesiredPage);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsChangePage - Entry");

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsChangePage - Exit");

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FtsConfigureFunctions(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	FTS devices such as this ST touch controller are organized
	as collections of logical functions. Discovered functions must be
	configured, which is done in this function (things like sleep
	timeouts, interrupt enables, report rates, etc.)

  Arguments:

	ControllerContext - A pointer to the current touch controller
	context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(SpbContext);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsConfigureFunctions - Entry");

	ControllerContext->MaxFingers = 8;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsConfigureFunctions - Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
FtsBuildFunctionsTable(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	FTS devices such as this ST touch controller are organized
	as collections of logical functions. When initially communicating
	with the chip, a driver must build a table of available functions,
	as is done in this routine.

  Arguments:

	ControllerContext - A pointer to the current touch controller context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(SpbContext);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsBuildFunctionsTable - Entry");

	ControllerContext->MaxFingers = 8;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsBuildFunctionsTable - Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
FtsConfigureInterruptEnable(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	LARGE_INTEGER delay;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsConfigureInterruptEnable - Entry");

	ControllerContext->MaxFingers = 8;

	status = SpbWriteDataSynchronously(SpbContext, 0xB6, cmd_system_reset, sizeof(cmd_system_reset));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error resetting controller - 0x%08lX",
			status);

		goto exit;
	}

	delay.QuadPart = -10 * 1000;
	delay.QuadPart *= 200;
	status = KeDelayExecutionThread(KernelMode, FALSE, &delay); // 200ms
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error delaying controller reset - 0x%08lX",
			status);
		goto exit;
	}

	status = SpbReadDataSynchronously(SpbContext, 0x85, EventDataBuffer, sizeof(EventDataBuffer));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error reading event buffer - 0x%08lX",
			status);
		goto exit;
	}

	status = KeDelayExecutionThread(KernelMode, FALSE, &delay); // 200ms
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error delaying controller reset - 0x%08lX",
			status);
		goto exit;
	}

	status = SpbReadDataSynchronously(SpbContext, 0x85, EventDataBuffer, sizeof(EventDataBuffer));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error reading event buffer - 0x%08lX",
			status);
		goto exit;
	}

	// active scan on
	// cmd_scanmode[1] = 0x00; // active scan
	// cmd_scanmode[2] = 0x01; // on

	status = SpbWriteDataSynchronously(SpbContext, 0xB6, cmd_enable_int, sizeof(cmd_enable_int));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error enabling interrupts - 0x%08lX",
			status);
		goto exit;
	}

	status = SpbWriteDataSynchronously(SpbContext, 0x93, cmd_sense_on, sizeof(cmd_sense_on));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error enabling sense - 0x%08lX",
			status);
		goto exit;
	}

	// drain event
	status = SpbReadDataSynchronously(SpbContext, 0x86, EventDataBuffer, sizeof(EventDataBuffer));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error draining event buffer - 0x%08lX",
			status);
		goto exit;
	}

exit:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsConfigureInterruptEnable - Exit");

	return status;
}

NTSTATUS
FtsGetFirmwareVersion(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	This function queries the firmware version of the current chip for
	debugging purposes.

  Arguments:

	ControllerContext - A pointer to the current touch controller context
	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(ControllerContext);
	UNREFERENCED_PARAMETER(SpbContext);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetFirmwareVersion - Entry");

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetFirmwareVersion - Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
FtsCheckInterrupts(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN ULONG* InterruptStatus
)
/*++

  Routine Description:

	This function handles controller interrupts. It currently only
	supports valid touch interrupts. Any other interrupt sources (such as
	device losing configuration or being reset) are unhandled, but noted
	in the controller context.

  Arguments:

	ControllerContext - A pointer to the current touch controller
	context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(ControllerContext);
	UNREFERENCED_PARAMETER(SpbContext);
	UNREFERENCED_PARAMETER(InterruptStatus);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsCheckInterrupts - Entry");

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsCheckInterrupts - Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
FtsChangeChargerConnectedState(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR ChargerConnectedState
)
{
	UNREFERENCED_PARAMETER(ControllerContext);
	UNREFERENCED_PARAMETER(SpbContext);
	UNREFERENCED_PARAMETER(ChargerConnectedState);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsChangeChargerConnectedState - Entry");

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsChangeChargerConnectedState - Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
FtsChangeSleepState(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR SleepState
)
/*++

Routine Description:

   Changes the SleepMode bits on the controller as specified

Arguments:

   ControllerContext - Touch controller context

   SpbContext - A pointer to the current i2c context

   SleepState - Either FTS_F11_DEVICE_CONTROL_SLEEP_MODE_OPERATING
				or FTS_F11_DEVICE_CONTROL_SLEEP_MODE_SLEEPING

Return Value:

   NTSTATUS indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(ControllerContext);
	UNREFERENCED_PARAMETER(SpbContext);
	UNREFERENCED_PARAMETER(SleepState);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsChangeSleepState - Entry");

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsChangeSleepState - Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
FtsSetReportingFlags(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR NewMode,
	OUT UCHAR* OldMode
)
/*++

	Routine Description:

		Changes the F12 Reporting Mode on the controller as specified

	Arguments:

		ControllerContext - Touch controller context

		SpbContext - A pointer to the current i2c context

		NewMode - Either REPORTING_CONTINUOUS_MODE
				  or REPORTING_REDUCED_MODE

		OldMode - Old value of reporting mode

	Return Value:

		NTSTATUS indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(ControllerContext);
	UNREFERENCED_PARAMETER(SpbContext);
	UNREFERENCED_PARAMETER(NewMode);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsSetReportingFlags - Entry");

	if (OldMode != NULL)
	{
		*OldMode = REPORTING_CONTINUOUS_MODE;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsSetReportingFlags - Exit");

	return STATUS_SUCCESS;
}