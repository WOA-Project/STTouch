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
#include <fts\ftsregs.h>
#include <fts\ftsevents.h>
#include <fts\ftsinternal.h>
#include <ftsinternal.tmh>

NTSTATUS
FtsEnableInterrupts(IN SPB_CONTEXT* SpbContext)
{
	NTSTATUS status;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsEnableInterrupts - Entry");

	BYTE Command[3] = {IER_ADDR, IER_ENABLE};

	status = SpbWriteDataSynchronously(SpbContext, FTS_CMD_HW_REG_W, Command, sizeof(Command));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsEnableInterrupts - Error enabling interrupts - 0x%08lX",
			status);
		goto exit;
	}

exit:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsEnableInterrupts - Exit");

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

	BYTE Command[3] = {SYSTEM_RESET_ADDRESS, SYSTEM_RESET_VALUE};
	status = SpbWriteDataSynchronously(SpbContext, FTS_CMD_HW_REG_W, Command, sizeof(Command));
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

	BYTE EventDataBuffer[FIFO_EVENT_SIZE] = {0};
	status = SpbReadDataSynchronously(SpbContext, FIFO_CMD_READONE, EventDataBuffer, sizeof(EventDataBuffer));
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

	status = SpbReadDataSynchronously(SpbContext, FIFO_CMD_READONE, EventDataBuffer, sizeof(EventDataBuffer));
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

	status = FtsEnableInterrupts(SpbContext);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsConfigureInterruptEnable - Error enabling interrupts - 0x%08lX",
			status);
		goto exit;
	}

	Command[0] = 0x00;
	Command[1] = 0x00;
	Command[2] = 0x00;

	status = SpbWriteDataSynchronously(SpbContext, FTS_CMD_MS_MT_SENSE_ON, Command, sizeof(Command));
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
	status = SpbReadDataSynchronously(SpbContext, FIFO_CMD_READALL, EventDataBuffer, sizeof(EventDataBuffer));
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