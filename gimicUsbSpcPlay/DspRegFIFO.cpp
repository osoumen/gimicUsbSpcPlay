//
//  DspRegFIFO.cpp
//  gimicUsbSpcPlay
//
//  Created by osoumen on 2014/10/26.
//  Copyright (c) 2014年 osoumen. All rights reserved.
//

#include "DspRegFIFO.h"

static DspRegFIFO  *sInstance = NULL;

DspRegFIFO::DspRegFIFO()
{
    sInstance = this;
}

DspRegFIFO::~DspRegFIFO()
{
    
}

DspRegFIFO* DspRegFIFO::GetInstance()
{
    return sInstance;
}

void DspRegFIFO::AddDspWrite(long time, unsigned char addr, unsigned char data)
{
    DspWrite dsp;
    dsp.time = time;
    dsp.addr = addr;
    dsp.data = data;
    mDspWrite.push_back(dsp);
}

DspRegFIFO::DspWrite DspRegFIFO::PopFront()
{
    DspWrite front = mDspWrite.front();
    mDspWrite.pop_front();
    return front;
}

void DspRegFIFO::Clear()
{
    mDspWrite.clear();
}
