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

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "flacscrubber.h"

FLACScrubber::FLACScrubber(std::string file) : FLAC::Decoder::File(), aOriginalFile(file) {
	aError = "";
	aScrubbedFile = file + ".scrubbing";
	error(aEncoder.set_verify(true), "Cannot set verification on.");
	error(aEncoder.set_compression_level(8), "Cannot enable compression.");
	set_md5_checking(true);
	FLAC__StreamDecoderInitStatus init_status = init(aOriginalFile);
	error(init_status == FLAC__STREAM_DECODER_INIT_STATUS_OK, "Cannot initialize decoder: " + std::string(FLAC__StreamDecoderInitStatusString[init_status]));
}

void FLACScrubber::setForceNonZero(bool forceNonZero) {
	aForceNonZero = forceNonZero;
}

void FLACScrubber::scrubFirstSamples(int maxOffset) {
	aFirstSamplesMaxOffset = maxOffset;
}

void FLACScrubber::scrubLastSamples(int maxOffset) {
	aLastSamplesMaxOffset = maxOffset;
}

void FLACScrubber::scrubOtherSamples(int maxOffset) {
	aOtherSamplesMaxOffset = maxOffset;
}

void FLACScrubber::setFirstSamplesSize(int samples) {
	aFirstSamplesSize = samples;
}

void FLACScrubber::setLastSamplesSize(int samples) {
	aLastSamplesSize = samples;
}
void FLACScrubber::setFirstSamplesScrubRate(float rate) {
	aFirstSamplesScrubRate = rate;
}

void FLACScrubber::setLastSamplesScrubRate(float rate) {
	aLastSamplesScrubRate = rate;
}

void FLACScrubber::setOtherSamplesScrubRate(float rate) {
	aOtherSamplesScrubRate = rate;
}

void FLACScrubber::processEverything(bool showProgress) {
	aShowProgress = showProgress;
	if(hasError()) {
		return;
	}
	error(process_until_end_of_stream(), "Could not process stream.");
	if(hasError()) {
		return;
	}
	error(finish(), "Could not finish the decoding process.");
	error(aEncoder.finish(), "Could not finish the encoding process.");
	if(hasError()) {
		return;
	}
	this->showProgress(aTotalSamples);
	std::cerr << std::endl;
	std::cerr.flush();
}

void FLACScrubber::cancel() {
	std::remove(aScrubbedFile.data());
}

void FLACScrubber::overwrite() {
	error(std::remove(aOriginalFile.data()) == 0, "Could not delete original file.");
	if(hasError()) {
		return;
	}
	error(std::rename(aScrubbedFile.data(), aOriginalFile.data()) == 0, "Could not replace the original file by the scrubbed version.");
}

inline FLAC__int32 FLACScrubber::getRandomSampleInner(int maxOffset, float rate) {
	if(!rate) {
		return 0;
	}
	if(rate != 1.f) {
		if((float) random() / (float) RAND_MAX > rate) {
			return 0;
		}
	}
	int sign = rand() % 2 ? -1 : 1;
	if(aForceNonZero) {
		if(maxOffset == 0 || maxOffset == 1) {
			return sign;
		}
		return sign * (1 + (rand() % (maxOffset - 1)));
	}
	if(!maxOffset) {
		return 0;
	}
	return sign * (rand() % maxOffset);
}

FLAC__int32 FLACScrubber::getRandomSample(int sampleNumber) {
	if(sampleNumber <= aFirstSamplesSize) {
		return getRandomSampleInner(aFirstSamplesMaxOffset, aFirstSamplesScrubRate);
	}
	if(sampleNumber >= aTotalSamples - aLastSamplesMaxOffset) {
		return getRandomSampleInner(aLastSamplesMaxOffset, aLastSamplesScrubRate);
	}
	if(aOtherSamplesMaxOffset) {
		return getRandomSampleInner(aOtherSamplesMaxOffset, aOtherSamplesScrubRate);
	}
	if(aForceNonZero) {
		return 1;
	}
	return 0;
}

