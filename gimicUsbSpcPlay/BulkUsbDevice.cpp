//
//  BulkUsbDevice.cpp
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/06/14.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#include <iostream>
#include <unistd.h>
#include <string.h>
#include "BulkUsbDevice.h"

using namespace std;

BulkUsbDevice::BulkUsbDevice()
: mCtx(NULL),
  mDevHandle(NULL),
  mWriteBufPtr(0),
  mTransferPtr(0),
  mNumTransfers(0)
{
    // USBライブラリの初期化を行う
    int r;
	r = libusb_init(&mCtx);
	if(r < 0) {
		cout<<"Init Error "<<r<<endl;
	}
    // デバッグレベルを3に設定する
	::libusb_set_debug(mCtx, 3);
}

BulkUsbDevice::~BulkUsbDevice()
{
    ::libusb_exit(mCtx);
}

int BulkUsbDevice::OpenDevice(int vid, int pid, int wpipe, int rpipe)
{
    int r = 0;
    
    if (mDevHandle) {
        CloseDevice();
    }
    
    mVendorID = vid;
    mProductID = pid;
    mWPipe = wpipe;
    mRPipe = rpipe;
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
    if (::libusb_kernel_driver_active(mDevHandle, 0) == 1) {
		cout<<"Kernel Driver Active"<<endl;
		if (::libusb_detach_kernel_driver(mDevHandle, 0) == 0) {
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
        while (mNumTransfers > 0) {
            usleep(1000);
            ::libusb_handle_events(mCtx);
        }
        r = ::libusb_release_interface(mDevHandle, 0);
        if(r!=0) {
            cout<<"Cannot Release Interface"<<endl;
            return 1;
        }
    }
    ::libusb_close(mDevHandle);
    return 0;
}

int BulkUsbDevice::WriteBytes(unsigned char *data, int *bytes)
{
    int r = 0;
    int inBytes = *bytes;
    r = ::libusb_bulk_transfer(mDevHandle, mWPipe, data, inBytes, bytes, 500);
#ifdef _DEBUG
    if (r == 0 && *bytes == inBytes)
		cout<<"Writing Successful!"<<endl;
	else
		cout<<"Write Error"<<endl;
#endif
    return r;
}

int BulkUsbDevice::WriteBytesAsync(unsigned char *data, int *bytes)
{
    int r = 0;
    int inBytes = *bytes;
    // データをリングバッファにコピーする
    if ((WRITE_BUF_SIZE - mWriteBufPtr) < inBytes) {
        mWriteBufPtr = 0;
    }
    memcpy(&mWriteBuf[mWriteBufPtr], data, inBytes);
    
    m_pTransferOut[mTransferPtr] = libusb_alloc_transfer(0);
    ::libusb_fill_bulk_transfer(m_pTransferOut[mTransferPtr],
                              mDevHandle,
                              mWPipe,
                              &mWriteBuf[mWriteBufPtr],
                              inBytes,
                              callbackOut,
                              reinterpret_cast<void *>(this),
                              0);
    while (mNumTransfers == WRITE_TRANSFER_NUM) {
        usleep(2);
        ::libusb_handle_events(mCtx);
    }
    mNumTransfers++;
    r = ::libusb_submit_transfer(m_pTransferOut[mTransferPtr]);
    mWriteBufPtr += inBytes;
#ifdef _DEBUG
    if (r == 0 && *bytes == inBytes)
		cout<<"Writing Successful!"<<endl;
	else
		cout<<"Write Error"<<endl;
#endif
    return r;
}

void BulkUsbDevice::HandleEvents()
{
    ::libusb_handle_events(mCtx);
}

void BulkUsbDevice::callbackOut(struct libusb_transfer *transfer)
{
    BulkUsbDevice *This = reinterpret_cast<BulkUsbDevice *>(transfer->user_data);
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED ||
        transfer->status == LIBUSB_TRANSFER_STALL) {
        ::libusb_free_transfer(transfer);
        This->mNumTransfers--;
    }
    else {
        usleep(100);
        ::libusb_submit_transfer(transfer);
    }
}

int BulkUsbDevice::ReadBytes(unsigned char *data, int *bytes, int timeOut)
{
    int r = 0;
    int reqBytes = *bytes;
    r = ::libusb_bulk_transfer(mDevHandle, mRPipe, data, reqBytes, bytes, timeOut);
#ifdef _DEBUG
    if (r == 0 && *bytes == reqBytes)
		cout<<"Reading Successful!"<<endl;
	else
		cout<<"Read Error"<<endl;
#endif
    return r;
}
