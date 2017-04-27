#ifndef MIDIKEYBOARD_H_INCLUDED
#define MIDIKEYBOARD_H_INCLUDED

/// \file
/// This file is the header for the Fl_MIDIKeyboard class.



#include <sys/types.h>
#include <cstring>		// memcpy
#include <cctype>       // toupper, isnumber in name_to_number
#include <cstdio>       // sprintf in number_to_note

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_draw.H>

#if FL_MAJOR_VERSION == 1 && FL_MINOR_VERSION < 3
    #error Fl_MIDIKeyboard requires at least FLTK 1.3.x
#endif // FL_MAJOR_VERSION
// due to a difference in the Fl_Scroll class, this is now incompatible with FLTK 1.1.x

#include "MIDIDriver.h"


#define MIDDLE_C 60                         ///< MIDI note number of middle C.




/// The class Fl_MIDIKeyboard implements a MIDI piano Keyboard, with several graphic and MIDI features.
/// The widget is implemented by a Fl_Scroll widget containig a Fl_Box (the actual keyboard). We can have a
/// MKB_HORIZONTAL keyboard (as in a piano playing program) or a MKB_VERTICAL one (as in the piano roll view of
/// many sequencers). This is decided automatically by the ctor according to the size (W and H) of the widget.
/// Please note that, when we say 'key height' and 'key width' we always refer to the longer and shorter sides of
/// the key: these are the usual FLTK w and h in an MKB_HORIZONTAL keyboard, but are reversed in an MKB_VERTICAL
/// one. The user can set the number of white keys in the keyboard, and choose several resizing and scrolling modes.
/// Moreover, can send messages to a MIDI device and play the keyboard.
class Fl_MIDIKeyboard : public Fl_Scroll, public MKB_MIDIDriver
{
    public:

        /// The placement of the keyboard
        enum {  MKB_HORIZONTAL,             ///< horizontal keyboard (as in a piano playing program)
                MKB_VERTICAL                ///< vertical keyboard (as in the piano roll view of many sequencers)
             };

        /// Some presets for the key ramge.
        enum {  MKB_PIANO,                  ///< a 7 octave piano keyboard (from A2 to C10)
                MKB_5OCTAVE,                ///< a 5 octave keyboard (from C3 to C8)
                MKB_4OCTAVE,                ///< a 4 octave keyboard (from C3 to C7)
                MKB_2OCTAVE                 ///< a 2 octave keyboard (from C5 to C7)
             };

        /// Scrolling modes.
        enum {  MKB_SCROLL_NONE = 0,        ///< no scrolling
                MKB_SCROLL_MOUSE = 1,       ///< hides the scrollbar and scroll with the mouse. Drag the mouse immediately
                                            ///< over, under, left or right to get scrolling
                MKB_SCROLL_KEYS = 2,        ///< hides the scrollbar and scroll by the left/right arrow keys
                                            ///<in the computer keyboard
                MKB_SCROLL_SCRBAR = 4       ///< makes the scrollbar visible and scroll with it
             };

        /// Key press modes.
        enum {  MKB_PRESS_NONE = 0,         ///< no key press (you can't play the keyboard)
                MKB_PRESS_MOUSE = 1,        ///< press by a mouse click on the keys
                MKB_PRESS_KEYS = 2,         ///< press by the computer keyboard (see \ref playing )
                MKB_PRESS_BOTH = 3          ///< press with both methods
             };

        /// These are aliases for the FLTK when() function, allowing you to launch the widget callback function
        /// under certain circumstances.
        enum {  MKB_WHEN_FOCUS = FL_WHEN_CHANGED,   ///< do the callback when the widget gets or loses focus
                MKB_WHEN_PRESS = FL_WHEN_ENTER_KEY, ///< do the callback when a key is pressed
                MKB_WHEN_RELEASE = FL_WHEN_RELEASE, ///< do the callback when a key is released
                MKB_WHEN_PRESS_RELEASE = MKB_WHEN_PRESS | MKB_WHEN_RELEASE
                                                    ///< do the callback either when pressed and released
             };

