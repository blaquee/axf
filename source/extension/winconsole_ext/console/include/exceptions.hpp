#pragma once

namespace db
{
    class win_exception : public std::exception {
    public:
        win_exception(DWORD error_code)
            : _error_code(error_code)
        {
            // Format the error message
            char* buffer = NULL;
            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                error_code,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&buffer),
                0,
                NULL
                );

            // Check result and store in string
            if (buffer) _error_string.assign(buffer);
        }

        virtual const char* what() const {
            return _error_string.c_str();
        }

        static void check_last_error() {
            DWORD last_error = GetLastError();
            if (last_error != ERROR_SUCCESS)
                throw win_exception(last_error);
        }

        static void check(HANDLE handle) {
            if (handle == NULL) check_last_error();
        }

    private:
        DWORD _error_code;
        std::string _error_string;
    };
}
