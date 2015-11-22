/*
 *  SPCFile.cpp
 *  C700
 *
 *  Created by osoumen on 12/10/10.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include "SPCFile.h"
//#include "brrcodec.h"
#include <stdio.h>
#include <string.h>

//-----------------------------------------------------------------------------
unsigned char boot_code[] =
{
    /*
     Apuplayより
     mov   $00,#$00
     mov   $01,#$01
     mov   $fc,#SPC_TIMER2
     mov   $fb,#SPC_TIMER1
     mov   $fa,#SPC_TIMER0
     mov   $f1,#SPC_CONTROL
     mov   x,#$53       ; 'S'をP0に書き込む
     mov   $f4,x
     -  mov   a,SPC_PORT0
     cmp   a,#SPC_PORT0  ;P0セット待ち
     bne   -
     -  mov   a,SPC_PORT1
     cmp   a,#SPC_PORT0  ;P1セット待ち
     bne   -
     -  mov   a,SPC_PORT2
     cmp   a,#SPC_PORT2  ;P3セット待ち
     bne   -
     -  mov   a,SPC_PORT3
     cmp   a,#SPC_PORT3  ;P3セット待ち
     bne   -
     mov   a,$fd
     mov   a,$fe
     mov   a,$ff
     mov   $f2,#$6c
     mov   $f3,#${DSP$6c}
     mov   $f2,#$4c
     mov   $f3,#${DSP$4c}
     mov   $f2,#{$00f2}
     mov   x,#{SP-3}
     mov   sp,x
     mov   a,#{A}
     mov   y,#{Y}
     mov   x,#{X}
     reti
     */
    0x8F, 0x00, 0x00, 0x8F, 0x00, 0x01, 0x8F, 0xFF, 0xFC, 0x8F, 0xFF, 0xFB, 0x8F, 0x4F, 0xFA, 0x8F,
    0x31, 0xF1, 0xCD, 0x53, 0xD8, 0xF4, 0xE4, 0xF4, 0x68, 0x00, 0xD0, 0xFA, 0xE4, 0xF5, 0x68, 0x00,
    0xD0, 0xFA, 0xE4, 0xF6, 0x68, 0x00, 0xD0, 0xFA, 0xE4, 0xF7, 0x68, 0x00, 0xD0, 0xFA, 0xE4, 0xFD,
    0xE4, 0xFE, 0xE4, 0xFF, 0x8F, 0x6C, 0xF2, 0x8F, 0x00, 0xF3, 0x8F, 0x4C, 0xF2, 0x8F, 0x00, 0xF3,
    0x8F, 0x7F, 0xF2, 0xCD, 0xF5, 0xBD, 0xE8, 0xFF, 0x8D, 0x00, 0xCD, 0x00, 0x7F,
};

//-----------------------------------------------------------------------------
unsigned char dspregAccCode[] =
{
    0xE8 ,0x00              //     	mov a,#$00
    ,0xC4 ,0x02             //     	mov $02,a
    ,0x8F ,0x77 ,0xF7       //      mov SPC_PORT3,#$77
                            // loop:
    ,0x64 ,0xF4             //     	cmp a,SPC_PORT0		; 3
    ,0xF0 ,0xFC             //     	beq loop			; 2
    ,0xE4 ,0xF4             //     	mov a,SPC_PORT0		; 3
    ,0x30 ,0x18             //     	bmi toram			; 2
    ,0xF8 ,0xF6             //     	mov x,SPC_PORT2		; 3
    ,0xD8 ,0xF2             //     	mov SPC_REGADDR,x	; 4
    ,0xFA ,0xF5 ,0xF3       //      mov SPC_REGDATA,SPC_PORT1
    ,0xC4 ,0xF4             //     	mov SPC_PORT0,a		; 4
                            // 	; wait 64 - 32 cycle
    ,0xC8 ,0x4C             //     	cmp x,#$4c	; 3
    ,0xF0 ,0x04             //     	beq wait	; 4
    ,0xC8 ,0x5C             //     	cmp x,#$5c	; 3
    ,0xD0 ,0xE7             //     	bne loop	; 4
                            // wait:
    ,0x8D ,0x05             //     	mov y,#5	; 2
                            // -
    ,0xFE ,0xFE             //     	dbnz y,-	; 4/6
    ,0x00                   //   	nop			; 2
    ,0x2F ,0xE0             //     	bra loop	; 4
                            // toram:
    ,0x5D                   //   	mov x,a
    ,0x8D ,0x00             //     	mov y,#0
    ,0xE4 ,0xF5             //     	mov a,SPC_PORT1
    ,0xD7 ,0xF6             //     	mov [SPC_PORT2]+y,a
    ,0x7D                   //   	mov a,x
    ,0xC4 ,0xF4             //     	mov SPC_PORT0,a
    ,0xF8 ,0x02             //     	mov x,$02
    ,0xF0 ,0xD2             //     	beq loop	; $0002に0以外が書き込まれたらIPLに飛ぶ
    ,0x8F ,0xB0 ,0xF1       //      mov $f1,#$b0
    ,0x5F ,0xCF ,0xFF       //      jmp !$ffcf
};

