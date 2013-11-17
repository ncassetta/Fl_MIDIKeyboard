//
// Copyright Notice: This file is adapted by Nicola Cassetta from the source code of improv,
// a MIDI cpp library by Craig Stuart Sapp.
// See improv at <http://improv.sapp.org>
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Filename:      ...sig/maint/code/control/Sequencer_alsa.cpp
// Web Address:   http://sig.sapp.org/src/sig/Sequencer_alsa.cpp
//

#if defined(LINUX) && defined(ALSA)

#include "MIDIDriver_alsa.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>           /* for reading filename for MIDI info */

// use the following include for older then ALSA 0.9:
//#include <sys/asoundlib.h>
// use the following include for ALSA 0.9 and later:
#include <alsa/asoundlib.h>


#include <iostream>
using namespace std;


// static variables for MIDI I/O information database

int MKB_MIDIDriver::num_out_devs     = -1;
vector<ALSA_ENTRY> MKB_MIDIDriver::rawmidi_info;
int MKB_MIDIDriver::instances = 0;


///////////////////////////////
//
// MKB_MIDIDriver::MKB_MIDIDriver --
//

MKB_MIDIDriver::MKB_MIDIDriver() :
    out_handle(NULL), port(0), channel(0), program(0),
    volume(100), pan(64), note_vel(100) {

   if (num_out_devs == -1) {
      buildInfoDatabase();
   }
   instances++;
}



//////////////////////////////
//
// MKB_MIDIDriver::~MKB_MIDIDriver --
//

MKB_MIDIDriver::~MKB_MIDIDriver() {

   if (instances == 1) {
      close();
      removeInfoDatabase();
   }
   instances--;
}



//////////////////////////////
//
// MKB_MIDIDriver::getNumOutputs -- returns the total number of
//     MIDI inputs that can be used.
//

int MKB_MIDIDriver::GetNumMIDIOutDevs() {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return num_out_devs;
}



//////////////////////////////
//
// MKB_MIDIDriver::getOutputName -- returns a string to the name of
//    the specified output device.  The string will remain valid as
//    long as there are any sequencer devices in existence.
//

const char* MKB_MIDIDriver::GetMIDIOutDevName(uint id) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return rawmidi_info[id].name;
}




bool MKB_MIDIDriver::OpenMIDIOutPort() {
   if (initialized == 0) {
       return 0;
   }

   char devname[128] = {0};
   int card = rawmidi_info[port].card;
   int device = rawmidi_info[port].device;
   int subdevice = rawmidi_info[port].subdevice;

   if (subdevice >= 0) {
      sprintf(devname, "hw:%d,%d,%d", card, device, subdevice);
   } else {
      sprintf(devname, "hw:%d,%d", card, device);
   }
   if ( snd_rawmidi_open(NULL, &out_handle, devname, SND_RAWMIDI_SYNC) == 0 ) {
      SetProgram(program);
      SetVolume(volume);
      SetPan(pan);
      return 1;
   }
   else
      return 0;
}


void MKB_MIDIDriver::CloseMIDIOutPort() {
   if (out_handle != NULL) {
      snd_rawmidi_close(out_handle);
      out_handle = NULL;
   }
}



//////////////////////////////
//
// MKB_MIDIDriver::is_open -- returns true if the
//     sequencer device is open, false otherwise.
//

int MKB_MIDIDriver::IsOpen() {
   return out_handle != NULL;
}


bool MKB_MIDIDriver::SendMIDIMessage ( uchar status, uchar byte1, uchar byte2 ) {
    static uchar bytes[3];
    if ( IsOpen() && status < 0xff && status != 0xf0 ) { // dont send sysex or meta-events
        bytes[0] = status;
        bytes[1] = byte1;
        bytes[2] = byte2;
        return (snd_rawmidi_write(out_handle, bytes, 3) == 3);   // all 3 bytes written
    }
    return false;
}


void MKB_MIDIDriver::AllNotesOff() {
    uchar status;
    for (uchar i = 0; i < 0x10; i++) {
        status=(uchar)(i << 4 | CONTROL_CHANGE);
        SendMIDIMessage (status, C_ALL_NOTES_OFF, 0);
    }
}


