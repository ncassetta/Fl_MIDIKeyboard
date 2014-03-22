#ifndef MIDIDRIVER_H
#define MIDIDRIVER_H


/// \file
/// This file is the header for the MKB_MIDIDriver class.

#include <string>
#include <vector>


#include "RtMidi-2.0.1/RtMidi.h"


/// The class MKB_MIDIDriver sends MIDI messages to the computer MIDI ports.
/// It can detect the MIDI ports present on the computer and send them some MIDI channel messages.
/// You can select the port, the channel, the volume, the pan and a default velocity for note messages.
/// The class Fl_MIDIKeyboard inherits from it.
/// The cross-platform support is obtained by mean of an RtMidiOut object.
class MKB_MIDIDriver {
    public:

        /// The constructor.
                            MKB_MIDIDriver();

        /// The destructor.
        virtual             ~MKB_MIDIDriver();

        /// Returns the number of MIDI ports present in the computer.
        int                 GetNumMIDIOutDevs()     { return midi_out->getPortCount(); }

        /// Returns the OS name of the port *id*.
        const char*         GetMIDIOutDevName(unsigned int id)
                                                    { return midi_out->getPortName(id).c_str(); }

        /// Opens the currently set MIDI port, assigning current program, volume and pan.
        void                OpenMIDIOutPort ();

        /// Closes the currently opened MIDI port.
        void                CloseMIDIOutPort();

        /// Sends a MIDI message to the currently opened port.
        /// \param status the MIDI status byte (MIDI channel and message type info)
        /// \param byte1, byte2 other MIDI bytes in the message, according to the message type
        void                SendMIDIMessage(unsigned char status, unsigned char byte1, unsigned char byte2);

        /// Turns off all the notes.
        void                AllNotesOff();

        /// Sets the active MIDI port.
        /// \param id an integer in the range 0 ... GetNumMIDIOutDevs() - 1.
        void                SetActivePort(unsigned int id);

        /// Returns the active MIDI port. You can call GetMIDIOutDevName() if you want to know the name of the
        /// port.
        int                 GetActivePort()         { return port; }

        /// Sets the MIDI channel (range is 1 ...16).
        void                SetChannel(unsigned char ch)    { channel = (ch - 1) & 0x0f; }

        /// Returns the currently set MIDI channel.
        unsigned char       GetChannel()            { return channel + 1; }

        /// Sets the MIDI program and sends it to the selected port (if open).
        void                SetProgram(unsigned char p);

        /// Returns the currently set MIDI program.
        unsigned char       GetProgram()            { return program; }

        /// Sets the MIDI volume and sends it to the selected port (if open).
        void                SetVolume(unsigned char v);

        /// Returns the currently set MIDI volume.
        unsigned char       GetVolume()             { return volume; }

        /// Sets the MIDI pan and sends it to the selected port (if open).
        void                SetPan(unsigned char p);

        /// Returns the currently set MIDI pan.
        unsigned char       GetPan()                { return pan; }

        /// Sets the default MIDI velocity for all subsequently sent notes.
        void                SetNoteVel(unsigned char v)     { note_vel = v; }

        /// Returns the currently set MIDI velocity.
        unsigned char       GetNoteVel()            { return note_vel; }

        /// Sends to the selected port a MIDI Note on message.
        /// \param note the MIDI note number (the velocity is the default velocity)
        void                NoteOn(unsigned char note);

        /// Sends to the selected port a MIDI Note off message.
        void                NoteOff(unsigned char note);


/// MIDI Messages status bytes (only channel messages will be output by the driver).
        enum {
            NOTE_OFF	        =0x80,
            NOTE_ON		        =0x90,
            POLY_PRESSURE	    =0xa0,
            CONTROL_CHANGE	    =0xb0,
            PROGRAM_CHANGE	    =0xc0,
            CHANNEL_PRESSURE    =0xd0,
            PITCH_BEND  	    =0xe0,
            SYSEX_START	        =0xf0,
            MTC		            =0xf1,
            SONG_POSITION	    =0xf2,
            SONG_SELECT	        =0xf3,
            TUNE_REQUEST	    =0xf6,
            SYSEX_END	        =0xf7,
            RESET		        =0xff,	// 0xff never used as reset in a MIDIMessage
            META_EVENT	        =0xff	// 0xff is for non MIDI messages
        };

/// MIDI Controller numbers.
        enum {
            C_LSB		    =0x20,	// add this to a non-switch controller to access the LSB.

