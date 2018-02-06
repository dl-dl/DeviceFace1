#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "Recv4222.h"
#include "Ftd2xx.h"
#include "LibFT4222.h"
#include "Protocol.h"
#include "Graph.h"

extern bool terminateComm;
extern void __cdecl _translateSEH(unsigned int, EXCEPTION_POINTERS*);

int masterDevIdx = 3;
int slaveDevIdx = 1;

static const coord_t INIT_LON = (coord_t)42.5;
static const coord_t INIT_LAT = (coord_t)101.1;

static void ShowMessage(_In_z_ const char* text)
{
	MessageBoxA(NULL, text, "FT4222 Interface", MB_OK);
}

static void getNewPosTest(_Out_ PositionRawData* pt)
{
	const int PRIME1 = 281;
	const int PRIME2 = 283;
	uint64_t t = __rdtsc();
	//DWORD t = GetTickCount();
	pt->id = t % 16;
	pt->lon = INIT_LON + coord_t(t % PRIME1);
	pt->lat = INIT_LAT + coord_t(t % PRIME2);
	pt->time = GetTickCount();
}

static void getNewCompassTest(_Out_ CompassRawData* pt)
{
	pt->dir = (GetTickCount() / 1000) % 360;
}

static void generateSpiMsgTest(_Out_ SpiMsg* msg)
{
	static int cnt = 0;
	++cnt;
	if (0 == cnt % 3)
	{
		CompassRawData pos;
		getNewCompassTest(&pos);
		msg->data[0] = SPI_MSG_COMPASS;
		size_t n = pos.serialize(msg->data + 1);
		ASSERT_DBG(n < sizeof(msg->data));
	}
	else
	{
		PositionRawData pos;
		getNewPosTest(&pos);
		msg->data[0] = SPI_MSG_POS;
		size_t n = pos.serialize(msg->data + 1);
		ASSERT_DBG(n < sizeof(msg->data));
	}
}

static void writeSpi(FT_HANDLE h, _In_ uint8_t* data, size_t sz)
{
	ASSERT_DBG(sz <= 0xFFFF);
	uint16 total = 0;
	do
	{
		uint16_t tranferred = 0;
		if (FT4222_SPIMaster_SingleWrite(h, data + total, (uint16_t)sz - total, &tranferred, TRUE) != FT_OK)
			throw "FT4222_SPIMaster_SingleWrite error";
		total += tranferred;
		ASSERT_DBG(total <= sz);
	}
	while (total < sz);
}

static DWORD WINAPI ftTransmitTest(LPVOID)
{
	_set_se_translator(_translateSEH);
	try
	{
		DWORD numOfDevices = 0;
		if (FT_CreateDeviceInfoList(&numOfDevices) != FT_OK)
			throw "FT_CreateDeviceInfoList failed";
		if ((int)numOfDevices <= masterDevIdx)
			throw "Master not found";

		FT_DEVICE_LIST_INFO_NODE devInfo;
		memset(&devInfo, 0, sizeof(devInfo));
		if (FT_GetDeviceInfoDetail(masterDevIdx, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId,
								   devInfo.SerialNumber, devInfo.Description, &devInfo.ftHandle) != FT_OK)
			throw("FT_GetDeviceInfoDetail failed");

		FT_HANDLE ftHandle = NULL;
		if (FT_OpenEx((PVOID)devInfo.LocId, FT_OPEN_BY_LOCATION, &ftHandle) != FT_OK)
			throw("FT_OpenEx failed");
		devInfo.ftHandle = ftHandle;

		if (FT_SetTimeouts(ftHandle, 1000, 1000) != FT_OK)
			throw "FT_SetTimeouts failed";
		if (FT_SetLatencyTimer(ftHandle, 0) != FT_OK)
			throw "FT_SetLatencyTimer failed";
		if (FT_SetUSBParameters(ftHandle, 64 * 1024, 0) != FT_OK)
			throw "FT_SetUSBParameters failed!";
		if (FT4222_SPIMaster_Init(ftHandle, SPI_IO_SINGLE, CLK_DIV_4, CLK_IDLE_LOW, CLK_LEADING, 0x01) != FT4222_OK)
			throw "FT4222_SPIMaster_Init failed";
		if (FT4222_SPI_SetDrivingStrength(ftHandle, DS_12MA, DS_16MA, DS_16MA) != FT4222_OK)
			throw "FT4222_SPI_SetDrivingStrength failed";

		while (!terminateComm)
		{
			Sleep(1000);
			SpiMsg msg;
			generateSpiMsgTest(&msg);
			writeSpi(ftHandle, msg.data, sizeof(msg.data));
		}
		FT_Close(ftHandle);
	}
	catch (const char* err)
	{
		ShowMessage(err);
	}
	catch (...)
	{
		ShowMessage("General error");
	}

	return 0;
}

static void processSpiMsg(_In_ const SpiMsg& msg)
{
	if (SPI_MSG_POS == msg.data[0])
	{
		PositionRawData* pt = new PositionRawData;
		size_t n = pt->deserialize(msg.data + 1);
		ASSERT_DBG(n < sizeof(msg.data));
		gPostMsg(WM_USER_MSG_POINT_DATA, 0, pt);
	}
	else if (SPI_MSG_COMPASS == msg.data[0])
	{
		CompassRawData* pt = new CompassRawData;
		size_t n = pt->deserialize(msg.data + 1);
		ASSERT_DBG(n < sizeof(msg.data));
		gPostMsg(WM_USER_MSG_COMPASS_DATA, 0, pt);
	}
}