        /// Callback status: when a callback is called in response to one of the above conditions,
        /// you can look at this to find out why the callback was called.
        enum {  MKB_FOCUS = 0x100,          ///< the keyboard got the focus
                MKB_UNFOCUS = 0x200,        ///< the keyboard lost the focus
                MKB_CLEAR = 0x400,          ///< the press status was cleared (all keys released)
                MKB_PRESS = 0x800,          ///< a key was pressed. Call callback_note() to get its number
                MKB_RELEASE = 0x1000        ///< a key was released. Call callback_note() to get its number
             };


    private:

        int    _type;                       // horizontal/vertical keyboard

        uchar        _firstkey;             // keyboard's first/last MIDI key
        uchar       _lastkey;
        uchar        _bottomkey;            // first/last visible key
        uchar        _topkey;
        uchar        _maxbottom;            // max for bottom key
        int         _white_keys;            // number of white keys

        int         _key_height;            // keys height
        float       _key_width;             // keys width
        float       _bw_height_ratio;       // black/white height ratio
        float       _bw_width_ratio;        // black/white width ratio
        int         _b_height;              // black keys height
        int         _b_width;               // black keys width
        int         _total_width;           // keyboard width

        bool        _autoresize;            // resizing mode if keyboard shorter or longer than widget
        bool        _autoresized;           // the keyboard is in autoresize state (we restore old values if we can)
        uchar       _kw_resize_min;	        // key min resizing width (percent of optimal ratio)
        uchar       _kw_resize_max;         // key max resizing width (percent of optimal ratio)

        float       _old_k_width ;          // height to restore (see above)

        char        _scrollmode;            // scrolling mode
        char        _pressmode;             // playing mode
        short       _below_mouse;           // key below mouse (-1 if no key)
        uchar       _base_keyinput;         // base octave for computer keyboard input
        short       _callback_status;       // callback status

        bool        pressed_keys[128];      // pressed keys
        bool        _autodrag;              // used for mouse scrolling

        uchar       _npressed;              // number of pressed keys
        uchar       _minpressed;            // minimum pressed key  (for speeding draw routine)
        uchar       _maxpressed;            // maximum pressed key

        int         keyscoord[128];         // coords of the keys (used internally)

        Fl_Box*     keyboard;				// keyboard box



    protected:

        static const float DEFAULT_WH_RATIO = 0.2;          ///< default ratio between width and height for white keys
        static const float DEFAULT_BW_HEIGHT_RATIO = 0.6;   ///< default black/white height ratio
        static const float DEFAULT_BW_WIDTH_RATIO = 0.6;    ///< default black/white width ratio
        static const int   DEFAULT_KW_RESIZE_MIN = 20;      ///< default min for resize_mode()
        static const int   DEFAULT_KW_RESIZE_MAX = 20;      ///< default max for resize_mode()
        static const int   DEFAULT_MIN_NUMBER_KEYS = 12;    ///< the minimum number of white keys (1 octave)

        /// Returns the x coordinate of the visible top-left corner of the keyboard.
        short       kbdx()
                            { return x()+Fl::box_dx(box()); }

        /// Returns the y coordinate of the visible top-left corner of the keyboard.
        short       kbdy()
                            { return y()+ Fl::box_dy(box()); }

        /// Returns the exact available space for keys height.
        /// It is the widget h() or w() (depending if it is \ref MKB_HORIZONTAL or \ref MKB_VERTICAL)
        /// minus the borders offset.
        short       kbdh()
                            { return _type == MKB_HORIZONTAL ?
                                     h() - Fl::box_dh(box()) - hscrollbar.visible() * hscrollbar.h() :
                                     w() - Fl::box_dw(box()) - scrollbar.visible() * scrollbar.w(); }

        /// Returns the exact size of the visible space for the keyboard width.
        /// If the keyboard is wider than this it must be scrolled. This value is the widget w() or h()
        /// (depending if it is \ref MKB_HORIZONTAL or \ref MKB_VERTICAL) minus the borders offset.
        short       kbdw()
                            { return _type == MKB_HORIZONTAL ?
                                     w() - Fl::box_dw(box()) - scrollbar.visible() * scrollbar.w() :
                                     h() - Fl::box_dh(box()) - hscrollbar.visible() * hscrollbar.h(); }

