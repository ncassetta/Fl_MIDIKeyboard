#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <Fl/Fl_Output.H>
#include <Fl/Fl_Spinner.H>
#include <Fl/Fl_Choice.H>
#include <Fl/Fl_Check_Button.H>
#include <Fl/Fl_ask.H>

#include "../src/Fl_MIDIKeyboard.h"

#include <cstdio>

Fl_Double_Window *window;
Fl_MIDIKeyboard *kb;
Fl_Group *group_settings, *group_midi, *group_output;
Fl_Spinner *spinner_range1, *spinner_range2, *spinner_bwh, *spinner_bww,
    *spinner_chan, *spinner_program, *spinner_vol, *spinner_pan, *spinner_vel;
Fl_Check_Button* check_autoresize;
Fl_Choice *choice_scroll, *choice_keypress, *choice_callback, *choice_port;
Fl_Output *out1, *out2, *out3, *out4, *out5, *out6, *out7;


void setrange_cb(Fl_Widget* w, void* p) {
    int low = (int)spinner_range1->value();
    int high = (int)spinner_range2->value();
    if (high - low < 7)
        fl_alert("Minimum extension is one octave!");
    else
        kb->set_range(low, high);
    spinner_range1->value(kb->first_key());
    spinner_range2->value(kb->last_key());
}

void setbwhratio_cb(Fl_Widget* w, void* p) {
    kb->bw_height_ratio(spinner_bwh->value());
    kb->redraw();
}

void setbwwratio_cb(Fl_Widget* w, void* p) {
    kb->bw_width_ratio(spinner_bww->value());
    kb->redraw();
}


void setscrolling_cb(Fl_Widget* w, void* p) {
    switch (choice_scroll->value()) {
        case 0:
            kb->scroll_mode(Fl_MIDIKeyboard::MKB_SCROLL_NONE);
            break;
        case 1:
            kb->scroll_mode(Fl_MIDIKeyboard::MKB_SCROLL_MOUSE);
            break;
        case 2:
            kb->scroll_mode(Fl_MIDIKeyboard::MKB_SCROLL_KEYS);
            kb->take_focus();
            break;
        case 3:
            kb->scroll_mode(Fl_MIDIKeyboard::MKB_SCROLL_SCRBAR);
            break;
    }
}

void setkeypress_cb(Fl_Widget* w, void* p) {
    switch (choice_keypress->value()) {
        case 0:
            kb->press_mode(Fl_MIDIKeyboard::MKB_PRESS_NONE);
            break;
        case 1:
            kb->press_mode(Fl_MIDIKeyboard::MKB_PRESS_MOUSE);
            break;
        case 2:
            kb->press_mode(Fl_MIDIKeyboard::MKB_PRESS_KEYS);
            kb->take_focus();
            break;
        case 3:
            kb->press_mode(Fl_MIDIKeyboard::MKB_PRESS_BOTH);
            break;
    }
}

void setcallback_cb(Fl_Widget* w, void* p) {
    switch (choice_callback->value()) {
        case 0:
            kb->when(FL_WHEN_NEVER);
            break;
        case 1:
            kb->when(Fl_MIDIKeyboard::MKB_WHEN_FOCUS);
            break;
        case 2:
            kb->when(Fl_MIDIKeyboard::MKB_WHEN_PRESS);
            break;
        case 3:
            kb->when(Fl_MIDIKeyboard::MKB_WHEN_RELEASE);
            break;
        case 4:
            kb->when(Fl_MIDIKeyboard::MKB_WHEN_PRESS_RELEASE);
            break;
    }
}

void setmidi_cb(Fl_Widget* w, void* p) {
    if (w == choice_port)
        kb->SetActivePort(((Fl_Choice *)w)->value());
    else if (w == spinner_chan)
        kb->SetChannel(((Fl_Spinner *)w)->value()-1);
    else if (w == spinner_program)
        kb->SetProgram(((Fl_Spinner *)w)->value());
    else if (w == spinner_vol)
        kb->SetVolume(((Fl_Spinner *)w)->value());
    else if (w == spinner_pan)
        kb->SetPan(((Fl_Spinner *)w)->value());
    else if (w == spinner_vel)
        kb->SetNoteVel(((Fl_Spinner *)w)->value());
}

