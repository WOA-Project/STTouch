/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Sample code. Dealpoint ID #843729.

	Module Name:

		ftsregs.h

	Abstract:

		Contains common types and defintions used internally
		by the multi touch screen driver.

	Environment:

		Kernel mode

	Revision History:

--*/

#pragma once

//#define FTM3_CHIP

#ifdef FTM3_CHIP
#define IER_ADDR			0x00, 0x1C
#else
#define IER_ADDR			0x00, 0x2C
#endif

#define IER_ENABLE			0x41

#ifdef FTM3_CHIP
#define FIFO_DEPTH			32
#else
#define FIFO_DEPTH			64
#endif

#define FIFO_CMD_READONE	0x85
#define FIFO_CMD_READALL	0x86

#define FIFO_EVENT_SIZE		8

#define FTS_CMD_MS_MT_SENSE_ON	0x93

#define FTS_CMD_HW_REG_W	0xB6

#define EVENTID_ENTER_POINTER	     0x03
#define EVENTID_LEAVE_POINTER	     0x04
#define EVENTID_MOTION_POINTER	     0x05