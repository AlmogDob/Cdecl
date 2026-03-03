#define ALMOG_STRING_MANIPULATION_IMPLEMENTATION
#include "Almog_String_Manipulation.h"

enum Type {
    IDENTIFIER,
    QUALIFIER,
    TYPE,
};

struct Token {
    char type;
    char string[ASM_MAX_LEN]; 
};

int top = -1;
struct Token stack[ASM_MAX_LEN];
struct Token this;

#define pop     stack[top--]
#define push(s) stack[++top] = (s)

char *declaration;
int declaration_cursor;

/* figure out the identifier type */
enum Type classify_string(void)
{
    char *s = this.string;
    if (asm_strncmp(s, "const", asm_length("const"))) {
        asm_strncpy(s, "read-only", asm_length("read-only"));
        return QUALIFIER;
    } else if (asm_strncmp(s, "volatile", asm_length("volatile"))) {
        return QUALIFIER;
    } else if (asm_strncmp(s, "void", asm_length("void"))) {
        return TYPE;
    } else if (asm_strncmp(s, "char", asm_length("char"))) {
        return TYPE;
    } else if (asm_strncmp(s, "signed", asm_length("signed"))) {
        return TYPE;
    } else if (asm_strncmp(s, "unsigned", asm_length("unsigned"))) {
        return TYPE;
    } else if (asm_strncmp(s, "short", asm_length("short"))) {
        return TYPE;
    } else if (asm_strncmp(s, "int", asm_length("int"))) {
        return TYPE;
    } else if (asm_strncmp(s, "long", asm_length("long"))) {
        return TYPE;
    } else if (asm_strncmp(s, "float", asm_length("float"))) {
        return TYPE;
    } else if (asm_strncmp(s, "double", asm_length("double"))) {
        return TYPE;
    } else if (asm_strncmp(s, "struct", asm_length("struct"))) {
        return TYPE;
    } else if (asm_strncmp(s, "union", asm_length("union"))) {
        return TYPE;
    } else if (asm_strncmp(s, "enum", asm_length("enum"))) {
        return TYPE;
    } else {
        return IDENTIFIER;
    }
}

char get_char_from_declaration(void)
{
    if (declaration_cursor >= (int)asm_length(declaration)) {
        return '\0';
    }
    return declaration[declaration_cursor++];
}

void unget_char_from_declaration(void)
{
    declaration_cursor--;
}

/* read next token into "this" */
void get_token(void)
{
    char *p = this.string;

    /* read past any spaces */
    while (asm_isspace(*p = get_char_from_declaration())) {
        ;
    }
    if (asm_isalnum(*p)) {
        /* it starts with A-Z, a-z, 0-9 read in identifier */
        while (asm_isalnum(*++p = get_char_from_declaration())) ;
        unget_char_from_declaration();
        *p = '\0';
        this.type = (char)classify_string();
        return;
    }
    if (*p == '*') {
        asm_strncpy(this.string, "pointer to", asm_length("pointer to"));
        this.type = '*';
        return;
    }
    this.string[1] = '\0';
    this.type = *p;
}

/* The piece of code that understand all parsing. */
void read_to_first_identifier(void)
{
    get_token();
    while (this.type != IDENTIFIER) {
        push(this);
        get_token();
    }
    printf("%s is ", this.string);
    get_token();
}

void deal_with_arrays(void) 
{
    while (this.type == '[') {
        printf("array ");
        get_token(); /* a number of ']' */
        if (asm_isdigit(this.string[0])) {
            printf("0..%d ", asm_str2int(this.string, NULL, 10) - 1);
            get_token(); /* read the ']' */
        }
        get_token(); /* read next past the ']' */
        printf("of ");
    }
}

void deal_with_function_args(void) 
{
    while (this.type != ')') {
        get_token();
    }
    get_token();
    printf("function returning ");
}

void deal_with_pointers(void)
{
    while (stack[top].type == '*') {
        printf("%s ", pop.string);
    }
}

/* deal with possible array/function following identifier */
void deal_with_declarator(void)
{
    switch (this.type) {
        case '[':
        {
            deal_with_arrays();
        } break;
        case '(':
        {
            deal_with_function_args();
        } break;
    }

    deal_with_pointers();

    /* process tokens that we stacked while reading identifier */
    while (top >= 0) {
        if (stack[top].type == '(') {
            ASM_UNUSED(pop);
            get_token(); /* read past ')' */
            deal_with_declarator();
        } else {
            printf("%s ", pop.string);
        }
    }
}

int main(int argc, char const *argv[])
{
    if (--argc != 1) {
        asm_dprintERROR("%s", "not right usage. Usage: 'temp.c' 'declaration'");
        return 1;
    }
    declaration_cursor = 0;
    declaration = (char *)argv[argc--];

    asm_dprintSTRING(declaration);

    read_to_first_identifier();
    deal_with_declarator();
    printf("\n");

    return 0;
}
