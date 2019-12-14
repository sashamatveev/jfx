/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#import "AVFAudioProcessor.h"
#import "AVFMediaPlayer.h"

#import <AVFoundation/AVFoundation.h>
#import <MediaToolbox/MediaToolbox.h>

#import <CoreFoundation/CoreFoundation.h>

#import <pthread.h>
#import <objc/message.h>

static void InitAudioTap(MTAudioProcessingTapRef tapRef, void *clientInfo, void **tapStorageOut);
static void FinalizeAudioTap(MTAudioProcessingTapRef tapRef);
static void PrepareAudioTap(MTAudioProcessingTapRef tapRef,
        CMItemCount maxFrames,
        const AudioStreamBasicDescription *processingFormat);
static void UnprepareAudioTap(MTAudioProcessingTapRef tapRef);
static void ProcessAudioTap(MTAudioProcessingTapRef tapRef, CMItemCount numberFrames,
        MTAudioProcessingTapFlags flags,
        AudioBufferList *bufferListInOut,
        CMItemCount *numberFramesOut,
        MTAudioProcessingTapFlags *flagsOut);
static OSStatus AVFTapRenderCallback(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData);

@implementation AVFAudioProcessor

- (id) init {
    if ((self = [super init]) != nil) {
        printf("AMDEBUG init()\n");
        _soundLevelUnit = AVFSoundLevelUnitPtr(new AVFSoundLevelUnit());
        _audioSpectrum = AVFAudioSpectrumUnitPtr(new AVFAudioSpectrumUnit());
        _audioEqualizer = AVFAudioEqualizerPtr(new AVFAudioEqualizer());

        _volume = 1.0f;
        _balance = 0.0f;
        _audioDelay = 0LL;
    }
    return self;
}

-(void) dealloc {
    _soundLevelUnit = nullptr;
    _audioSpectrum = nullptr;
    _audioEqualizer = nullptr;
}

-(void) setAudioTrack : (AVAssetTrack *) track {
    if (track != _audioTrack) {
        // reset the audio mixer if it's already been created
        // this theoretically should never happen...
        _mixer = nil;
    }
    _audioTrack = track;
}

-(AVAudioMix*) mixer {
    if (!self.audioTrack) {
        return nil;
    }

    if (!_mixer) {
        AVMutableAudioMix *mixer = [AVMutableAudioMix audioMix];
        if (mixer) {
            AVMutableAudioMixInputParameters *audioMixInputParameters =
                    [AVMutableAudioMixInputParameters audioMixInputParametersWithTrack : self.audioTrack];
            if (audioMixInputParameters &&
                    [audioMixInputParameters respondsToSelector : @selector(setAudioTapProcessor :)]) {
                MTAudioProcessingTapCallbacks callbacks;

                callbacks.version = kMTAudioProcessingTapCallbacksVersion_0;
                callbacks.clientInfo = (__bridge void *) self;
                callbacks.init = InitAudioTap;
                callbacks.finalize = FinalizeAudioTap;
                callbacks.prepare = PrepareAudioTap;
                callbacks.unprepare = UnprepareAudioTap;
                callbacks.process = ProcessAudioTap;

                MTAudioProcessingTapRef audioProcessingTap;
                if (noErr == MTAudioProcessingTapCreate(kCFAllocatorDefault, &callbacks,
                        kMTAudioProcessingTapCreationFlag_PreEffects,
                        &audioProcessingTap)) {
                    objc_msgSend(audioMixInputParameters,
                            @selector(setAudioTapProcessor :),
                            audioProcessingTap);

                    CFRelease(audioProcessingTap); // owned by the mixer now
                    mixer.inputParameters = @[audioMixInputParameters];

                    _mixer = mixer;
                }
            }
        }
    }
    return _mixer;
}

-(void) setVolume : (float) volume {
    printf("AMDEBUG setVolume() %f\n", volume);
    _volume = volume;
    if (_soundLevelUnit != nullptr) {
        _soundLevelUnit->setVolume(volume);
    }
}

-(void) setBalance : (float) balance {
    printf("AMDEBUG setBalance() %f\n", balance);
    _balance = balance;
    if (_soundLevelUnit != nullptr) {
        _soundLevelUnit->setBalance(balance);
    }
}

@end

AVFTapContext::AVFTapContext(AVFSoundLevelUnitPtr slu, AVFAudioSpectrumUnitPtr spectrum,
                             AVFAudioEqualizerPtr eq) : audioSLU(slu),
                                                        audioSpectrum(spectrum),
                                                        audioEQ(eq),
                                                        mSampleRate(48000), // Some reasonable defaults
                                                        mChannels(2) {
    printf("AMDEBUG AVFTapContext() audioSLU %p\n", audioSLU.get());
    printf("AMDEBUG AVFTapContext() audioSpectrum %p\n", audioSpectrum.get());
    printf("AMDEBUG AVFTapContext() audioEQ %p\n", audioEQ.get());
}

