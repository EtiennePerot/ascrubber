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
#include <stdio.h>
#include <sstream>
#include <vector>
#include "flacscrubber.h"
#include "optionparser.h"

#define _STR_EXPAND(token) #token
#define _STR(token) _STR_EXPAND(token)

struct Arguments : public option::Arg {
	static option::ArgStatus argumentError(bool msg, const char * msg1, const option::Option & option, const char * msg2)
	{
		if(msg) {
			std::cerr << "Error: " << msg1 << std::string(option.name).substr(0, option.namelen) << msg2 << std::endl;
		}
		return option::ARG_ILLEGAL;
	}
	static option::ArgStatus Integer(const option::Option & option, bool msg) {
		if(!option.arg) {
			return argumentError(msg, "Option ", option, " cannot be empty.");
		}
		std::istringstream stream(option.arg);
		int i;
		stream >> std::noskipws >> i;
		if(!stream.eof() || stream.fail()) {
			return argumentError(msg, "Option ", option, " must be an integer.");
		}
		return option::ARG_OK;
	}
	static option::ArgStatus Rate(const option::Option & option, bool msg) {
		if(!option.arg) {
			return argumentError(msg, "Option ", option, " cannot be empty.");
		}
		std::istringstream stream(option.arg);
		float f;
		stream >> std::noskipws >> f;
		if(!stream.eof() || stream.fail()) {
			return argumentError(msg, "Option ", option, " must be an number.");
		}
		if(f < 0.f || f > 1.f) {
			return argumentError(msg, "Option ", option, " must be between 0 and 1.");
		}
		return option::ARG_OK;
	}
};

