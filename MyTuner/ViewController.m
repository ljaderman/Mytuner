//
//  ViewController.m
//  MyTuner
//
//  Created by Air on 2016. 6. 6..
//  Copyright © 2016년 Air. All rights reserved.
//

#import "ViewController.h"
#import "AudioController.h"
#import "AudioProcessor.h"
#import "BufferManager.h"
#import <QuartzCore/QuartzCore.h>

#import <CoreAudio/CoreAudioTypes.h>
#import <AudioUnit/AudioUnit.h>



@interface ViewController ()

@end

static char * NOTES[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };


@implementation ViewController
@synthesize hzLabel, noteLabel;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Do any additional setup after loading the view, typically from a nib.
    
    audioController = [[AudioController alloc] init];
    
    //BufferManager b = new BufferManager();
    [audioController startIOUnit];
    //AudioProcessor *audioProcessor = [[AudioProcessor alloc] init];
    //[audioProcessor start];
    //[audioController getBufferManagerInstance];
    //freq/note tables

    for( int i=0; i<8192; ++i ) {
        freqTable[i] = ( 8000 * i ) / (float) ( 8192 );
    }
    for( int i=0; i<8192; ++i ) {
        noteNameTable[i] = NULL;
        notePitchTable[i] = -1;
    }

    
    for( int i=0; i<127; ++i ) {
        float pitch = ( 440.0 / 32.0 ) * pow( 2, (i-9.0)/12.0 ) ;
        if( pitch > SAMPLE_RATE / 2.0 )
            break;
        //find the closest frequency using brute force.
        float min = 1000000000.0;
        int index = -1;
        for( int j=0; j<8192; ++j ) {
            if( fabsf( freqTable[j]-pitch ) < min ) {
                min = fabsf( freqTable[j]-pitch );
                index = j;
            }
        }
        noteNameTable[index] = NOTES[i%12];
        notePitchTable[index] = pitch;
        //printf( "%f %d %s\n", pitch, index, noteNameTable[index] );
    }

    
    l_fftData = (Float32*) calloc([audioController getBufferManagerInstance]->GetFFTOutputBufferLength(), sizeof(Float32));
    [NSTimer scheduledTimerWithTimeInterval:0.1
                                     target:self
                                   selector:@selector(animateNote)
                                   userInfo:nil
                                    repeats:YES];
    
}

-(void)animateNote {
    
    BufferManager *bufferManager = [audioController getBufferManagerInstance];
    if (bufferManager->HasNewFFTData())
    {
        
        bufferManager->GetFFTOutput(l_fftData);
        Float32 max = l_fftData[0];
        
        //Float32 sArray[4096];
        //memcpy(sArray, l_fftData, 4096*sizeof(Float32));

        
        int max_i = 0;
        for (int i=0; i<4096; i++) {
            if (l_fftData[i] > max) {
                max = l_fftData[i];
                max_i = i;
            }
            
        }
        
        if (max < -80) {
           // return;
        }
        float freq = freqTable[max_i];
        int nearestNoteDelta=0;
        while( true ) {
            if( nearestNoteDelta < max_i && noteNameTable[max_i-nearestNoteDelta] != NULL ) {
                nearestNoteDelta = -nearestNoteDelta;
                break;
            } else if( nearestNoteDelta + max_i < 8192 && noteNameTable[max_i+nearestNoteDelta] != NULL ) {
                break;
            }
            ++nearestNoteDelta;
        }
        char * nearestNoteName = noteNameTable[max_i+nearestNoteDelta];
        float nearestNotePitch = notePitchTable[max_i+nearestNoteDelta];
        float centsSharp = 1200 * log( freq / nearestNotePitch ) / log( 2.0 );

        if( nearestNoteDelta != 0 ) {
            [self.view setBackgroundColor:[UIColor whiteColor]];
            if( centsSharp > 0 )
                printf( "%f cents sharp.\n", centsSharp );
            if( centsSharp < 0 )
                printf( "%f cents flat.\n", -centsSharp );
            if (std::abs(centsSharp) < 10)
                [self.view setBackgroundColor:[UIColor greenColor]];
        } else {
            printf( "in tune!\n" );
            [self.view setBackgroundColor:[UIColor greenColor]];
            
            
        }
        [self.view setBackgroundColor:[UIColor greenColor]];
        printf("this is %f , amp : %f %d %s , %f\n", (float)(max_i) * 8000/8192, max,  max_i, nearestNoteName, centsSharp);
        [hzLabel setText:[NSString stringWithFormat:@"%f", (float)(max_i) * 8000/8192]];
        [noteLabel setText:[NSString stringWithFormat:@"%s", nearestNoteName]];
    }
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