AVFTapContext::~AVFTapContext() {
    printf("AMDEBUG AVFTapContext::~AVFTapContext()\n");
    // AudioUnits have already been deallocated by now
    // shared_ptrs get freed automatically
}

void InitAudioTap(MTAudioProcessingTapRef tapRef, void *clientInfo, void **tapStorageOut) {
    // retain the AU kernels so they don't get freed while we're running
    AVFAudioProcessor *processor = (__bridge AVFAudioProcessor *) clientInfo;
    if (processor) {
        AVFTapContext *context = new AVFTapContext(processor.soundLevelUnit,
                processor.audioSpectrum,
                processor.audioEqualizer);
        *tapStorageOut = context;
        //processor.tapContext = context;
    }
}

void FinalizeAudioTap(MTAudioProcessingTapRef tapRef) {
    AVFTapContext *context = (AVFTapContext*) MTAudioProcessingTapGetStorage(tapRef);
    if (context) {
        delete context;
    }
}

static OSStatus SetupAudioUnit(AudioUnit unit,
                               const AudioStreamBasicDescription *processingFormat,
                               UInt32 maxFrames) {
    OSStatus status = noErr;
    if (noErr == status) {
        status = AudioUnitSetProperty(unit,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input, 0,
                                      processingFormat, sizeof(AudioStreamBasicDescription));
    }
    if (noErr == status) {
        status = AudioUnitSetProperty(unit,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Output, 0,
                                      processingFormat, sizeof(AudioStreamBasicDescription));
    }
    if (noErr == status) {
        status = AudioUnitSetProperty(unit,
                                      kAudioUnitProperty_MaximumFramesPerSlice,
                                      kAudioUnitScope_Global, 0,
                                      &maxFrames, sizeof(UInt32));
    }
    if (noErr == status) {
        status = AudioUnitInitialize(unit);
    }
    return status;
}

void PrepareAudioTap(MTAudioProcessingTapRef tapRef,
        CMItemCount maxFrames,
        const AudioStreamBasicDescription *processingFormat) {
    printf("AMDEBUG PrepareAudioTap()\n");
    AVFTapContext *context = (AVFTapContext*) MTAudioProcessingTapGetStorage(tapRef);

    printf("AMDEBUG PrepareAudioTap() maxFrames %ld\n", maxFrames);
    printf("AMDEBUG PrepareAudioTap() mChannelsPerFrame %d\n", processingFormat->mChannelsPerFrame);
    if (processingFormat->mFormatID == kAudioFormatLinearPCM) {
        printf("AMDEBUG PrepareAudioTap() mFormatID kAudioFormatLinearPCM\n");
    }
    printf("AMDEBUG PrepareAudioTap() mSampleRate %f\n", processingFormat->mSampleRate);

    // Validate the audio format before we enable the processor

    // Failures here should rarely, if ever, happen so leave the NSLogs in for
    // easier diagnosis in the field
    if (processingFormat->mFormatID != kAudioFormatLinearPCM) {
        NSLog(@"AVFAudioProcessor needs linear PCM");
        return;
    }

    // Use the convenient kAudioFormatFlagsNativeFloatPacked to check if we can
    // process the incoming audio
    if ((processingFormat->mFormatFlags & kAudioFormatFlagsNativeFloatPacked)
            != kAudioFormatFlagsNativeFloatPacked) {
        NSLog(@"AVFAudioProcessor needs native endian packed float samples!!");
        return;
    }

    context->mSampleRate = processingFormat->mSampleRate;
    context->mChannels = processingFormat->mChannelsPerFrame;
    context->mMaxFrames = maxFrames;

    // Configure audio equalizer
    printf("AMDEBUG PrepareAudioTap() audioEQ %p\n", context->audioEQ.get());
    if (context->audioEQ != nullptr) {
        context->audioEQ.get()->SetSampleRate(context->mSampleRate);
        context->audioEQ.get()->SetChannels(context->mChannels);
        context->audioEQ.get()->ResetBandParameters();
    }

    // Configure spectrum
    printf("AMDEBUG PrepareAudioTap() audioSpectrum %p\n", context->audioSpectrum.get());
    if (context->audioSpectrum != nullptr) {
        context->audioSpectrum.get()->SetSampleRate(context->mSampleRate);
        context->audioSpectrum.get()->SetChannels(context->mChannels);
        context->audioSpectrum.get()->SetMaxFrames(context->mMaxFrames);
    }

    printf("AMDEBUG PrepareAudioTap() audioSLU %p\n", context->audioSLU.get());
    if (context->audioSLU != nullptr) {
        context->audioSLU.get()->SetChannels(context->mChannels);
    }

    /*
    // Load audio render unit
    AudioComponentDescription ioUnitDescription;
    ioUnitDescription.componentType          = kAudioUnitType_Output;
    ioUnitDescription.componentSubType       = kAudioUnitSubType_GenericOutput;
    ioUnitDescription.componentManufacturer  = kAudioUnitManufacturer_Apple;
    ioUnitDescription.componentFlags         = 0;
    ioUnitDescription.componentFlagsMask     = 0;

    AudioComponent foundIoUnitReference = AudioComponentFindNext(NULL, &ioUnitDescription);
    AudioComponentInstanceNew(foundIoUnitReference, &context->renderUnit);
    printf("AMDEBUG PrepareAudioTap() renderUnit %p\n", context->renderUnit);

    if (context->renderUnit) {
        OSStatus status = SetupAudioUnit(context->renderUnit,
                                         processingFormat,
                                         (UInt32)maxFrames);
        if (noErr != status) {
            NSLog(@"Error setting up Sound Level Unit: %d", status);
            AudioComponentInstanceDispose(context->renderUnit);
            context->renderUnit = NULL;
        }

        AURenderCallbackStruct renderCB;
        renderCB.inputProc = (AURenderCallback)AVFTapRenderCallback;
        renderCB.inputProcRefCon = (void*)tapRef;
        AudioUnitSetProperty(context->renderUnit,
                             kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Input, 0,
                             &renderCB, sizeof(renderCB));
    }
    context->totalFrames = 0;
    */
}

