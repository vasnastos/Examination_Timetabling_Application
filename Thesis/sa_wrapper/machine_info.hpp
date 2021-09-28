#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <Lmcons.h>
// https://stackoverflow.com/questions/33282680/retrieve-the-name-of-the-computer-and-saved-it-in-a-variable
std::string get_computer_name()
{
    const int buffer_size = MAX_COMPUTERNAME_LENGTH + 1;
    char buffer[buffer_size];
    DWORD lpnSize = buffer_size;
    if (GetComputerNameA(buffer, &lpnSize) == FALSE)
        throw std::runtime_error("Something went wrong.");
    return std::string{ buffer };
}

std::string get_username()
{
    char username[UNLEN+1];
    DWORD username_len = UNLEN+1;
    GetUserName(username, &username_len);
    std::string person_login_credentials=username;
    return person_login_credentials;
}

#endif

#ifdef linux
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#define HOST_NAME_MAX 255

std::string get_computer_name()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    std::string computer_name=hostname;
    return computer_name;
}

std::string get_username()
{
    char username[HOST_NAME_MAX];
    getlogin_r(username,HOST_NAME_MAX);
    std::string user=username;
    return user;
}

#endif

std::string get_fullname()
{
    return get_username()+"_"+get_computer_name();
}