//
// Copyright Notice: This file is adapted by Nicola Cassetta from the source code of improv,
// a MIDI cpp library by Craig Stuart Sapp.
// See improv at <http://improv.sapp.org>
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Filename:      ...sig/maint/code/control/MidiOutPort/Sequencer_alsa.h
// Web Address:   http://sig.sapp.org/include/sig/Sequencer_alsa.h
// Syntax:        C++
//

#ifndef MIDIDRIVER_ALSA_H_INCLUDED
#define MIDIDRIVER_ALSA_H_INCLUDED

#include <iostream>

#if defined(LINUX) && defined(ALSA)

// use this include for older versions of ALSA before 0.9:
// #include <sys/asoundlib.h>
// use this include for newer versions of ALSA 0.9 and higher:
#include <alsa/asoundlib.h>
#include <vector>


#define MIDI_EXTERNAL  (1)
#define MIDI_INTERNAL  (2)


/// \file
/// This file contains the ALSA implementation of the MKB_MIDIDriver class. It is a MIDI driver capable
/// of basic MIDI output to installed hardware

class ALSA_ENTRY {
   public:
           ALSA_ENTRY(void) { clear(); };
      void clear     (void)
                     { card = device = subdevice = -1;
                       strcpy(name, ""); };
      int  card;
      int  device;
      int  subdevice;
      char name[1024];
};


typedef unsigned char uchar;
typedef unsigned int uint;


/// The class MKB_MIDIDriver is a simple object which sends MIDI messages to the computer MIDI ports.
/// It can detect the MIDI out ports present on the computer and send them some MIDI channel messages.
/// You can select the port, the channel, the volume, the pan and a default velocity for note messages.
/// The class Fl_MIDIKeyboard inherits from it.

class MKB_MIDIDriver {
    public:

        /// The constructor detects for MIDI port present in the computer.
                            MKB_MIDIDriver();

        /// The destructor.
                            ~MKB_MIDIDriver();

        /// Returns the number of MIDI ports (devices) present in the computer.
        static int          GetNumMIDIOutDevs();

        /// Returns the OS name of the port (device) id.
        static const char*  GetMIDIOutDevName(uint id);

        /// Opens the currently set MIDI port, assigning current program, volume and pan.
        bool                OpenMIDIOutPort();

        /// Closes the currently opened MIDI port.
        void                CloseMIDIOutPort();

        /// Returns true if the port is open
        bool                IsOpen();

        /// Sends a MIDI message to the currently opened port.
        /// \param status the MIDI status byte (MIDI channel and message type info)
        /// \param byte1, byte2 other MIDI bytes in the message, according to the message type
        bool                SendMIDIMessage(uchar status, uchar byte1, uchar byte2);

        /// Turns off all the notes.
        void                AllNotesOff();

        /// Sets the active MIDI port.
        void                SetActivePort(uint id);

        ///Returns the active MIDI port.
        int                 GetActivePort()         { return port; }

        /// Sets the MIDI channel (range is 1 ...16).
        void                SetChannel(uchar ch)    { channel = (ch - 1) & 0x0f; }

        /// Returns the currently set MIDI channel.
        UCHAR               GetChannel()            { return channel + 1; }

        /// Sets the MIDI program and sends it to the selected port (if open).
        void                SetProgram(uchar p);

        /// Returns the currently set MIDI program.
        uchar               GetProgram()            { return program; }

        /// Sets the MIDI volume and sends it to the selected port (if open).
        void                SetVolume(uchar v);

        /// Returns the currently set MIDI volume.
        uchar               GetVolume()             { return volume; }

        /// Sets the MIDI pan and sends it to the selected port (if open).
        void                SetPan(uchar p);

        /// Returns the currently set MIDI pan.
        uchar               GetPan()                { return pan; }

        /// Sets the default MIDI velocity for all subsequently sent notes.
        void                SetNoteVel(uchar v)     { note_vel = v; }

        /// Returns the currently set MIDI velocity.
        uchar               GetNoteVel()            { return note_vel; }

        /// Sends to the selected port a MIDI Note on message.
        /// \param note the MIDI note number (the velocity is the default velocity)
        bool                NoteOn(uchar note);

        /// Sends to the selected port a MIDI Note off message.
        bool                NoteOff(uchar note);

    protected:

        static int          num_out_devs;           // number of MIDI output devices
        static vector<ALSA_ENTRY>
                            rawmidi_info;           // device data
        static int          instances;          ///< Existing instances

        HMIDIOUT            out_handle;         ///< Windows handle to the current MIDI port
        uint                port;               ///< Number of the selected port
        uchar               channel;            ///< Number of the default channel (1 - 16)
        uchar               program;            ///< Number of the default MIDI program (0 - 127)
        uchar               volume;             ///< Amount of the MIDI volume (0 - 127)
        uchar               pan;                ///< Amount of the MIDI pan (0 - 127)
        uchar               note_vel;           ///< Default velocity for Note On messages

    private:

      static void   buildInfoDatabase     (void);
      static void   getDeviceInfo         (vector<ALSA_ENTRY>& info);
      static void   searchForMidiDevicesOnCard(int card, vector<ALSA_ENTRY>& info);
      static void   searchForMidiSubdevicesOnDevice(snd_ctl_t* ctl,
                                           int card, int device, vector<ALSA_ENTRY>& info);
      static int    is_output             (snd_ctl_t *ctl, int card, int device, int sub);

      int           getOutDeviceValue     (int aDevice) const;
      int           getOutSubdeviceValue  (int aDevice) const;
      int           getOutCardValue       (int aDevice) const;

      void          removeInfoDatabase    ();
};

#endif /* ALSA */

#endif  /* _SEQUENCER_ALSA_H_INCLUDED */
