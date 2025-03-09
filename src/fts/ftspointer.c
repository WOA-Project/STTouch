/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		ftspointer.c

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
#include <fts\ftspointer.h>
#include <ftspointer.tmh>

NTSTATUS
FtsProcessEnterPointerEvent(
	FTS_CONTROLLER_CONTEXT* ControllerContext,
	PREPORT_CONTEXT ReportContext,
	BYTE* EventData
)
{
	NTSTATUS status = STATUS_SUCCESS;

	int x, y;
	BYTE X_MSB = 0;
	BYTE X_LSB = 0;
	BYTE Y_MSB = 0;
	BYTE Y_LSB = 0;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Enter Pointer");

	BYTE touchId = (EventData[2] & 0x0F);

	if (touchId >= MAX_TOUCHES || touchId < 0)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_REPORTING,
			"FtsProcessOneEvent - Invalid touch id %d",
			touchId);
		goto exit;
	}

	X_MSB = EventData[3];
	X_LSB = (EventData[5] & 0xF0) >> 4;

	Y_MSB = EventData[4];
	Y_LSB = EventData[5] & 0x0F;

	x = (X_MSB << 4) | X_LSB;
	y = (Y_MSB << 4) | Y_LSB;

	ControllerContext->DetectedObjects.States[touchId] = OBJECT_STATE_FINGER_PRESENT_WITH_ACCURATE_POS;
	ControllerContext->DetectedObjects.Positions[touchId].X = x;
	ControllerContext->DetectedObjects.Positions[touchId].Y = y;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Touch %d at (x=%d, y=%d)",
		touchId,
		x,
		y);

	status = ReportObjects(
		ReportContext,
		ControllerContext->DetectedObjects);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"FtsProcessOneEvent - Error while reporting objects - 0x%08lX",
			status);

		goto exit;
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
FtsProcessMotionPointerEvent(
	FTS_CONTROLLER_CONTEXT* ControllerContext,
	PREPORT_CONTEXT ReportContext,
	BYTE* EventData
)
{
	NTSTATUS status = STATUS_SUCCESS;

	int x, y;
	BYTE X_MSB = 0;
	BYTE X_LSB = 0;
	BYTE Y_MSB = 0;
	BYTE Y_LSB = 0;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Motion Pointer");

	BYTE touchId = (EventData[2] & 0x0F);

	if (touchId >= MAX_TOUCHES || touchId < 0)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_REPORTING,
			"FtsProcessOneEvent - Invalid touch id %d",
			touchId);
		goto exit;
	}

	X_MSB = EventData[3];
	X_LSB = (EventData[5] & 0xF0) >> 4;

	Y_MSB = EventData[4];
	Y_LSB = EventData[5] & 0x0F;

	x = (X_MSB << 4) | X_LSB;
	y = (Y_MSB << 4) | Y_LSB;

	ControllerContext->DetectedObjects.States[touchId] = OBJECT_STATE_FINGER_PRESENT_WITH_ACCURATE_POS;
	ControllerContext->DetectedObjects.Positions[touchId].X = x;
	ControllerContext->DetectedObjects.Positions[touchId].Y = y;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Touch %d at (x=%d, y=%d)",
		touchId,
		x,
		y);

	status = ReportObjects(
		ReportContext,
		ControllerContext->DetectedObjects);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"FtsProcessOneEvent - Error while reporting objects - 0x%08lX",
			status);

		goto exit;
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
FtsProcessLeavePointerEvent(
	FTS_CONTROLLER_CONTEXT* ControllerContext,
	PREPORT_CONTEXT ReportContext,
	BYTE* EventData
)
{
	NTSTATUS status = STATUS_SUCCESS;

	int x, y;
	BYTE X_MSB = 0;
	BYTE X_LSB = 0;
	BYTE Y_MSB = 0;
	BYTE Y_LSB = 0;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Leave Pointer");

	BYTE touchId = (EventData[2] & 0x0F);

	if (touchId >= MAX_TOUCHES || touchId < 0)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_REPORTING,
			"FtsProcessOneEvent - Invalid touch id %d",
			touchId);
		goto exit;
	}

	X_MSB = EventData[3];
	X_LSB = (EventData[5] & 0xF0) >> 4;

	Y_MSB = EventData[4];
	Y_LSB = EventData[5] & 0x0F;

	x = (X_MSB << 4) | X_LSB;
	y = (Y_MSB << 4) | Y_LSB;

	ControllerContext->DetectedObjects.States[touchId] = OBJECT_STATE_NOT_PRESENT;
	ControllerContext->DetectedObjects.Positions[touchId].X = x;
	ControllerContext->DetectedObjects.Positions[touchId].Y = y;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Touch %d at (x=%d, y=%d) left",
		touchId,
		x,
		y);

	status = ReportObjects(
		ReportContext,
		ControllerContext->DetectedObjects);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_VERBOSE,
			TRACE_SAMPLES,
			"FtsProcessOneEvent - Error while reporting objects - 0x%08lX",
			status);

		goto exit;
	}

exit:

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_REPORTING,
		"FtsProcessOneEvent - Exit - 0x%08lX",
		status);

	return status;
}