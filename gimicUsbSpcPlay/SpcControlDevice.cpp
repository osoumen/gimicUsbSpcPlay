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
    mWriteBytes = BLOCKWRITE_CMD_LEN;    // 0xFD,0xB2,0xNN分
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
    cmd[0] = addr;
    cmd[1] = data;
    int wb = sizeof(cmd);
    mUsbDev->WriteBytes(cmd, &wb);
}

unsigned char SpcControlDevice::PortRead(int addr)
{
    unsigned char cmd[] = {0xfd, 0xb0, 0x00, 0x00, 0xff};
    cmd[2] = addr;
    int wb = sizeof(cmd);
    mUsbDev->WriteBytes(cmd, &wb);
    
    int rb = 64;
    mUsbDev->ReadBytes(mReadBuf, &rb, 500);  // 500msでタイムアウト
    return mReadBuf[0];
}

void SpcControlDevice::BlockWrite(int addr, unsigned char data)
{
    // 残り2バイト未満なら書き込んでから追加する
    if (mWriteBytes > (PACKET_SIZE-2)) {
        WriteBuffer();
    }
    mWriteBuf[mWriteBytes] = addr & 0x03;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data;
    mWriteBytes++;
}

void SpcControlDevice::BlockWrite(int addr, unsigned char data, unsigned char data2)
{
    // 残り3バイト未満なら書き込んでから追加する
    if (mWriteBytes > (PACKET_SIZE-3)) {
        WriteBuffer();
    }
    mWriteBuf[mWriteBytes] = (addr & 0x03) | 0x10;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data2;
    mWriteBytes++;
}

void SpcControlDevice::BlockWrite(int addr, unsigned char data, unsigned char data2, unsigned char data3)
{
    // 残り4バイト未満なら書き込んでから追加する
    if (mWriteBytes > (PACKET_SIZE-4)) {
        WriteBuffer();
    }
    mWriteBuf[mWriteBytes] = (addr & 0x03) | 0x20;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data2;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data3;
    mWriteBytes++;
}

void SpcControlDevice::BlockWrite(int addr, unsigned char data, unsigned char data2, unsigned char data3, unsigned char data4)
{
    // 残り5バイト未満なら書き込んでから追加する
    if (mWriteBytes > (PACKET_SIZE-5)) {
        WriteBuffer();
    }
    mWriteBuf[mWriteBytes] = (addr & 0x03) | 0x30;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data2;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data3;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = data4;
    mWriteBytes++;
}

void SpcControlDevice::ReadAndWait(int addr, unsigned char waitValue)
{
    if (mWriteBytes > (PACKET_SIZE-2)) {
        WriteBuffer();
    }
    mWriteBuf[mWriteBytes] = addr | 0x80;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = waitValue;
    mWriteBytes++;
}

void SpcControlDevice::WriteAndWait(int addr, unsigned char waitValue)
{
    if (mWriteBytes > (PACKET_SIZE-2)) {
        WriteBuffer();
    }
    mWriteBuf[mWriteBytes] = addr | 0xc0;
    mWriteBytes++;
    mWriteBuf[mWriteBytes] = waitValue;
    mWriteBytes++;
}

void SpcControlDevice::WriteBuffer()
{
    /*
    if (mWriteBytes > 62) {
        // TODO: Assert
        return;
    }
    */
    if (mWriteBytes > BLOCKWRITE_CMD_LEN) {
        mWriteBuf[0] = 0xfd;
        mWriteBuf[1] = 0xb2;
        mWriteBuf[2] = 0x00;
        mWriteBuf[3] = 0x00;
        for (int i=0; i<1; i++) {
            if (mWriteBytes < 64) {
                mWriteBuf[mWriteBytes] = 0xff;
                mWriteBytes++;
            }
        }
        /*
        puts("\n--Dump--");
        for (int i=3; i<64; i+=2) {
            int blockaddr = mWriteBuf[i];
            int blockdata = mWriteBuf[i+1];
            printf("Block : 0x%02X / 0x%02X\n", blockaddr, blockdata);
            if (blockaddr == 0xFF && blockdata == 0xFF)break;
        }
         */
        //printf("mWriteBytes:%d\n", mWriteBytes);
        // GIMIC側のパケットは64バイト固定なので満たない場合0xffを末尾に追加
        if (mWriteBytes < 64) {
            mWriteBuf[mWriteBytes++] = 0xff;
        }
        mUsbDev->WriteBytes(mWriteBuf, &mWriteBytes);
        mWriteBytes = BLOCKWRITE_CMD_LEN;
    }
}

