#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXTOKEN 256
#define MAXOUT 4096

enum {
    NAME,
    PARENS,
    BRACKETS,
    STAR,
    LPAREN,
    RPAREN,
    END
};

static char token[MAXTOKEN];
static int tokentype;
static const char *src;
static char name[MAXTOKEN];
static char datatype[MAXTOKEN];
static char out[MAXOUT];

static void nexttoken(void) {
    while (*src == ' ' || *src == '\t') {
        src++;
    }

    if (*src == '\0') {
        tokentype = END;
        token[0] = '\0';
        return;
    }

    if (isalpha((unsigned char)*src) || *src == '_') {
        int i = 0;
        while (isalnum((unsigned char)*src) || *src == '_') {
            if (i < MAXTOKEN - 1) {
                token[i++] = *src;
            }
            src++;
        }
        token[i] = '\0';
        tokentype = NAME;
        return;
    }

    if (*src == '(') {
        if (*(src + 1) == ')') {
            strcpy(token, "()");
            src += 2;
            tokentype = PARENS;
            return;
        }
        token[0] = *src++;
        token[1] = '\0';
        tokentype = LPAREN;
        return;
    }

    if (*src == ')') {
        token[0] = *src++;
        token[1] = '\0';
        tokentype = RPAREN;
        return;
    }

    if (*src == '[') {
        int i = 0;
        while (*src != '\0' && *src != ']') {
            if (i < MAXTOKEN - 1) {
                token[i++] = *src;
            }
            src++;
        }
        if (*src == ']' && i < MAXTOKEN - 1) {
            token[i++] = *src++;
        }
        token[i] = '\0';
        tokentype = BRACKETS;
        return;
    }

    if (*src == '*') {
        token[0] = *src++;
        token[1] = '\0';
        tokentype = STAR;
        return;
    }

    token[0] = *src++;
    token[1] = '\0';
    tokentype = END;
}

static void append(const char *s) {
    size_t have = strlen(out);
    size_t need = strlen(s);
    if (have + need + 1 < MAXOUT) {
        strcat(out, s);
    }
}

static void dcl(void);
static void dirdcl(void);

static void parse_param_list(char *buf, size_t bufsz) {
    int depth = 1;
    size_t i = 0;

    buf[0] = '\0';

    while (*src != '\0' && depth > 0) {
        char c = *src++;

        if (c == '(') {
            depth++;
        } else if (c == ')') {
            depth--;
            if (depth == 0) {
                break;
            }
        }

        if (depth > 0 && i + 1 < bufsz) {
            buf[i++] = c;
        }
    }

    buf[i] = '\0';

    if (depth != 0) {
        fprintf(stderr, "error: missing ')' in parameter list\n");
        exit(1);
    }

    while (i > 0 && isspace((unsigned char)buf[i - 1])) {
        buf[--i] = '\0';
    }

    {
        size_t start = 0;
        while (isspace((unsigned char)buf[start])) {
            start++;
        }
        if (start > 0) {
            memmove(buf, buf + start, strlen(buf + start) + 1);
        }
    }
}

static void dcl(void) {
    int ns = 0;

    while (tokentype == STAR) {
        ns++;
        nexttoken();
    }

    dirdcl();

    while (ns-- > 0) {
        append("pointer to ");
    }
}

static void dirdcl(void) {
    if (tokentype == LPAREN) {
        nexttoken();
        dcl();
        if (tokentype != RPAREN) {
            fprintf(stderr, "error: missing ')'\n");
            exit(1);
        }
        nexttoken();
    } else if (tokentype == NAME) {
        strncpy(name, token, MAXTOKEN - 1);
        name[MAXTOKEN - 1] = '\0';
        nexttoken();
    } else {
        fprintf(stderr, "error: expected name or (dcl)\n");
        exit(1);
    }

    while (tokentype == PARENS || tokentype == BRACKETS || tokentype == LPAREN) {
        if (tokentype == PARENS) {
            append("function returning ");
            nexttoken();
        } else if (tokentype == BRACKETS) {
            append("array");
            append(token);
            append(" of ");
            nexttoken();
        } else if (tokentype == LPAREN) {
            char params[MAXTOKEN];
            parse_param_list(params, sizeof(params));
            append("function(");
            append(params);
            append(") returning ");
            nexttoken();
        }
    }
}

static void parse_datatype(void) {
    datatype[0] = '\0';

    while (tokentype == NAME) {
        if (datatype[0] != '\0') {
            strncat(datatype, " ", MAXTOKEN - strlen(datatype) - 1);
        }
        strncat(datatype, token, MAXTOKEN - strlen(datatype) - 1);
        nexttoken();
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s \"declaration\"\n", argv[0]);
        return 1;
    }

    src = argv[1];
    name[0] = '\0';
    datatype[0] = '\0';
    out[0] = '\0';

    nexttoken();
    parse_datatype();
    dcl();

    if (tokentype != END) {
        fprintf(stderr, "error: unexpected trailing input near '%s'\n", token);
        return 1;
    }

    printf("%s is %s%s\n", name, out, datatype);
    return 0;
}