void autoresize_cb(Fl_Widget* w, void* p) {
    Fl_Check_Button* cb = (Fl_Check_Button*) w;
    kb->resize_mode(cb->value());
}

void kb_callback_cb(Fl_Widget* w, void* p) {
    Fl_MIDIKeyboard* kb = (Fl_MIDIKeyboard *)w;
    int status = kb->callback_status();
    if ( (kb->when() | Fl_MIDIKeyboard::MKB_WHEN_FOCUS &&
                (status == Fl_MIDIKeyboard::MKB_FOCUS || status == Fl_MIDIKeyboard::MKB_UNFOCUS) ) ||
          ((kb->when() | Fl_MIDIKeyboard::MKB_WHEN_PRESS) && status == Fl_MIDIKeyboard::MKB_PRESS) ||
          ((kb->when() | Fl_MIDIKeyboard::MKB_WHEN_RELEASE) &&
                (status == Fl_MIDIKeyboard::MKB_RELEASE || status == Fl_MIDIKeyboard::MKB_CLEAR)) ) {
        out7->color(rand() % 256);
        out7->textcolor(rand() % 256);
        out7->redraw();
    }
}

void setoutput_to(void*) {
    char s[15];

    out1->value(Fl_MIDIKeyboard::number_to_note(kb->bottom_key()));
    out2->value(Fl_MIDIKeyboard::number_to_note(kb->top_key()));
    out3->value(kb->below_mouse() == -1 ? "NONE" : Fl_MIDIKeyboard::number_to_note(kb->below_mouse()));
    sprintf(s, "%d", Fl::event_x());
    out4->value(s);
    sprintf(s, "%d", Fl::event_y());
    out5->value(s);
    if (!kb->npressed()) out6->value("NONE");
    else if (kb->npressed() == 1)
        out6->value(Fl_MIDIKeyboard::number_to_note(kb->minpressed()));
    else
        out6->value("CHORD");
}






