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
#include <fts\ftsevents.h>
#include <ftsevents.tmh>

BYTE EventDataBuffer[FIFO_EVENT_SIZE * FIFO_DEPTH];

// TODO: Dynamic memory allocation
NTSTATUS FtsGetAllEvents(
	SPB_CONTEXT* SpbContext, 
	BYTE* DataBuffer, 
	DWORD DataBufferLength)
{
	NTSTATUS status;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetAllEvents - Entry");
	
	status = SpbReadDataSynchronously(SpbContext, FIFO_CMD_READONE, DataBuffer, FIFO_EVENT_SIZE);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsGetAllEvents - Error reading one event from the chip - 0x%08lX",
			status);
		goto exit;
	}

	// 00011111
	DWORD leftEvents = DataBuffer[7] & 0x1F;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetAllEvents - %d events detected",
		leftEvents + 1);

	if ((leftEvents > 0) && (leftEvents < FIFO_DEPTH))
	{
		if ((leftEvents + 1) * FIFO_EVENT_SIZE > DataBufferLength)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"FtsGetAllEvents - Not enough space in the buffer for all events - %d",
				leftEvents);
			status = STATUS_BUFFER_TOO_SMALL;
			goto exit;
		}

		status = SpbReadDataSynchronously(SpbContext, FIFO_CMD_READALL, DataBuffer + FIFO_EVENT_SIZE, leftEvents * FIFO_EVENT_SIZE);
		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INTERRUPT,
				"FtsGetAllEvents - Error reading all remaining events - 0x%08lX",
				status);

			// Process the one that was fine instead
			// 11100000
			DataBuffer[7] &= 0xE0;

			status = STATUS_SUCCESS;
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
FtsGetObjectStatusFromController(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext
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

	status = FtsGetAllEvents(SpbContext, EventDataBuffer, sizeof(EventDataBuffer));
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsGetObjectStatusFromController - Error reading all events from the chip - 0x%08lX",
			status);
		goto exit;
	}

	BYTE* CurrentDataBuffer = EventDataBuffer;

	BYTE EventID = 0;
	BYTE X_MSB = 0;
	BYTE X_LSB = 0;
	BYTE Y_MSB = 0;
	BYTE Y_LSB = 0;

	BOOLEAN bGotOneTouchReport = FALSE;

	while (TRUE)
	{
		EventID = CurrentDataBuffer[0];

		switch (EventID)
		{
			case EVENTID_ENTER_POINTER:
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_REPORTING,
					"FtsGetObjectStatusFromController - Enter Pointer");

				BYTE touchId = (CurrentDataBuffer[2] & 0x0F);

				if (touchId >= MAX_TOUCHES || touchId < 0)
				{
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_REPORTING,
						"FtsGetObjectStatusFromController - Invalid touch id %d",
						touchId);
					break;
				}

				X_MSB = CurrentDataBuffer[3];
				X_LSB = (CurrentDataBuffer[5] & 0xF0) >> 4;

				Y_MSB = CurrentDataBuffer[4];
				Y_LSB = CurrentDataBuffer[5] & 0x0F;

				x = (X_MSB << 4) | X_LSB;
				y = (Y_MSB << 4) | Y_LSB;

				controller->DetectedObjects.States[touchId] = OBJECT_STATE_FINGER_PRESENT_WITH_ACCURATE_POS;
				controller->DetectedObjects.Positions[touchId].X = x;
				controller->DetectedObjects.Positions[touchId].Y = y;

				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_REPORTING,
					"FtsGetObjectStatusFromController - Touch %d at (x=%d, y=%d)",
					touchId,
					x,
					y);

				bGotOneTouchReport = TRUE;

				break;
			}
			case EVENTID_MOTION_POINTER:
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_REPORTING,
					"FtsGetObjectStatusFromController - Motion Pointer");

				BYTE touchId = (CurrentDataBuffer[2] & 0x0F);

				if (touchId >= MAX_TOUCHES || touchId < 0)
				{
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_REPORTING,
						"FtsGetObjectStatusFromController - Invalid touch id %d",
						touchId);
					break;
				}

				X_MSB = CurrentDataBuffer[3];
				X_LSB = (CurrentDataBuffer[5] & 0xF0) >> 4;

				Y_MSB = CurrentDataBuffer[4];
				Y_LSB = CurrentDataBuffer[5] & 0x0F;

				x = (X_MSB << 4) | X_LSB;
				y = (Y_MSB << 4) | Y_LSB;

				controller->DetectedObjects.States[touchId] = OBJECT_STATE_FINGER_PRESENT_WITH_ACCURATE_POS;
				controller->DetectedObjects.Positions[touchId].X = x;
				controller->DetectedObjects.Positions[touchId].Y = y;

				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_REPORTING,
					"FtsGetObjectStatusFromController - Touch %d at (x=%d, y=%d)",
					touchId,
					x,
					y);

				bGotOneTouchReport = TRUE;

				break;
			}
			case EVENTID_LEAVE_POINTER:
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_REPORTING,
					"FtsGetObjectStatusFromController - Leave Pointer");

				BYTE touchId = (CurrentDataBuffer[2] & 0x0F);

				if (touchId >= MAX_TOUCHES || touchId < 0)
				{
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_REPORTING,
						"FtsGetObjectStatusFromController - Invalid touch id %d",
						touchId);
					break;
				}

				X_MSB = CurrentDataBuffer[3];
				X_LSB = (CurrentDataBuffer[5] & 0xF0) >> 4;

				Y_MSB = CurrentDataBuffer[4];
				Y_LSB = CurrentDataBuffer[5] & 0x0F;

				x = (X_MSB << 4) | X_LSB;
				y = (Y_MSB << 4) | Y_LSB;

				controller->DetectedObjects.States[touchId] = OBJECT_STATE_NOT_PRESENT;
				controller->DetectedObjects.Positions[touchId].X = x;
				controller->DetectedObjects.Positions[touchId].Y = y;

				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_REPORTING,
					"FtsGetObjectStatusFromController - Touch %d at (x=%d, y=%d) left",
					touchId,
					x,
					y);

				bGotOneTouchReport = TRUE;

				break;
			}
			default:
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_REPORTING,
					"FtsGetObjectStatusFromController - Unknown event id %d",
					EventID);
				break;
			}
		}

		CurrentDataBuffer += FIFO_EVENT_SIZE;

		// Was the last event the last one?
		// 00011111
		if (!(CurrentDataBuffer[7 - FIFO_EVENT_SIZE] & 0x1F))
		{
			break;
		}
	}

	// Re-enable interrupts
	status = FtsEnableInterrupts(SpbContext);
	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"FtsGetObjectStatusFromController - Error enabling interrupts - 0x%08lX",
			status);
		goto exit;
	}

	if (!bGotOneTouchReport)
	{
		status = STATUS_NO_DATA_DETECTED;
	}

	for (i = 0; i < MAX_TOUCHES; i++)
	{
		if (controller->DetectedObjects.States[i] != OBJECT_STATE_NOT_PRESENT)
		{
			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_REPORTING,
				"FtsGetObjectStatusFromController/Cache - Finger ID: %d, State: %d, X: %d, Y: %d",
				i,
				controller->DetectedObjects.States[i],
				controller->DetectedObjects.Positions[i].X,
				controller->DetectedObjects.Positions[i].Y);
		}
	}

exit:
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsGetObjectStatusFromController - Exit\n");

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

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"TchServiceObjectInterrupts - Entry");

	//
	// See if new touch data is available
	//
	status = FtsGetObjectStatusFromController(
		ControllerContext,
		SpbContext
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
		ControllerContext->DetectedObjects);

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