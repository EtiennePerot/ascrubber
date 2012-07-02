/*
    Copyright (c) 2012, Etienne Perot <etienne@perot.me>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef DECODER_H
#define DECODER_H

#include "FLAC++/decoder.h"
#include "FLAC++/encoder.h"

#define PROGRESS_BAR_LENGTH 40

class FLACScrubber : public FLAC::Decoder::File
{
	public:
		FLACScrubber(std::string file);
		void setForceNonZero(bool forceNonZero);
		void scrubFirstSamples(int maxOffset);
		void scrubLastSamples(int maxOffset);
		void scrubOtherSamples(int maxOffset);
		void setFirstSamplesSize(int samples);
		void setLastSamplesSize(int samples);
		void setFirstSamplesScrubRate(float rate);
		void setLastSamplesScrubRate(float rate);
		void setOtherSamplesScrubRate(float rate);
		bool hasError();
		void processEverything(bool showProgress);
		void cancel();
		void overwrite();
	protected:
		virtual FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame * frame, const FLAC__int32 * const buffer[]);
		virtual void metadata_callback(const FLAC__StreamMetadata * metadata);
		virtual void error_callback(FLAC__StreamDecoderErrorStatus status);
	private:
		bool aShowProgress = false;
		int aLastPercentage = -1;
		bool aForceNonZero = false;
		int aFirstSamplesSize = 4096;
		int aLastSamplesSize = 2048;
		float aFirstSamplesScrubRate = 1.f;
		float aLastSamplesScrubRate = 1.f;
		float aOtherSamplesScrubRate = .2f;
		int aFirstSamplesMaxOffset = 256;
		int aLastSamplesMaxOffset = 256;
		int aOtherSamplesMaxOffset = 2;
		FLAC__int64 aTotalSamples;
		std::string aOriginalFile;
		std::string aScrubbedFile;
		std::string aError;
		FLAC::Encoder::File aEncoder;
		void error(std::string errorMessage);
		void error(bool condition, std::string errorMessage);
		FLAC__int32 getRandomSample(int sampleNumber);
		FLAC__int32 getRandomSampleInner(int maxOffset, float rate);
		void showProgress(FLAC__int64 currentSample);
};

#endif // DECODER_H