static DWORD WINAPI ftRecv(LPVOID)
{
	_set_se_translator(_translateSEH);
	try
	{
		DWORD numOfDevices = 0;
		if (FT_CreateDeviceInfoList(&numOfDevices) != FT_OK)
			throw "FT_CreateDeviceInfoList failed";
		if ((int)numOfDevices <= slaveDevIdx)
			throw "Slave not found";

		FT_DEVICE_LIST_INFO_NODE devInfo = { 0 };

		if (FT_GetDeviceInfoDetail(slaveDevIdx, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId,
								   devInfo.SerialNumber, devInfo.Description, &devInfo.ftHandle) != FT_OK)
			throw "FT_GetDeviceInfoDetail failed";

		FT_HANDLE ftHandle = NULL;
		if (FT_OpenEx((PVOID)devInfo.LocId, FT_OPEN_BY_LOCATION, &ftHandle) != FT_OK)
			throw "FT_OpenEx failed";
		devInfo.ftHandle = ftHandle;

		if (FT_SetTimeouts(ftHandle, 1000, 1000) != FT_OK)
			throw "FT_SetTimeouts failed";
		if (FT_SetLatencyTimer(ftHandle, 0) != FT_OK)
			throw "FT_SetLatencyTimer failed";
		if (FT_SetUSBParameters(ftHandle, 64 * 1024, 0) != FT_OK)
			throw "FT_SetUSBParameters failed!";
		if (FT4222_SPISlave_InitEx(ftHandle, SPI_SLAVE_NO_PROTOCOL) != FT4222_OK)
			throw "FT4222_SPISlave_InitEx failed";
		if (FT4222_SPI_SetDrivingStrength(ftHandle, DS_12MA, DS_16MA, DS_16MA) != FT4222_OK)
			throw "FT4222_SPI_SetDrivingStrength failed";

		HANDLE hEvent = CreateEvent(NULL, false, false, NULL);
		if (NULL == hEvent)
			throw "CreateEvent failed";

		if (FT_SetEventNotification(ftHandle, FT_EVENT_RXCHAR, hEvent) != FT_OK)
			// if ( FT4222_SetEventNotification(FtHandle, FT4222_EVENT_RXCHAR, hEvent) != FT_OK ) // does not work
			throw "FT_SetEventNotification failed";

		SpiMsg rxBuffer;
		while (!terminateComm)
		{
			WaitForSingleObject(hEvent, 1000);

			uint16_t rxSize = 0, sizeTransferred = 0;
			if (FT4222_SPISlave_GetRxStatus(ftHandle, &rxSize) != FT_OK)
				throw "FT4222_SPISlave_GetRxStatus failed";
			if (rxSize > 0)
			{
				if (rxSize == sizeof(rxBuffer.data))
				{
					if (FT4222_SPISlave_Read(ftHandle, rxBuffer.data, rxSize, &sizeTransferred) != FT_OK)
						throw "FT4222_SPISlave_Read failed";
					processSpiMsg(rxBuffer);
				}
				else // incomplete message
				{
					for (size_t i = 0; i < rxSize; ++i)
						if (FT4222_SPISlave_Read(ftHandle, rxBuffer.data, 1, &sizeTransferred) != FT_OK)
							throw "FT4222_SPISlave_Read failed";
				}
			}
		}
		FT_Close(ftHandle);
	}
	catch (const char* err)
	{
		ShowMessage(err);
	}
	catch (...)
	{
		ShowMessage("General error");
	}

	return 0;
}

static std::string deviceFlagToString(DWORD flags)
{
	std::string msg;
	msg += (flags & 0x1) ? "DEVICE_OPEN" : "DEVICE_CLOSED";
	msg += ", ";
	msg += (flags & 0x2) ? "High-speed USB" : "Full-speed USB";
	return msg;
}

int listFtUsbDevices(char* s)
{
	DWORD numOfDevices = 0;
	FT_STATUS ftStatus = FT_CreateDeviceInfoList(&numOfDevices);

	for (DWORD iDev = 0; iDev < numOfDevices; ++iDev)
	{
		FT_DEVICE_LIST_INFO_NODE devInfo;
		memset(&devInfo, 0, sizeof(devInfo));

		ftStatus = FT_GetDeviceInfoDetail(iDev, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId,
										  devInfo.SerialNumber, devInfo.Description, &devInfo.ftHandle);

		if (FT_OK == ftStatus)
		{
			s += sprintf_s(s, 128, "Dev %d:\r\n", iDev);
			s += sprintf_s(s, 128, "  Flags= 0x%x, (%s)\r\n", devInfo.Flags, deviceFlagToString(devInfo.Flags).c_str());
			s += sprintf_s(s, 128, "  Type= 0x%x    ", devInfo.Type);
			s += sprintf_s(s, 128, "  ID= 0x%x    ", devInfo.ID);
			s += sprintf_s(s, 128, "  LocId= 0x%X    ", devInfo.LocId);
//			s += sprintf_s(s, 128, "  SerialNumber= %s     ", devInfo.SerialNumber);
			s += sprintf_s(s, 128, "  Description= %s\r\n", devInfo.Description);
//			s += sprintf_s(s, 128, "  ftHandle= 0x%x\r\n", (int)devInfo.ftHandle);
		}
	}
	return numOfDevices;
}


HANDLE revcFT4222()
{
	if(masterDevIdx >= 0)
		CloseHandle(CreateThread(NULL, 0, ftTransmitTest, 0, 0, NULL));
	if(slaveDevIdx < 0)
		return NULL;
	return CreateThread(NULL, 0, ftRecv, 0, 0, NULL);
}
