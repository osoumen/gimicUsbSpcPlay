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

void sigcatch(int);
int transferSpc(SpcControlDevice *device, unsigned char *dspReg, unsigned char *ram, int bootPtr);

static SpcControlDevice    *device = NULL;
static int port0state = 0x01;

//#define SMC_EMU

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
    
    device = new SpcControlDevice();
    
    if (device->Init() != 0) {
        cout << "device initialize error." << endl;
        exit(1);
    }
    
    signal(SIGTERM, sigcatch);
    
    // 時間計測
    timeval startTime;
    gettimeofday(&startTime, NULL );
    
    // 失敗したら一定回数までリトライする
    int retry = 5;
    int trErr = 0;
    do {
        trErr = transferSpc(device, spc->GetDspReg(), spc->GetRamData(), spc->GetBootPtr());
        if (trErr == 0) break;
        usleep(100000);
        retry--;
    } while (retry > 0);
    if (trErr) {
        cout << "transfer error." << endl;
        device->Close();
        delete device;
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
    // 再生出来ない可能性のあるもの
    // $00f0,$00f1 をいじってるもの
    // ダイレクトページ1を使ってるもの
    // タイミングがシビアなもの
    
    err = spcPlay.load_spc(spc->GetOriginalData(), spc->GetSPCReadSize());
    if (err) {
        printf("load_spc:%s\n", err);
        exit(1);
    }
    
    port0state = 0x01;
    
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
            device->BlockWrite(1, dspData[i], 1);
            device->WriteAndWait(0, port0state);
            port0state = port0state ^ 0x01;
        }
        device->BlockWrite(1, flg, 0x6c);
        device->WriteAndWait(0, port0state);
        port0state = port0state ^ 0x01;
        device->BlockWrite(1, kon, 0x4c);
        device->WriteAndWait(0, port0state);
        port0state = port0state ^ 0x01;
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
                if (write.isRam) {
                    // RAM
                    device->BlockWrite(1, write.data, write.addr & 0xff, (write.addr>>8) & 0xff);
                    device->WriteAndWait(0, port0state | 0x80);
                }
                else {
                    // DSPレジスタ
                    device->BlockWrite(1, write.data, write.addr);
                    device->WriteAndWait(0, port0state);
                }
                port0state = port0state ^ 0x01;
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
    
#ifndef SMC_EMU
#if 0
    // 0ページとDSPレジスタを転送
    err = device->UploadDSPReg2(dspReg);
    if (err < 0) {
        return err;
    }
    cout << "dspreg OK." << endl;
    
    // 0ページ以降のRAMを転送
    cout << "Writing to RAM";
    err = device->UploadRAMDataFast(ram+0x100, 0x100);
    if (err < 0) {
        return err;
    }
    
    err = device->UploadZeroPageIPL(ram);
    if (err < 0) {
        return err;
    }
    cout << "zeropage OK." << endl;
#else
    // 0ページとDSPレジスタを転送
    err = device->UploadDSPReg(dspReg);
    if (err < 0) {
        return err;
    }
    
    err = device->UploadZeroPageIPL(ram);
    if (err < 0) {
        return err;
    }
    cout << "dspreg, zeropage OK." << endl;
    // 0ページ以降のRAMを転送
    cout << "Writing to RAM";
    err = device->UploadRAMDataIPL(ram+0x100, 0x100, 0x10000 - 0x100, err+1);
#endif
#else
    cout << "Writing to RAM";
    err = device->UploadRAMDataIPL(ram+0x100, 0x100, 0x10000 - 0x100, 0xcc);
#endif
    if (err < 0) {
        return err;
    }
    
    // ブートローダーへジャンプ
#ifdef SMC_EMU
    err = device->JumpToDspCode(bootPtr, err+1);
    if (err < 0) {
        return err;
    }
#else
    err = device->JumpToBootloader(bootPtr, err+1,
                                   ram[0xf4], ram[0xf5],
                                   ram[0xf6], ram[0xf7]);
    if (err < 0) {
        return err;
    }
#endif

    return 0;
}

void sigcatch(int sig)
{
    if (device) {
        device->BlockWrite(1, 0, 0x6c);
        device->WriteAndWait(0, port0state);
        port0state = port0state ^ 0x01;
        device->WriteBuffer();
        // kof
        device->BlockWrite(1, 0xff, 0x5c);
        device->WriteAndWait(0, port0state);
        port0state = port0state ^ 0x01;
        device->WriteBuffer();
        
        device->Close();
        delete device;
    }
    exit(1);
}
