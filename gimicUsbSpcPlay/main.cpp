#include <iostream>
#include <iomanip>
#include "SpcControlDevice.h"

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
    if (device->Init() != 0) {
        exit(1);
    }
    
    // 書き込みと読み込みのテスト
    while (device->PortRead(0) != 0xaa || device->PortRead(1) != 0xbb) {
        usleep(100);
    }
    
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
