//
//  SpcControlDevice.cpp
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/06/14.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#include <iostream>
#include "unistd.h"
#include "SpcControlDevice.h"

#define NO_VERIFY

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
    if (mUsbDev->IsInitialized() == false) {
        return 1;
    }
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
    mUsbDev->WriteBytesAsync(cmd, &wb);
}

unsigned char SpcControlDevice::PortRead(int addr)
{
    unsigned char cmd[] = {0xfd, 0xb0, 0x00, 0x00, 0xff};
    cmd[2] = addr & 0x03;
    int wb = sizeof(cmd);
    mUsbDev->WriteBytes(cmd, &wb);
    
    int rb = 64;
    mUsbDev->ReadBytes(mReadBuf, &rb, 500);  // 500msでタイムアウト
    return mReadBuf[0];
}

void SpcControlDevice::WaitReady()
{
    if (mUsbDev->IsInitialized()) {
        while (PortRead(0) != 0xaa || PortRead(1) != 0xbb) {
            usleep(2);
        }
    }
}

void SpcControlDevice::UploadDSPRegAndZeroPage(unsigned char *dspReg, unsigned char *zeroPageRam)
{
    uploadDSPRamLoadCode(0x0002);   // $0002 にコードを書き込む
    // DSPロードプログラムのアドレスをP2,P3にセットして、P0に16+1を書き込む
    PortWrite(2, 0x02);
    PortWrite(3, 0x00);
    PortWrite(1, 0x00); // 0なのでP2,P3はジャンプ先アドレス
    PortWrite(0, 0x11); // 16バイト書き込んだので１つ飛ばして17
    while (PortRead(0) != 0x11) {
        usleep(2);
    }
    unsigned char port0state = 0;
    for (int i=0; i<128; i++) {
        PortWrite(1, dspReg[i]);
        PortWrite(0, port0state);
        if (i < 127) {
            while (PortRead(0) != port0state) {
                usleep(2);
            }
        }
        port0state++;
    }
    // 正常に128バイト書き込まれたなら、プログラムはここで $ffc7 へジャンプされ、
    // P0に $AA が書き込まれる
    while (PortRead(0) != 0xaa) {
        usleep(2);
    }
    // 次に、IPLを利用して、0ページに書き込む
    port0state = 0;
    PortWrite(2, 0x02);
    PortWrite(3, 0x00);
    PortWrite(1, 0x01); // 非0なのでP2,P3は書き込み開始アドレス
    PortWrite(0, 0xcc); // 起動直後と同じ
    while (PortRead(0) != 0xcc) {
        usleep(2);
    }
    for (int i=2; i<0xf0; i++) {
        PortWrite(1, zeroPageRam[i]);
        PortWrite(0, port0state);
        while (PortRead(0) != port0state) {
            usleep(2);
        }
        port0state++;
    }
}

void SpcControlDevice::UploadRAMData(unsigned char *ram, int addr, int size)
{
    PortWrite(2, addr & 0xff);
    PortWrite(3, (addr >> 8) & 0xff);
    PortWrite(1, 0x01); // 非0なのでP2,P3は書き込み開始アドレス
    unsigned char port0State = PortRead(0);
    port0State += 2;
    PortWrite(0, port0State & 0xff);
    while (PortRead(0) != (port0State&0xff)) {
        usleep(2);
    }
    port0State = 0;
    for (int i=0; i<size; i++) {
        PortWrite(1, ram[i]);
        PortWrite(0, port0State);
#ifdef NO_VERIFY
        // P0の確認を取らずに次々に送信する
        // 4倍程度速くなったが、データの正確性が保証されなくなる
        usleep(80);
        mUsbDev->HandleEvents();
#else
        // 1バイトずつP0を確認しながら送信する本来の方法
        while (PortRead(0) != port0State) {
            usleep(2);
        }
#endif
        port0State++;
        if ((i % 256) == 255) {
            std::cout << ".";
        }
    }
}

void SpcControlDevice::uploadDSPRamLoadCode(int addr)
{
    unsigned char dspram_write_code[16] =
    {   //128バイトの DSPレジスタをロードするためのコード
        /*
         --
         mov   $f2,a
         -
         cmp   a,$f4
         bne   -
         mov   $f3,$f5
         mov   $f4,a
         inc   a
         bpl   --
         bra   $b7 (->ffc7)
         */
        0xC4, 0xF2, 0x64, 0xF4, 0xD0, 0xFC, 0xFA, 0xF5, 0xF3, 0xC4, 0xF4, 0xBC, 0x10, 0xF2, 0x2F, 0xB7,
    };
    
    PortWrite(3, (addr >> 8) & 0xff);
    PortWrite(2, addr & 0xff);
    PortWrite(1, 1);
    PortWrite(0, 0xcc);
    while (PortRead(0) != 0xcc) {
        usleep(2);
    }
    for (int i=0; i<sizeof(dspram_write_code); i++) {
        PortWrite(1, dspram_write_code[i]);
        PortWrite(0, i & 0xff);
        while (PortRead(0) != (i&0xff)) {
            usleep(2);
        }
    }
}

void SpcControlDevice::JumpToBootloader(int addr,
                                        unsigned char p0, unsigned char p1,
                                        unsigned char p2, unsigned char p3)
{
    PortWrite(3, (addr >> 8) & 0xff);
    PortWrite(2, addr & 0xff);
    PortWrite(1, 0);    // 0なのでP2,P3はジャンプ先アドレス
    unsigned char port0state = PortRead(0);
    port0state += 2;
    PortWrite(0, port0state);
    // ブートローダーがP0に'S'を書き込むのを待つ
    while (PortRead(0) != 'S') {
        usleep(2);
    }
    // P0-P3を復元
    PortWrite(0, p0);
    PortWrite(1, p1);
    PortWrite(2, p2);
    PortWrite(3, p3);
}