            C_GM_BANK	    =0x00,	// general midi bank select
            C_MODULATION	=0x01,	// modulation
            C_BREATH	    =0x02,	// breath controller
            C_FOOT		    =0x04,	// foot controller
            C_PORTA_TIME	=0x05,	// portamento time
            C_DATA_ENTRY	=0x06,	// data entry value
            C_MAIN_VOLUME	=0x07,	// main volume control
            C_BALANCE	    =0x08,	// balance control
            C_PAN		    =0x0a,	// panpot stereo control
            C_EXPRESSION	=0x0b,	// expression control
            C_EFF_CONTROL_1 =0x0c,  // effect control 1
            C_EFF_CONTROL_2 =0x0d,  // effect control 2
            C_GENERAL_1	    =0x10,	// general purpose controller 1
            C_GENERAL_2	    =0x11,	// general purpose controller 2
            C_GENERAL_3	    =0x12,	// general purpose controller 3
            C_GENERAL_4	    =0x13,	// general purpose controller 4

            C_DAMPER	    =0x40,	// hold pedal (sustain)
            C_PORTA		    =0x41,	// portamento switch
            C_SOSTENUTO	    =0x42,	// sostenuto switch
            C_SOFT_PEDAL	=0x43,	// soft pedal
            C_HOLD_2	    =0x45,	// hold pedal 2
            C_SND_VARIATION =0x46,  // sound controller 1 (default: Sound Variation)
            C_SND_HARMONIC  =0x47,  // sound controller 2 (default: Timbre/Harmonic Intens.)
            C_SND_RELEASE   =0x48,  // sound controller 3(default: Release Time)
            C_SND_ATTACK    =0x49,  // sound controller 4(default: Attack Time)
            C_SND_BRIGHTNESS=0x4a,  // sound controller 5(default: Brightness)
            C_SND_DECAY     =0x4b,  // sound controller 6(default: Decay Time
            C_SND_VIB_RATE  =0x4c,  // sound controller 7(default: Vibrato Rate
            C_SND_VIB_DEPTH =0x4d,  // sound controller 8(default: Vibrato Depth -
            C_SND_VIB_DELAY =0x4e,  // sound controller 9(default: Vibrato Delay
            C_SND_UNDEFINED =0x4f,  // sound controller 10(default undefined

            C_GENERAL_5	    =0x50,	// general purpose controller 5
            C_GENERAL_6	    =0x51,	// general purpose controller 6
            C_GENERAL_7	    =0x52,	// general purpose controller 7
            C_GENERAL_8	    =0x53,	// general purpose controller 8

            C_HIGH_PREFIX   =0x58,  // high resolution velocity prefix

            C_REVERB_DEPTH	=0x5b,	// reverb depth
            C_TREMELO_DEPTH	=0x5c,	// tremelo depth
            C_CHORUS_DEPTH	=0x5d,	// chorus depth
            C_CELESTE_DEPTH	=0x5e,	// celeste (detune) depth
            C_PHASER_DEPTH	=0x5f,	// phaser effect depth

            C_DATA_INC	    =0x60,	// increment data value
            C_DATA_DEC	    =0x61,	// decrement data value

            C_NONRPN_LSB	=0x62,	// non registered parameter LSB
            C_NONRPN_MSB	=0x63,	// non registered parameter MSB
            C_RPN_LSB	    =0x64,	// registered parameter LSB
            C_RPN_MSB	    =0x65,	// registered parameter MSB

            C_ALL_SOUND_OFF =0x78,  // all sound off
            C_RESET		    =0x79,	// reset all controllers
            C_LOCAL		    =0x7a,	// local control on/off
            C_ALL_NOTES_OFF	=0x7b,	// all notes off
            C_OMNI_OFF	    =0x7c,	// omni off, all notes off
            C_OMNI_ON	    =0x7d,	// omni on, all notes off
            C_MONO		    =0x7e,	// mono on, all notes off
            C_POLY		    =0x7f	// poly on, all notes off
        };


    protected:

        bool                out_open;           ///< True if the port is open

        unsigned int        port;               ///< Number of the selected port
        unsigned char       channel;            ///< Number of the default channel (1 - 16)
        unsigned char       program;            ///< Number of the default MIDI program (0 - 127)
        unsigned char       volume;             ///< Amount of the MIDI volume (0 - 127)
        unsigned char       pan;                ///< Amount of the MIDI pan (0 - 127)
        unsigned char       note_vel;           ///< Default velocity for Note On messages

        RtMidiOut*          midi_out;           ///< The object which sends messages to hardware

    private:

        std::vector<unsigned char>
                            message;
};



#endif
