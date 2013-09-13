#include "Fl_MIDIKeyboard.h"

#include <iostream>


//
//      Static functions
//


int Fl_MIDIKeyboard::white_keys(uchar from, uchar to) {
    int nkeys = 0;

    if (is_black(from)) from--;     // if from or to are black, start with white keys
    if (is_black(to)) to++;
    while (from + 12 <= to) {
        nkeys += 7;
        from += 12;
    }
    while (from <= to) {
        if (!is_black(from)) nkeys++;
        from++;
    }
    return nkeys;
}


uchar Fl_MIDIKeyboard::note_to_number(const char* name) {
    static const uchar values[] = { 9, 11, 0, 2, 4, 5, 7 };
    // MIDI offsets of 'A' 'B' 'C' 'D' 'E' 'F' 'G'
    uchar n;
    int c;
    const char* p = name;

    c = toupper(*p) - 'A';
    if (c > 6) return 0;
    n = values[c];
    p++;
    if (*p == 'b') {
        n--;
        p++;
    }
    else if (*p =='#') {
        n++;
        p++;
    }
    if (!isdigit(*p)) return 0;
    if (*(p + 1) == 0) n += 12 * (*p - '0');
    else if (*p == '1' && *(p + 1) == '0') n += 120;
    return n;
}


const char* Fl_MIDIKeyboard::number_to_note(uchar k) {
    static char s[6];
    static char const names[][12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    strcpy(s, names[k % 12]);
    sprintf(s+strlen(s), "%d", k / 12);
    return s;
}


//
//      Constructor
//

Fl_MIDIKeyboard::Fl_MIDIKeyboard (int X, int Y, int W, int H, const char *l) :
    Fl_Scroll(X, Y, W, H, l),
    _bw_height_ratio(DEFAULT_BW_HEIGHT_RATIO),
    _bw_width_ratio(DEFAULT_BW_WIDTH_RATIO),
    _autoresize(false),
    _autoresized(false),
    _kw_resize_min(DEFAULT_KW_RESIZE_MIN),
    _kw_resize_max(DEFAULT_KW_RESIZE_MAX),
    _base_keyinput(MIDDLE_C) {

    box(FL_DOWN_FRAME);
    _type = (W >= H) ? MKB_HORIZONTAL : MKB_VERTICAL;   // set horizontal/vertical

    keyboard = new Fl_Box(kbdx(), kbdy(), 1, 1);        // create the keyboard container
    keyboard->box(FL_BORDER_BOX);
    keyboard->color(FL_WHITE);
    end();

    set_key_height();                                   // set keys width and height
    _key_width = _old_k_width = _key_height * DEFAULT_WH_RATIO;
    _b_width = (int)(_key_width * _bw_width_ratio);
    hscrollbar.callback(hscrollbar_cb);                 // set scrollbar callbacks to overriden functions
    scrollbar.callback(scrollbar_cb);
    set_range(MKB_2OCTAVE);                             // default : two octaves width
    scroll_mode(MKB_SCROLL_KEYS);                       // default : scrolling only with PGUP/PGDOWN
    press_mode(MKB_PRESS_NONE);                         // default : playing not active

    center_keyboard();
    _below_mouse = find_key(Fl::event_x(), Fl::event_y());
}


//
//      Other public functions
//



void Fl_MIDIKeyboard::key_width(float w) {
    resize_mode(false);                                 // resizing will be manual, so no autoresize
    _key_width = _old_k_width = w;                      // set white keys width
    _b_width = (int)(w * _bw_width_ratio);              // set black keys width
    set_keyboard_width();                               // set global width of the keyboard
    redraw();
}


void  Fl_MIDIKeyboard::bw_height_ratio(float r) {
    if (0.2 <= r && r <= 0.8) {
        _bw_height_ratio = r;
        _b_height = (int)(_key_height * _bw_height_ratio);  // set black keys height
        redraw();
    }
}


void Fl_MIDIKeyboard::bw_width_ratio(float r) {
    if (0.4 <= r && r <= 0.8) {
        _bw_width_ratio = r;
        _b_width = (int)(_key_width * _bw_width_ratio);     // set black keys width
        set_keyboard_width();
        redraw();
    }
}


bool Fl_MIDIKeyboard::set_range(uchar fk, uchar lk) {
    if (fk >= lk) return false;
    if (is_black(fk))
        _firstkey > fk ? fk-- : fk++;                   // the keyboard can't begin or end with a black key
    if (is_black(lk))                                   // so add keys if needed
        _lastkey < lk ? lk++ : lk--;
    if (lk - fk < DEFAULT_MIN_NUMBER_KEYS) return false;
    _firstkey = fk;
    _lastkey = lk;
    _white_keys = white_keys(fk, lk);                   // calculate the number of white keys
    set_keyboard_width();                               // set global width of the keyboard
    redraw();
    return true;
}


bool Fl_MIDIKeyboard::set_range(const char* fk, const char* lk) {
    return set_range(note_to_number(fk), note_to_number(lk));
}

bool Fl_MIDIKeyboard::set_range(int r) {
    switch (r) {
        case MKB_PIANO :
            set_range(21, 120);
            break;
        case MKB_5OCTAVE :
            set_range(36, 96);
            break;
        case MKB_4OCTAVE :
            set_range(36, 84);
            break;
        case MKB_2OCTAVE :
            set_range (48, 72);
            break;
        default :
            return false;
    }
    return true;
}


 void Fl_MIDIKeyboard::resize_mode(bool res, uchar min /* = DEFAULT_KW_RESIZE_MIN */,
                                             uchar max /* = DEFAULT_KW_RESIZE_MAX */) {
    _autoresize = res;
    if (!res)
        _autoresized = false;
    _kw_resize_min = min;
    _kw_resize_max = max;
    set_keyboard_width();                       // if needed, recalculates the width of the keyboard
}


void Fl_MIDIKeyboard::scroll_mode(char c) {
    _scrollmode = c;
    if (_total_width <= kbdw()) {               // if the keyboard is smaller then the widget. hides the scrollbars
        Fl_Scroll::type(0);
        scrollbar.hide();
        hscrollbar.hide();
    }
    else {
        if (!(c & MKB_SCROLL_SCRBAR)) {
            Fl_Scroll::type(0);                 //  hide scrollbars
            scrollbar.hide();
            hscrollbar.hide();
        }
        else {                                  // show appropriate scrollbar
            if (_type == MKB_HORIZONTAL) {
                Fl_Scroll::type(HORIZONTAL_ALWAYS);
                hscrollbar.align(FL_ALIGN_BOTTOM);
                hscrollbar.show();
            }
            else {
                Fl_Scroll::type(VERTICAL_ALWAYS);
                scrollbar.align(FL_ALIGN_RIGHT);
                scrollbar.show();
            }
        }
    }
    set_key_height();                           // if a scrollbars was hidden, change the key height
}


void Fl_MIDIKeyboard::press_mode(char c)  {
    clear_pressed_status();
    _pressmode = c;
    if (_pressmode == MKB_PRESS_NONE)
        CloseMIDIOutPort();
    else
        OpenMIDIOutPort();
}


void Fl_MIDIKeyboard::kbd_position(short pos){
    if( _type == MKB_HORIZONTAL)
        position(pos, yposition());
    else {
        position(xposition(),  pos);
    }
    visible_keys();
}


void Fl_MIDIKeyboard::key_position(uchar k) {
    if (k < _firstkey) k = _firstkey;
    else if (k > _maxbottom) k = _maxbottom;
    if(_type == MKB_HORIZONTAL)
        kbd_position(_total_width <= kbdw() ? 0 : keyscoord[k]);
    else
        kbd_position(_total_width <= kbdw() ? 0 : _total_width - kbdw() - keyscoord[k]);
//    cout << "_total_width = " << _total_width << "  k = " << (int)k << " coords[k] = " << keyscoord[k] << "  h() = " << h()
//    << "  t_w - keysc - h = " << _total_width - keyscoord[k] - h()+ Fl::box_dh(box()) << endl;
}


void Fl_MIDIKeyboard::center_keyboard(uchar k) {
    if (k <= _firstkey) {                                   // k is too low
        key_position(_firstkey);
        return;
    }
    int nkeys = int(kbdw() / _key_width) + 1;               // number of visible white keys
    int offset = nkeys / 2;                                 // we shift 1/2 nkeys from k
    if (k > _lastkey || white_keys(k, _lastkey) < nkeys) {  // k is too big
        key_position(_maxbottom);
        return;
    }
    for (int i = 0; i < offset; i++) {
        if (white_keys(k, _lastkey) == nkeys) break;
        if (k == _firstkey) break;
        k--;                                                // shift one white key
        if(is_black(k)) k--;
    }
    key_position(k);
}


void Fl_MIDIKeyboard::set_pressed_status(bool* keys_array) {
    memcpy(pressed_keys, keys_array, sizeof(pressed_keys));
    _npressed = 0;
    _minpressed = 0;
    _maxpressed = 0;
    for (int i = 0; i < 128; i++) {
        if (!pressed_keys[i]) continue;
        if (!_npressed) _minpressed = i;
        _npressed++;
        _maxpressed = i;
    }
    redraw();
}


void Fl_MIDIKeyboard::get_pressed_status(const bool* keys_array, uchar& n, uchar& min, uchar& max) {
    keys_array = pressed_keys;
    n = _npressed;
    min = _minpressed;
    max = _maxpressed;
}


void Fl_MIDIKeyboard::clear_pressed_status() {
    memset(pressed_keys, 0, sizeof(bool[128]));
    _npressed = 0;
    _minpressed = 0;
    _maxpressed = 0;
    redraw();
    if(when() & MKB_WHEN_RELEASE) {
        _callback_status = MKB_CLEAR;
        do_callback();
    }
}


void Fl_MIDIKeyboard::press_key(uchar k) {
    if (!pressed_keys[k]) {
#ifdef FL_MIDI_BUILTIN_DRIVER
        NoteOn(k);                          // play the key with the MIDI driver
#else
#ifdef FL_MIDI_CUSTOM_DRIVER
        // place here your statements for the output of a MIDI note_on message
#endif
#endif
        pressed_keys[k] = true;             // adjust the pressed status variables
        _npressed++;
        if (_npressed == 1) _maxpressed = _minpressed = k;
        else if (k > _maxpressed) _maxpressed = k;
        else if (k < _minpressed) _minpressed = k;
        redraw();
        if (when() & MKB_WHEN_PRESS) {
            _callback_status = MKB_PRESS | k;
            do_callback();                  // if needed, calls the callback
        }
        //cout << "Pressed " << (char)k << " npressed = " << (int)_npressed << endl;
    }
}


void Fl_MIDIKeyboard::release_key(uchar k) {
    if (pressed_keys[k]) {
#ifdef FL_MIDI_BUILTIN_DRIVER
        NoteOff(k);
#else
#ifdef FL_MIDI_CUSTOM_DRIVER
        // place here your statements for the output of a MIDI note_on message
#endif
#endif
        pressed_keys[k] = false;
        _npressed--;
        if (_npressed) {
            if (k == _maxpressed) {
                do k--;
                    while (!pressed_keys[k]);
                _maxpressed = k;
            }
            else if (k == _minpressed) {
                do k++;
                    while (!pressed_keys[k]);
                _minpressed = k;
            }
        }
    redraw();
    if (when() & MKB_WHEN_RELEASE) {
        _callback_status = MKB_RELEASE | k;
        do_callback();
    }
    //cout << "Released " << (char)k << " npressed = " << (int)_npressed << endl;
    }
}


void Fl_MIDIKeyboard::set_keyboard_width(void) {
    bool _maxbottom_found = false;

    //_total_width = (int)(_key_width * _white_keys);
    if (_autoresize) {
        // autoresize mode: try to autoresize the keyboard and fit it in the widget without need of scrolling
        int res_min = (int)(kbdw() * (100.0 - _kw_resize_min) / 100);   // minimum acceptable width
        int res_max = (int)(kbdw() * (100.0 + _kw_resize_max) / 100);   // maximum acceptable width
        int old_width = (int)(_old_k_width * _white_keys);

        if (old_width >= res_min && old_width <= res_max) {
            if (!_autoresized) {                        // we are autoresizing for the first time
                _old_k_width = _key_width;              // save old key width
                _autoresized = true;
            }
            _key_width = (float)kbdw() / _white_keys;
            _total_width = kbdw();
            _b_width = (int)(_key_width * _bw_width_ratio);
            scroll_mode(_scrollmode);                 // sets scrollbars and keys height
            //_maxbottom = _firstkey;
            //_maxbottom_found = true;
        }
        else {
            if (old_width < res_min) {       // width less than minimum
                _key_width = res_min / _white_keys;
                _total_width = (int)(_key_width * _white_keys);
                scroll_mode(_scrollmode);
                _maxbottom = _firstkey;      // _maxbottom search would fail if width less than kbdw()
                _maxbottom_found = true;
            }
            else {                            // width greater than maximum
                if (_autoresized) {           // we are switching from autoresized to normal
                    _autoresized = false;
                    _key_width = _old_k_width;
                }
                _total_width = (int)(_key_width * _white_keys);
                scroll_mode(_scrollmode);
            }
        }
    }
    else {                                              // not autoresize
        _key_width = _old_k_width;
        _total_width = (int)(_key_width * _white_keys);
        scroll_mode(_scrollmode);
        if (_total_width <= kbdw()) {       // _maxbottom search would fail if width less than kbdw()
            _maxbottom = _firstkey;
            _maxbottom_found = true;
        }
    }
    _b_width = (int)(_key_width * _bw_width_ratio);     // adjust black keys width
    _type == MKB_HORIZONTAL ?                           // resizes the keyboard
        keyboard->size(_total_width, _key_height + 1) :
        keyboard->size(_key_height + 1, _total_width);

    float offs = 0.0;
    for (uchar i = _firstkey; i <= _lastkey; i++) {
        keyscoord[i] = (int)offs;
        if (is_black(i)) keyscoord[i] -= (int)(_b_width / 2);
        else offs += _key_width;
    }
    if (!_maxbottom_found)
        _maxbottom = find_key_from_offset(_total_width - kbdw(), false);
    center_keyboard();
    //key_position(_firstkey);        // calls visible_keys()
}


void Fl_MIDIKeyboard::set_key_height() {
    if (_type == MKB_HORIZONTAL) {
        keyboard->size(keyboard->w(), kbdh() - hscrollbar.h() * hscrollbar.visible() - 1);
        _key_height = keyboard->h();
    }
    else {
        keyboard->size(kbdh() - scrollbar.w() * scrollbar.visible() - 1, keyboard->h());
        _key_height = keyboard->w();
    }
     _b_height = (int)(_key_height * _bw_height_ratio);
}



uchar Fl_MIDIKeyboard::find_key_from_offset(int off, bool low) {
    if (off < 0 || off > _total_width) return 0;
    uchar min = _firstkey;                                      // binary search
    uchar max = _lastkey;
    uchar mid = min + (max - min) / 2;
    if (off >= keyscoord[max]) mid = max;
    else {
        do {
            if (off >= keyscoord[mid]) min = mid;
            else max = mid;
            mid = min + (max - min) / 2;
        } while (max - min > 1);
    }
    if (low && mid > _firstkey) {
                    // if a black and a white key overlap and low == true, the function returns
                    // the lower key, else the upper
        if (is_black(mid-1) && keyscoord[mid-1]+_b_width > off) mid--;
        else if (!is_black(mid-1) && keyscoord[mid-1]+_key_width > off) mid--;
    }
    return mid;
}


short Fl_MIDIKeyboard::find_key(int X, int Y) {
    X -= keyboard->x();
    Y = (_type == MKB_HORIZONTAL ? Y - keyboard->y() : keyboard->y() + keyboard->h() - Y);
    if (X < 0 || Y < 0 || X > keyboard->w() || Y > keyboard->h()) return -1;
    uchar min = _firstkey;                                      // binary search
    uchar max = _lastkey;
    uchar mid = min + (max - min) / 2;
    if(_type == MKB_HORIZONTAL) {
        if (X >= keyscoord[max]) return max;
        do {
            if (X >= keyscoord[mid]) min = mid;
            else max = mid;
            mid = min + (max - min) / 2;
        } while (max - min > 1);
        if (Y > _b_height) return (is_black(min) ? mid -1 : mid);   // (X, Y) is below black keys
        else if (is_black(mid)) return mid;                         // (X, Y) is on the left side of a black key
        else if (isCF(mid)) return mid;                             // (X, Y) is on a C or F
        else if (X - keyscoord[mid] <= _b_width / 2 && mid > _firstkey) return mid - 1;
                                                                    // (X, Y) is on the right side of a black key
        else return mid;                                            // (X, Y) is between two black keys
    }
    else {
        if (Y >= keyscoord[max]) return max;
        do {
            if (Y >= keyscoord[mid]) min = mid;
            else max = mid;
            mid = min + (max - min) / 2;
        } while (max - min > 1);
        if (X > _b_height) return (is_black(min) ? mid -1 : mid);
        else if (is_black(mid)) return mid;
        else if (isCF(mid)) return mid;
        else if (Y - keyscoord[mid] <= _b_width / 2 && mid > _firstkey) return mid - 1;
        else return mid;
    }
}


void Fl_MIDIKeyboard::visible_keys(void) {
    if (_type == MKB_HORIZONTAL) {
        if (_total_width < w()) {
            _bottomkey = _firstkey;
            _topkey = _lastkey;
        }
        else {
            _bottomkey = find_key_from_offset(x()+Fl::box_dx(box())-keyboard->x(), true);
            _topkey = find_key_from_offset(x()+Fl::box_dx(box())+w()-Fl::box_dw(box())-keyboard->x(), false);
        }
    }
    else {
        if (_total_width < h()) {
            _bottomkey = _firstkey;
            _topkey = _lastkey;
        }
        else {
            _bottomkey = find_key_from_offset(keyboard->y()+_total_width-y()-Fl::box_dy(box())-h()+Fl::box_dh(box()), true);
            _topkey = find_key_from_offset(keyboard->y()+_total_width-y()-Fl::box_dy(box()), false);
        }
    }
    redraw();
}





void Fl_MIDIKeyboard::hscrollbar_cb(Fl_Widget* w, void*) {      // called only if MKB_HORIZONTAL
    Fl_MIDIKeyboard* mk = (Fl_MIDIKeyboard*)(w->parent());
    Fl_Scrollbar* scbar = (Fl_Scrollbar*)w;
    mk->position((int)(scbar->value()), mk->yposition());       // do the scrolling
    mk->visible_keys();                                         // set the visible keys range
}


void Fl_MIDIKeyboard::scrollbar_cb(Fl_Widget* w, void*) {       // called only if MKB_VERTICAL
    Fl_MIDIKeyboard* mk = (Fl_MIDIKeyboard*)(w->parent());
    Fl_Scrollbar* scbar = (Fl_Scrollbar*)w;
    mk->position(mk->xposition(), (int)(scbar->value()));       // as above
    mk->visible_keys();
}


int Fl_MIDIKeyboard::handle(int e) {
    int ret = Fl_Scroll::handle(e);
    if (ret && (                                // if the event was a keyboard scrolling ...
        (_type == MKB_HORIZONTAL && Fl::event_inside(&hscrollbar)) ||
        (_type == MKB_VERTICAL &&Fl::event_inside(&scrollbar))))
         return 1;                              // exit

    _callback_status = 0;
    switch(e) {
        case FL_PUSH :
            take_focus();
            if (Fl::event_button1() && (_pressmode & MKB_PRESS_MOUSE)) {
                press_key(_below_mouse);        // press the key below mouse
                return 1;
            }
        case FL_DRAG :
            if (!Fl::event_inside(this) || Fl::event_inside(&hscrollbar) || Fl::event_inside(&scrollbar)){
                release_key(_below_mouse);      // if the mouse leaved the keyboard, release the key
                _below_mouse = -1;
                Fl::pushed(0);
            }
            else {
                short new_below_mouse = find_key(Fl::event_x(), Fl::event_y());
                if (new_below_mouse != _below_mouse) {  // the key below mouse changed
                    if (_pressmode & MKB_PRESS_MOUSE) {
                        release_key(_below_mouse);
                        press_key(new_below_mouse);
                    }
                    _below_mouse = new_below_mouse;
                }
            }
            return 1;
        case FL_RELEASE :
            release_key(_below_mouse);
            return 1;
        case FL_ENTER :
            return 1;
        case FL_MOVE :
            _below_mouse = find_key(Fl::event_x(), Fl::event_y());
            return 1;
        case FL_LEAVE :
            clear_pressed_status();
            _below_mouse = -1;
            return 1;

        case FL_FOCUS :
            if (when() & MKB_WHEN_FOCUS) {
                _callback_status = MKB_FOCUS;
                do_callback();
            }
            return 1;
        case FL_UNFOCUS :
            clear_pressed_status();
            if (when() & MKB_WHEN_FOCUS) {
                _callback_status = MKB_UNFOCUS;
                do_callback();
            }
            return 1;

        case FL_KEYDOWN :
        case FL_KEYUP :
            if ( (_scrollmode & MKB_SCROLL_KEYS) && (e == FL_KEYDOWN) )  {  // handle arrow keys for scrolling
                switch(Fl::event_key()) {
                    case FL_Left :
                        key_position(is_black(_bottomkey-1) ? _bottomkey-2 : _bottomkey-1);
                        return 1;
                    case FL_Right :
                        key_position(is_black(_bottomkey+1) ? _bottomkey+2 : (is_black(_bottomkey+2) ? _bottomkey+3 : _bottomkey+2));
                        return 1;
                    default :
                        break;
                }
            }
            if (_pressmode & MKB_PRESS_KEYS) {      // handle playback with computer keyboard
                uchar offs;                         // is the offset from base C
                switch(Fl::event_key()) {
                    case 'z' :
                        offs = 0;                   // C
                        break;
                    case 's' :
                        offs = 1;                   // C#
                        break;
                    case 'x' :
                        offs = 2;                   // D
                        break;
                    case 'd' :
                        offs = 3;                   // D#
                        break;
                    case 'c' :
                        offs = 4;                   // E
                        break;
                    case 'v' :
                        offs = 5;                   // F
                        break;
                    case 'g' :
                        offs = 6;                   // F#
                        break;
                    case 'b' :
                        offs = 7;                   // G
                        break;
                    case 'h' :
                        offs = 8;                   // G#
                        break;
                    case 'n' :
                        offs = 9;                   // A
                        break;
                    case 'j' :
                        offs = 10;                  // A#
                        break;
                    case 'm' :
                        offs = 11;                  // B
                        break;
                    case ',' :
                        offs = 12;                  // C (upper octave)
                        break;
                    case FL_Up :
                        if (_base_keyinput + 12 < _lastkey && e == FL_KEYDOWN) {
                            _base_keyinput += 12;   // raise one octave
                            center_keyboard(_base_keyinput + 6);
                        }
                        return 1;
                    case FL_Down :
                        if (_base_keyinput - 12 > _firstkey && e == FL_KEYDOWN) {
                            _base_keyinput -= 12;
                            center_keyboard(_base_keyinput + 6);
                        }
                        return 1;                   // lower one octave
                    default :
                        return 0;                   // other keys not recognized
                }
                offs += _base_keyinput;             // get the actual MIDI note number
                if (offs < _firstkey || offs > _lastkey)    // the key is not in the extension
                    return 0;
                if (e == FL_KEYDOWN && !pressed_keys[offs]) {
                    //cout << "handle  Pressed " << (char)offs << "  ";
                    press_key (offs);
                }
                if (e == FL_KEYUP && pressed_keys[offs]) {
                    //cout << "handle  Released " << (char)offs << "  ";
                    release_key(offs);
                }
                return 1;
            }
            break;
        default :
            break;
    }
    return ret;
}


void Fl_MIDIKeyboard::resize(int X, int Y, int W, int H) {	// fltk resize() override
    Fl_Scroll::resize(X, Y, W, H);
    visible_keys();
}


void Fl_MIDIKeyboard::draw(void) {                          // fltk draw() override
    Fl_Scroll::draw();

    fl_push_clip(x()+Fl::box_dx(box()), y()+Fl::box_dy(box()), w()-Fl::box_dw(box()), h()-Fl::box_dh(box()));
    int X = keyboard->x(), Y = keyboard->y();
    int press_diam = _b_width-2;
    int press_w_h_offs = _b_height + (_key_height - _b_height - press_diam) / 2;
    int press_w_w_offs = (int)((_key_width-press_diam) / 2);
    int press_b_h_offs = _b_height - press_diam - 2;
    uchar bk = is_black(_bottomkey) ? _bottomkey-1 : _bottomkey;    // need to begin with a white key

    fl_color(FL_BLACK);
    if (_type == MKB_HORIZONTAL) {
        int y_b_offs = Y + _b_height;
        for (int i = bk, cur_x = X + keyscoord[bk]; i <= _topkey; i++, cur_x = X + keyscoord[i]) {
            if (is_black(i)) {
                fl_rectf(cur_x, Y, _b_width, _b_height);
                if (pressed_keys[i]) {
                    fl_color(FL_RED);
                    fl_pie(cur_x, Y + press_b_h_offs, press_diam, press_diam, 0, 360);
                    fl_color(FL_BLACK);
                }
            }
            else {
                if (pressed_keys[i]) {
                    fl_color(FL_RED);
                    fl_pie(cur_x + press_w_w_offs, Y + press_w_h_offs, press_diam, press_diam, 0, 360);
                    fl_color(FL_BLACK);
                }
                isCF(i) ? fl_line(cur_x, Y, cur_x, Y + _key_height) :
                          fl_line(cur_x, y_b_offs, cur_x, Y + _key_height);

            }
        }
    }
    else {
        Y += keyboard->h();
        int x_b_offs = X + _b_height;
        for (int i = bk, cur_y = Y - keyscoord[bk]; i <= _topkey; i++, cur_y = Y - keyscoord[i]) {
            if (is_black(i)) {
                fl_rectf(X, cur_y - _b_width, _b_height, _b_width);
                if (pressed_keys[i]) {
                    fl_color(FL_RED);
                    fl_pie(X + press_b_h_offs, cur_y - press_diam,  press_diam, press_diam, 0, 360);
                    fl_color(FL_BLACK);
                }
            }
            else {
                if (pressed_keys[i]) {
                    fl_color(FL_RED);
                    fl_pie(X + press_w_h_offs, cur_y - press_w_w_offs - press_diam, press_diam, press_diam, 0, 360);
                    fl_color(FL_BLACK);
                }
                isCF(i) ? fl_line(X, cur_y, X + _key_height, cur_y) :
                          fl_line(x_b_offs, cur_y, X + _key_height, cur_y);
            }
        }
    }
    fl_pop_clip();
}