//-----------------------------------------------------------------------------
SPCFile::SPCFile( const char *path, bool isWriteable )
: FileAccess(path, isWriteable)
{
    m_pOriinalData = new unsigned char[SPC_READ_SIZE];
	m_pFileData = new unsigned char[SPC_READ_SIZE];
	m_pRamData = m_pFileData + 0x100;
    m_pDspReg  = m_pFileData + 0x100 + 0x10000;
    m_pExram   = m_pFileData + 0x100 + 0x10000 + 0xc0;
    
    mEchoRegion = 0;
    mEchoSize = 0;
}

//-----------------------------------------------------------------------------
SPCFile::~SPCFile()
{
	delete [] m_pFileData;
    delete [] m_pOriinalData;
}

//-----------------------------------------------------------------------------
bool SPCFile::Load()
{
#if MAC
	CFURLRef	url = CFURLCreateFromFileSystemRepresentation(NULL, (UInt8*)mPath, strlen(mPath), false);
	
	CFReadStreamRef	filestream = CFReadStreamCreateWithFile(NULL, url);
	if (CFReadStreamOpen(filestream) == false) {
		CFRelease( url );
		return false;
	}
	
	CFIndex	readbytes=CFReadStreamRead(filestream, m_pFileData, SPC_READ_SIZE);
	if (readbytes < SPC_READ_SIZE) {
		CFRelease( url );
		CFReadStreamClose(filestream);
		return false;
	}
	CFReadStreamClose(filestream);
	CFRelease( url );
#elif _WIN32
	//WindowsVSTのときのSPCファイル読み込み処理
	HANDLE	hFile;
	
	hFile = CreateFile( mPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE ) {
		DWORD	readSize;
		ReadFile( hFile, m_pFileData, SPC_READ_SIZE, &readSize, NULL );
		CloseHandle( hFile );
	}
#else
    FILE *fp;
    fp = fopen(mPath, "rb");
    if (fp) {
        size_t  readBytes;
        readBytes = fread(m_pFileData, 1, SPC_READ_SIZE, fp);
        fclose(fp);
    }
#endif
	
    //オリジナルのデータをコピー
    memcpy(m_pOriinalData, m_pFileData, SPC_READ_SIZE);
    
	//ファイルチェック
//	if ( strncmp((char*)m_pFileData, "SNES-SPC700 Sound File Data v0.30", 33) != 0 ) {
	if ( strncmp((char*)m_pFileData, "SNES-SPC700 Sound File Data v", 29) != 0 ) {
		return false;
	}
	
    /*
	mSrcTableAddr = (int)m_pRamData[0x1005d] << 8;
	for (int i=0; i<128; i++ ) {
		int	startaddr;
		int	loopaddr;
		startaddr	= (int)m_pRamData[mSrcTableAddr + i*4];
		startaddr	+= (int)m_pRamData[mSrcTableAddr + i*4 + 1] << 8;
		loopaddr	= (int)m_pRamData[mSrcTableAddr + i*4 + 2];
		loopaddr	+= (int)m_pRamData[mSrcTableAddr + i*4 + 3] << 8;
		mSampleStart[i]	= startaddr;
		mLoopSize[i]	= loopaddr-startaddr;
		mIsLoop[i]		= checkbrrsize(&m_pRamData[startaddr], &mSampleBytes[i]) == 1?true:false;
		
		if ( startaddr == 0 || startaddr == 0xffff ||
			mLoopSize[i] < 0 || mSampleBytes[i] < mLoopSize[i] || (mLoopSize[i]%9) != 0 ) {
			mSampleBytes[i] = 0;
		}
	}
	*/
    
    //Extra RAM領域が有効ならram内に移動する
    if ( m_pRamData[0xf1] & 0x80 ) {
        moveExRamToRam();
    }
    
    // エコー設定を読み込む
    mEchoRegion = static_cast<int>(m_pDspReg[0x6d]) << 8;
    mEchoSize = (m_pDspReg[0x7d] & 0x0f) * 2048;
    printf("echo_region: $%04x\n", mEchoRegion);
    printf("echo_size=%04x\n", mEchoSize);
    if (mEchoSize == 0) {
        mEchoSize=4;
    }
    
	mIsLoaded = true;

	return true;
}

