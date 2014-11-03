//
//  BulkUsbDevice.h
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/06/14.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#ifndef __gimicUsbSpcPlay__BulkUsbDevice__
#define __gimicUsbSpcPlay__BulkUsbDevice__

#include <libusb-1.0/libusb.h>

class BulkUsbDevice {
public:
    BulkUsbDevice();
    ~BulkUsbDevice();
    
    bool IsInitialized() { return mDevHandle?true:false; }
    // USBデバイスを開く
    int OpenDevice(int vid, int pid, int wpipe, int rpipe);
    // USBデバイスを解放する
    int CloseDevice();
    // データを書き込む
    int WriteBytes(unsigned char *data, int *bytes);
    int WriteBytesAsync(unsigned char *data, int *bytes);
    // データを読み出す
    int ReadBytes(unsigned char *data, int *bytes, int timeOut);
    int ReadBytesAsync(int bytes);
    int GetAvailableInBytes();
    unsigned char *GetReadBytesPtr();
    void CancelAllAsyncRead(bool quitting);
    
    void HandleEvents();

private:
    static const int WRITE_BUF_SIZE = 4096;
    static const int READ_BUF_SIZE = 4096;
    static const int WRITE_TRANSFER_NUM = 1024;
    static const int READ_TRANSFER_NUM = 1;
    
    libusb_context          *mCtx;
    libusb_device_handle    *mDevHandle;
    int                     mVendorID;
    int                     mProductID;
    int                     mWPipe;
    int                     mRPipe;
    
    unsigned char           mWriteBuf[WRITE_BUF_SIZE];
    int                     mWriteBufPtr;
    struct libusb_transfer *m_pTransferOut[WRITE_TRANSFER_NUM];
    int                     mTransferOutPtr;
    int                     mNumTransfersOut;
    
    unsigned char           mReadBuf[READ_BUF_SIZE];
    int                     mReadBufPtr;
    struct libusb_transfer *m_pTransferIn[READ_TRANSFER_NUM];
    int                     mTransferInPtr;
    int                     mNumTransfersIn;
    unsigned char           mReadAvailableBuf[READ_BUF_SIZE];
    int                     mAvailableInBytes;
    
    bool                    mIsQuitting;
    
    static void LIBUSB_CALL callbackOut(struct libusb_transfer *transfer);
    static void LIBUSB_CALL callbackIn(struct libusb_transfer *transfer);
};

#endif /* defined(__gimicUsbSpcPlay__BulkUsbDevice__) */