        /// This function sets all internal parameters for draw() method, assuming that the correct key widths
        /// have been set.
        /// If you implement your own draw routines you should call this every time the width of the keys or of
        /// the keyboard changes (for example if black or white keys widths are resized, or if the range changes).
        /// Public functions already call it, so you do not have to worry if you use them.
        void        set_keyboard_width();   // set the width of the scrolling keyboard

        /// Sets the height of white and black keys, taking account of scrollbars and black/white height ratio.
        void        set_key_height();

        /// Returns the MIDI note number of a key, given the width coord offset from the keyboard begin.
        /// Used internally in drawing routines. For parameters see the implementation.
        uchar       find_key_from_offset(int off, bool low);

        /// Returns the MIDI note number of the key at X, Y coordinates (relative to the window
        /// containing the widget). If no key corresponds to X, Y returns -1.
        short       find_key(int X, int Y);

        /// Sets the currently visible key range. This is called internally at every scrolling or resizing,
        /// and sets internal variables _bottomkey and _topkey).
        /// \see bottom_key(), top_key()
        void        visible_keys();

        /// Overrides FLTK method for horizontal scrollbar.
        static void hscrollbar_cb(Fl_Widget*, void*);

        /// Overrides FLTK method for vertical scrollbar.
        static void scrollbar_cb(Fl_Widget*, void*);

        /// Returns true if k is a black key.
        static bool is_black(uchar note)  {
            note %= 12;
            return (note == 1 || note == 3 || note == 6 || note == 8 || note == 10); }

        /// Returns true if k is C or F.\ Used internally in draw() method.
        static bool isCF(uchar note) {
            note %= 12;
            return (note == 0 || note == 5); }

        /// The FLTK handle() method override.
        virtual int handle(int e);

        /// The FLTK draw() method override.
        virtual void draw(void);

        /// Used internally for mouse scrolling
        static void autodrag_to( void* p);

    public:

        /// Returns the number of white keys between given MIDI note numbers (including first and last).
        /// \param[in]	from, to the lower and upper MIDI note number.
        static int      white_keys(uchar from, uchar to);

        /// Converts a string as "C#4", "Bb2" to the corresponding MIDI note number. The note name can be
        /// upper or lower case, the accident can be b or #, the octave number starts with 0 (middle C = C5).
        /// No spaces are allowed.
        static uchar    note_to_number(const char* name);

        /// Converts the MIDI note number to a string (as "C#4"). For black keys uses the '#'.
        static const char* number_to_note(uchar k);

        /// The constructor decides the placement ( \ref MKB_HORIZONTAL or \ref MKB_VERTICAL) of the keyboard.
        /// This is done according to the size (W and H) of the widget, and cannot be changed, even resizing
        /// the widget. Then sets some default values:
        /// -autoresize = _FALSE_
        /// -range = \ref MKB_2OCTAVE
        /// -scroll_mode = \ref MKB_SCROLL_KEYS
        /// -press_mode = \ref MKB_PRESS_NONE
        /// \par X,Y,W,H,l	as usual in FLTK
        /// \see resize_mode(), set_range(), scroll_mode(), press_mode()
        Fl_MIDIKeyboard(int X, int Y, int W, int H, const char *l=0);

        /// The destructor.
        virtual     ~Fl_MIDIKeyboard() {}

        /// Gets the horizontal/vertical placement of the keyboard.
        /// It depends from the keyboard width/height and cannot be changed.
        /// \return one	of \ref MKB_HORIZONTAL, \ref MKB_VERTICAL
        int         type() const
                        { return _type; }

        /// Returns the total width of the keyboard (i.e.\ the internal scrolling Fl_Box) in pixels.
        int         total_width() const
                        { return _total_width; }

        /// Returns the number of white keys of the keyboard.
        int         white_keys() const
                        { return _white_keys; }

        /// Returns the white keys height in pixels.
        /// This is the lenght of the longer side of the key, and may be a range on the y-axis if the keyboard is
        /// \ref MKB_HORIZONTAL or on the x-axis if it is \ref MKB_VERTICAL. This value cannot be changed:
        /// it automatically fits the widget h() or w() taking into account borders, scrollbars, etc
        int         key_height() const
                        { return _key_height; }

