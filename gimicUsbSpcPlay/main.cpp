#include <iostream>
#include <iomanip>
#include <chrono>
#include "SpcControlDevice.h"
#include "SPCFile.h"

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
    spc->FindAndLocateBootCode();
    
    SpcControlDevice    *device = new SpcControlDevice();
    if (device->Init() != 0) {
        exit(1);
    }
    // 時間計測
    const auto startTime = chrono::system_clock::now();
    
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
    
    const auto endTime = std::chrono::system_clock::now();
    const auto timeSpan = endTime - startTime;
    cout << "転送時間: "
    << chrono::duration_cast<chrono::milliseconds>(timeSpan).count() / 1000.0
    << "秒" << endl;
    
    // 解放処理
    device->Close();
    delete device;
    
	return 0;
}