void MKB_MIDIDriver::SetActivePort(uint id) {
    if (id < num_out_devs) {
        bool was_open = IsOpen();
        CloseMIDIOutPort();
        port = id;
        if (was_open)
            OpenMIDIOutPort();
    }
}


void MKB_MIDIDriver::SetProgram(uchar p) {
    program = p & 0x7f;

    uchar status=(uchar)(channel << 4 | PROGRAM_CHANGE);
    SendMIDIMessage(status, program, 0);
}


void MKB_MIDIDriver::SetVolume(uchar v) {
    volume = v & 0x7f;

    uchar status=(uchar)(channel << 4 | CONTROL_CHANGE);
    SendMIDIMessage(status, C_MAIN_VOLUME, volume);
}


void MKB_MIDIDriver::SetPan(uchar p) {
    pan = p & 0x7f;
    uchar status=(uchar)(channel << 4 | CONTROL_CHANGE);
    SendMIDIMessage(status, C_PAN, pan);
}


bool MKB_MIDIDriver::NoteOn(uchar note) {
    uchar status=(uchar)(channel << 4 | NOTE_ON);
    return SendMIDIMessage(status, note, note_vel);
}


bool MKB_MIDIDriver::NoteOff(uchar note) {
    uchar status=(uchar)(channel << 4 | NOTE_ON);
    return SendMIDIMessage(status, note, 0);

}




///////////////////////////////////////////////////////////////////////////
//
// private functions
//

//////////////////////////////
//
// buildInfoDatabase -- determines the number
//     of MIDI input and output devices and determines their names.
//
void MKB_MIDIDriver::buildInfoDatabase(void) {
   if (initialized) {
      return;
   }
   initialized = 1;

   if ( num_out_devs != 0) {
      cerr << "Error: MKB_MIDIDriver is already running" << endl;
      exit(1);
   }

   // read number of MIDI inputs/output available
   getDeviceInfo(rawmidi_info);

   num_out_devs = rawmidi_info.size();
}


//////////////////////////////
//
// getDeviceInfo -- fills the vector <info> with devices data
//

void MKB_MIDIDriver::getDeviceInfo(vector<ALSA_ENTRY>& info) {
   info.clear();

   int status;
   int card = -1;

   if ((status = snd_card_next(&card)) < 0) {
      cerr << "Cannot read MIDI information" << ": "
           << snd_strerror(status) << endl;
      return;
   }
   if (card < 0) {
      cerr << "No sound cards found" << endl;
      return;
   }
   while (card >= 0 && status >= 0) {
      searchForMidiDevicesOnCard(card, info);
      status = snd_card_next(&card);
   }
}
////////////////////////
//
// searchForMidiDevicesOnCard -- for a particular "card" look at all
//   of the "devices/subdevices" on it and store the information about
//   triplets of numbers which can handle MIDI output.
//

void MKB_MIDIDriver::searchForMidiDevicesOnCard(int card,
      vector<ALSA_ENTRY>& info) {
   snd_ctl_t *ctl;
   char name[64] = {0};
   int device = -1;
   int status;

   sprintf(name, "hw:%d", card);
   if ((status = snd_ctl_open(&ctl, name, 0)) < 0) {
      cerr << "Cannot open control for card " << card << ": "
           << snd_strerror(status) << endl;
      return;
   }
   do {
      status = snd_ctl_rawmidi_next_device(ctl, &device);
      if (status < 0) {
         cerr << "Cannot determine device number: "
              << snd_strerror(status) << endl;
         break;
      }
      if (device >= 0) {
         searchForMidiSubdevicesOnDevice(ctl, card, device, info);
      }
   } while (device >= 0);
   snd_ctl_close(ctl);
}



//////////////////////////////
//
// MKB_MIDIDriver::searchForMidiSubdevicesOnDevice -- for a particular "device"
//   look at all of the "subdevices" on it and store the information about
//   triplets of numbers which can handle MIDI output.
//

