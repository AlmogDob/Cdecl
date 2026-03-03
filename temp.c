#include <stdio.h>
#define ALMOG_STRING_MANIPULATION_IMPLEMENTATION
#include "Almog_String_Manipulation.h"



int main(int argc, char const *argv[])
{
    if (--argc != 1) {
        asm_dprintERROR("%s", "not right usage. Usage: 'temp.c' 'declaration'");
        return 1;
    }
    char *declaration = (char *)argv[argc--];

    asm_dprintSTRING(declaration);

    return 0;
}
