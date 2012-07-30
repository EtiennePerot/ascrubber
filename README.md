ascrubber
=========

Introduction
------------

Those lossless music files you just downloaded from some defensive, backwards-thinking record label's online store?

Those recordings of sensitive plans that were given or leaked to you from a certain source, on condition of secrecy?

They are likely to be **fingerprinted**.

Fingerprinting is the process of slightly modifying a file in order to encode information into it, without making the file substantially different; in fact, it is meant to be undetectably different from the true original file. It is the process of [steganographically][1] inserting a uniquely-identifying but very-hard-to-detect piece of information into the file.

Then, should that file be published in a context where the source didn't intend that file to be published, the source can then recover this fingerprint out of the file, and know without doubt which entity caused the leak.

[Steganography][1] has its uses and is a legitimate means of private, covert communication. However, using it to track people without telling them is not something the trackee would desire. This program attempts to give them the option to mitigate such tracking.

Purpose
-------

The purpose of this program is to scrub a lossless audio file from potential fingerprints.

Since fingerprints are meant to be as hard to detect as possible, this program uses a somewhat brutal approach to achieve its purpose:

* It randomly offsets a randomized portion of the samples in the audio file.
    * The first and last sets of samples of the file (of configurable size) can be set to be scrubbed harder, because if there is a fingerprint, it is most likely to be in there.
    * The offset applied is also random (and configurable), as well as the probability that a given sample is offset.
* It removes any tag that is not in a (configurable) whitelist of tags in which it is typically hard to put fingerprints into.
    * This means that any non-standard tag is not copied.
    * Only tags that really matter to the user are kept; tags that may be accurate and interesting but are typically not visible to the user are dropped.
    * Album art is *always* removed, for it is trivial to embed a fingerprint in it as well, and a whole other problem to try to scrub it.
* The entire audio file is completely reencoded, to prevent the possibility of keeping information hidden in things like padding blocks or gaps between data structures.
* The seek table is always recomputed and set to strictly regular intervals, to prevent the possibility of encoding information inside slight offset to points within the seek table.

Compiling
---------

There is a package for [ascrubber in the AUR](https://aur.archlinux.org/packages.php?ID=61092).

If you want to compile it manually, you need `libflac` and `libflac++`, which typically come with `flac` itself. You also need `cmake` and the rest of the regular build toolchain (`gcc` and friends).

Once you have that, you can build it:

    # Check out the code
    git clone git://perot.me/ascrubber
    cd ascrubber
    # Generate build files
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
    # Compile it
    make

The resulting binary should be located at `ascrubber/build/ascrubber`.

If you want to install it system-side, you can can use `make install` to move the binary to `/usr/bin`.

Usage
-----

Typical usage:

    ascrubber [options] audio.flac                # Scrub a specific file
    ascrubber [options] file1.flac file2.flac ... # Scrub multiple files
    ascrubber [options] *.flac                    # Scrub all files that end in .flac in the current directory

Use `ascrubber --help` command-line parameter to get a list of all possible arguments, what they do, and their default value.

Q & A
-----

* *What formats are supported?*
    * Currently, only [FLAC][2] is supported. This may change to include other lossless formats such as WAV or even ALAC, but if you are using one of those formats, you should consider converting your audio files to FLAC. You will get some disk space back (converting from WAV) and you will not depend on a proprietary audio format (converting from ALAC).
* *Why only lossless audio?*
    * Because lossy files are much harder to hide fingerprints in, at least when trying to embed them into the samples. This is because the audio samples resulting from decoding the file depend on the decoder in use, and because doing any conversion on it to another lossy format would make such a fingerprint vanish.
    * This being said, the other possible fingerprint vectors (tags, seek table, album art, etc.) still apply. As such, it may be worthwhile to implement support for lossy formats.
* *Doesn't this ruin the quality of the audio file?*
    * If you overdo it, yes, it will. The default settings are quite harmless however, and you would need to scrub a file about 80 times or so before you could hear any difference on most sound equipments.
    * Also keep in mind that you can disable scrubbing in any combination of the 3 areas the file (the first few samples, the last few samples, and everything in between). Disabling scrubbing in the middle of the file will leave the file effectively intact, excluding the first and last deciseconds of the file or so.
* *What inspired the creation of this program?*
      * Other programs that have the same goal. Most of them work on image files or numerical data sets, not audio files. This program is an extension to audio files of the same concept. Here are some examples:
          * [Metadata Anonymisation Toolkit][3] (*can also clean metadata from MP3 and OGG files*), included with [Tails][4]
          * [DICOM Anonymizer][5]
          * [Hachoir][6]
          * [Cornell Anonymization Toolkit][7]

 [1]: https://en.wikipedia.org/wiki/Steganography
 [2]: http://flac.sourceforge.net/
 [3]: https://gitweb.torproject.org/user/jvoisin/mat.git
 [4]: https://tails.boum.org/
 [5]: http://sourceforge.net/projects/dicomanonymizer/
 [6]: https://bitbucket.org/haypo/hachoir/wiki/Home
 [7]: http://sourceforge.net/projects/anony-toolkit/