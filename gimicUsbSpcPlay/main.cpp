#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "SpcControlDevice.h"
#include "SPCFile.h"
#include "SNES_SPC.h"

using namespace std;

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

static unsigned char SPC_ROM_CODE[64] =
{
    0xCD,0xEF,0xBD,0xE8,0x00,0xC6,0x1D,0xD0,
    0xFC,0x8F,0xAA,0xF4,0x8F,0xBB,0xF5,0x78,
    0xCC,0xF4,0xD0,0xFB,0x2F,0x19,0xEB,0xF4,
    0xD0,0xFC,0x7E,0xF4,0xD0,0x0B,0xE4,0xF5,
    0xCB,0xF4,0xD7,0x00,0xFC,0xD0,0xF3,0xAB,
    0x01,0x10,0xEF,0x7E,0xF4,0x10,0xEB,0xBA,
    0xF6,0xDA,0x00,0xBA,0xF4,0xC4,0xF4,0xDD,
    0x5D,0xD0,0xDB,0x1F,0x00,0x00,0xC0,0xFF
};

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
    
    spc->Fill0EchoRegion();
    //spc->FindAndLocateBootCode();
    spc->FindAndLocateDspAccCode();
    /*
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
    device->JumpToBootloader(spc->GetBootPtr(),
                             spcRam[0xf4], spcRam[0xf5],
                             spcRam[0xf6], spcRam[0xf7]);
    
    cout << "finished." << endl;
    
    timeval endTime;
    gettimeofday(&endTime, NULL );
    cout << "転送時間: "
    << (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) * 1.0e-6
    << "秒" << endl;
    */
    // エミュレーションでSPCを再生
    SNES_SPC spcPlay;
    blargg_err_t err;
    spcPlay.init();
    spcPlay.init_rom(SPC_ROM_CODE);
    err = spcPlay.load_spc(spc->GetOriginalData(), spc->GetSPCReadSize());
    if (err) {
        printf("load_spc:%s\n", err);
        exit(1);
    }
    //spcPlay.clear_echo();
    for (int i=0; i<10000; i++) {
        err = spcPlay.play(32, NULL);
        if (err) {
            printf("%d:%s\n", i, err);
            exit(1);
        }
    }
    
    // 解放処理
    //device->Close();
    //delete device;
    
	return 0;
}
