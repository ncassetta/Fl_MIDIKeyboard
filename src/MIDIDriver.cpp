#include "MIDIDriver.h"




MKB_MIDIDriver::MKB_MIDIDriver() :
    out_open(false), port(0), channel(0), program(0),
    volume(100), pan(64), note_vel(100) {

    midi_out = new RtMidiOut;
}


MKB_MIDIDriver::~MKB_MIDIDriver() {
    CloseMIDIOutPort();
    delete midi_out;
}


void MKB_MIDIDriver::OpenMIDIOutPort () {
    if ( !out_open ) {

        midi_out->openPort(port);
        out_open=true;
        SetProgram(program);
        SetVolume(volume);
        SetPan(pan);
    }
}


void MKB_MIDIDriver::CloseMIDIOutPort() {
    if ( out_open ) {
        midi_out->closePort();
        out_open=false;
    }
}


void MKB_MIDIDriver::SendMIDIMessage ( unsigned char status, unsigned char byte1, unsigned char byte2 ) {
    if ( out_open && status < 0xff && status != 0xf0 ) {   // dont send sysex or meta-events
        message.clear();
        message.push_back(status);
        message.push_back(byte1);
        message.push_back(byte2);
        midi_out->sendMessage(&message);
    }
}


void MKB_MIDIDriver::AllNotesOff() {
    unsigned char status;
    for (unsigned char i = 0; i < 0x10; i++) {
        status=(unsigned char)(i << 4 | CONTROL_CHANGE);
        SendMIDIMessage (status, C_ALL_NOTES_OFF, 0);
    }
}


void MKB_MIDIDriver::SetActivePort(unsigned int id) {
    bool was_open = out_open;
    CloseMIDIOutPort();
    port = id;
    if (was_open)
        OpenMIDIOutPort();
}


void MKB_MIDIDriver::SetProgram(unsigned char p) {
    program = p & 0x7f;

    unsigned char status=(unsigned char)(channel << 4 | PROGRAM_CHANGE);
    SendMIDIMessage(status, program, 0);
}


void MKB_MIDIDriver::SetVolume(unsigned char v) {
    volume = v & 0x7f;

    unsigned char status=(unsigned char)(channel << 4 | CONTROL_CHANGE);
    SendMIDIMessage(status, C_MAIN_VOLUME, volume);
}


void MKB_MIDIDriver::SetPan(unsigned char p) {
    pan = p & 0x7f;
    unsigned char status=(unsigned char)(channel << 4 | CONTROL_CHANGE);
    SendMIDIMessage(status, C_PAN, pan);
}


void MKB_MIDIDriver::NoteOn(unsigned char note) {
    unsigned char status=(unsigned char)(channel << 4 | NOTE_ON);
    SendMIDIMessage(status, note, note_vel);
}


void MKB_MIDIDriver::NoteOff(unsigned char note) {
    unsigned char status=(unsigned char)(channel << 4 | NOTE_OFF);
    SendMIDIMessage(status, note, 0);

}
