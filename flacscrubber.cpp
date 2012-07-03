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
#include <sstream>
#include <algorithm>
#include <string.h>
#include "flacscrubber.h"

FLACScrubber::FLACScrubber(std::string file) : FLAC::Decoder::File(), aOriginalFile(file) {
	aError = "";
	aScrubbedFile = file + ".scrubbing";
	error(aEncoder.set_verify(true), "Cannot set verification on the encoder.");
	error(aEncoder.set_compression_level(8), "Cannot enable compression on the encoder.");
	error(set_md5_checking(true), "Cannot enable MD5 checking on the decoder.");
	error(set_metadata_respond_all(), "Cannot listen to all metadata on the decoder.");
	FLAC__StreamDecoderInitStatus init_status = init(aOriginalFile);
	error(init_status == FLAC__STREAM_DECODER_INIT_STATUS_OK, "Cannot initialize decoder: " + std::string(FLAC__StreamDecoderInitStatusString[init_status]));
	aAllowedTags = new std::vector<std::string>();
	std::string allowedTags(FLACSCRUBBER_DEFAULT_ALLOWEDTAGS);
	std::transform(allowedTags.begin(), allowedTags.end(), allowedTags.begin(), tolower);
	std::stringstream tempStream(allowedTags);
	std::string item;
	while(std::getline(tempStream, item, ',')) {
		aAllowedTags->push_back(item);
	}
}

