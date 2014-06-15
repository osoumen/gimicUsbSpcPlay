//
//  BulkUsbDevice.cpp
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/06/14.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#include <iostream>
#include "BulkUsbDevice.h"

using namespace std;

BulkUsbDevice::BulkUsbDevice()
: mCtx(NULL),
  mDevHandle(NULL)
{
    // USBライブラリの初期化を行う
    int r;
	r = libusb_init(&mCtx);
	if(r < 0) {
		cout<<"Init Error "<<r<<endl;
	}
    // デバッグレベルを3に設定する
	libusb_set_debug(mCtx, 3);
}

BulkUsbDevice::~BulkUsbDevice()
{
    libusb_exit(mCtx);
}

int BulkUsbDevice::OpenDevice(int vid, int pid, int wpipe, int rpipe)
{
    int r = 0;
    
    if (mDevHandle) {
        CloseDevice();
    }
    
    mVendorID = vid;
    mProductID = pid;
    // VendorIDとProductIDを指定してデバイスを開く
    mDevHandle = libusb_open_device_with_vid_pid(mCtx, vid, pid);
    if(mDevHandle == NULL) {
		cout<<"Cannot open device"<<endl;
        return 1;
    }
	else {
		cout<<"Device Opened"<<endl;
    }
    
    // カーネルドライバが有効な場合、無効にして制御を取得する
    if (libusb_kernel_driver_active(mDevHandle, 0) == 1) {
		cout<<"Kernel Driver Active"<<endl;
		if (libusb_detach_kernel_driver(mDevHandle, 0) == 0) {
			cout<<"Kernel Driver Detached!"<<endl;
        }
	}
	r = libusb_claim_interface(mDevHandle, 0); //とりあえず1個目のインターフェースのみ対応
	if(r < 0) {
		cout<<"Cannot Claim Interface"<<endl;
		return 1;
	}
    
    return 0;
}

int BulkUsbDevice::CloseDevice()
{
    int r = 0;
    if (mDevHandle) {
        r = libusb_release_interface(mDevHandle, 0);
        if(r!=0) {
            cout<<"Cannot Release Interface"<<endl;
            return 1;
        }
    }
    libusb_close(mDevHandle);
    return 0;
}

int BulkUsbDevice::WriteBytes(unsigned char *data, int *bytes)
{
    int r = 0;
    int inBytes = *bytes;
    r = libusb_bulk_transfer(mDevHandle, (2 | LIBUSB_ENDPOINT_OUT), data, inBytes, bytes, 0);
#ifdef _DEBUG
    if (r == 0 && *bytes == inBytes)
		cout<<"Writing Successful!"<<endl;
	else
		cout<<"Write Error"<<endl;
#endif
    return r;
}

int BulkUsbDevice::ReadBytes(unsigned char *data, int *bytes, int timeOut)
{
    int r = 0;
    int reqBytes = *bytes;
    r = libusb_bulk_transfer(mDevHandle, (5 | LIBUSB_ENDPOINT_IN), data, reqBytes, bytes, timeOut);
#ifdef _DEBUG
    if (r == 0 && *bytes == reqBytes)
		cout<<"Reading Successful!"<<endl;
	else
		cout<<"Read Error"<<endl;
#endif
    return r;
}
