//
//  SpcControlDevice.h
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/06/14.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#ifndef __gimicUsbSpcPlay__SpcControlDevice__
#define __gimicUsbSpcPlay__SpcControlDevice__

#include "BulkUsbDevice.h"

class SpcControlDevice {
public:
    SpcControlDevice();
    ~SpcControlDevice();
    
    int Init();
    int Close();
    void HwReset();
    void SwReset();
    void PortWrite(int addr, unsigned char data);
    unsigned char PortRead(int addr);
    void BlockWrite(int addr, unsigned char data);
    void ReadAndWait(int addr, unsigned char waitValue);
    void WriteBuffer();
    void UploadDSPReg(unsigned char *dspReg);
    void UploadZeroPageIPL(unsigned char *zeroPageRam);
    void UploadRAMDataIPL(unsigned char *ram, int addr, int size);
    void WaitReady();
    void JumpToBootloader(int addr,
                          unsigned char p0, unsigned char p1,
                          unsigned char p2, unsigned char p3);
    void JumpToDspCode(int addr);

private:
    static const int GIMIC_USBVID = 0x16c0;
    static const int GIMIC_USBPID = 0x05e5;
    static const int GIMIC_USBWPIPE = 0x02;
    static const int GIMIC_USBRPIPE = 0x85;
    static const int BLOCKWRITE_CMD_LEN = 4;

    BulkUsbDevice   *mUsbDev;
    unsigned char   mWriteBuf[64];
    unsigned char   mReadBuf[64];
    int             mWriteBytes;
    
    void uploadDSPRamLoadCode(int addr);
};

#endif /* defined(__gimicUsbSpcPlay__SpcControlDevice__) */
