/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		ftsevents.c

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
#include <fts\ftsregs.h>
#include <fts\ftspointer.h>
#include <fts\ftsevents.h>
#include <ftsevents.tmh>

/*
	@brief Reads all events from the FIFO buffer
	The caller is responsible for freeing the memory returned by this function

	@param SpbContext - A pointer to the current i2c context
	@param DataBuffer - A pointer to the buffer that will contain the events
	@param DataBufferLength - The length of the buffer
	@return NTSTATUS
*/
NTSTATUS FtsGetAllEvents(
	SPB_CONTEXT* SpbContext, 
	BYTE** DataBuffer, 
	DWORD* DataBufferLength)
{
	NTSTATUS status;
	BYTE* allEventsBuffer = NULL;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetAllEvents - Entry");
	
	if (DataBuffer == NULL || DataBufferLength == NULL)
	{
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	*DataBuffer = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		FIFO_EVENT_SIZE,
		TOUCH_POOL_TAG_F12
	);

	if (*DataBuffer == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		*DataBufferLength = 0;
		goto exit;
	}

	*DataBufferLength = FIFO_EVENT_SIZE;

	status = SpbReadDataSynchronously(SpbContext, FIFO_CMD_READONE, *DataBuffer, FIFO_EVENT_SIZE);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsGetAllEvents - Error reading one event from the chip - 0x%08lX",
			status);

		ExFreePoolWithTag(
			*DataBuffer,
			TOUCH_POOL_TAG_F12
		);

		*DataBuffer = NULL;
		*DataBufferLength = 0;

		goto exit;
	}

	// 00011111
	DWORD leftEvents = (*DataBuffer)[7] & 0x1F;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetAllEvents - %d events detected",
		leftEvents + 1);

	if ((leftEvents > 0) && (leftEvents < FIFO_DEPTH))
	{
		allEventsBuffer = ExAllocatePoolWithTag(
			NonPagedPoolNx,
			(leftEvents + 1) * FIFO_EVENT_SIZE,
			TOUCH_POOL_TAG_F12
		);

		if (allEventsBuffer == NULL)
		{
			// Process the one that was fine instead
			// 11100000
			(*DataBuffer)[7] &= 0xE0;

			goto exit;
		}

		status = SpbReadDataSynchronously(SpbContext, FIFO_CMD_READALL, allEventsBuffer + FIFO_EVENT_SIZE, leftEvents * FIFO_EVENT_SIZE);
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"FtsGetAllEvents - Error reading all remaining events - 0x%08lX",
				status);

			// Process the one that was fine instead
			// 11100000
			(*DataBuffer)[7] &= 0xE0;

			status = STATUS_SUCCESS;

			ExFreePoolWithTag(
				allEventsBuffer,
				TOUCH_POOL_TAG_F12
			);
		}
		else
		{
			// Copy the first event
			RtlCopyMemory(allEventsBuffer, *DataBuffer, FIFO_EVENT_SIZE);

			ExFreePoolWithTag(
				*DataBuffer,
				TOUCH_POOL_TAG_F12
			);

			*DataBuffer = allEventsBuffer;
			allEventsBuffer = NULL;
			*DataBufferLength = (leftEvents + 1) * FIFO_EVENT_SIZE;
		}
	}

exit:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetAllEvents - Exit");

	return status;
}

NTSTATUS
FtsProcessOneEvent(
	FTS_CONTROLLER_CONTEXT* ControllerContext,
	PREPORT_CONTEXT ReportContext,
	BYTE* EventData
)
{
	NTSTATUS status = STATUS_SUCCESS;

	BYTE EventID = EventData[0];

	switch (EventID)
	{
	case EVENTID_ENTER_POINTER:
	{
		status = FtsProcessEnterPointerEvent(ControllerContext, ReportContext, EventData);
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"FtsProcessOneEvent - Error while processing enter pointer event - 0x%08lX",
				status);

			goto exit;
		}

		break;
	}
	case EVENTID_MOTION_POINTER:
	{
		status = FtsProcessMotionPointerEvent(ControllerContext, ReportContext, EventData);
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"FtsProcessOneEvent - Error while processing motion pointer event - 0x%08lX",
				status);
			goto exit;
		}

		break;
	}
	case EVENTID_LEAVE_POINTER:
	{
		status = FtsProcessLeavePointerEvent(ControllerContext, ReportContext, EventData);
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"FtsProcessOneEvent - Error while processing leave pointer event - 0x%08lX",
				status);
			goto exit;
		}

		break;
	}
	default:
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_REPORTING,
			"FtsProcessOneEvent - Unknown event id %d",
			EventID);
		break;
	}
	}

exit:

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Exit - 0x%08lX",
		status);

	return status;
}

NTSTATUS
TchServiceObjectInterrupts(
	IN FTS_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
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
		"TchServiceObjectInterrupts - Entry");

	controller = (FTS_CONTROLLER_CONTEXT*)ControllerContext;

	if (controller->MaxFingers == 0)
	{
		status = STATUS_SUCCESS;
		goto exit;
	}

	BYTE* EventDataBuffer = NULL;
	DWORD EventDataBufferLength = 0;

	status = FtsGetAllEvents(SpbContext, &EventDataBuffer, &EventDataBufferLength);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"TchServiceObjectInterrupts - Error reading all events from the chip - 0x%08lX",
			status);
		goto exit;
	}

	if (EventDataBuffer == NULL || EventDataBufferLength == 0)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"TchServiceObjectInterrupts - No events to process");
		status = STATUS_SUCCESS;
		goto exit;
	}

	// Process all events
	for (DWORD i = 0; i < EventDataBufferLength; i += FIFO_EVENT_SIZE) {

		DWORD CurrentEventId = i / FIFO_EVENT_SIZE;

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_REPORTING,
			"TchServiceObjectInterrupts - Processing event %d",
			CurrentEventId);

		// Process the event
		status = FtsProcessOneEvent(controller, ReportContext, EventDataBuffer + i);
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"TchServiceObjectInterrupts - Error processing event %d - 0x%08lX",
				CurrentEventId,
				status);
			goto free_buffer;
		}
	}

	// Re-enable interrupts
	status = FtsEnableInterrupts(SpbContext);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"TchServiceObjectInterrupts - Error enabling interrupts - 0x%08lX",
			status);
		goto free_buffer;
	}

free_buffer:
	if (EventDataBuffer != NULL)
	{
		ExFreePoolWithTag(
			EventDataBuffer,
			TOUCH_POOL_TAG_F12
		);

		EventDataBuffer = NULL;
		EventDataBufferLength = 0;
	}

exit:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"TchServiceObjectInterrupts - Exit\n");

	return status;
}