int main (int argc, char ** argv) {
    window = new Fl_Double_Window (800, 600);
    kb = new Fl_MIDIKeyboard (20, 20, 200, 380, "MIDI Keyboard");
    //kb = new Fl_MIDIKeyboard (20, 320, 380, 200, "MIDI Keyboard");
    kb->color(FL_DARK_BLUE);    // FOR DEBUG: you should NOT see any background blue!
    kb->callback(kb_callback_cb);
    kb->set_range(Fl_MIDIKeyboard::MKB_PIANO);
    kb->center_keyboard(MIDDLE_C);

    group_settings = new Fl_Group (290, 20, 280, 310);
    group_settings->box(FL_ENGRAVED_FRAME);

    spinner_range1 = new Fl_Spinner(300 , 30 ,60, 20);
    spinner_range1->callback(setrange_cb);
    spinner_range1->range(0, 120);
    spinner_range1->value(kb->first_key());
    spinner_range2 = new Fl_Spinner(370 , 30 ,60, 20, "Extension");
    spinner_range2->align(FL_ALIGN_RIGHT);
    spinner_range2->callback(setrange_cb);
    spinner_range2->range(7, 127);
    spinner_range2->value(kb->last_key());

    check_autoresize = new Fl_Check_Button(300, 60, 25, 25, "Autoresize");
    check_autoresize->align(FL_ALIGN_RIGHT);
    check_autoresize->callback(autoresize_cb);
    check_autoresize->value(1);
    check_autoresize->do_callback();

    spinner_bwh = new Fl_Spinner(300, 90, 60, 20, "B/W height ratio");
    spinner_bwh->align(FL_ALIGN_RIGHT);
    spinner_bwh->callback(setbwhratio_cb);
    spinner_bwh->type(FL_FLOAT_INPUT);
    spinner_bwh->range(0.2, 0.8);
    spinner_bwh->step(0.05);
    spinner_bwh->value(kb->bw_height_ratio());
    spinner_bww = new Fl_Spinner(300, 120, 60, 20, "B/W width ratio");
    spinner_bww->align(FL_ALIGN_RIGHT);
    spinner_bww->callback(setbwwratio_cb);
    spinner_bww->type(FL_FLOAT_INPUT);
    spinner_bww->range(0.4, 0.8);
    spinner_bww->step(0.05);
    spinner_bww->value(kb->bw_width_ratio());

    choice_scroll = new Fl_Choice(300, 150, 150, 20, "Scroll mode");
    choice_scroll->align(FL_ALIGN_RIGHT);
    choice_scroll->callback(setscrolling_cb);
    choice_scroll->add ("No scrolling");
    choice_scroll->add ("With mouse");
    choice_scroll->add ("With arrow keys");
    choice_scroll->add ("With scrollbar");
    choice_scroll->value(0);        // default value for scroll mode
    choice_scroll->do_callback();
    choice_keypress = new Fl_Choice(300, 180, 150, 20, "Playing mode");
    choice_keypress->align(FL_ALIGN_RIGHT);
    choice_keypress->callback(setkeypress_cb);
    choice_keypress->add ("No playing");
    choice_keypress->add ("With mouse");
    choice_keypress->add ("With computer keys");
    choice_keypress->add ("Both mouse and computer keys");
    choice_keypress->value(0);      // default value for keypress mode
    choice_callback = new Fl_Choice(300, 210, 150, 20, "Callback mode");
    choice_callback->align(FL_ALIGN_RIGHT);
    choice_callback->callback(setcallback_cb);
    choice_callback->add ("No callback");
    choice_callback->add ("When focus");
    choice_callback->add ("When a key is pressed");
    choice_callback->add ("When a key is released");
    choice_callback->add ("When a key is both pressed and released");
    choice_callback->value(0);      // default value for callback
    choice_callback->do_callback();

    group_settings->end();

    group_output = new Fl_Group (600, 20, 190, 540);
    group_output->box(FL_ENGRAVED_FRAME);

    out1 = new Fl_Output(710, 30, 60, 20, "First key :");
    out2 = new Fl_Output(710, 60, 60, 20, "Last  key :");
    out3 = new Fl_Output(710, 110, 60, 20, "Mouse  key :");
    out4 = new Fl_Output(710, 160, 60, 20, "Mouse x :");
    out5 = new Fl_Output(710, 190, 60, 20, "Mouse y :");
    out6 = new Fl_Output(710, 240, 60, 20, "Pressed :");
    out7 = new Fl_Output(610, 290, 160, 20);
    out7->value("CALLBACK");
    group_output->end();

    group_midi = new Fl_Group(290, 360, 280, 200);
    group_midi->box(FL_ENGRAVED_FRAME);

    choice_port = new Fl_Choice(300, 380, 240, 20, "MIDI Port");
    choice_port->align(FL_ALIGN_TOP);
    for (int i = 0; i < kb->GetNumMIDIOutDevs(); i++)
        choice_port->add(kb->GetMIDIOutDevName(i));
    choice_port->value(0);
    choice_port->callback(setmidi_cb);
    choice_port->do_callback();
    spinner_chan = new Fl_Spinner(390, 420, 60, 20, "MIDI Channel");
    spinner_chan->range(1, 16);
    spinner_chan->value(kb->GetChannel());
    spinner_chan->callback(setmidi_cb);
    spinner_program = new Fl_Spinner(390, 460, 60, 20, "Prg");
    spinner_program->range(0, 127);
    spinner_program->value(kb->GetProgram());
    spinner_program->callback(setmidi_cb);
    spinner_vol = new Fl_Spinner(500, 460, 60, 20, "Vol" );
    spinner_vol->range(0, 127);
    spinner_vol->value(kb->GetVolume());
    spinner_vol->callback(setmidi_cb);
    spinner_pan = new Fl_Spinner(390, 500, 60, 20, "Pan");
    spinner_pan->range(0, 127);
    spinner_pan->value(kb->GetPan());
    spinner_pan->callback(setmidi_cb);
    spinner_vel = new Fl_Spinner(500, 500, 60, 20, "Vel");
    spinner_vel->range(0, 127);
    spinner_vel->value(kb->GetNoteVel());
    spinner_vel->callback(setmidi_cb);
    group_midi->end();

    window->end ();
    window->show (argc, argv);

    Fl::add_idle(setoutput_to);

    return(Fl::run());
}
