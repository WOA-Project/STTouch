/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Copyright (c) Bingxing Wang. All Rights Reserved.
	Copyright (c) LumiaWoA authors. All Rights Reserved.

	Module Name:

		init.c

	Abstract:

		Contains ST initialization code

	Environment:

		Kernel mode

	Revision History:

--*/

#include <Cross Platform Shim\compat.h>
#include <spb.h>
#include <fts\ftsinternal.h>
#include <init.tmh>

NTSTATUS
TchStartDevice(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

  Routine Description:

	This routine is called in response to the KMDF prepare hardware call
	to initialize the touch controller for use.

  Arguments:

	ControllerContext - A pointer to the current touch controller
	context

	SpbContext - A pointer to the current i2c context

  Return Value:

	NTSTATUS indicating success or failure

--*/
{
	FTS_CONTROLLER_CONTEXT* controller;
	ULONG interruptStatus;
	NTSTATUS status;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchStartDevice - Entry");

	controller = (FTS_CONTROLLER_CONTEXT*)ControllerContext;
	interruptStatus = 0;
	status = STATUS_SUCCESS;

	//
	// Populate context with FTS function descriptors
	//
	status = FtsBuildFunctionsTable(
		ControllerContext,
		SpbContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not build table of FTS functions - 0x%08lX",
			status);
		goto exit;
	}

	//
	// Initialize FTS function control registers
	//
	status = FtsConfigureFunctions(
		ControllerContext,
		SpbContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Could not configure chip - 0x%08lX",
			status);

		goto exit;
	}

	status = FtsConfigureInterruptEnable(
		ControllerContext,
		SpbContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not configure interrupt enablement - 0x%08lX",
			status);
		goto exit;
	}

	//
	// Read and store the firmware version
	//
	status = FtsGetFirmwareVersion(
		ControllerContext,
		SpbContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not get FTS firmware version - 0x%08lX",
			status);
		goto exit;
	}

	//
	// Clear any pending interrupts
	//
	status = FtsCheckInterrupts(
		ControllerContext,
		SpbContext,
		&interruptStatus
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not get interrupt status - 0x%08lX%",
			status);
	}

exit:

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchStartDevice - Exit - 0x%08lX",
		status);

	return status;
}

NTSTATUS
TchStopDevice(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
/*++

Routine Description:

	This routine cleans up the device that is stopped.

Argument:

	ControllerContext - Touch controller context

	SpbContext - A pointer to the current i2c context

Return Value:

	NTSTATUS indicating sucess or failure
--*/
{
	FTS_CONTROLLER_CONTEXT* controller;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchStopDevice - Entry");

	UNREFERENCED_PARAMETER(SpbContext);

	controller = (FTS_CONTROLLER_CONTEXT*)ControllerContext;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchStopDevice - Exit");

	return STATUS_SUCCESS;
}

NTSTATUS
TchAllocateContext(
	OUT VOID** ControllerContext,
	IN WDFDEVICE FxDevice
)
/*++

Routine Description:

	This routine allocates a controller context.

Argument:

	ControllerContext - Touch controller context
	FxDevice - Framework device object

Return Value:

	NTSTATUS indicating sucess or failure
--*/
{
	FTS_CONTROLLER_CONTEXT* context;
	NTSTATUS status;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchAllocateContext - Entry");

	context = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		sizeof(FTS_CONTROLLER_CONTEXT),
		TOUCH_POOL_TAG);

	if (NULL == context)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not allocate controller context!");

		status = STATUS_UNSUCCESSFUL;
		goto exit;
	}

	RtlZeroMemory(context, sizeof(FTS_CONTROLLER_CONTEXT));
	context->FxDevice = FxDevice;

	//
	// Get Touch settings and populate context
	//
	TchGetTouchSettings(&context->TouchSettings);

	//
	// Allocate a WDFWAITLOCK for guarding access to the
	// controller HW and driver controller context
	//
	status = WdfWaitLockCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		&context->ControllerLock);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not create lock - 0x%08lX",
			status);

		TchFreeContext(context);
		goto exit;

	}

	*ControllerContext = context;

exit:

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchAllocateContext - Exit - 0x%08lX",
		status);

	return status;
}

NTSTATUS
TchFreeContext(
	IN VOID* ControllerContext
)
/*++

Routine Description:

	This routine frees a controller context.

Argument:

	ControllerContext - Touch controller context

Return Value:

	NTSTATUS indicating sucess or failure
--*/
{
	FTS_CONTROLLER_CONTEXT* controller;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchFreeContext - Entry");

	controller = (FTS_CONTROLLER_CONTEXT*)ControllerContext;

	if (controller != NULL)
	{

		if (controller->ControllerLock != NULL)
		{
			WdfObjectDelete(controller->ControllerLock);
		}

		ExFreePoolWithTag(controller, TOUCH_POOL_TAG);
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"TchFreeContext - Exit");

	return STATUS_SUCCESS;
}