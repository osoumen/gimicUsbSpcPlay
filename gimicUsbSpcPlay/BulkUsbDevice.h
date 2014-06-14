//
//  BulkUsbDevice.h
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/06/14.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#ifndef __gimicUsbSpcPlay__BulkUsbDevice__
#define __gimicUsbSpcPlay__BulkUsbDevice__

#include <iostream>
#include <libusb.h>

class BulkUsbDevice {
public:
    BulkUsbDevice();
    ~BulkUsbDevice();
    
    // USBデバイスを開く
    int OpenDevice(int vid, int pid, int wpipe, int rpipe);
    // USBデバイスを解放する
    int CloseDevice();
    // データを書き込む
    int WriteBytes(unsigned char *data, int *bytes);
    // データを読み出す
    int ReadBytes(unsigned char *data, int *bytes, int timeOut);

    
    libusb_context          *mCtx;
    libusb_device_handle    *mDevHandle;
    int                     mVendorID;
    int                     mProductID;
};

#endif /* defined(__gimicUsbSpcPlay__BulkUsbDevice__) */
