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
    err = spcPlay.load_spc(spc->GetOriginalData(), spc->GetSPCReadSize());
    if (err) {
        printf("load_spc:%s\n", err);
        exit(1);
    }
    spcPlay.clear_echo();
    
    int port0state = 0x01;
    timeval prevTime;
    timeval nowTime;
    gettimeofday(&prevTime, NULL);
    for (;;) {
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
            }
            device->WriteBuffer();
        }

        //1ms待つ
        for (;;) {
            gettimeofday(&nowTime, NULL);
            int elapsedTime = (nowTime.tv_sec - prevTime.tv_sec) * 1e6 + (nowTime.tv_usec - prevTime.tv_usec);
            if (elapsedTime >= 1000) {
                break;
            }
        }
        prevTime = nowTime;
    }
#endif
    
    // 解放処理
    device->Close();
    delete device;
    
	return 0;
}
