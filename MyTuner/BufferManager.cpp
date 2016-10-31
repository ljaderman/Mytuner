/*
 
     File: BufferManager.cpp
 Abstract: This class handles buffering of audio data that is shared between the view and audio controller
  Version: 2.0
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2014 Apple Inc. All Rights Reserved.
 
 
 */

#include "BufferManager.h"
#include "libfft.h"


#define min(x,y) (x < y) ? x : y


BufferManager::BufferManager( UInt32 inMaxFramesPerSlice ) :
mDisplayMode(0 /*aurioTouchDisplayModeOscilloscopeWaveform*/),
mDrawBuffers(),
mDrawBufferIndex(0),
mCurrentDrawBufferLen(kDefaultDrawSamples),
mFFTInputBuffer(NULL),
mFFTInputBufferFrameIndex(0),
mFFTInputBufferLen(inMaxFramesPerSlice),
mHasNewFFTData(0),
mNeedsNewFFTData(0),
mFFTHelper(NULL)
{
    for(UInt32 i=0; i<kNumDrawBuffers; ++i) {
        mDrawBuffers[i] = (Float32*) calloc(inMaxFramesPerSlice, sizeof(Float32));
    }
    
    mFFTInputBuffer = (Float32*) calloc(inMaxFramesPerSlice, sizeof(Float32));
    mFFTHelper = new FFTHelper(inMaxFramesPerSlice);
    OSAtomicIncrement32Barrier(&mNeedsNewFFTData);
    
}


BufferManager::~BufferManager()
{
    for(UInt32 i=0; i<kNumDrawBuffers; ++i) {
        free(mDrawBuffers[i]);
        mDrawBuffers[i] = NULL;
    }
    
    free(mFFTInputBuffer);
    delete mFFTHelper; mFFTHelper = NULL;
}


void BufferManager::CopyAudioDataToDrawBuffer( Float32* inData, UInt32 inNumFrames )
{
    if (inData == NULL) return;
    
    for (UInt32 i=0; i<inNumFrames; i++)
    {
        if ((i+mDrawBufferIndex) >= mCurrentDrawBufferLen)
        {
            CycleDrawBuffers();
            mDrawBufferIndex = -i;
        }
        mDrawBuffers[0][i + mDrawBufferIndex] = inData[i];
    }
    mDrawBufferIndex += inNumFrames;
}


void BufferManager::CycleDrawBuffers()
{
    // Cycle the lines in our draw buffer so that they age and fade. The oldest line is discarded.
	for (int drawBuffer_i=(kNumDrawBuffers - 2); drawBuffer_i>=0; drawBuffer_i--)
		memmove(mDrawBuffers[drawBuffer_i + 1], mDrawBuffers[drawBuffer_i], mCurrentDrawBufferLen);
}


void BufferManager::CopyAudioDataToFFTInputBuffer( Float32* inData, UInt32 numFrames )
{
//    Float32 sInputArray[186] = {0, };
//    memcpy(&sInputArray, inData, numFrames*sizeof(Float32));

    UInt32 framesToCopy = min(numFrames, mFFTInputBufferLen - mFFTInputBufferFrameIndex);
    memcpy(mFFTInputBuffer + mFFTInputBufferFrameIndex, inData, framesToCopy * sizeof(Float32));
    mFFTInputBufferFrameIndex += framesToCopy;// * sizeof(Float32);
    
//    Float32 sArray[4096];
//    memcpy(sArray, mFFTInputBuffer, 4096*sizeof(Float32));

    if (mFFTInputBufferFrameIndex >= mFFTInputBufferLen) {
//        Float32* l_fftData = (Float32*) calloc(4096, sizeof(Float32));
        
        
        //GetFFTOutput(l_fftData);
//        Float32 sArray[4096];
//        memcpy(sArray, l_fftData, 4096*sizeof(Float32));

//        Float32 max = l_fftData[0];
//        int max_i = 0;
//        for (int i=0; i<3000; i++) {
//            if (l_fftData[i] > max) {
//                max = l_fftData[i];
//                max_i = i;
//            }
//            
//        }
//        printf("thsithththis : %d, %f\n", max_i, (Float32)max_i * 8000/8192) ;
        OSAtomicIncrement32(&mHasNewFFTData);
        OSAtomicDecrement32(&mNeedsNewFFTData);
    }
}

float processSecondOrderFilter( float x, float *mem, float *a, float *b )
{
    float ret = b[0] * x + b[1] * mem[0] + b[2] * mem[1]
    - a[0] * mem[2] - a[1] * mem[3] ;
    
    mem[1] = mem[0];
    mem[0] = x;
    mem[3] = mem[2];
    mem[2] = ret;
    
    return ret;
}

void buildHanWindow( float *window, int size )
{
    for( int i=0; i<size; ++i )
        window[i] = .5 * ( 1 - cos( 2 * M_PI * i / (size-1.0) ) );
}

void applyWindow( float *window, float *data, int size )
{
    for( int i=0; i<size; ++i )
        data[i] *= window[i] ;
}

void computeSecondOrderLowPassParameters( float srate, float f, float *a, float *b )
{
    float a0;
    float w0 = 2 * M_PI * f/srate;
    float cosw0 = cos(w0);
    float sinw0 = sin(w0);
    //float alpha = sinw0/2;
    float alpha = sinw0/2 * sqrt(2);
    
    a0   = 1 + alpha;
    a[0] = (-2*cosw0) / a0;
    a[1] = (1 - alpha) / a0;
    b[0] = ((1-cosw0)/2) / a0;
    b[1] = ( 1-cosw0) / a0;
    b[2] = b[0];
}


void BufferManager::GetFFTOutput( Float32* outFFTData )
{
//    for(int i =0; i< 3000; i++)
//        printf("%f\n", mFFTInputBuffer[i]);
    /*
    buildHanWindow( window, 8192 );
    fft = initfft( 13 );
    computeSecondOrderLowPassParameters( 8000, 330, a, b );
    mem1[0] = 0; mem1[1] = 0; mem1[2] = 0; mem1[3] = 0;
    mem2[0] = 0; mem2[1] = 0; mem2[2] = 0; mem2[3] = 0;

    memcpy(data, mFFTInputBuffer, 8192 * sizeof(Float32));
    
    for( int j=0; j<8192; ++j ) {
        data[j] = processSecondOrderFilter( data[j], mem1, a, b );
        data[j] = processSecondOrderFilter( data[j], mem2, a, b );
    }
    // window
    applyWindow( window, data, 8192 );
    
    // do the fft
    for( int j=0; j<8192; ++j )
        datai[j] = 0;
    applyfft( fft, data, datai, false );
    
    //find the peak
    float maxVal = -1;
    int maxIndex = -1;
    for( int j=0; j<8192/2; ++j ) {
        float v = data[j] * data[j] + datai[j] * datai[j] ;

        if( v > maxVal ) {
            maxVal = v;
            maxIndex = j;
        }
    }
    */
    //printf("index : %d , %f\n", maxIndex, (Float32)maxIndex * 8000 / 8192 );
    
    
//    Float32 sArray[4096];
//    memcpy(sArray, mFFTInputBuffer, 4096*sizeof(Float32));

    mFFTHelper->ComputeFFT(mFFTInputBuffer, outFFTData);
    mFFTInputBufferFrameIndex = 0;
    OSAtomicDecrement32Barrier(&mHasNewFFTData);
    OSAtomicIncrement32Barrier(&mNeedsNewFFTData);
}