void FLACScrubber::error(std::string errorMessage) {
	aError = errorMessage;
	std::cerr << std::endl;
	std::cerr << " ****************" << std::endl;
	std::cerr << " * Error message: " << errorMessage << std::endl;
	std::cerr << " * Decoder state: " << FLAC__StreamDecoderStateString[get_state()] << std::endl;
	std::cerr << " * Encoder state: " << FLAC__StreamEncoderStateString[aEncoder.get_state()] << std::endl;
	std::cerr << " ****************" << std::endl;
	std::cerr.flush();
}

void FLACScrubber::error(bool condition, std::string errorMessage) {
	if(!condition) {
		error(errorMessage);
	}
}

bool FLACScrubber::hasError() {
	return !aError.empty();
}

void FLACScrubber::showProgress(FLAC__int64 currentSample) {
	int percentage = (int) (100.d * (double) currentSample / (double) aTotalSamples);
	if(percentage != aLastPercentage) {
		aLastPercentage = percentage;
		int numEqualSigns = (int) ((double) PROGRESS_BAR_LENGTH * (double) currentSample / (double) aTotalSamples);
		std::cerr << "\r[";
		for(int i = 0; i < numEqualSigns; i++) {
			std::cerr << "=";
		}
		for(int i = numEqualSigns; i < PROGRESS_BAR_LENGTH; i++) {
			std::cerr << " ";
		}
		std::cerr << "] " << percentage << "% (" << currentSample << "/" << aTotalSamples << " samples)";
		if(percentage == 100) {
			std::cerr << " Done" << std::endl;
		}
		std::cerr.flush();
	}
}

FLAC__StreamDecoderWriteStatus FLACScrubber::write_callback(const FLAC__Frame * frame, const FLAC__int32 * const buffer[]) {
	int numChannels = frame->header.channels;
	// See http://flac.sourceforge.net/format.html#frame_header
	if(numChannels == 0b1000 || numChannels == 0b1001 || numChannels == 0b1010) {
		numChannels = 2;
	}
	// Do the actual scrubbing
	unsigned int blockSize = frame->header.blocksize;
	FLAC__int64 sampleNumber = frame->header.number.sample_number;
	FLAC__int32 ** newBuffer = new FLAC__int32*[numChannels];
	for(int channel = 0; channel < numChannels; channel++) {
		newBuffer[channel] = new FLAC__int32[blockSize];
		for(int sample = 0; sample < blockSize; sample++) {
			newBuffer[channel][sample] = buffer[channel][sample] + getRandomSample(sampleNumber);
			sampleNumber++;
		}
	}
	showProgress(sampleNumber + blockSize);
	aEncoder.process(newBuffer, blockSize);
	for(int channel = 0; channel < numChannels; channel++) {
		delete newBuffer[channel];
	}
	delete newBuffer;
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


void FLACScrubber::metadata_callback(const FLAC__StreamMetadata * metadata) {
	aTotalSamples = metadata->data.stream_info.total_samples;
	error(aEncoder.set_bits_per_sample(metadata->data.stream_info.bits_per_sample), "Cannot set bits per sample.");
	error(aEncoder.set_channels(metadata->data.stream_info.channels), "Cannot set number of channels.");
	error(aEncoder.set_sample_rate(metadata->data.stream_info.sample_rate), "Cannot set sample rate.");
	error(aEncoder.set_total_samples_estimate(aTotalSamples), "Cannot set total samples estimate.");
	FLAC__StreamEncoderInitStatus init_status = aEncoder.init(aScrubbedFile);
	error(init_status == FLAC__STREAM_ENCODER_INIT_STATUS_OK, "Cannot initialize encoder: " + std::string(FLAC__StreamEncoderInitStatusString[init_status]));
}

void FLACScrubber::error_callback(FLAC__StreamDecoderErrorStatus status) {
	error(FLAC__StreamDecoderErrorStatusString[status]);
}