        /// Sets the white keys width in pixels.
        /// This is the lenght of the shorter side of the key, and may be a range on the y-axis if the keyboard
        /// is \ref MKB_HORIZONTAL or on the x-axis if it is \ref MKB_VERTICAL.
        /// In order to mantain your fixed width this disable autoresizing.
        /// \note The param is a float (even if X, Y coords are int) for better rounding in the draw() routine.
        void        key_width(float W);

        /// Returns the white keys width in pixels (see key_width(float W)).
        float       key_width() const
                        { return _key_width; }

        /// Returns the black/white key height ratio.
        float       bw_height_ratio() const
                        { return(_bw_height_ratio); }

        /// Sets the black/white key height ratio./ Value range is 0.2 <= r <= 0.8.
        void        bw_height_ratio(float r);

        /// Returns the black/white key width ratio.
        float       bw_width_ratio() const
                        { return(_bw_width_ratio); }

        /// Sets the black/white key width ratio./ Value range is 0.4 <= r <= 0.8.
        void        bw_width_ratio(float r);

        /// Returns the MIDI note number of the first (lower) key of the keyboard (60 = middle c).
        uchar       first_key() const
                        { return(_firstkey); }

        /// Returns the MIDI note number of the last (upper) key of the keyboard (60 = middle c).
        uchar       last_key() const
                        { return(_lastkey); }

        /// Sets the key range. If autoresizing is on, it tries to resize the keys in order to fit
        /// to the widget width. Returns true if the change was effectively done without errors.
        /// \param[in]	fk, lk the lower and upper key MIDI note numbers
        bool        set_range(uchar fk, uchar lk);

        /// Same, but you can specify the notes by mean of their names.
        /// \param[in] fk, lk two strings (as "c#2" or "Bb5") giving the lower and upper note
        /// \see note_to_number()
        bool        set_range(const char* fk, const char* lk);

        /// Same, but you can use one of predefined macros.
        /// \param[in] r one of \ref MKB_PIANO, \ref MKB_5OCTAVE, \ref MKB_4OCTAVE, \ref MKB_2OCTAVE
        bool        set_range(int);

        /// Sets the autoresize mode. If it is off (FALSE) the widget will not try to autoresize its keys.
        /// Otherwise, every time you resize the widget or change its key range, it tries to resize the key width
        /// in order to fit the keyboard into the widget avoiding scrolling. The new white keys width
        /// is computed and if it is in the range between (100 - min)/100 and (100 + max) / 100 of the keyboard
        /// width the keys (both black and white) are autoresized.
        /// Note that you can set your own width calling key_width(float W), which disables autoresizing.
        /// \param[in] res autoresizing on/off
        /// \param[in] min, max	the minimum and maximum (in percent) for autoresizing
        void        resize_mode(bool res, uchar min = DEFAULT_KW_RESIZE_MIN, uchar max = DEFAULT_KW_RESIZE_MAX);

        /// Returns the min percent for autoresize. You can set it calling resize_mode().
        uchar       resize_min() const
                        { return _kw_resize_min; }

        /// Returns the max percent for autoresize. You can set it calling resize_mode().
        uchar       resize_max() const
                        { return _kw_resize_max; }

        /// Sets the scroll mode for the keyboard.
        /// There are several modes because you may not want to show the scrollbar.
        /// \param c \ref MKB_SCROLL_NONE, \ref MKB_SCROLL_MOUSE, \ref MKB_SCROLL_KEYS, \ref MKB_SCROLL_SCRBAR
        void        scroll_mode(char c);

        /// Returns the scroll mode.
        /// \return one	of \ref MKB_SCROLL_NONE, \ref MKB_SCROLL_MOUSE, \ref MKB_SCROLL_KEYS, \ref MKB_SCROLL_SCRBAR
        char        scroll_mode() const
                        { return _scrollmode; }

