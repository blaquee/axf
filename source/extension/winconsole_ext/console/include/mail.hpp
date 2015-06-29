#pragma once

namespace db
{
    class mail {
    public:
        typedef std::wstring string;
        enum type { MESSAGE_Mail, MESSAGE_Windows, MESSAGE_Quit };
        
        struct message {
            type type;
            string mail;
            MSG windows;
            DWORD status;
        };

        mail(int mailboxes); virtual ~mail();
        bool send(const string& mail, unsigned long timeout);
        bool recv(string& buffer, unsigned long timeout);
        bool recv(message& message, unsigned long timeout);

    private:
        std::vector<string> _boxes;
        int _next_filled;
        int _next_empty;
        HANDLE _sem_empty;
        HANDLE _sem_filled;
        lock _lock;
    };
}