void MKB_MIDIDriver::searchForMidiSubdevicesOnDevice(snd_ctl_t* ctl, int card,
      int device, vector<ALSA_ENTRY>& rawmidi_info) {
   snd_rawmidi_info_t *info;
   ALSA_ENTRY entry;
   const char *name;
   const char *sub_name;
   int subs_out;
   int status;

   snd_rawmidi_info_alloca(&info);                      // allocate an snd_rawmidi_info_t struct
   snd_rawmidi_info_set_device(info, device);           // set device info

   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
   snd_ctl_rawmidi_info(ctl, info);
   subs_out = snd_rawmidi_info_get_subdevices_count(info);

   name = snd_rawmidi_info_get_name(info);
   for (sub = 0; sub < subs_out; sub++) {

      if ((status = is_output(ctl, card, device, sub)) < 0) {
         cerr << "Cannot get rawmidi information " << card << ":"
              << device << ": " << snd_strerror(status) << endl;
         return;
      }
      else if (status) {
         snd_rawmidi_info_set_subdevice(info, sub);
         sub_name = snd_rawmidi_info_get_subdevice_name(info);
         if (sub_name[0] == '\0') {
            entry.card = card;
            entry.device = device;
            entry.subdevice = -1;
            strcpy(entry.name, name);
         }
         else {
            entry.card = card;
            entry.device = device;
            entry.subdevice = sub;
            strcpy(entry.name, sub_name);
         }
         rawmidi_info.push_back(entry);
      }
   }
}


//////////////////////////////
//
// MKB_MIDIDriver::is_output -- returns true if specified card/device/sub
//     can write MIDI data.
//

int MKB_MIDIDriver::is_output(snd_ctl_t *ctl, int card, int device, int sub) {
   snd_rawmidi_info_t *info;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);
   snd_rawmidi_info_set_subdevice(info, sub);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);

   if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
      return status;
   } else if (status == 0) {
      return 1;
   }

   return 0;
}


//////////////////////////////
//
// MKB_MIDIDriver::getOutDeviceValue --
//

int MKB_MIDIDriver::getOutDeviceValue(int aDevice) const {
   return rawmidi_info[midiout_index[aDevice]].device;
}


//////////////////////////////
//
// MKB_MIDIDriver::getOutSubdeviceValue --
//

int MKB_MIDIDriver::getOutSubdeviceValue(int aDevice) const {
   return rawmidi_info[midiout_index[aDevice]].subdevice;
}


//////////////////////////////
//
// MKB_MIDIDriver::getOutCardValue --
//

int MKB_MIDIDriver::getOutCardValue(int aDevice) const {
   return rawmidi_info[midiout_index[aDevice]].card;
}


//////////////////////////////
//
// MKB_MIDIDriver::removeInfoDatabase --
//

void MKB_MIDIDriver::removeInfoDatabase(void) {
   if (IsOpen()) {
      CloseMIDIOutPort();
   }

   rawmidi_info.clear();

   num_out_devs = 0;
   initialized = 0;
}





