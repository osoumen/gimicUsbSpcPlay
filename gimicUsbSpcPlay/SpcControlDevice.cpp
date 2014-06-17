//
//  SpcControlDevice.cpp
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/06/14.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#include "SpcControlDevice.h"


SpcControlDevice::SpcControlDevice()
{
    mUsbDev = new BulkUsbDevice();
}

SpcControlDevice::~SpcControlDevice()
{
    delete mUsbDev;
}

int SpcControlDevice::Init()
{
    int r = 0;
    r = mUsbDev->OpenDevice(GIMIC_USBVID, GIMIC_USBPID, GIMIC_USBWPIPE, GIMIC_USBRPIPE);
    HwReset();
    return r;
}

int SpcControlDevice::Close()
{
    int r = 0;
    r = mUsbDev->CloseDevice();
    return r;
}

void SpcControlDevice::HwReset()
{
    unsigned char cmd[] = {0xfd, 0x81, 0xff};
    int wb = sizeof(cmd);
    mUsbDev->WriteBytes(cmd, &wb);
}

void SpcControlDevice::SwReset()
{
    unsigned char cmd[] = {0xfd, 0x82, 0xff};
    int wb = sizeof(cmd);
    mUsbDev->WriteBytes(cmd, &wb);
}

void SpcControlDevice::PortWrite(int addr, unsigned char data)
{
    unsigned char cmd[] = {0x00, 0x00, 0xff};
    cmd[0] = addr & 0x03;
    cmd[1] = data;
    int wb = sizeof(cmd);
    mUsbDev->WriteBytes(cmd, &wb);
}

unsigned char SpcControlDevice::PortRead(int addr)
{
    unsigned char cmd[] = {0xfd, 0xb0, 0x00, 0x00, 0xff};
    cmd[2] = addr & 0x03;
    int wb = sizeof(cmd);
    mUsbDev->WriteBytes(cmd, &wb);
    
    int rb = 64;
    mUsbDev->ReadBytes(mReadBuf, &rb, 10);  // 10msでタイムアウト
    return mReadBuf[0];
}