int SpcControlDevice::CatchTransferError()
{
    if (mUsbDev->GetAvailableInBytes()) {
        unsigned char *msg = mUsbDev->GetReadBytesPtr();
        int err = *(reinterpret_cast<unsigned int*>(msg));
        if (err == 0xfefefefe) {
            return err;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------

int SpcControlDevice::WaitReady()
{
    if (mUsbDev->IsInitialized()) {
        ReadAndWait(0, 0xaa);
        ReadAndWait(1, 0xbb);
        WriteBuffer();
    }
    
    int err = CatchTransferError();
    if (err) {
        return err;
    }
    return 0;
}

int SpcControlDevice::UploadDSPReg(unsigned char *dspReg)
{
    int err = uploadDSPRamLoadCode(0x0002); // $0002 にコードを書き込む
    if (err < 0) {
        return err;
    }
    
    // DSPロードプログラムのアドレスをP2,P3にセットして、P0に16+1を書き込む
    BlockWrite(1, 0x00, 0x02, 0x00); // 0なのでP2,P3はジャンプ先アドレス
    WriteAndWait(0, err+1); // 16バイト書き込んだので１つ飛ばして17
    WriteBuffer();
    unsigned char port0state = 0;
    for (int i=0; i<128; i++) {
        BlockWrite(1, dspReg[i]);
        if (i < 127) {
            WriteAndWait(0, port0state);
        }
        else {
            BlockWrite(0, port0state);
        }
        port0state++;
        if (i == 127) {
            WriteBuffer();
        }
        err = CatchTransferError();
        if (err) {
            return err;
        }
    }
    // 正常に128バイト書き込まれたなら、プログラムはここで $ffc7 へジャンプされ、
    // P0に $AA が書き込まれる
    ReadAndWait(0, 0xaa);
    WriteBuffer();
    return 0;
}

int SpcControlDevice::UploadDSPReg2(unsigned char *dspReg)
{
    int err = uploadDSPRamLoadCode2(0x0002);   // $0002 にコードを書き込む
    if (err < 0) {
        return err;
    }
    // DSPロードプログラムのアドレスをP2,P3にセットして、P0にcodeSize+1を書き込む
    BlockWrite(2, 0x02);
    BlockWrite(3, 0x00);
    BlockWrite(1, 0x00); // 0なのでP2,P3はジャンプ先アドレス
    BlockWrite(0, err+1);
    //ReadAndWait(0, codeSize+1);
    ReadAndWait(2, 0x53);
    unsigned char port0state = 0;
    for (int i=0; i<128; i++) {
        BlockWrite(1, dspReg[i]);
        WriteAndWait(0, port0state);
        port0state++;
        err = CatchTransferError();
        if (err) {
            return err;
        }
    }
    BlockWrite(0, 0);
    ReadAndWait(2, 0);
    WriteBuffer();
    return 0;
}

int SpcControlDevice::UploadZeroPageIPL(unsigned char *zeroPageRam)
{
    // IPLを利用して、0ページに書き込む
    unsigned char port0state = 0;
    BlockWrite(1, 0x01, 0x02, 0x00); // 非0なのでP2,P3は書き込み開始アドレス
    WriteAndWait(0, 0xcc); // 起動直後と同じ
    WriteBuffer();
    for (int i=2; i<0xf0; i++) {
        BlockWrite(1, zeroPageRam[i]);
        WriteAndWait(0, port0state);
        port0state++;
        if (i == 0xef) {
            WriteBuffer();
        }
        int err = CatchTransferError();
        if (err) {
            return err;
        }
    }
    return port0state;
}

int SpcControlDevice::UploadRAMDataFast(unsigned char *ram, int addr)
{
    int startAddr = 0x0100; // 転送開始アドレス
	int endAddr = 0xFFBD; // 転送終了アドレス
    unsigned char port0state = 1;
    
    for (int i=startAddr; i<=endAddr; i+=3) {
        if ((i % 256) == 255) {
            std::cout << ".";
        }
        BlockWrite(1, ram[i-addr], ram[i-addr+1], ram[i-addr+2]);
        WriteAndWait(0, port0state);
        port0state ^= 1;
	}
	BlockWrite(0, 0x80);
    ReadAndWait(0, 0xaa);
    WriteBuffer();
    return port0state;
}

int SpcControlDevice::UploadRAMDataIPL(unsigned char *ram, int addr, int size, unsigned char initialP0state)
{
    BlockWrite(1, 0x01, addr & 0xff, (addr >> 8) & 0xff); // 非0なのでP2,P3は書き込み開始アドレス
    unsigned char port0State = initialP0state;
    WriteAndWait(0, port0State&0xff);
    WriteBuffer();
    port0State = 0;
    for (int i=0; i<size; i++) {
        BlockWrite(1, ram[i]);
        WriteAndWait(0, port0State);
        port0State++;
        if ((i % 256) == 255) {
            std::cout << ".";
        }
        if (i == (size-1)) {
            WriteBuffer();
        }
        int err = CatchTransferError();
        if (err) {
            return err;
        }
    }
    return port0State;
}

int SpcControlDevice::uploadDSPRamLoadCode(int addr)
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
    BlockWrite(1, 1, addr & 0xff, (addr >> 8) & 0xff);
    WriteAndWait(0, 0xcc);
    WriteBuffer();
    for (int i=0; i<sizeof(dspram_write_code); i++) {
        BlockWrite(1, dspram_write_code[i]);
        WriteAndWait(0, i&0xff);
        if (i == (sizeof(dspram_write_code)-1)) {
            WriteBuffer();
        }
        int err = CatchTransferError();
        if (err) {
            return err;
        }
    }
    WriteBuffer();
    return sizeof(dspram_write_code);
}

int SpcControlDevice::uploadDSPRamLoadCode2(int addr)
{
    unsigned char dspram_write_code[] =
    {
        // --- DSP LOADER ---
		0xcd, 0x53,			// MOV X, #$53		;
		0xd8, 0xf6,			// MOV $f6, X		;
        
		0xc4, 0xf2,			// MOV $f2, A		; START:
		0x64, 0xf4,			// CMP A, $f4		; LOOP:
		0xd0, 0xfc,			// BNE LOOP:		;
        
		0xfa, 0xf5, 0xf3,	// MOV $f3, $f5		;
		0xc4, 0xf4,			// MOV $f4, A		;
		0xbc,				// INC A			;
		0x10, 0xf2,			// BPL START:		;
		
		// --- RAM LOADER ---
		0xe8, 0x00,			// MOV A, #$0		;
		0x8d, 0x01,			// MOV Y, #$1		;
		0xda, 0x00,			// MOVW dp, YA		; dp = $100
		0x8d, 0x00,			// MOV Y, #$0		;
        
		0x64, 0xf4,			// CMP A, $f4		; LOOP:
		0xd0, 0xfc,			// BNE LOOP:		;
		0xc4, 0xf6,			// MOV $f6, A		;
        
		0x64, 0xf4,			// CMP A, $f4		; LOOP:
		0xf0, 0xfc,			// BEQ LOOP:		;
		0xf8, 0xf4,			// MOV X, $f4		;
		0x30, 0x1a,			// BMI EXIT:		;
        
		0x48, 0x01,			// EOR A, #$1		;
		0x5d,				// MOV X, A			;
		0xe4, 0xf5,			// MOV A, $f5		;
		0xd7, 0x00,			// MOV [dp]+Y, A	;
		0x3a, 0x00,			// INCW dp			;
		0xe4, 0xf6,			// MOV A, $f6		;
		0xd7, 0x00,			// MOV [dp]+Y, A	;
		0x3a, 0x00,			// INCW dp			;
		0xe4, 0xf7,			// MOV A, $f7		;
		0xd7, 0x00,			// MOV [dp]+Y, A	;
		0x3a, 0x00,			// INCW dp			;
		0x7d,				// MOV A, X			;
		0xc4, 0xf4,			// MOV $f4, A		;
		0x2f, 0xde,			// BRA LOOP:		;
        
		0x2f, 0x00			// BRA $??			; jump inside rom
    };
    
    // IPLへジャンプ
    dspram_write_code[sizeof(dspram_write_code)-1] = 0xc7 - sizeof(dspram_write_code);
    
    BlockWrite(3, (addr >> 8) & 0xff);
    BlockWrite(2, addr & 0xff);
    BlockWrite(1, 1);
    WriteAndWait(0, 0xcc);
    for (int i=0; i<sizeof(dspram_write_code); i++) {
        BlockWrite(1, dspram_write_code[i]);
        WriteAndWait(0, i&0xff);
        int err = CatchTransferError();
        if (err) {
            return err;
        }
    }
    WriteBuffer();
    return sizeof(dspram_write_code);
}

int SpcControlDevice::JumpToBootloader(int addr, unsigned char initialP0state,
                                        unsigned char p0, unsigned char p1,
                                        unsigned char p2, unsigned char p3)
{
    BlockWrite(3, (addr >> 8) & 0xff);
    BlockWrite(2, addr & 0xff);
    BlockWrite(1, 0);    // 0なのでP2,P3はジャンプ先アドレス
    unsigned char port0state = initialP0state;
    BlockWrite(0, port0state);
    // ブートローダーがP0に'S'を書き込むのを待つ
    ReadAndWait(0, 'S');
    // P0-P3を復元
    BlockWrite(0, p0);
    BlockWrite(1, p1);
    BlockWrite(2, p2);
    BlockWrite(3, p3);
    WriteBuffer();
    
    int err = CatchTransferError();
    if (err) {
        return err;
    }
    return 0;
}

int SpcControlDevice::JumpToDspCode(int addr, unsigned char initialP0state)
{
    BlockWrite(3, (addr >> 8) & 0xff);
    BlockWrite(2, addr & 0xff);
    BlockWrite(1, 0);    // 0なのでP2,P3はジャンプ先アドレス
    unsigned char port0state = initialP0state;
    BlockWrite(0, port0state);
    // ブートローダーがP3に0x77を書き込むのを待つ
    ReadAndWait(3, 0x77);
    WriteBuffer();
    
    int err = CatchTransferError();
    if (err) {
        return err;
    }
    return 0;
}
