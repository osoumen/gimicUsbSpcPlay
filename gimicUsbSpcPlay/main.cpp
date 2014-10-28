#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "SpcControlDevice.h"
#include "SPCFile.h"
#include "SNES_SPC.h"
#include "DspRegFIFO.h"

using namespace std;

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
        exit(1);
    }
    
    // 時間計測
    timeval startTime;
    gettimeofday(&startTime, NULL );
    
    // ハードウェアリセット
    device->HwReset();
    // ソフトウェアリセット
    device->SwReset();
    
    // $BBAA 待ち
    device->WaitReady();
    
    // 0ページとDSPレジスタを転送
    device->UploadDSPRegAndZeroPage(spc->GetDspReg(), spc->GetRamData());
    cout << "dspreg, zeropage OK." << endl;
    
    // 0ページ以降のRAMを転送
    cout << "Writing to RAM";
    unsigned char *spcRam = spc->GetRamData();
    device->UploadRAMData(spcRam+0x100, 0x100, 0x10000 - 0x100);
    
    // ブートローダーへジャンプ
#ifdef SMC_EMU
    device->JumpToDspCode(spc->GetBootPtr());
#else
    device->JumpToBootloader(spc->GetBootPtr(),
                             spcRam[0xf4], spcRam[0xf5],
                             spcRam[0xf6], spcRam[0xf7]);
#endif
    
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
    for (;;) {
        //1ms待つ
        for (;;) {
            gettimeofday(&nowTime, NULL);
            int elapsedTime = (nowTime.tv_sec - prevTime.tv_sec) * 1e6 + (nowTime.tv_usec - prevTime.tv_usec);
            if (elapsedTime >= 1000) {
                break;
            }
            usleep(100);
        }
        prevTime.tv_usec += 1000;
        if (prevTime.tv_usec >= 1000000) {
            prevTime.tv_usec -= 1000000;
            prevTime.tv_sec++;
        }

        err = spcPlay.play(64, NULL);   //1ms動作させる
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
                if (write.addr == 0x5c) {       // KOFとKOFが同タイミングで起こるのを防ぐ
                    device->WriteBuffer();
                }
            }
            device->WriteBuffer();
        }
    }
#endif
    
    // 解放処理
    device->Close();
    delete device;
    
	return 0;
}
