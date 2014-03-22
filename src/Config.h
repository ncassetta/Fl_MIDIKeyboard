#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/// \file
/// This file defines some macros to compile the RtMidi class according to the operating system. It has been
/// included in RtMidi.h.



#ifdef __linux__
    #ifdef ALSA
        #define __LINUX_ALSA__
        // link with asound, pthread
    #else
        #define __UNIX_JACK__
        // link with jack
    #endif // ALSA

#elif __APPLE__
    #define __MACOSX_CORE__
    // framework CoreMidi, CoreAudio, CoreFoundation

#elif _WIN32
    #define __WINDOWS_MM__
    // link with winmm

#endif // __linux__

#endif // CONFIG_H_INCLUDED