//-----------------------------------------------------------------------------
unsigned char *SPCFile::GetSampleIndex( int sampleIndex, int *size )
{
	if ( mIsLoaded ) {
		if ( size ) {
			*size = mSampleBytes[sampleIndex];
		}
		if ( mSampleBytes[sampleIndex] == 0 ) {
			return NULL;
		}
		return &m_pRamData[mSampleStart[sampleIndex]];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
int SPCFile::GetLoopSizeIndex( int samleIndex )
{
	if ( mIsLoaded ) {
		if ( mIsLoop[samleIndex] ) {
			return mLoopSize[samleIndex];
		}
		else {
			return -1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
void SPCFile::moveExRamToRam()
{
    for (int i=0; i<64; i++) {
        m_pRamData[0xffc0+i] = m_pExram[i];
    }
}

//-----------------------------------------------------------------------------
void SPCFile::Fill0EchoRegion()
{
    //エコー領域を0で埋める
    int echo_clear = 1;
    // ECEN=0 なら必ずクリアする
    if ((m_pDspReg[0x6c] & 0x20) == 0) {
        echo_clear = 1;
    }
    if (echo_clear) {
        for (int i=mEchoRegion; (i<0x10000)&&(i<mEchoRegion+mEchoSize); i++) {
            m_pRamData[i] = 0;
        }
    }
}

//-----------------------------------------------------------------------------
int SPCFile::findFreeArea(int codeSize)
{
    int count = 0;
    int addr = 0;
    for (int i=255; i>=0; i--) {
        count = 0;
        for (addr=0xffbf; addr>=0x100; addr--) {
            if ( (addr>(mEchoRegion+mEchoSize)) || (addr<mEchoRegion) ) {
                if (static_cast<int>(m_pRamData[addr]) == i) {
                    count++;
                }
                else {
                    count = 0;
                }
                if (count == codeSize) {
                    break;
                }
            }
            else {
                count=0;
            }
        }
        if (count == codeSize) {
            break;
        }
    }
    return addr;
}

//-----------------------------------------------------------------------------
void SPCFile::FindAndLocateDspAccCode()
{
    int codeSize = sizeof(dspregAccCode);
    
    // RAM終端から探し、エコー領域を除いて同じ値が16byte以上続く場所にブートローダーを仕込む
    int addr = findFreeArea(codeSize);
    if (addr == 0xff) {
        if (mEchoSize < codeSize) {
            printf("Not enough RAM for boot code.\n");
            return;
        }
        else {
            //見つからなければ強制的にスタック領域に入れる
            printf("Not found space area. Use Stack area\n");
            addr = 0x0100;
        }
    }
    for (int i=addr; i<(addr+codeSize); i++) {
        m_pRamData[i] = dspregAccCode[i-addr];
    }
    
    mBootPtr = addr;
    
    m_pDspReg[0x6C] = 0x60;
    m_pDspReg[0x4C] = 0x00;
    
    printf ("bootloader located: $%04x\n", mBootPtr);
}

//-----------------------------------------------------------------------------
void SPCFile::FindAndLocateBootCode()
{
    int codeSize = sizeof(boot_code);
    
    // RAM終端から探し、エコー領域を除いて同じ値が77byte以上続く場所にブートローダーを仕込む
    int addr = findFreeArea(codeSize);
    if (addr == 0xff) {
        if (mEchoSize < codeSize) {
            printf("Not enough RAM for boot code.\n");
            return;
        }
        else {
            //見つからなければエコー領域を使う
            printf("Not found space area. Use echo area\n");
            addr = mEchoRegion;
        }
    }
    for (int i=addr; i<(addr+codeSize); i++) {
        m_pRamData[i] = boot_code[i-addr];
    }
    
    mBootPtr = addr;
    
    // ブートローダーの後に各種レジスタ値を配置
    m_pRamData[addr+0x19] = m_pRamData[0xF4];
    m_pRamData[addr+0x1F] = m_pRamData[0xF5];
    m_pRamData[addr+0x25] = m_pRamData[0xF6];
    m_pRamData[addr+0x2B] = m_pRamData[0xF7];
    m_pRamData[addr+0x01] = m_pRamData[0x00];
    m_pRamData[addr+0x04] = m_pRamData[0x01];
    m_pRamData[addr+0x07] = m_pRamData[0xFC];
    m_pRamData[addr+0x0A] = m_pRamData[0xFB];
    m_pRamData[addr+0x0D] = m_pRamData[0xFA];
    m_pRamData[addr+0x10] = m_pRamData[0xF1];
    m_pRamData[addr+0x38] = m_pDspReg[0x6C];
    m_pDspReg[0x6C] = 0x60;
    m_pRamData[addr+0x3E] = m_pDspReg[0x4C];
    m_pDspReg[0x4C] = 0x00;
    m_pRamData[addr+0x41] = m_pRamData[0xF2];
    m_pRamData[addr+0x44] = static_cast<int>(m_pFileData[HDR_SP]) - 3;
    m_pRamData[0x100 + static_cast<int>(m_pFileData[HDR_SP]) - 0] = m_pFileData[HDR_PCH];    //符号に注意
    m_pRamData[0x100 + static_cast<int>(m_pFileData[HDR_SP]) - 1] = m_pFileData[HDR_PCL];    //符号に注意
    m_pRamData[0x100 + static_cast<int>(m_pFileData[HDR_SP]) - 2] = m_pFileData[HDR_PSW];     //符号に注意
    m_pRamData[addr+0x47] = m_pFileData[HDR_A];
    m_pRamData[addr+0x49] = m_pFileData[HDR_Y];
    m_pRamData[addr+0x4B] = m_pFileData[HDR_X];
    
    printf ("bootloader located: $%04x\n", mBootPtr);
}
