/*
 *  SPCFile.h
 *  C700
 *
 *  Created by osoumen on 12/10/10.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "FileAccess.h"

class SPCFile : public FileAccess {
public:
	SPCFile( const char *path, bool isWriteable );
	virtual ~SPCFile();
	
	unsigned char	*GetRamData() { return m_pRamData; }
    unsigned char	*GetDspReg() { return m_pDspReg; }
    unsigned char	*GetExram() { return m_pExram; }
    unsigned char   *GetOriginalData() { return m_pOriinalData; }
    int             GetSPCReadSize() { return SPC_READ_SIZE; }
    int             GetBootPtr() { return mBootPtr; }
	unsigned char	*GetSampleIndex( int sampleIndex, int *size );
	int				GetLoopSizeIndex( int samleIndex );	//負数でループ無し
	
	virtual bool	Load();
	
    void Fill0EchoRegion();
    void FindAndLocateDspAccCode();
    void FindAndLocateBootCode();

private:
	static const int SPC_READ_SIZE = 0x101c0;
    static const int HDR_PCL = 0x25;
    static const int HDR_PCH = 0x26;
    static const int HDR_A   = 0x27;
    static const int HDR_X   = 0x28;
    static const int HDR_Y   = 0x29;
    static const int HDR_PSW = 0x2a;
    static const int HDR_SP  = 0x2b;
	
    unsigned char	*m_pOriinalData;
	unsigned char	*m_pFileData;
	unsigned char	*m_pRamData;
    unsigned char   *m_pDspReg;
    unsigned char   *m_pExram;
	
	int				mSrcTableAddr;
	int				mSampleStart[128];
	int				mLoopSize[128];
	int				mSampleBytes[128];
	bool			mIsLoop[128];
    
    int             mEchoRegion;
    int             mEchoSize;
    int             mBootPtr;
    
private:
    void moveExRamToRam();
    int findFreeArea(int codeSize);
};
