/*
 *  libjdkmidi-2004 C++ Class Library for MIDI
 *
 *  Copyright (C) 2004  J.D. Koftinoff Software, Ltd.
 *  www.jdkoftinoff.com
 *  jeffk@jdkoftinoff.com
 *
 *  *** RELEASED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) April 27, 2004 ***
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef WIN32
#include "MIDIDriverWin32.h"

MKB_MIDIDriver::MKB_MIDIDriver() :
    out_handle(0), out_open(false), port(MIDI_MAPPER), channel(0), program(0),
    volume(100), pan(64), note_vel(100) {

    MIDIOUTCAPS OutCaps;
    num_out_devs = midiOutGetNumDevs();
    dev_names = new char*[num_out_devs];
    for(UINT i = 0; i < num_out_devs; i++) {
        if (midiOutGetDevCaps( i, &OutCaps, sizeof(OutCaps)) == MMSYSERR_NOERROR)
            dev_names[i] = new char[80];
            strncpy(dev_names[i], OutCaps.szPname, 79);
            dev_names[i][79] = 0;
    }
}


bool MKB_MIDIDriver::OpenMIDIOutPort () {
    if ( !out_open ) {
        int e = midiOutOpen (&out_handle, port, 0, 0, CALLBACK_NULL);

        if ( e!=0 )
            return false;
        out_open=true;
        SetProgram(program);
        SetVolume(volume);
        SetPan(pan);
    }
    return true;
}


void MKB_MIDIDriver::CloseMIDIOutPort() {
    if ( out_open ) {
        midiOutClose ( out_handle );
        out_open=false;
        Reset();
    }
}


MKB_MIDIDriver::~MKB_MIDIDriver() {
    CloseMIDIOutPort();
    delete[] dev_names;
}


void MKB_MIDIDriver::Reset() {
    if (out_open)
        midiOutReset (out_handle);
}


bool MKB_MIDIDriver::SendMIDIMessage ( UCHAR status, UCHAR byte1, UCHAR byte2 ) {
    if ( out_open ) {
         if ( status < 0xff && status != 0xf0 ) {   // dont send sysex or meta-events
            DWORD winmsg;
            winmsg = (((DWORD) status &0xff ) <<0) |
                     (((DWORD) byte1 &0xff ) <<8) |
                     (((DWORD) byte2 &0xff ) <<16 );
            if ( midiOutShortMsg (out_handle, winmsg) !=0 )
                return false;
        }
        return true;
    }
    return false;
}


void MKB_MIDIDriver::AllNotesOff() {
    UCHAR status;
    for (UCHAR i = 0; i < 0x10; i++) {
        status=(unsigned char)(i << 4 | CONTROL_CHANGE);
        SendMIDIMessage (status, C_ALL_NOTES_OFF, 0);
    }
}


void MKB_MIDIDriver::SetActivePort(UINT id) {
    bool was_open = out_open;
    CloseMIDIOutPort();
    port = id;
    if (was_open)
        OpenMIDIOutPort();
}


void MKB_MIDIDriver::SetProgram(UCHAR p) {
    program = p & 0x7f;

    UCHAR status=(unsigned char)(channel << 4 | PROGRAM_CHANGE);
    SendMIDIMessage(status, program, 0);
}


void MKB_MIDIDriver::SetVolume(UCHAR v) {
    volume = v & 0x7f;

    UCHAR status=(unsigned char)(channel << 4 | CONTROL_CHANGE);
    SendMIDIMessage(status, C_MAIN_VOLUME, volume);
}


void MKB_MIDIDriver::SetPan(UCHAR p) {
    pan = p & 0x7f;
    UCHAR status=(unsigned char)(channel << 4 | CONTROL_CHANGE);
    SendMIDIMessage(status, C_PAN, pan);
}


bool MKB_MIDIDriver::NoteOn(UCHAR note) {
    UCHAR status=(unsigned char)(channel << 4 | NOTE_ON);
    return SendMIDIMessage(status, note, note_vel);
}


bool MKB_MIDIDriver::NoteOff(UCHAR note) {
    UCHAR status=(unsigned char)(channel << 4 | NOTE_ON);
    return SendMIDIMessage(status, note, 0);

}





#endif