void UnprepareAudioTap(MTAudioProcessingTapRef tapRef) {
    printf("AMDEBUG UnprepareAudioTap()\n");
    AVFTapContext *context = (AVFTapContext*) MTAudioProcessingTapGetStorage(tapRef);

    /*
    if (context->renderUnit) {
        AudioUnitUninitialize(context->renderUnit);
        AudioComponentInstanceDispose(context->renderUnit);
        context->renderUnit = NULL;
    }
    */
}

void ProcessAudioTap(MTAudioProcessingTapRef tapRef,
        CMItemCount numberFrames,
        uint32_t flags,
        AudioBufferList *bufferListInOut,
        CMItemCount *numberFramesOut,
        uint32_t *flagsOut) {
    AVFTapContext *context = (AVFTapContext*) MTAudioProcessingTapGetStorage(tapRef);
    printf("AMDEBUG ProcessAudioTap()\n");
    OSStatus status = MTAudioProcessingTapGetSourceAudio(tapRef, numberFrames, bufferListInOut,
            flagsOut, NULL, numberFramesOut);
//    OSStatus status = MTAudioProcessingTapGetSourceAudio(tapRef, numberFrames, bufferListInOut,
//            NULL, NULL, NULL);
    if (status != noErr) {
        printf("AMDEBUG ProcessAudioTap() error\n");
        NSLog(@"MTAudioProcessingTapGetSourceAudio failed: %d", status);
        return; // Do we need better error handling?
    }
    /*
    if (context->renderUnit) {
        printf("AMDEBUG ProcessAudioTap() flags %d flagsOut %d\n", flags, *flagsOut);
        AudioTimeStamp audioTimeStamp;
        audioTimeStamp.mSampleTime = context->totalFrames;
        audioTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;

        OSStatus status = AudioUnitRender(context->renderUnit,
                                 0,
                                 &audioTimeStamp,
                                 0,
                                 (UInt32)numberFrames,
                                 bufferListInOut);
        if (noErr != status) {
            return;
        }
        context->totalFrames += numberFrames;
        *numberFramesOut = numberFrames;
    } else {
        printf("AMDEBUG ProcessAudioTap() 2\n");
        MTAudioProcessingTapGetSourceAudio(tapRef, numberFrames, bufferListInOut,
                                flagsOut, NULL, numberFramesOut);
    }
     * */

    if (context->audioEQ != nullptr) {
        context->audioEQ.get()->ProcessBufferLists(*bufferListInOut, numberFrames);
    }

    if (context->audioSpectrum != nullptr) {
        context->audioSpectrum.get()->ProcessBufferLists(*bufferListInOut, numberFrames);
    }

    if (context->audioSLU != nullptr) {
        context->audioSLU.get()->ProcessBufferLists(*bufferListInOut, numberFrames);
    }
}