/* original private functions
///////////////////////////////////////////////////////////////////////////
//
// private functions
//

//////////////////////////////
//
// MKB_MIDIDriver::buildInfoDatabase -- determines the number
//     of MIDI input and output devices and determines their names.
//

void MKB_MIDIDriver::buildInfoDatabase(void) {
   if (initialized) {
      return;
   }
   initialized = 1;

   if (indevcount != 0 || outdevcount != 0) {
      cout << "Error: MKB_MIDIDriver is already running" << endl;
      cout << "Indevcout = " << indevcount << " and "
           << " outdevcount = " << outdevcount << endl;
      exit(1);
   }

   indevcount  = 0;
   outdevcount = 0;

   // read number of MIDI inputs/output available
   getDeviceInfo(rawmidi_info);

   // store data into separate input/output arrays:
   midiin_index.setSize(rawmidi_info.getSize());
   midiin_index.setSize(0);
   midiin_index.allowGrowth(1);
   midiout_index.setSize(rawmidi_info.getSize());
   midiout_index.setSize(0);
   midiout_index.allowGrowth(1);

   int i;
   for (i=0; i<rawmidi_info.getSize(); i++) {
      if (rawmidi_info[i].output) {
         midiout_index.append(i);
      }
      if (rawmidi_info[i].input) {
         midiin_index.append(i);
      }
   }
   midiin_index.allowGrowth(0);
   midiout_index.allowGrowth(0);
   indevcount  = midiin_index.getSize();
   outdevcount = midiout_index.getSize();

   rawmidi_in.setSize(indevcount);
   for (i=0; i<rawmidi_in.getSize(); i++) {
      rawmidi_in[i] = NULL;
   }
   rawmidi_out.setSize(outdevcount);
   for (i=0; i<rawmidi_out.getSize(); i++) {
      rawmidi_out[i] = NULL;
   }
}



//////////////////////////////
//
// MKB_MIDIDriver::getInDeviceValue --
//

int MKB_MIDIDriver::getInDeviceValue(int aDevice) const {
   return rawmidi_info[midiin_index[aDevice]].device;
}



//////////////////////////////
//
// MKB_MIDIDriver::getInSubdeviceValue --
//

int MKB_MIDIDriver::getInSubdeviceValue(int aDevice) const {
   return rawmidi_info[midiin_index[aDevice]].subdevice;
}



//////////////////////////////
//
// MKB_MIDIDriver::getInCardValue --
//

int MKB_MIDIDriver::getInCardValue(int aDevice) const {
   return midiin_index[aDevice];
}



//////////////////////////////
//
// MKB_MIDIDriver::getOutDeviceValue --
//

int MKB_MIDIDriver::getOutDeviceValue(int aDevice) const {
   return rawmidi_info[midiout_index[aDevice]].device;
}



//////////////////////////////
//
// MKB_MIDIDriver::getOutSubdeviceValue --
//

int MKB_MIDIDriver::getOutSubdeviceValue(int aDevice) const {
   return rawmidi_info[midiout_index[aDevice]].subdevice;
}



//////////////////////////////
//
// MKB_MIDIDriver::getOutCardValue --
//

int MKB_MIDIDriver::getOutCardValue(int aDevice) const {
   return rawmidi_info[midiout_index[aDevice]].card;
}



//////////////////////////////
//
// MKB_MIDIDriver::removeInfoDatabase --
//

void MKB_MIDIDriver::removeInfoDatabase(void) {
   if (rawmidi_in.getSize() != 0) {
      close();
   }

   if (rawmidi_out.getSize() != 0) {
      close();
   }

   rawmidi_in.setSize(0);
   rawmidi_out.setSize(0);
   rawmidi_info.setSize(0);
   midiin_index.setSize(0);
   midiout_index.setSize(0);

   indevcount = 0;
   outdevcount = 0;
   initialized = 0;
}



//////////////////////////////
//
// getDeviceInfo --
//

void MKB_MIDIDriver::getDeviceInfo(SigCollection<ALSA_ENTRY>& info) {
   info.setSize(0);
   info.allowGrowth(1);

   int status;
   int card = -1;

   if ((status = snd_card_next(&card)) < 0) {
      cerr << "Cannot read MIDI information" << endl;
      cerr << "Reason: " << snd_strerror(status) << endl;
      return;
   }
   if (card < 0) {
      cerr << "No sound cards found" << endl;
      return;
   }
   while (card >= 0) {
      searchForMidiDevicesOnCard(card, info);
      status = snd_card_next(&card);
      if (status < 0) {
         break;
      }
   }

   info.allowGrowth(0);
}



////////////////////////
//
// searchForMidiDevicesOnCard -- for a particular "card" look at all
//   of the "devices/subdevices" on it and store the information about
//   triplets of numbers which can handle MIDI input and/or output.
//

void MKB_MIDIDriver::searchForMidiDevicesOnCard(int card,
      SigCollection<ALSA_ENTRY>& info) {
   snd_ctl_t *ctl;
   char name[64] = {0};
   int device = -1;
   int status;

   sprintf(name, "hw:%d", card);
   if ((status = snd_ctl_open(&ctl, name, 0)) < 0) {
      cerr << "Cannot open control for card " << card << ": "
           << snd_strerror(status) << endl;
      return;
   }
   do {
      status = snd_ctl_rawmidi_next_device(ctl, &device);
      if (status < 0) {
         cerr << "Cannot determine device number: "
              << snd_strerror(status) << endl;
         break;
      }
      if (device >= 0) {
         searchForMidiSubdevicesOnDevice(ctl, card, device, info);
      }
   } while (device >= 0);
   snd_ctl_close(ctl);
}



//////////////////////////////
//
// MKB_MIDIDriver::searchForMidiSubdevicesOnDevice --
//

void MKB_MIDIDriver::searchForMidiSubdevicesOnDevice(snd_ctl_t* ctl, int card,
      int device, SigCollection<ALSA_ENTRY>& rawmidi_info) {
   snd_rawmidi_info_t *info;
   const char *name;
   const char *sub_name;
   int subs, subs_in, subs_out;
   int sub, in, out;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);

   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
   snd_ctl_rawmidi_info(ctl, info);
   subs_in = snd_rawmidi_info_get_subdevices_count(info);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
   snd_ctl_rawmidi_info(ctl, info);
   subs_out = snd_rawmidi_info_get_subdevices_count(info);
   subs = subs_in > subs_out ? subs_in : subs_out;

   sub = 0;
   in = out = 0;
   if ((status = is_output(ctl, card, device, sub)) < 0) {
      cerr << "Cannot get rawmidi information " << card << ":"
           << device << ": " << snd_strerror(status) << endl;
      return;
   } else if (status)
      out = 1;

   if (status == 0) {
      if ((status = is_input(ctl, card, device, sub)) < 0) {
         cerr << "Cannot get rawmidi information " << card << ":"
              << device <<": " << snd_strerror(status) << endl;
         return;
      }
   } else if (status)
      in = 1;

   if (status == 0)
      return;

   int index;
   name = snd_rawmidi_info_get_name(info);
   sub_name = snd_rawmidi_info_get_subdevice_name(info);
   if (sub_name[0] == '\0') {
      if (subs == 1) {

         rawmidi_info.setSize(rawmidi_info.getSize()+1);
         index = rawmidi_info.getSize()-1;
         rawmidi_info[index].card = card;
         rawmidi_info[index].device = device;
         rawmidi_info[index].subdevice = -1;
         strcpy(rawmidi_info[index].name, name);
         rawmidi_info[index].input = in;
         rawmidi_info[index].output = out;
      } else

         rawmidi_info.setSize(rawmidi_info.getSize()+1);
         index = rawmidi_info.getSize()-1;
         rawmidi_info[index].card = card;
         rawmidi_info[index].device = device;
         rawmidi_info[index].subdevice = -1;
         strcpy(rawmidi_info[index].name, name);
         rawmidi_info[index].input = in;
         rawmidi_info[index].output = out;

   } else {
      sub = 0;
      for (;;) {
         rawmidi_info.setSize(rawmidi_info.getSize()+1);
         index = rawmidi_info.getSize()-1;
         rawmidi_info[index].card = card;
         rawmidi_info[index].device = device;
         rawmidi_info[index].subdevice = sub;
         strcpy(rawmidi_info[index].name, sub_name);
         rawmidi_info[index].input = in;
         rawmidi_info[index].output = out;

         if (++sub >= subs)
            break;

         in = is_input(ctl, card, device, sub);
         out = is_output(ctl, card, device, sub);
         snd_rawmidi_info_set_subdevice(info, sub);
         if (out) {
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
            if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
               cerr << "Cannot get rawmidi information " << card << ":"
                    << device << ":" << sub << ": " << snd_strerror(status)
                    << endl;
               break;
            }
         } else {
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
            if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
               cerr << "Cannot get rawmidi information " << card << ":"
                    << device << ":" << sub << ": "
                    << snd_strerror(status) << endl;
               break;
            }
         }
         sub_name = snd_rawmidi_info_get_subdevice_name(info);
      }
   }
}



//////////////////////////////
//
// MKB_MIDIDriver::is_input -- returns true if specified card/device/sub
//      can read MIDI data.
//

int MKB_MIDIDriver::is_input(snd_ctl_t *ctl, int card, int device, int sub) {
   snd_rawmidi_info_t *info;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);
   snd_rawmidi_info_set_subdevice(info, sub);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);

   if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
      return status;
   } else if (status == 0) {
      return 1;
   }

   return 0;
}



//////////////////////////////
//
// MKB_MIDIDriver::is_output -- returns true if specified card/device/sub
//     can write MIDI data.
//

int MKB_MIDIDriver::is_output(snd_ctl_t *ctl, int card, int device, int sub) {
   snd_rawmidi_info_t *info;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);
   snd_rawmidi_info_set_subdevice(info, sub);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);

   if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
      return status;
   } else if (status == 0) {
      return 1;
   }

   return 0;
}
*/


#endif   /* LINUX and ALSA */

