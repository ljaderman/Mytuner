//
//  ViewController.h
//  MyTuner
//
//  Created by Air on 2016. 6. 6..
//  Copyright © 2016년 Air. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import "AudioController.h"

@class AudioController;
@interface ViewController : UIViewController {
    
    AudioController*            audioController;
    Float32*					l_fftData;
    float freqTable[8192];
    
    char * noteNameTable[8192];
    float notePitchTable[8192];

}

//IBOutlet UILabel *lbl;
@property (nonatomic, retain) IBOutlet UILabel *hzLabel;
@property (nonatomic, retain) IBOutlet UILabel *noteLabel;

@end