FLACScrubber::~FLACScrubber()
{
	delete aAllowedTags;
	if(aTags != nullptr) {
		FLAC__metadata_object_delete(aTags);
	}
	if(aSeektable != nullptr) {
		FLAC__metadata_object_delete(aSeektable);
	}
	delete [] aMetadata;
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

void FLACScrubber::setAllowedTags(std::vector<std::string> * allowedTags) {
	aAllowedTags->clear();
	for(std::vector<std::string>::iterator it = allowedTags->begin(); it != allowedTags->end(); it++) {
		std::string lowercaseTag = *it;
		std::transform(lowercaseTag.begin(), lowercaseTag.end(), lowercaseTag.begin(), tolower);
		aAllowedTags->push_back(lowercaseTag);
	}
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
}

void FLACScrubber::cancel() {
	std::remove(aScrubbedFile.c_str());
}

void FLACScrubber::overwrite() {
	error(std::remove(aOriginalFile.c_str()) == 0, "Could not delete original file.");
	if(hasError()) {
		return;
	}
	error(std::rename(aScrubbedFile.c_str(), aOriginalFile.c_str()) == 0, "Could not replace the original file by the scrubbed version.");
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

inline FLAC__int32 FLACScrubber::getRandomSample(int sampleNumber) {
	if(sampleNumber <= aFirstSamplesSize) {
		return getRandomSampleInner(aFirstSamplesMaxOffset, aFirstSamplesScrubRate);
	}
	if(sampleNumber >= aTotalSamples - aLastSamplesSize) {
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

inline FLAC__int32 FLACScrubber::clampSample(FLAC__int32 sampleData) {
	if(sampleData > aMaxSampleValue) {
		return aMaxSampleValue;
	}
	if(sampleData < -aMaxSampleValue) {
		return -aMaxSampleValue;
	}
	return sampleData;
}

void FLACScrubber::error(std::string errorMessage) {
	aError = errorMessage;
	std::cerr << "\n";
	std::cerr << " *****************\n";
	std::cerr << " * Error message: " << aError << "\n";
	std::cerr << " * Decoder state: " << FLAC__StreamDecoderStateString[get_state()] << "\n";
	std::cerr << " * Encoder state: " << FLAC__StreamEncoderStateString[aEncoder.get_state()] << "\n";
	std::cerr << " *****************" << std::endl;
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
		int numEqualSigns = (int) ((double) FLACSCRUBBER_PROGRESS_BAR_LENGTH * (double) currentSample / (double) aTotalSamples);
		std::cerr << "\r[";
		for(int i = 0; i < numEqualSigns; i++) {
			std::cerr << "=";
		}
		for(int i = numEqualSigns; i < FLACSCRUBBER_PROGRESS_BAR_LENGTH; i++) {
			std::cerr << " ";
		}
		std::cerr << "] " << percentage << "% (" << currentSample << "/" << aTotalSamples << " samples)";
		if(percentage == 100) {
			std::cerr << " Done" << std::endl;
		}
		std::cerr.flush();
	}
}

void FLACScrubber::initializeEncoder() {
	if(!aEncoderInitialized) {
		aEncoderInitialized = true;
		// See metadata_callback as to why this uses the C API instead of the C++ one
		aSeektable = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
		FLAC__metadata_object_seektable_template_append_spaced_points_by_samples(aSeektable, aSampleRate * FLACSCRUBBER_SEEKTABLE_SECONDS, aTotalSamples);
		FLAC__metadata_object_seektable_template_sort(aSeektable, true);
		if(aTags == nullptr) {
			aMetadata[0] = aSeektable;
			error(aEncoder.set_metadata(aMetadata, 1), "Cannot set metadata (without tags) on the encoder.");
		} else {
			aMetadata[0] = aTags;
			aMetadata[1] = aSeektable;
			error(aEncoder.set_metadata(aMetadata, 2), "Cannot set metadata (with tags) on the encoder.");
		}
		FLAC__StreamEncoderInitStatus init_status = aEncoder.init(aScrubbedFile);
		error(init_status == FLAC__STREAM_ENCODER_INIT_STATUS_OK, "Cannot initialize encoder: " + std::string(FLAC__StreamEncoderInitStatusString[init_status]));
	}
}

FLAC__StreamDecoderWriteStatus FLACScrubber::write_callback(const FLAC__Frame * frame, const FLAC__int32 * const buffer[]) {
	initializeEncoder();
	int numChannels = frame->header.channels;
	// See http://flac.sourceforge.net/format.html#frame_header
	if(numChannels == 0b1000 || numChannels == 0b1001 || numChannels == 0b1010) {
		numChannels = 2;
	}
	// Do the actual scrubbing
	unsigned int blockSize = frame->header.blocksize;
	FLAC__int64 sampleNumber = frame->header.number.sample_number;
	FLAC__int32 * newBuffer = new FLAC__int32[numChannels * blockSize];
	for(int sample = 0; sample < blockSize; sample++) {
		for(int channel = 0; channel < numChannels; channel++) {
			newBuffer[channel + sample * numChannels] = clampSample(buffer[channel][sample] + getRandomSample(sampleNumber));
		}
		sampleNumber++;
	}
	showProgress(sampleNumber + blockSize);
	aEncoder.process_interleaved(newBuffer, blockSize);
	delete [] newBuffer;
	if(hasError()) {
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


void FLACScrubber::metadata_callback(const FLAC__StreamMetadata * metadata) {
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		aTotalSamples = metadata->data.stream_info.total_samples;
		aSampleRate = metadata->data.stream_info.sample_rate;
		aMaxSampleValue = pow(2, metadata->data.stream_info.bits_per_sample - 1);
		error(aEncoder.set_bits_per_sample(metadata->data.stream_info.bits_per_sample), "Cannot set bits per sample.");
		error(aEncoder.set_channels(metadata->data.stream_info.channels), "Cannot set number of channels.");
		error(aEncoder.set_sample_rate(aSampleRate), "Cannot set sample rate.");
		error(aEncoder.set_total_samples_estimate(aTotalSamples), "Cannot set total samples estimate.");
	} else if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		FLAC::Metadata::VorbisComment comment(metadata);
		if(comment.is_valid()) {
			// The C++ metadata interface is pretty broken when it comes to manually inserting blocks.
			// http://lists.xiph.org/pipermail/flac/2006-May/000563.html
			// http://lists.xiph.org/pipermail/flac-dev/2009-February/002638.html
			// So we use the C interface.instead. Not pretty, but at least it works.
			FLAC__StreamMetadata * cleanBlock = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
			for(int commentIndex = 0; commentIndex < comment.get_num_comments(); commentIndex++) {
				FLAC::Metadata::VorbisComment::Entry entry = comment.get_comment(commentIndex);
				FLAC__StreamMetadata_VorbisComment_Entry cleanEntry;
				// Make a null-terminated copy of the field name and value to ensure they are null-terminated strings, as the C API expects
				char * name = (char *) calloc(entry.get_field_name_length() + 1, sizeof(char));
				memcpy(name, entry.get_field_name(), entry.get_field_name_length());
				std::string lowercaseTag(name);
				std::transform(lowercaseTag.begin(), lowercaseTag.end(), lowercaseTag.begin(), tolower);
				bool keepTag = false;
				for(std::vector<std::string>::iterator it = aAllowedTags->begin(); it != aAllowedTags->end(); it++) {
					if((*it).compare(lowercaseTag) == 0) {
						keepTag = true;
						break;
					}
				}
				if(keepTag) {
					char * value = (char *) calloc(entry.get_field_value_length() + 1, sizeof(char));
					memcpy(value, entry.get_field_value(), entry.get_field_value_length());
					FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&cleanEntry, name, value);
					FLAC__metadata_object_vorbiscomment_append_comment(cleanBlock, cleanEntry, false);
					free(value);
				}
				free(name);
			}
			aTags = cleanBlock;
		}
	}
}

void FLACScrubber::error_callback(FLAC__StreamDecoderErrorStatus status) {
	error(FLAC__StreamDecoderErrorStatusString[status]);
}
