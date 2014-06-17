#include <iostream>
#include <iomanip>
#include "SpcControlDevice.h"

#define GIMIC_USBVID 0x16c0
#define GIMIC_USBPID 0x05e5

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

int main()
{
    SpcControlDevice    *device = new SpcControlDevice();
    device->Init();
    
    // 書き込みと読み込みのテスト
    usleep(1000);
    
    cout << setw(2) << setfill('0') << hex << uppercase << (int)device->PortRead(0) << endl;
    device->PortWrite(0, 0xcc);
    unsigned char readData;
    while ((readData = device->PortRead(0)) != 0xcc) {
        usleep(100);
    }
    cout << setw(2) << setfill('0') << hex << uppercase << (int)readData << endl;
    
    // 解放処理
    device->Close();
    delete device;
    
	return 0;
}