int main(int argc, char ** argv) {
	enum optionIndex {
		UNKNOWN,
		HELP,
		FIRST_SIZE,
		LAST_SIZE,
		FIRST_MAX_OFFSET,
		LAST_MAX_OFFSET,
		OTHER_MAX_OFFSET,
		MAX_OFFSET,
		FIRST_RATE,
		LAST_RATE,
		OTHER_RATE,
		RATE,
		FORCE_NONZERO,
		TAGS
	};
	option::Descriptor usage[] = {
		{UNKNOWN,          0, "", "",                 option::Arg::None,  std::string("Usage: " + std::string(argc > 0 ? argv[0] : "ascrubber") + " [options] file1.flac file2.flac ...\n\n"
		                                                                              "This program replaces the files you give it. Make backups as necessary prior to using this program.\n\n"
		                                                                              "Options:").c_str()},
		{HELP,             0, "", "help",             option::Arg::None,  "  --help               \tPrint usage and exit.\n"},
		{FIRST_SIZE,       0, "", "first-size",       Arguments::Integer, "  --first-size N       \tSize of the samples window considered to be the beginning of the file.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_FIRSTSAMPLESIZE) " samples.\n"},
		{LAST_SIZE,        0, "", "last-size",        Arguments::Integer, "  --last-size N        \tSize of the samples window considered to be the end of the file.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_LASTSAMPLESIZE) " samples.\n"},
		{FIRST_MAX_OFFSET, 0, "", "first-max-offset", Arguments::Integer, "  --first-max-offset N \tMaximum offset that can be applied to samples in the beginning sample window, in any direction.\n"
		                                                                  "                       \tIf set to 0, no samples in the beginning window will be scrubbed.\n"
		                                                                  "                       \tSamples range over all 32-bit integers.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_FIRSTSAMPLESMAXOFFSET) ".\n"},
		{LAST_MAX_OFFSET,  0, "", "last-max-offset",  Arguments::Integer, "  --last-max-offset N  \tMaximum offset that can be applied to samples in the end sample window, in any direction.\n"
		                                                                  "                       \tIf set to 0, no samples in the end window will be scrubbed.\n"
		                                                                  "                       \tSamples range over all 32-bit integers.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_LASTSAMPLESMAXOFFSET) ".\n"},
		{OTHER_MAX_OFFSET, 0, "", "other-max-offset", Arguments::Integer, "  --other-max-offset N \tMaximum offset that can be applied to samples in the middle of the file, in any direction.\n"
		                                                                  "                       \tIf set to 0, no samples in the middle of the file will be scrubbed.\n"
		                                                                  "                       \tSamples range over all 32-bit integers.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_OTHERSAMPLESMAXOFFSET) ".\n"},
		{MAX_OFFSET,       0, "", "all-max-offset",   Arguments::Integer, "  --all-max-offset N   \tShortcut to specify the maximum allowed offset to all 3 possible locations of a sample.\n"
		                                                                  "                       \tIf set to 0, nothing will be scrubbed, which is probably not what you want."},
		{FIRST_RATE,       0, "", "first-rate",       Arguments::Rate,    "  --first-rate R       \tProbably to scrubbing a sample in the beginning sample window.\n"
		                                                                  "                       \tFor example, a rate of 0.25 would scrub about a fourth of all the samples in the beginning window.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_FIRSTSAMPLESSCRUBRATE) ".\n"},
		{LAST_RATE,        0, "", "last-rate",        Arguments::Rate,    "  --last-rate R        \tProbably to scrubbing a sample in the end sample window.\n"
		                                                                  "                       \tFor example, a rate of 0.75 would scrub about three fourths of all the samples in the end window.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_LASTSAMPLESSCRUBRATE) ".\n"},
		{OTHER_RATE,       0, "", "other-rate",       Arguments::Rate,    "  --other-rate R       \tProbably to scrubbing a sample in the middle of the file.\n"
		                                                                  "                       \tFor example, a rate of 0 would scrub leave all the samples in middle of the file intact.\n"
		                                                                  "                       \tDefault value: " _STR(FLACSCRUBBER_DEFAULT_OTHERSAMPLESSCRUBRATE) ".\n"},
		{RATE,             0, "", "all-rate",         Arguments::Rate,    "  --all-rate R         \tShortcut to specify the probability to scrub a sample in all 3 possible locations.\n"
		                                                                  "                       \tFor example, a rate of 1 would scrub every single sample in the file.\n"},
		{FORCE_NONZERO,    0, "", "force-nonzero",    option::Arg::None,  (FLACSCRUBBER_DEFAULT_FORCENONZERO ?
		                                                                  "  --force-nonzero      \tIf specified, the random offset applied to any scrubbed sample cannot be 0.\n"
		                                                                  "                       \tThis ensures that the scrubbed samples are different to the original\n"
		                                                                  "                       \tBy default, this feature is on (scrubbing cannot leave a sample untouched).\n"
		                                                                  :
		                                                                  "  --force-nonzero      \tIf specified, the random offset applied to any scrubbed sample cannot be 0.\n"
		                                                                  "                       \tThis ensures that the scrubbed samples are different to the original\n"
		                                                                  "                       \tBy default, this feature is off (scrubbing can leave a sample untouched).\n")},
		{TAGS,             0, "", "tags",             option::Arg::None,  "  --tags t1,t2,t3,...  \tA comma-separated list of Vorbis tags to keep from the original file.\n"
		                                                                  "                       \tSeveral tags, including custom or obscure ones, may be used to fingerprint a file.\n"
		                                                                  "                       \tAs such, this program uses a whitelist approach to keeping tags, "
		                                                                                           "and by default only keeps the tags that are either user-facing or unlikely to contain fingerprints.\n"
		                                                                  "                       \tNote that it is especially dangerous to keep embeded album art images, as those may contain "
		                                                                                           "steganographical fingerprints inside the picture, which are very hard to detect.\n"
		                                                                  "                       \tDefault value: " FLACSCRUBBER_DEFAULT_ALLOWEDTAGS},
		{0,                0, 0,  0,                  0,                  0}
	};
	if(argc > 0) { // Strip argv[0]
		argc--;
		argv++;
	}
	option::Stats stats(usage, argc, argv);
	option::Option options[stats.options_max], buffer[stats.buffer_max];
	option::Parser parse(usage, argc, argv, options, buffer);
	if(parse.error()) {
		return 1;
	}
	if(options[HELP] || !argc) {
		option::printUsage(std::cerr, usage);
		return 0;
	}
	std::vector<std::string> allowedTags;
	if(options[TAGS]) {
		std::stringstream tempStream(options[TAGS].arg);
		std::string item;
		while(std::getline(tempStream, item, ',')) {
			allowedTags.push_back(item);
		}
	}
	for (int i = 0; i < parse.nonOptionsCount(); i++) {
		std::cerr << "Processing file: " << parse.nonOption(i) << std::endl;
		FLACScrubber scrubber(parse.nonOption(i));
		if(scrubber.hasError()) {
			scrubber.cancel();
			continue;
		}
		if(options[FIRST_SIZE]) {
			scrubber.setFirstSamplesSize(atoi(options[FIRST_SIZE].arg));
		}
		if(options[LAST_SIZE]) {
			scrubber.setLastSamplesSize(atoi(options[LAST_SIZE].arg));
		}
		if(options[MAX_OFFSET]) {
			scrubber.scrubFirstSamples(atoi(options[MAX_OFFSET].arg));
			scrubber.scrubLastSamples(atoi(options[MAX_OFFSET].arg));
			scrubber.scrubOtherSamples(atoi(options[MAX_OFFSET].arg));
		}
		if(options[FIRST_MAX_OFFSET]) {
			scrubber.scrubFirstSamples(atoi(options[FIRST_MAX_OFFSET].arg));
		}
		if(options[LAST_MAX_OFFSET]) {
			scrubber.scrubLastSamples(atoi(options[LAST_MAX_OFFSET].arg));
		}
		if(options[OTHER_MAX_OFFSET]) {
			scrubber.scrubOtherSamples(atoi(options[OTHER_MAX_OFFSET].arg));
		}
		if(options[RATE]) {
			scrubber.setFirstSamplesScrubRate(atof(options[RATE].arg));
			scrubber.setLastSamplesScrubRate(atof(options[RATE].arg));
			scrubber.setOtherSamplesScrubRate(atof(options[RATE].arg));
		}
		if(options[FIRST_RATE]) {
			scrubber.setFirstSamplesScrubRate(atof(options[FIRST_RATE].arg));
		}
		if(options[LAST_RATE]) {
			scrubber.setLastSamplesScrubRate(atof(options[LAST_RATE].arg));
		}
		if(options[OTHER_RATE]) {
			scrubber.setOtherSamplesScrubRate(atof(options[OTHER_RATE].arg));
		}
		if(options[FORCE_NONZERO]) {
			scrubber.setForceNonZero(true);
		}
		if(options[TAGS]) {
			scrubber.setAllowedTags(&allowedTags);
		}
		scrubber.processEverything(true);
		if(scrubber.hasError()) {
			scrubber.cancel();
			continue;
		}
		scrubber.overwrite();
	}
}

#undef _RATE
#undef _STR
#undef _STR_EXPAND