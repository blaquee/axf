#pragma once

// STL
#include <vector>
#include <string>
#include <exception>

// Windows
#include <Windows.h>

#ifdef CONSOLE_DYNAMIC
#ifdef CONSOLE_EXPORTS
#define CONSOLE_API __declspec(dllexport)
#else
#define CONSOLE_API __declspec(dllimport)
#endif
#else
#define CONSOLE_API
#endif

// Project
#include "lock.hpp"
#include "exceptions.hpp"
#include "mail.hpp"

namespace db
{
    class CONSOLE_API console {
    public:

        struct rgb {
            rgb(unsigned char red, unsigned char green, unsigned char blue)
                : red(red), green(green), blue(blue) {};
            unsigned char red, green, blue;
        };

       /**
        * Construct a new console
        *
        * @param title the window title for the console
        * @param background the rgb colour to use for the background
        * @param buffers the number of buffers to reserve for storing i/o
        */
        console(int buffers);
        
       /**
        * Destroys internal resources used by the console
        */
        virtual ~console();

       /**
        * Show or hide the console
        *
        * @param visible whether or not the console should be visible
        */
        console& show(bool visible = true);

       /**
        * Toggles the visibility of console
        */
        console& toggle();

       /**
        * Sets a new title for the console
        *
        * @param text the new title to set
        */
        console& title(const wchar_t* text);

       /**
        * Sets a new icon for the console
        *
        * @param icon the new icon to set
        */
        console& icon(HICON handle);

       /**
        * Sets a new background colour
        *
        * @param colour the new background colour
        */
        console& background(rgb colour);

        /**
        * Resizes the console
        *
        * @param width the new width of the console
        * @param height the new height of the console
        */
        console& resize(int width, int height);

       /**
        * Returns the whether or not the console is visible
        */
        bool visible();

       /**
        * Write rich text to the console
        *
        * @param richtext the rich text to write to the console
        * @param timeout how long to wait, in milliseconds, for a successful write
        * @return whether or not the write succeeded
        */
        bool write(const std::wstring& richtext, unsigned long timeout);

       /**
        * Read text sent from the console
        *
        * @param buffer a buffer to hold the text
        * @param timeout how long to wait, in milliseconds, for a successful read
        * @return whether or not the read succeeded
        */
        bool read(std::wstring& buffer, unsigned long timeout);

    private:

        //
        // Internal thread context
        //
        bool _thread_initialize();
        bool _thread_messagepump();
        void _thread_handler_mail(mail::string& mail);
        LRESULT _thread_handler_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT _thread_handler_input(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        void _thread_resize(DWORD width, DWORD height);
        bool _thread_finalize();

        //
        // Windows callbacks
        //
        static LRESULT CALLBACK _callback_winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static DWORD CALLBACK _calback_threadproc(void* self);

        //
        // Helpers
        //
        static HINSTANCE _get_instance();
        static void _wndclass_register();
        static void _wndclass_unregister();

        //
        // Handles
        //
        HINSTANCE _hinstance;
        HANDLE _thread_console;
        HANDLE _event_initialized;
        HWND _hwnd_console;
        HWND _hwnd_console_input;
        HWND _hwnd_console_output;

        //
        // Message passing
        //
        mail _mail_input;
        mail _mail_output;

        //
        // Reference counting
        //
        static void _ref_acquire();
        static void _ref_release();
        static lock _ref_lock;
        static int _ref_count;
    };
}
