#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "SpcControlDevice.h"
#include "SPCFile.h"
#include "SNES_SPC.h"
#include "DspRegFIFO.h"

#ifdef _MSC_VER

#include <thread>
#include <chrono>

typedef long long MSTime;
typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> OSTime;
inline MSTime calcusTime(const OSTime &end, const OSTime &st) {
	return std::chrono::duration_cast<std::chrono::microseconds>(end - st).count();
}
inline void getNowOSTime(OSTime &time) {
	time = std::chrono::system_clock::now();
}
inline void operator += (OSTime &time, MSTime addus) {
	time += std::chrono::microseconds(addus);
}
inline void WaitMicroSeconds(MSTime usec) {
	std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

#else

#include <unistd.h>
#include <sys/time.h>

typedef useconds_t MSTime;
typedef timeval OSTime;
inline MSTime calcusTime(const OSTime &end, const OSTime &st) {
	return ((end.tv_sec - st.tv_sec) * 1e6 + (end.tv_usec - st.tv_usec));
}
inline void getNowOSTime(OSTime &time) {
	gettimeofday(&time, NULL);
}
inline void operator += (OSTime &time, MSTime addus) {
	time.tv_usec += addus;
	if (time.tv_usec >= 1000000) {
		time.tv_usec -= 1000000;
		time.tv_sec++;
	}
}
inline void WaitMicroSeconds(MSTime usec) {
	usleep(usec);
}

#endif

using namespace std;

void sigcatch(int);
int transferSpc(SpcControlDevice *device, unsigned char *dspReg, unsigned char *ram, int bootPtr);
int uploadDSPReg(SpcControlDevice *device, unsigned char *dspReg);

static SpcControlDevice    *device = NULL;
static int port0state = 0x01;

#define SMC_EMU

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
    
    device->SetClock(49152000);
    //device->SetClock(61440000);
    
    signal(SIGTERM, sigcatch);
	signal(SIGINT, sigcatch);
	signal(SIGABRT, sigcatch);

    // 時間計測
	OSTime startTime;
    getNowOSTime(startTime);
    
    // 失敗したら一定回数までリトライする
    int retry = 5;
    int trErr = 0;
    do {
        trErr = transferSpc(device, spc->GetDspReg(), spc->GetRamData(), spc->GetBootPtr());
        if (trErr == 0) break;
		WaitMicroSeconds(100000);
		retry--;
    } while (retry > 0);
    if (trErr) {
        cout << "transfer error." << endl;
        device->Close();
        delete device;
        exit(1);
    }

    cout << "finished." << endl;
    
	OSTime endTime;
    getNowOSTime(endTime);
	MSTime diff = calcusTime(endTime, startTime);
	std::cout << "転送時間: " << diff * 1.0e-6 << "秒" << endl;
    
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
            device->BlockWrite(1, dspData[i], i);
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

	OSTime prevTime;
    getNowOSTime(prevTime);
	OSTime nowTime;
	static const MSTime cycle_us = 1000;
    for (;;) {
        //1ms待つ
        for (;;) {
			getNowOSTime(nowTime);
			MSTime elapsedTime = calcusTime(nowTime, prevTime);
			if (elapsedTime >= cycle_us) {
				break;
			}
			WaitMicroSeconds(100);
        }
		prevTime += cycle_us;

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
                    device->BlockWrite(1, write.data, static_cast<unsigned char>(write.addr));
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

int uploadDSPReg(SpcControlDevice *device, unsigned char *dspReg)
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
    
    int err = device->UploadRAMDataIPL(dspram_write_code, 0x0002, sizeof(dspram_write_code), 0xcc); // $0002 にコードを書き込む
    if (err < 0) {
        return err;
    }
    
    // DSPロードプログラムのアドレスをP2,P3にセットして、P0に16+1を書き込む
    device->BlockWrite(1, 0x00, 0x02, 0x00); // 0なのでP2,P3はジャンプ先アドレス
    device->WriteAndWait(0, err+1); // 16バイト書き込んだので１つ飛ばして17
    device->WriteBuffer();
    unsigned char port0state = 0;
    for (int i=0; i<128; i++) {
        device->BlockWrite(1, dspReg[i]);
        if (i < 127) {
            device->WriteAndWait(0, port0state);
        }
        else {
            device->BlockWrite(0, port0state);
        }
        port0state++;
        if (i == 127) {
            device->WriteBuffer();
        }
        err = device->CatchTransferError();
        if (err) {
            return err;
        }
    }
    // 正常に128バイト書き込まれたなら、プログラムはここで $ffc7 へジャンプされ、
    // P0に $AA が書き込まれる
    device->ReadAndWait(0, 0xaa);
    device->WriteBuffer();
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
    
    err = device->UploadRAMDataIPL(ram, 0x0002, 0xf0-2, 0xcc);
    if (err < 0) {
        return err;
    }
    cout << "zeropage OK." << endl;
    
    // 残り
    err = device->UploadRAMDataIPL(ram+0xffbe, 0xffbe, 0x10000 - 0xffbe, err+1);
    if (err < 0) {
        return err;
    }
#else
    // 0ページとDSPレジスタを転送
    err = uploadDSPReg(device, dspReg);
    if (err < 0) {
        return err;
    }
    
    err = device->UploadRAMDataIPL(ram, 0x0002, 0xf0-2, 0xcc);
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
    
    device->SetClock(24576000);
    
    // ブートローダーへジャンプ
#ifdef SMC_EMU
    err = device->JumpToCode(bootPtr, err+1);
    if (err < 0) {
        return err;
    }
    device->ReadAndWait(3, 0xee);
    device->WriteBuffer();
#else
    err = device->JumpToCode(bootPtr, err+1);
    if (err < 0) {
        return err;
    }
    device->ReadAndWait(0, 'S');
    // P0-P3を復元
    device->BlockWrite(0, ram[0xf4], ram[0xf5], ram[0xf6], ram[0xf7]);
    device->WriteBuffer();
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
