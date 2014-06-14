#include <iostream>
#include <iomanip>
#include "BulkUsbDevice.h"

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
    int r;
    
    BulkUsbDevice *device = new BulkUsbDevice();
    device->OpenDevice(GIMIC_USBVID, GIMIC_USBPID, 0x02, 0x85);
    
    // 書き込みと読み込みのテスト
	unsigned char *data = new unsigned char[64]; //data to write
    int transBytes;
    
    // Hard Reset
	data[0] = 0xfd;
    data[1] = 0x81;
    data[2] = 0xff;
    transBytes = 3;
    r = device->WriteBytes(data, &transBytes);
    
    usleep(1000);
    
	data[0] = 0xfd;
    data[1] = 0xb0;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0xff;
    transBytes = 5;
    r = device->WriteBytes(data, &transBytes);
    transBytes = 64;
    r = device->ReadBytes(data, &transBytes, 10);
    PrintHexData(data, transBytes);
    
    
    data[0] = 0x00;
    data[1] = 0xcc;
    data[2] = 0xff;
    transBytes = 3;
    r = device->WriteBytes(data, &transBytes);
    
    usleep(50000);
    
	data[0] = 0xfd;
    data[1] = 0xb0;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0xff;
    transBytes = 5;
    r = device->WriteBytes(data, &transBytes);
    transBytes = 64;
    r = device->ReadBytes(data, &transBytes, 10);
    PrintHexData(data, transBytes);
    
    // 解放処理
    device->CloseDevice();
    delete device;
    
	delete[] data; //delete the allocated memory for data
	return 0;
}
