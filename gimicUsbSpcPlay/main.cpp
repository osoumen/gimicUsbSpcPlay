#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include "SpcControlDevice.h"
#include "SPCFile.h"
#include "SNES_SPC.h"
#include "DspRegFIFO.h"

using namespace std;

int transferSpc(SpcControlDevice *device, unsigned char *dspReg, unsigned char *ram, int bootPtr);

#define RETRY_NUM (5)

#define SMC_EMU
/*
void PrintHexData(const unsigned char *data, int bytes)
{
    for (int i=0; i<bytes; i++) {
        cout    << setw( 2 )       // フィールド幅 2
        << setfill( '0' )  // 0で埋める
        << hex             // 16進数
        << uppercase       // 大文字表示
        << ( int )data[i]
        << " ";
        
        if( i % 16 == 15 )  std::cout << std::endl;
        
    }
}
*/
int main(int argc, char *argv[])
{
    if (argc < 2) {
        cout << "usage:" << endl << "gimicUsbSpcPlay [spcfile]" << endl;
        exit(1);
    }
    SPCFile     *spc = new SPCFile(argv[1], false);
    spc->Load();
    if (!spc->IsLoaded()) {
        cout << "Invalid SPC File!" << endl;
        exit(1);
    }
    cout << "Loaded " << argv[1] << endl;
    
#ifdef SMC_EMU
    // 数秒間エミュレーションを回した後のRAMを使う
    DspRegFIFO dspRegFIFO;
    SNES_SPC spcPlay;
    blargg_err_t err;
    spcPlay.init();
    err = spcPlay.load_spc(spc->GetOriginalData(), spc->GetSPCReadSize());
    if (err) {
        printf("load_spc:%s\n", err);
        exit(1);
    }
    err = spcPlay.play(32000*4, NULL);   //空動作させる
    dspRegFIFO.Clear();
    if (err) {
        printf("%s\n", err);
        exit(1);
    }
    memcpy(spc->GetRamData(), spcPlay.GetRam(), 0x10000);
    // エコー領域をクリアしておく
    spc->SetEchoRegion(spcPlay.GetDspReg(0x6d) << 8);
    spc->SetEchoSize((spcPlay.GetDspReg(0x7d) & 0x0f) * 2048);
    spc->Fill0EchoRegion();
    spc->FindAndLocateDspAccCode();
#else
    spc->Fill0EchoRegion();
    spc->FindAndLocateBootCode();
#endif
    
    SpcControlDevice    *device = new SpcControlDevice();
    
    if (device->Init() != 0) {
        cout << "device initialize error." << endl;
        exit(1);
    }
    
    // 時間計測
    timeval startTime;
    gettimeofday(&startTime, NULL );
    
    // 失敗したら一定回数までリトライする
    int retry = RETRY_NUM;
    int trErr = 0;
    do {
        trErr = transferSpc(device, spc->GetDspReg(), spc->GetRamData(), spc->GetBootPtr());
        if (trErr == 0) break;
        usleep(100000);
        retry--;
    } while (retry > 0);
    if (trErr) {
        cout << "transfer error." << endl;
        exit(1);
    }

    cout << "finished." << endl;
    
    timeval endTime;
    gettimeofday(&endTime, NULL );
    cout << "転送時間: "
    << (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) * 1.0e-6
    << "秒" << endl;
    
    // エミュレーションでSPCを再生
#ifdef SMC_EMU
    // 再生出来ないもの
    // 波形テーブルの内容を演奏中に書き換えるもの
    // 波形自身を演奏中に書き換えるもの
    // 空きRAMスペースがエコー領域しか無いようなもの
    
    err = spcPlay.load_spc(spc->GetOriginalData(), spc->GetSPCReadSize());
    if (err) {
        printf("load_spc:%s\n", err);
        exit(1);
    }
    
    int port0state = 0x01;
    
    //SPCファイルのDSPレジスタを復元
    {
        unsigned char *dspData = spc->GetOriginalDspReg();
        unsigned char kon = dspData[0x4c];
        unsigned char flg = dspData[0x6c];
        
        // サラウンド無効的なもの
        if (dspData[0x0c] >= 0x80) {
            dspData[0x0c] = ~dspData[0x0c] + 1;
        }
        if (dspData[0x1c] >= 0x80) {
            dspData[0x1c] = ~dspData[0x1c] + 1;
        }

        for (int i=0; i<128; i++) {
            if (i == 0x6c || i == 0x4c) {
                continue;
            }
            device->BlockWrite(1, i);
            device->BlockWrite(2, dspData[i]);
            device->BlockWrite(0, port0state);
            device->ReadAndWait(0, port0state);
            port0state = (port0state+1) & 0xff;
            device->WriteBuffer();
        }
        device->BlockWrite(1, 0x6c);
        device->BlockWrite(2, flg);
        device->BlockWrite(0, port0state);
        device->ReadAndWait(0, port0state);
        port0state = (port0state+1) & 0xff;
        device->WriteBuffer();
        device->BlockWrite(1, 0x4c);
        device->BlockWrite(2, kon);
        device->BlockWrite(0, port0state);
        device->ReadAndWait(0, port0state);
        port0state = (port0state+1) & 0xff;
        device->WriteBuffer();
    }

    timeval prevTime;
    timeval nowTime;
    gettimeofday(&prevTime, NULL);
    static const int cycle_us = 1000;
    for (;;) {
        //1ms待つ
        for (;;) {
            gettimeofday(&nowTime, NULL);
            int elapsedTime = (nowTime.tv_sec - prevTime.tv_sec) * 1e6 + (nowTime.tv_usec - prevTime.tv_usec);
            if (elapsedTime >= cycle_us) {
                break;
            }
            usleep(100);
        }
        prevTime.tv_usec += cycle_us;
        if (prevTime.tv_usec >= 1000000) {
            prevTime.tv_usec -= 1000000;
            prevTime.tv_sec++;
        }

        err = spcPlay.play((cycle_us / 125) * 8, NULL);   //1ms動作させる
        if (err) {
            printf("%s\n", err);
            exit(1);
        }
        size_t numWrites = dspRegFIFO.GetNumWrites();
        if (numWrites > 0) {
            for (size_t i=0; i<numWrites; i++) {
                DspRegFIFO::DspWrite write = dspRegFIFO.PopFront();
                device->BlockWrite(1, write.addr);
                device->BlockWrite(2, write.data);
                device->BlockWrite(0, port0state);
                device->ReadAndWait(0, port0state);
                port0state = (port0state+1) & 0xff;
                /*
                if (write.addr == 0x5c) {       // KOFとKOFが同タイミングで起こるのを防ぐ
                    device->WriteBuffer();
                }*/
            }
            device->WriteBuffer();
            int err = device->CatchTransferError();
            if (err) {
                
            }
        }
    }
#endif
    
    // 解放処理
    device->Close();
    delete device;
    
	return 0;
}

int transferSpc(SpcControlDevice *device, unsigned char *dspReg, unsigned char *ram, int bootPtr)
{
    int err = 0;
    
    // ハードウェアリセット
    device->HwReset();
    // ソフトウェアリセット
    device->SwReset();
    
    // $BBAA 待ち
    err = device->WaitReady();
    if (err) {
        return err;
    }
    
    // 0ページとDSPレジスタを転送
    err = device->UploadDSPReg(dspReg);
    if (err) {
        return err;
    }
    
    err = device->UploadZeroPageIPL(ram);
    if (err) {
        return err;
    }
    cout << "dspreg, zeropage OK." << endl;
    
    // 0ページ以降のRAMを転送
    cout << "Writing to RAM";
    err = device->UploadRAMDataIPL(ram+0x100, 0x100, 0x10000 - 0x100);
    if (err) {
        return err;
    }
    
    // ブートローダーへジャンプ
#ifdef SMC_EMU
    err = device->JumpToDspCode(bootPtr);
    if (err) {
        return err;
    }
#else
    err = device->JumpToBootloader(bootPtr,
                                   ram[0xf4], ram[0xf5],
                                   ram[0xf6], ram[0xf7]);
    if (err) {
        return err;
    }
#endif

    return 0;
}