        /// Sets the press mode for the keyboard, i.e.\ the playing mode.
        /// \see playing
        /// \param[in] c \ref MKB_PRESS_NONE, \ref MKB_PRESS_MOUSE, \ref MKB_PRESS_KEYS, \ref MKB_PRESS_BOTH.
        void        press_mode(char c);

        /// Returns the key press mode.
        /// \return one	of \ref MKB_PRESS_NONE, \ref MKB_PRESS_MOUSE, \ref MKB_PRESS_KEYS, \ref MKB_PRESS_BOTH
        char        press_mode() const
                        { return _pressmode; }

        /// Returns the MIDI Note number of the first (lower) visible key.
        uchar       bottom_key() const
                        { return _bottomkey; }

        /// Returns the MIDI Note number of the last (higher) visible key.
        uchar       top_key() const
                        { return _topkey; }

        /// Does a "manual scroll" of the keyboard, similarly to the FLTK position().
        /// If the keyboard is \ref MKB_HORIZONTAL pos is the X coordinate of the left side of the keyboard,
        /// if it is MKB_VERTICAL pos is the Y coordinate of the bottom side of the keyboard
        void        kbd_position(short pos);

        /// Does a "manual scroll" of the keyboard, setting the key k (MIDI Note number) as the bottom (lower)
        /// visible key. If k is too low or high sets as first key the maximum or minimum available.
        void        key_position(uchar k);

        /// Centers the keyboard on key k (MIDI Note number). If the key cannot be centered tries to put it
        /// closer possible the centre of the keyboard.
        void        center_keyboard(uchar k = MIDDLE_C);

        /// Returns the MIDI note number of the key below the mouse. If no key returns -1
        short       below_mouse() const
                        { return _below_mouse; }

        /// Returns the MIDI note number of the lower pressed key.
        uchar       minpressed() const
                        { return _minpressed; }

        /// Returns the MIDI note number of the higher pressed key.
        uchar       maxpressed() const
                        { return _maxpressed; }

        /// Returns the number of pressed keys.
        uchar       npressed() const
                        { return _npressed; }

        /// Returns true if key k is pressed. k is the MIDI note number of the key.
        bool        is_pressed(uchar k) const
                        { return pressed_keys[k]; }

        /// Same, but k is a string identifying the note.
        /// \see note_to_number(), number_to_note()
        bool        is_pressed(const char* k) const
                        { return pressed_keys[note_to_number(k)]; }

        /// Sets the pressed status. The keyboard holds internally an array of 128 bool for tracking which keys
        /// are pressed or released. This loads the array with an user supplied status and sets other internal variables.
        /// \param[in] keys_array an array of 128 bool holding the status (pressed/released) for every key
        void        set_pressed_status(bool* keys_array);

        /// Returns all variables related to the pressed keys status.
        /// \param[out]	keys_array an array of 128 bool getting the status (pressed/released) for every key
        /// \param[out]	n the number of pressed keys
        /// \param[out] min,max	the lower an upper key preessed MIDI note number
        /// \see set_pressed_status()
        void        get_pressed_status(const bool* keys_array, uchar& n, uchar& min, uchar& max);

        /// Sets all keys as released.
        void        clear_pressed_status();

        /// Press the key k (k is the MIDI note number).
        /// If press_mode is not \ref MKB_PRESS_NONE this sends a MIDI note on message to the open MIDI port.
        /// You can set the MIDI velocity with the inherited function SetNoteVel().
        void        press_key(uchar k);

        /// Release the key k (k is the MIDI note number).
        /// If k is not a pressed key this does nothing, else it sends a MIDI note off message to the open port.
        void        release_key(uchar k);

        /// Returns the condition that generated the callback.
        /// \return one of \ref MKB_FOCUS, \ref MKB_UNFOCUS, \ref MKB_CLEAR, \ref MKB_PRESS, \ref MKB_RELEASE
        int         callback_status() const
                        { return _callback_status & 0xff00; }

        /// Returns the note (pressed or released) that generated the callback.
        uchar       callback_note()
                        { return _callback_status & 0xff; }

        /// FLTK method override.
        virtual void resize(int X, int Y, int W, int H);
};


#endif // MIDIKEYBOARD_H_INCLUDED
