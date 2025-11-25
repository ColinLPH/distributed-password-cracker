#ifndef PASSWORD_H
#define PASSWORD_H

#include <string>

const char CHAR_SET[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "@#%^&*()_+-=.,:;?";  
    
constexpr auto CHAR_SET_SIZE = sizeof(CHAR_SET) - 1;

#endif // PASSWORD_H
