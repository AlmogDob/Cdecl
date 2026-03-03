/**
 * @file
 * @brief Lightweight string and line manipulation helpers.
 *
 * @details
 * This single-header module provides small utilities for working with
 * C strings:
 *  - Reading a single line from a FILE stream
 *  - Measuring string length
 *  - Extracting the next token from a string using a delimiter
 *    (does not skip whitespace)
 *  - Cutting the extracted token from the source buffer (optionally also
 *    removing the delimiter)
 *  - Copying a substring by indices
 *  - Counting occurrences of a substring (see asm_str_in_str() notes for the
 *    exact scan pattern)
 *  - A boolean-style strncmp (returns 1 on equality, 0 otherwise)
 *  - Case-insensitive boolean-style strncmp (ASCII-only; see
 *    asm_strncmp_case_insensitive() notes/constraints)
 *  - ASCII-only character classification helpers (isalnum, isalpha, ...)
 *  - ASCII case conversion (toupper / tolower)
 *  - In-place whitespace stripping and left padding
 *  - Base-N string-to-number conversion for int, size_t, float, and double
 *
 * Usage
 *  - In exactly one translation unit, define
 *    ALMOG_STRING_MANIPULATION_IMPLEMENTATION before including this header
 *    to compile the implementation.
 *  - In all other files, include the header without the macro to get
 *    declarations only.
 *
 * Notes and limitations
 *  - All destination buffers must be large enough; functions do not grow or
 *    allocate buffers.
 *  - asm_get_line stores at most ASM_MAX_LEN - 1 characters (plus '\0').
 *    Lines longer than that cause an early return with an error message.
 *    asm_length uses ASM_MAX_LEN as a sanity limit when scanning for '\0'.
 *  - asm_strncmp differs from the standard C strncmp: this version returns
 *    1 if equal and 0 otherwise.
 *  - Character classification and case-conversion helpers are ASCII-only and
 *    not locale aware.
 */

#ifndef ALMOG_STRING_MANIPULATION_H_
#define ALMOG_STRING_MANIPULATION_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef ASM_MALLOC
#include <stdlib.h>
#define ASM_MALLOC malloc
#endif

#ifndef ASM_FREE
#include <stdlib.h>
#define ASM_FREE free
#endif

/**
 * @def ASM_MAX_LEN
 * @brief Maximum number of characters processed in some string operations.
 *
 * @details
 * This constant is used as a fixed, caller-provided buffer size / sanity
 * limit:
 *  - asm_get_line() writes at most ASM_MAX_LEN - 1 characters to the
 *    destination buffer and always reserves 1 byte for the terminating '\0'.
 *  - asm_length() uses ASM_MAX_LEN as a safety bound while searching for '\0'
 *    (it returns SIZE_MAX if no terminator is found within that bound).
 *
 * If asm_get_line reads ASM_MAX_LEN characters without encountering '\n' or
 * EOF, it prints an error to stderr and returns -1. In that error case, the
 * buffer is truncated and null-terminated by overwriting the last stored
 * character (so the resulting string length is ASM_MAX_LEN - 1).
 */
#ifndef ASM_MAX_LEN
#define ASM_MAX_LEN (int)1e3
#endif

/**
 * @def asm_dprintSTRING(expr)
 * @brief Debug-print a C string expression as "expr = value\n".
 *
 * @param expr An expression that yields a pointer to char (const or
 *             non-const). The expression is evaluated exactly once.
 */
#define asm_dprintSTRING(expr) printf(#expr " = %s\n", expr)

/**
 * @def asm_dprintCHAR(expr)
 * @brief Debug-print a character expression as "expr = c\n".
 *
 * @param expr An expression that yields a character (or an int promoted from
 *             a character). The expression is evaluated exactly once.
 */
#define asm_dprintCHAR(expr) printf(#expr " = %c\n", expr)

/**
 * @def asm_dprintINT(expr)
 * @brief Debug-print an integer expression as "expr = n\n".
 *
 * @param expr An expression that yields an int. The expression is evaluated
 *             exactly once.
 */
#define asm_dprintINT(expr) printf(#expr " = %d\n", expr)

/**
 * @def asm_dprintFLOAT(expr)
 * @brief Debug-print a float expression as "expr = n\n".
 *
 * @param expr An expression that yields a float. The expression is evaluated
 *             exactly once.
 */
#define asm_dprintFLOAT(expr) printf(#expr " = %#g\n", expr)

/**
 * @def asm_dprintDOUBLE(expr)
 * @brief Debug-print a double expression as "expr = n\n".
 *
 * @param expr An expression that yields a double. The expression is evaluated
 *             exactly once.
 */
#define asm_dprintDOUBLE(expr) printf(#expr " = %#g\n", expr)

/**
 * @def asm_dprintSIZE_T(expr)
 * @brief Debug-print a size_t expression as "expr = n\n".
 *
 * @param expr An expression that yields a size_t. The expression is evaluated
 *             exactly once.
 */
#define asm_dprintSIZE_T(expr) printf(#expr " = %zu\n", expr)

/**
 * @def asm_dprintERROR(fmt, ...)
 * @brief Print a formatted error message to stderr with file/line/function
 *        context.
 *
 * @param fmt printf-style format string.
 * @param ... printf-style arguments for @p fmt.
 *
 * @note This macro requires at least one variadic argument in addition to
 *       @p fmt (because it unconditionally uses __VA_ARGS__).
 */
#define asm_dprintERROR(fmt, ...) \
    fprintf(stderr, "\n%s:%d:\n[Error] in function '%s':\n        " \
    fmt "\n\n", __FILE__, __LINE__, __func__, __VA_ARGS__)

/**
 * @def asm_min
 * @brief Return the smaller of two values (macro).
 *
 * @param a First value.
 * @param b Second value.
 * @return The smaller of @p a and @p b.
 *
 * @note Each parameter may be evaluated more than once. Do not pass
 *       expressions with side effects (e.g., ++i, function calls with state).
 */
#define asm_min(a, b) ((a) < (b) ? (a) : (b))

/**
 * @def asm_max
 * @brief Return the larger of two values (macro).
 *
 * @param a First value.
 * @param b Second value.
 * @return The larger of @p a and @p b.
 *
 * @note Each parameter may be evaluated more than once. Do not pass
 *       expressions with side effects (e.g., ++i, function calls with state).
 */
#define asm_max(a, b) ((a) > (b) ? (a) : (b))

bool    asm_check_char_belong_to_base(const char c, const size_t base);
void    asm_copy_array_by_indexes(char * const target, const int start, const int end, const char * const src);
int     asm_get_char_value_in_base(const char c, const size_t base);
int     asm_get_line(FILE *fp, char * const dst);
int     asm_get_next_token_from_str(char * const dst, const char * const src, const char delimiter);
int     asm_get_token_and_cut(char * const dst, char *src, const char delimiter, const bool leave_delimiter);
bool    asm_isalnum(const char c);
bool    asm_isalpha(const char c);
bool    asm_isbdigit(const char c);
bool    asm_iscntrl(const char c);
bool    asm_isdigit(const char c);
bool    asm_isgraph(const char c);
bool    asm_islower(const char c);
bool    asm_isodigit(const char c);
bool    asm_isprint(const char c);
bool    asm_ispunct(const char c);
bool    asm_isspace(const char c);
bool    asm_isupper(const char c);
bool    asm_isxdigit(const char c);
bool    asm_isXdigit(const char c);
#define asm_length(str) __asm_length(str, __FILE__, __LINE__, __func__)
size_t  __asm_length(const char * const str, char *file_name, int line_num, char *function_name);
void *  asm_memset(void * const des, const unsigned char value, const size_t n);
void    asm_pad_left(char * const s, const size_t padding, const char pad);
void    asm_print_many_times(const char * const str, const size_t n);
void    asm_remove_char_from_string(char * const s, const size_t index);
void    asm_shift_left(char * const s, const size_t shift);
int     asm_str_in_str(const char * const src, const size_t src_len, const char * const word_to_search, const char **first_occurrence);
int     asm_str_in_str_case_insensitive(const char * const src, const size_t src_len, const char * const word_to_search, const char **first_occurrence);
double  asm_str2double(const char * const s, const char ** const end, const size_t base);
float   asm_str2float(const char * const s, const char ** const end, const size_t base);
int     asm_str2int(const char * const s, const char ** const end, const size_t base);
size_t  asm_str2size_t(const char * const s, const char ** const end, const size_t base);
void    asm_strip_whitespace(char * const s);
bool    asm_str_is_whitespace(const char * const s);
char *  asm_strdup(const char * const s, size_t length);
int     asm_strncat(char * const s1, const char * const s2, const size_t N);
int     asm_strncmp(const char * const s1, const char * const s2, const size_t N);
int     asm_strncmp_case_insensitive(const char * const s1, const char * const s2, const size_t N);
int     asm_strncpy(char * const s1, const char * const s2, const size_t N);
void    asm_tolower(char * const s, const size_t len);
void    asm_toupper(char * const s, const size_t len);
void    asm_trim_left_whitespace(char *s);

#endif /*ALMOG_STRING_MANIPULATION_H_*/

#ifdef ALMOG_STRING_MANIPULATION_IMPLEMENTATION
#undef ALMOG_STRING_MANIPULATION_IMPLEMENTATION

/**
 * @brief Check if a character is a valid digit in a given base.
 *
 * @param c    Character to test (e.g., '0'–'9', 'a'–'z', 'A'–'Z').
 * @param base Numeric base in the range [2, 36].
 * @return true if @p c is a valid digit for @p base, false otherwise.
 *
 * @note If @p base is outside [2, 36], an error is printed to stderr and
 *       false is returned.
 */
bool asm_check_char_belong_to_base(const char c, const size_t base)
{
    if (base > 36 || base < 2) {
        #ifndef ASM_NO_ERRORS
        asm_dprintERROR("Supported bases are [2...36]. Inputted: %zu", base);
        #endif
        return false;
    }
    if (base <= 10) {
        return c >= '0' && c <= '9'+(char)base-10;
    }
    if (base > 10) {
        return asm_isdigit(c) || (c >= 'A' && c <= ('A'+(char)base-11)) || (c >= 'a' && c <= ('a'+(char)base-11));
    }

    return false;
}

/**
 * @brief Copy a substring from @p src into @p target by indices and
 *        null-terminate.
 *
 * Copies characters with indices i = start, start + 1, ..., end from @p src
 * into @p target (note: @p end is inclusive in this implementation), then
 * ensures @p target is null-terminated.
 *
 * @param target Destination buffer. Must be large enough to hold
 *               (end - start + 1) characters plus the null terminator.
 * @param start  Inclusive start index within @p src (0-based).
 * @param end    Inclusive end index within @p src (must satisfy end >= start).
 * @param src    Source string buffer.
 *
 * @warning No bounds checking is performed. The caller must ensure valid
 *          indices and sufficient target capacity.
 * @note If @p start > @p end, this function returns immediately and leaves
 *       @p target unchanged (it does not write a terminator in that case).
 * @note If the copied range includes a '\0' from @p src, @p target will also
 *       contain that '\0' at the corresponding position and no extra '\0' is
 *       appended.
 */
void asm_copy_array_by_indexes(char * const target, const int start, const int end, const char * const src)
{
    if (start > end) return;
    int j = 0;
    for (int i = start; i <= end; i++) {
        target[j] = src[i];
        j++;
    }
    if (target[j-1] != '\0') {
        target[j] = '\0';
    }
}

/**
 * @brief Convert a digit character to its numeric value in base-N.
 *
 * @param c Digit character ('0'–'9', 'a'–'z', 'A'–'Z').
 * @param base Numeric base in the range [2, 36] (used for validation).
 * @return The numeric value of @p c in the range [0, 35].
 *
 * @note Returns -1 if @p c is not valid for @p base.
 */
int asm_get_char_value_in_base(const char c, const size_t base)
{
    if (!asm_check_char_belong_to_base(c, base)) return -1;
    if (asm_isdigit(c)) {
        return c - '0';
    } else if (asm_isupper(c)) {
        return c - 'A' + 10;
    } else {
        return c - 'a' + 10;
    }
}

/**
 * @brief Read a single line from a stream into a buffer.
 *
 * Reads characters from the FILE stream until a newline ('\n') or EOF is
 * encountered. The newline, if present, is not copied. The result is
 * always null-terminated on normal (non-error) completion.
 *
 * @param fp  Input stream (must be non-NULL).
 * @param dst Destination buffer. Must have capacity of at least ASM_MAX_LEN bytes.
 * @return Number of characters stored in @p dst (excluding the terminating
 *         null byte).
 * @retval -1 EOF was encountered before any character was read, or the line
 *         exceeded ASM_MAX_LEN characters (error).
 *
 * @note If the line reaches ASM_MAX_LEN characters before a newline or EOF
 *       is seen, the function prints an error message to stderr and returns
 *       -1. In that case, @p dst is truncated and null-terminated by
 *       overwriting the last stored character.
 * @note On the "line too long" error path, this function returns immediately
 *       after truncating @p dst and does not consume the rest of the current
 *       line from @p fp. A subsequent call will continue reading from the
 *       same (still-unfinished) line.
 * @note An empty line (just '\n') returns 0 (not -1).
 */
int asm_get_line(FILE *fp, char * const dst)
{
    int i = 0;
    int c;
    while ((c = fgetc(fp)) != '\n' && c != EOF) {
        dst[i++] = (char)c;
        if (i >= ASM_MAX_LEN) {
            #ifndef ASM_NO_ERRORS
            asm_dprintERROR("%s", "index exceeds ASM_MAX_LEN. Line in file is too long.");
            #endif
            dst[i-1] = '\0';
            return -1;
        }
    }
    dst[i] = '\0';
    if (c == EOF && i == 0) {
        return -1;
    }
    return i;
}

/**
 * @brief Copy characters from the start of a string into a token buffer.
 *
 * Copies characters from @p src into @p dst until one of the following is
 * encountered in @p src:
 *  - the delimiter character,
 *  - or the string terminator ('\0').
 *
 * The delimiter (if present) is not copied into @p dst. The resulting token in
 * @p dst is always null-terminated.
 *
 * @param dst       Destination buffer for the extracted token. Must be large
 *                  enough to hold the token plus the null terminator.
 * @param src       Source C string to parse (not modified by this function).
 * @param delimiter Delimiter character to stop at.
 *
 * @return The number of characters copied into @p dst (excluding the null
 *         terminator). This is also the index in @p src of the delimiter or
 *         '\0' that stopped the copy.
 *
 * @note This function does not skip leading whitespace and does not treat
 *       newline ('\n') specially; newlines are copied like any other character.
 *
 * @note If @p src starts with @p delimiter or '\0', an empty token is produced
 *       (@p dst becomes ""), and 0 is returned.
 */
int asm_get_next_token_from_str(char * const dst, const char * const src, const char delimiter)
{
    int i = 0, j = 0;
    char c;
    while ((c = src[i]) != delimiter && c != '\0') {
        dst[j++] = src[i++];
    }

    dst[j] = '\0';

    return j;
}

/**
 * @brief Extract the next token into @p dst and remove the corresponding prefix
 *        from @p src.
 *
 * Calls asm_get_next_token_from_str(@p dst, @p src, @p delimiter) to extract a
 * token from the beginning of @p src into @p dst. Then modifies @p src in-place
 * by left-shifting it.
 *
 *  - If @p leave_delimiter is true:
 *      - @p src is shifted left by the token length.
 *      - If a delimiter was present, it becomes the first character of the
 *        updated @p src.
 *  - If @p leave_delimiter is false:
 *      - If a delimiter is present immediately after the token, @p src is
 *        shifted left by (token length + 1), removing exactly one delimiter.
 *      - If no delimiter is present (the token reaches '\0'), @p src is set
 *        to the empty string.
 *
 * @param dst             Destination buffer for the extracted token (must be
 *                        large enough for the token plus the null terminator).
 * @param src             Source buffer, modified in-place by this function.
 * @param delimiter       Delimiter character used to stop token extraction.
 * @param leave_delimiter If true, do not remove the delimiter from @p src; if
 *                        false, remove one additional character after the token.
 *
 * @return 1 if a non-empty token was extracted (token length != 0),
 *         otherwise 0. (Note: an empty token may still cause @p src to change,
 *         e.g., when @p src begins with the delimiter and @p leave_delimiter is
 *         false, the delimiter is removed but 0 is returned.)
 *
 * @note This function does not skip whitespace. Any leading whitespace is part
 *       of the extracted token (until the delimiter or '\0').
 */
int asm_get_token_and_cut(char * const dst, char *src, const char delimiter, const bool leave_delimiter)
{
    int new_src_start_index = asm_get_next_token_from_str(dst, src, delimiter);
    bool delimiter_at_start = src[new_src_start_index] == delimiter;

    if (leave_delimiter) {
        asm_shift_left(src, new_src_start_index);
    } else if (delimiter_at_start) {
        asm_shift_left(src, new_src_start_index + 1);
    } else {
        src[0] = '\0';
    }
    return new_src_start_index ? 1 : 0;
}

/**
 * @brief Test for an alphanumeric character (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is '0'–'9', 'A'–'Z', or 'a'–'z'; false otherwise.
 */
bool asm_isalnum(char c)
{
    return asm_isalpha(c) || asm_isdigit(c);
}

/**
 * @brief Test for an alphabetic character (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is 'A'–'Z' or 'a'–'z'; false otherwise.
 */
bool asm_isalpha(char c)
{
    return asm_isupper(c) || asm_islower(c);
}

/**
 * @brief Test for a binary digit (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is '0' or '1'; false otherwise.
 */
bool asm_isbdigit(const char c)
{
    if (c == '0' || c == '1') {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for a control character (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is in the range [0, 31] or 127; false otherwise.
 */
bool asm_iscntrl(char c)
{
    if ((c >= 0 && c <= 31) || c == 127) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for a decimal digit (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is '0'–'9'; false otherwise.
 */
bool asm_isdigit(char c)
{
    if (c >= '0' && c <= '9') {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for any printable character except space (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is in the range [33, 126]; false otherwise.
 */
bool asm_isgraph(char c)
{
    if (c >= 33 && c <= 126) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for a lowercase letter (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is 'a'–'z'; false otherwise.
 */
bool asm_islower(char c)
{
    if (c >= 'a' && c <= 'z') {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for an octal digit (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is '0'–'7'; false otherwise.
 */
bool asm_isodigit(const char c)
{
    if ((c >= '0' && c <= '7')) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for any printable character including space (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is space (' ') or asm_isgraph(c) is true; false
 *         otherwise.
 */
bool asm_isprint(char c)
{
    return asm_isgraph(c) || c == ' ';
}

/**
 * @brief Test for a punctuation character (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is a printable, non-alphanumeric, non-space
 *         character; false otherwise.
 */
bool asm_ispunct(char c)
{
    if ((c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126)) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for a whitespace character (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is one of ' ', '\n', '\t', '\v', '\f', or '\r';
 *         false otherwise.
 */
bool asm_isspace(char c)
{
    if (c == ' ' || c == '\n' || c == '\t' ||
        c == '\v'|| c == '\f' || c == '\r') {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for an uppercase letter (ASCII).
 *
 * @param c Character to test.
 * @return true if @p c is 'A'–'Z'; false otherwise.
 */
bool asm_isupper(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for a hexadecimal digit (lowercase or decimal).
 *
 * @param c Character to test.
 * @return true if @p c is '0'–'9' or 'a'–'f'; false otherwise.
 */
bool asm_isxdigit(char c)
{
    if ((c >= 'a' && c <= 'f') || asm_isdigit(c)) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Test for a hexadecimal digit (uppercase or decimal).
 *
 * @param c Character to test.
 * @return true if @p c is '0'–'9' or 'A'–'F'; false otherwise.
 */
bool asm_isXdigit(char c)
{
    if ((c >= 'A' && c <= 'F') || asm_isdigit(c)) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Compute the length of a null-terminated C string.
 *
 * @param str Null-terminated string (must be non-NULL).
 * @return The number of characters before the terminating null byte.
 *
 * @note If more than ASM_MAX_LEN characters are scanned without encountering
 *       a null terminator, an error is printed to stderr and __SIZE_MAX__
 *       is returned.
 */
size_t  __asm_length(const char * const str, char *file_name, int line_num, char *function_name)
{
    char c;
    size_t i = 0;

    while ((c = str[i++]) != '\0') {
        if (i > ASM_MAX_LEN) {
            #ifndef ASM_NO_ERRORS
            asm_dprintERROR("index exceeds ASM_MAX_LEN. Probably no NULL termination.\nCalled in function: '%s' in: %s:%d", function_name, file_name, line_num);
            #endif
            return SIZE_MAX;
        }
    }
    return --i;
}

/**
 * @brief Set a block of memory to a repeated byte value.
 *
 * Writes @p value into each of the first @p n bytes of the memory region
 * pointed to by @p des. This function mirrors the behavior of the standard
 * C memset(), but implements it using a simple byte-wise loop.
 *
 * @param des   Destination memory block to modify. Must point to a valid
 *              buffer of at least @p n bytes.
 * @param value Unsigned byte value to store repeatedly.
 * @param n     Number of bytes to set.
 *
 * @return The original pointer @p des.
 *
 * @note This implementation performs no optimizations (such as word-sized
 *       writes); the memory block is filled one byte at a time.
 * @note Behavior is undefined if @p des overlaps with invalid or
 *       non-writable memory.
 */
void * asm_memset(void * const des, const unsigned char value, const size_t n)
{
    unsigned char *ptr = (unsigned char *)des;
    for (size_t i = n; i-- > 0;) {
        *ptr++ = value;
    }
    return des;
}

/**
 * @brief Left-pad a string in-place.
 *
 * Shifts the contents of @p s to the right by @p padding positions and
 * fills the vacated leading positions with @p pad.
 *
 * @param s       String to pad. Modified in-place.
 * @param padding Number of leading pad characters to insert.
 * @param pad     The padding character to insert.
 *
 * @warning The buffer backing @p s must have enough capacity for the
 *          original string length plus @p padding and the terminating
 *          null byte. No bounds checking is performed.
 */
void asm_pad_left(char * const s, const size_t padding, const char pad)
{
    int len = (int)asm_length(s);
    for (int i = len; i >= 0; i--) {
        s[i+(int)padding] = s[i];
    }
    for (int i = 0; i < (int)padding; i++) {
        s[i] = pad;
    }
}

/**
 * @brief Print a string @p n times, then print a newline.
 *
 * @param str String to print (as-is with printf("%s", ...)).
 * @param n   Number of times to print @p str.
 */
void asm_print_many_times(const char * const str, const size_t n)
{
    for (size_t i = 0; i < n; i++) {
        printf("%s", str);
    }
    printf("\n");
}

/**
 * @brief Remove a single character from a string by index.
 *
 * Deletes the character at position @p index from @p s by shifting
 * subsequent characters one position to the left.
 *
 * @param s     String to modify in-place. Must be null-terminated.
 * @param index Zero-based index of the character to remove.
 *
 * @note If @p index is out of range, an error is printed to stderr and
 *       the string is left unchanged.
 */
void asm_remove_char_from_string(char * const s, const size_t index)
{
    size_t len = asm_length(s);
    if (len == 0) return;
    if (index >= len) {
        #ifndef ASM_NO_ERRORS
        asm_dprintERROR("%s", "index exceeds array length.");
        #endif
        return;
    }

    for (size_t i = index; i < len; i++) {
        s[i] = s[i+1];
    }
}

/**
 * @brief Shift a string left in-place by @p shift characters.
 *
 * Removes the first @p shift characters from @p s by moving the remaining
 * characters to the front. The resulting string is always null-terminated.
 *
 * @param s     String to modify in-place. Must be null-terminated.
 * @param shift Number of characters to remove from the front.
 *
 * @note If @p shift is 0, @p s is unchanged.
 * @note If @p shift is greater than or equal to the string length,
 *       @p s becomes the empty string.
 */
void asm_shift_left(char * const s, const size_t shift)
{
    size_t len = asm_length(s);

    if (shift == 0) return;
    if (len <= shift) {
        s[0] = '\0';
        return;
    }

    size_t i;
    for (i = shift; i < len; i++) {
        s[i-shift] = s[i];
    }
    s[i-shift] = '\0';
}

/**
 * @brief Count occurrences of a substring within a string.
 *
 * Scans @p src from left to right and tests for a match of @p word_to_search
 * starting at each index \(i = 0, 1, 2, \dots\) until either:
 *  - a terminating '\0' is encountered in @p src, or
 *  - \(i\) reaches the configured cap.
 *
 * On each index \(i\), this function checks equality using
 * asm_strncmp(src + i, word_to_search, asm_length(word_to_search)).
 * Matches may overlap.
 *
 * @param src             String to search in (must be null-terminated).
 * @param src_len         If non-zero, limits the scan to start indices
 *                        \(i < \) @p src_len (the scan also stops earlier at
 *                        '\0'). If zero, ASM_MAX_LEN is used as the cap.
 * @param word_to_search  Substring to find (must be null-terminated).
 * @param first_occurrence Output parameter. On the first match found, this
 *                         function stores a pointer to the matched position
 *                         within @p src into *@p first_occurrence.
 *
 * @return The number of matches found. Matches may overlap.
 *
 * @note If no matches are found, *@p first_occurrence is left unchanged. If
 *       you need a deterministic “not found” value, initialize it to NULL
 *       before calling.
 */
int asm_str_in_str(const char * const src, const size_t src_len, const char * const word_to_search, const char **first_occurrence)
{
    size_t word_to_search_len = asm_length(word_to_search);
    if (word_to_search_len == 0) {
        if (first_occurrence) *first_occurrence = (const char *)src;
        return 0;
    }
    size_t num_of_accur = 0;
    size_t n = src_len == 0 ? ASM_MAX_LEN : src_len;
    for (size_t i = 0; src[i] != '\0' && i < n - word_to_search_len; i++) {
        if (asm_strncmp(src+i, word_to_search, asm_length(word_to_search))) {
            num_of_accur++;
            if (num_of_accur == 1) {
                if (first_occurrence) *first_occurrence = &(src[i]);
            }
        }
    }
    return (int)num_of_accur;
}

/**
 * @brief Count occurrences of a substring within a string, case-insensitively
 *        (ASCII-only; even indices only).
 *
 * Scans @p src from left to right and tests for a match of @p word_to_search
 * starting at each index \(i = 0, 1, 2, \dots\) until either:
 *  - a terminating '\0' is encountered in @p src, or
 *  - \(i\) reaches the configured cap.
 *
 * Matching is performed by calling:
 * asm_strncmp_case_insensitive(src + i, word_to_search,
 *                              asm_length(word_to_search)).
 *
 * @param src              String to search in (must be null-terminated).
 * @param src_len          If non-zero, limits the scan to start indices
 *                         \(i <\) @p src_len (the scan also stops earlier at
 *                         '\0'). If zero, ASM_MAX_LEN is used as the cap.
 * @param word_to_search   Substring to find (must be null-terminated).
 * @param first_occurrence Output parameter. On the first match found, this
 *                         function stores a pointer to the matched position
 *                         within @p src into *@p first_occurrence.
 *
 * @return The number of matches found. Matches may overlap.
 *
 * @warning Due to the current implementation of asm_strncmp_case_insensitive()
 *          (it lowercases exactly N bytes in temporary buffers), this function
 *          can invoke undefined behavior when a tested position @p src + i is
 *          shorter than N bytes (i.e., too close to the terminating '\0').
 *          To keep behavior defined, the caller should bound the scan so that
 *          every tested start index has at least N characters available, e.g.:
 *          let needle_len = asm_length(word_to_search) and hay_len = asm_length(src);
 *          then use src_len <= (hay_len >= needle_len ? hay_len - needle_len + 1 : 0).
 *
 * @note If no matches are found, *@p first_occurrence is left unchanged. If you
 *       need a deterministic “not found” value, initialize it to NULL before
 *       calling.
 * @note ASCII-only: locale-specific case mappings are not supported.
 */
int asm_str_in_str_case_insensitive(const char * const src, const size_t src_len, const char * const word_to_search, const char **first_occurrence)
{
    size_t word_to_search_len = asm_length(word_to_search);
    if (word_to_search_len == 0) {
        if (first_occurrence) *first_occurrence = (const char *)src;
        return 0;
    }
    size_t num_of_accur = 0;
    size_t n = src_len == 0 ? ASM_MAX_LEN : src_len;
    for (size_t i = 0; src[i] != '\0' && i < n - word_to_search_len; i++) {
        if (asm_strncmp_case_insensitive(src+i, word_to_search, asm_length(word_to_search))) {
            num_of_accur++;
            if (num_of_accur == 1) {
                if (first_occurrence) *first_occurrence = &(src[i]);
            }
        }
    }
    return (int)num_of_accur;
}

/**
 * @brief Convert a string to double in the given base with exponent support.
 *
 * Parses an optional sign, then a sequence of base-N digits, optionally
 * a fractional part separated by a '.' character, and optionally an
 * exponent part indicated by 'e' or 'E' followed by an optional sign
 * and decimal digits.
 *
 * @param s    String to convert. Leading ASCII whitespace is skipped.
 * @param end  If non-NULL, *end is set to point to the first character
 *             not used in the conversion.
 * @param base Numeric base in the range [2, 36].
 * @return The converted double value. Returns 0.0 on invalid base.
 *
 * @note Only digits '0'–'9', 'a'–'z', and 'A'–'Z' are recognized as
 *       base-N digits for the mantissa (the part before the exponent).
 * @note The exponent is always parsed in base 10 and represents the
 *       power of the specified base. For example, "1.5e2" in base 10
 *       means 1.5 * 10^2 = 150, while "A.8e2" in base 16 means
 *       10.5 * 16^2 = 2688.
 * @note The exponent is parsed via asm_str2int(), which skips leading ASCII
 *       whitespace. As a result, strings like "1e 2" may be accepted (expo=2)
 *       even though standard C conversions typically stop at the 'e'.
 * @note The exponent can be positive or negative (e.g., "1e-3" = 0.001).
 * @note On invalid base, an error is printed to stderr, *end (if non-NULL)
 *       is set to @p s, and 0.0 is returned.
 *
 * @par Examples:
 * @code
 * asm_str2double("1.5e2", NULL, 10)    // Returns 150.0
 * asm_str2double("-3.14e-1", NULL, 10) // Returns -0.314
 * asm_str2double("FF.0e1", NULL, 16)   // Returns 4080.0 (255 × 16^1)
 * @endcode
 */
double asm_str2double(const char * const s, const char ** const end, const size_t base)
{
    if (base < 2 || base > 36) {
        #ifndef ASM_NO_ERRORS
        asm_dprintERROR("Supported bases are [2...36]. Input: %zu", base);
        #endif
        if (end) *end = s;
        return 0.0;
    }
    int num_of_whitespace = 0;
    while (asm_isspace(s[num_of_whitespace])) {
        num_of_whitespace++;
    }

    int i = 0;
    if (s[0+num_of_whitespace] == '-' || s[0+num_of_whitespace] == '+') {
        i++;
    }
    int sign = s[0+num_of_whitespace] == '-' ? -1 : 1;

    size_t left = 0;
    double right = 0.0;
    int expo = 0;
    for (; asm_check_char_belong_to_base(s[i+num_of_whitespace], base); i++) {
        left = base * left + asm_get_char_value_in_base(s[i+num_of_whitespace], base);
    }

    if (s[i+num_of_whitespace] == '.') {
        i++; /* skip the point */

        size_t divider = base;
        for (; asm_check_char_belong_to_base(s[i+num_of_whitespace], base); i++) {
            right = right + asm_get_char_value_in_base(s[i+num_of_whitespace], base) / (double)divider;
            divider *= base;
        }
    }

    if ((s[i+num_of_whitespace] == 'e') || (s[i+num_of_whitespace] == 'E')) {
        expo = asm_str2int(&(s[i+num_of_whitespace+1]), end, 10);
    } else {
        if (end) *end = s + i + num_of_whitespace;
    }

    double res = sign * (left + right);

    if (expo > 0) {
        for (int index = 0; index < expo; index++) {
            res *= (double)base;
        }
    } else {
        for (int index = 0; index > expo; index--) {
            res /= (double)base;
        }
    }

    return res;
}

/**
 * @brief Convert a string to float in the given base with exponent support.
 *
 * Identical to asm_str2double semantically, but returns a float and uses
 * float arithmetic for the fractional part.
 *
 * @param s    String to convert. Leading ASCII whitespace is skipped.
 * @param end  If non-NULL, *end is set to point to the first character
 *             not used in the conversion.
 * @param base Numeric base in the range [2, 36].
 * @return The converted float value. Returns 0.0f on invalid base.
 *
 * @note Only digits '0'–'9', 'a'–'z', and 'A'–'Z' are recognized as
 *       base-N digits for the mantissa (the part before the exponent).
 * @note The exponent is always parsed in base 10 and represents the
 *       power of the specified base. For example, "1.5e2" in base 10
 *       means 1.5 * 10^2 = 150, while "A.8e2" in base 16 means
 *       10.5 * 16^2 = 2688.
 * @note The exponent is parsed via asm_str2int(), which skips leading ASCII
 *       whitespace. As a result, strings like "1e 2" may be accepted (expo=2)
 *       even though standard C conversions typically stop at the 'e'.
 * @note The exponent can be positive or negative (e.g., "1e-3" = 0.001).
 * @note On invalid base, an error is printed to stderr, *end (if non-NULL)
 *       is set to @p s, and 0.0f is returned.
 *
 * @par Examples:
 * @code
 * asm_str2float("1.5e2", NULL, 10)    // Returns 150.0f
 * asm_str2float("-3.14e-1", NULL, 10) // Returns -0.314f
 * asm_str2float("FF.0e1", NULL, 16)   // Returns 4080.0f (255 × 16^1)
 * @endcode
 */
float asm_str2float(const char * const s, const char ** const end, const size_t base)
{
    if (base < 2 || base > 36) {
        #ifndef ASM_NO_ERRORS
        asm_dprintERROR("Supported bases are [2...36]. Input: %zu", base);
        #endif
        if (end) *end = s;
        return 0.0f;
    }
    int num_of_whitespace = 0;
    while (asm_isspace(s[num_of_whitespace])) {
        num_of_whitespace++;
    }

    int i = 0;
    if (s[0+num_of_whitespace] == '-' || s[0+num_of_whitespace] == '+') {
        i++;
    }
    int sign = s[0+num_of_whitespace] == '-' ? -1 : 1;

    int left = 0;
    float right = 0.0f;
    int expo = 0;
    for (; asm_check_char_belong_to_base(s[i+num_of_whitespace], base); i++) {
        left = (int)base * left + asm_get_char_value_in_base(s[i+num_of_whitespace], base);
    }

    if (s[i+num_of_whitespace] == '.') {
        i++; /* skip the point */

        size_t divider = base;
        for (; asm_check_char_belong_to_base(s[i+num_of_whitespace], base); i++) {
            right = right + asm_get_char_value_in_base(s[i+num_of_whitespace], base) / (float)divider;
            divider *= base;
        }
    }

    if ((s[i+num_of_whitespace] == 'e') || (s[i+num_of_whitespace] == 'E')) {
        expo = asm_str2int(&(s[i+num_of_whitespace+1]), end, 10);
    } else {
        if (end) *end = s + i + num_of_whitespace;
    }

    float res = sign * (left + right);

    if (expo > 0) {
        for (int index = 0; index < expo; index++) {
            res *= (float)base;
        }
    } else {
        for (int index = 0; index > expo; index--) {
            res /= (float)base;
        }
    }

    return res;
}

/**
 * @brief Convert a string to int in the given base.
 *
 * Parses an optional sign and then a sequence of base-N digits.
 *
 * @param s    String to convert. Leading ASCII whitespace is skipped.
 * @param end  If non-NULL, *end is set to point to the first character
 *             not used in the conversion.
 * @param base Numeric base in the range [2, 36].
 * @return The converted int value. Returns 0 on invalid base.
 *
 * @note Only digits '0'–'9', 'a'–'z', and 'A'–'Z' are recognized as
 *       base-N digits.
 * @note On invalid base, an error is printed to stderr,
 *       *end (if non-NULL) is set to @p s, and 0 is returned.
 */
int asm_str2int(const char * const s, const char ** const end, const size_t base)
{
    if (base < 2 || base > 36) {
        #ifndef ASM_NO_ERRORS
        asm_dprintERROR("Supported bases are [2...36]. Input: %zu", base);
        #endif
        if (end) *end = s;
        return 0;
    }
    int num_of_whitespace = 0;
    while (asm_isspace(s[num_of_whitespace])) {
        num_of_whitespace++;
    }

    int n = 0, i = 0;
    if (s[0+num_of_whitespace] == '-' || s[0+num_of_whitespace] == '+') {
        i++;
    }
    int sign = s[0+num_of_whitespace] == '-' ? -1 : 1;

    for (; asm_check_char_belong_to_base(s[i+num_of_whitespace], base); i++) {
        n = (int)base * n + asm_get_char_value_in_base(s[i+num_of_whitespace], base);
    }

    if (end) *end = s + i+num_of_whitespace;

    return n * sign;
}

/**
 * @brief Convert a string to size_t in the given base.
 *
 * Parses an optional leading '+' sign, then a sequence of base-N digits.
 * Negative numbers are rejected.
 *
 * @param s    String to convert. Leading ASCII whitespace is skipped.
 * @param end  If non-NULL, *end is set to point to the first character
 *             not used in the conversion.
 * @param base Numeric base in the range [2, 36].
 * @return The converted size_t value. Returns 0 on invalid base or if
 *         a negative sign is encountered.
 *
 * @note On invalid base or a negative sign, an error is printed to stderr,
 *       *end (if non-NULL) is set to @p s, and 0 is returned.
 */
size_t asm_str2size_t(const char * const s, const char ** const end, const size_t base)
{
    if (end) *end = s;

    int num_of_whitespace = 0;
    while (asm_isspace(s[num_of_whitespace])) {
        num_of_whitespace++;
    }

    if (s[0+num_of_whitespace] == '-') {
        #ifndef ASM_NO_ERRORS
        asm_dprintERROR("%s", "Unable to convert a negative number to size_t.");
        #endif
        return 0;
    }

    if (base < 2 || base > 36) {
        #ifndef ASM_NO_ERRORS
        asm_dprintERROR("Supported bases are [2...36]. Input: %zu", base);
        #endif
        if (end) *end = s+num_of_whitespace;
        return 0;
    }

    size_t n = 0, i = 0;
    if (s[0+num_of_whitespace] == '+') {
        i++;
    }

    for (; asm_check_char_belong_to_base(s[i+num_of_whitespace], base); i++) {
        n = base * n + asm_get_char_value_in_base(s[i+num_of_whitespace], base);
    }

    if (end) *end = s + i+num_of_whitespace;

    return n;
}

/**
 * @brief Remove all ASCII whitespace characters from a string in-place.
 *
 * Scans @p s and deletes all characters for which asm_isspace() is true,
 * compacting the string and preserving the original order of non-whitespace
 * characters.
 *
 * @param s String to modify in-place. Must be null-terminated.
 */
void asm_strip_whitespace(char * const s)
{
    size_t len = asm_length(s);
    size_t i;
    for (i = 0; i < len; i++) {
        if (asm_isspace(s[i])) {
            asm_remove_char_from_string(s, i);
            len--;
            i--;
        }
    } 
    s[i] = '\0';
}

/**
 * @brief Check whether a string contains only ASCII whitespace characters.
 *
 * @param s Null-terminated string to test.
 * @return true if every character in @p s satisfies asm_isspace(), or if
 *         @p s is the empty string; false otherwise.
 */
bool asm_str_is_whitespace(const char * const s)
{
    size_t len = asm_length(s);
    for (size_t i = 0; i < len; i++) {
        if (!asm_isspace(s[i])) {
            return false;
        }
    }

    return true;
}

 /**
  * @brief Allocate and copy up to @p length characters from @p s.
  *
  * Allocates a new buffer of size (length + 1) bytes using ASM_MALLOC, copies up
  * to @p length characters from @p s, and always null-terminates the result.
  *
  * @param s      Source string (must be null-terminated).
  * @param length Maximum number of characters to copy (excluding '\0').
  * @return Newly allocated string.
  *
  * @note This is not the same as POSIX strdup(): it does not compute length by
  *       itself and may intentionally truncate.
+ * @note Allocation failure is not handled: if ASM_MALLOC returns NULL, this
+ *       implementation will pass NULL to asm_strncpy(), resulting in undefined
+ *       behavior.
+ * @note The returned pointer must be released with the matching deallocator
+ *       for ASM_MALLOC.
  */
char * asm_strdup(const char * const s, size_t length)
{
    char * res = (char *)ASM_MALLOC(sizeof(char) * length+1);
    asm_strncpy((char * const)res, s, length);

    return res;
}

/**
 * @brief Append up to @p N characters from @p s2 to the end of @p s1.
 *
 * Appends characters from @p s2 to the end of @p s1 until either:
 *  - @p N characters were appended, or
 *  - a '\0' is encountered in @p s2.
 *
 * After appending, this implementation writes a terminating '\0' to @p s1.
 *
 * @param s1 Destination string buffer (must be null-terminated).
 * @param s2 Source string buffer (must be null-terminated).
 * @param N  Maximum number of characters to append. If N == 0, the limit
 *           defaults to ASM_MAX_LEN.
 *
 * @return The number of characters appended to @p s1.
 *
 * @warning This function uses ASM_MAX_LEN as an upper bound for the resulting
 *          buffer size and enforces a maximum resulting string length of
 *          ASM_MAX_LEN - 1 (excluding the terminating '\0'). The caller must
 *          ensure @p s1 has capacity of at least ASM_MAX_LEN bytes.
 */
int asm_strncat(char * const s1, const char * const s2, const size_t N)
{
    size_t len_s1 = asm_length(s1);

    size_t limit = N;
    if (limit == 0) {
        limit = ASM_MAX_LEN;
    }

    size_t i = 0;
    while (i < limit && s2[i] != '\0') {
        if (len_s1 + (size_t)i >= ASM_MAX_LEN-1) {
            #ifndef ASM_NO_ERRORS
            asm_dprintERROR("s2 or the first N=%zu digit of s2 does not fit into s1.", N);
            #endif
            return (int)i;
        }

        s1[len_s1+i] = s2[i];
        i++;
    }
    s1[len_s1+i] = '\0';

    return (int)i;
}

/**
 * @brief Compare up to N characters for equality (boolean result).
 *
 * Returns 1 if the first @p N characters of @p s1 and @p s2 are all equal;
 * otherwise returns 0. Unlike the standard C strncmp, which returns 0 on
 * equality and a non-zero value on inequality/order, this function returns
 * a boolean-like result (1 == equal, 0 == different).
 *
 * @param s1 First string (may be shorter than @p N).
 * @param s2 Second string (may be shorter than @p N).
 * @param N  Number of characters to compare. If N == 0, this implementation
 *           compares up to ASM_MAX_LEN characters.
 * @return 1 if equal for the first @p N characters, 0 otherwise.
 *
 * @note If both strings terminate ('\0') at the same position before @p N,
 *       they are considered equal.
 * @note If either string ends before the other (within @p N), the strings are
 *       considered different.
 */
int asm_strncmp(const char *s1, const char *s2, const size_t N)
{
    size_t n = N == 0 ? ASM_MAX_LEN : N;
    size_t i = 0;
    while (i < n) {
        if (s1[i] == '\0' && s2[i] == '\0') {
            break;
        }
        if (s1[i] != s2[i] || (s1[i] == '\0') || (s2[i] == '\0')) {
            return 0;
        }
        i++;
    }
    return 1;
}

/**
 * @brief Compare up to N characters for equality, ASCII case-insensitively.
 *
 * Returns 1 if the first @p N characters of @p s1 and @p s2 are equal when
 * compared case-insensitively using ASCII rules; otherwise returns 0.
 *
 * Internally, this implementation duplicates both strings (up to @p N bytes),
 * lowercases exactly @p N bytes in each duplicate, and then compares.
 *
 * @param s1 First string (must be null-terminated).
 * @param s2 Second string (must be null-terminated).
 * @param N  Number of characters to compare.
 *
 * @return 1 if equal (case-insensitive) for the first @p N characters,
 *         otherwise 0.
 *
 * @warning For defined behavior, N should not exceed the length (in bytes) of
 *          either input string (i.e., avoid N > asm_length(s1) or
 *          N > asm_length(s2)). Otherwise, the internal lowercasing step may
 *          read and modify uninitialized bytes in the temporary buffers.
 *
 * @note ASCII-only: locale-specific case mappings are not supported.
 */
int asm_strncmp_case_insensitive(const char * const s1, const char * const s2, const size_t N)
{
    size_t n = N == 0 ? ASM_MAX_LEN : N;
    size_t i = 0;

    char *s1dup = asm_strdup(s1, n);
    char *s2dup = asm_strdup(s2, n);

    asm_tolower(s1dup, N);
    asm_tolower(s2dup, N);

    while (i < n) {
        if (s1dup[i] == '\0' && s2dup[i] == '\0') {
            break;
        }
        if (s1dup[i] != s2dup[i] || (s1dup[i] == '\0') || (s2dup[i] == '\0')) {
            free(s1dup);
            free(s2dup);
            return 0;
        }
        i++;
    }

    
    ASM_FREE(s1dup);
    ASM_FREE(s2dup);

    return 1;
}

/**
 * @brief Copy up to @p N characters from @p s2 into @p s1 (non-standard).
 *
 * Copies characters from @p s2 into @p s1 until either:
 *  - @p N characters were copied, or
 *  - a '\0' is encountered in @p s2,
 * and then writes a terminating '\0' to @p s1.
 *
 * This differs from the standard strncpy(): it does not pad with additional
 * '\0' bytes up to @p N.
 *
 * @param s1 Destination string buffer (need not be null-terminated on entry).
 * @param s2 Source string buffer (must be null-terminated).
 * @param N  Maximum number of characters to copy from @p s2.
 *
 * @return The number of characters copied (i.e., \(n\)).
 */
int asm_strncpy(char * const s1, const char * const s2, const size_t N)
{
    size_t n = N == 0 ? ASM_MAX_LEN : N;

    size_t i;
    for (i = 0; i < n && s2[i] != '\0'; i++) {
        s1[i] = s2[i];
    }
    s1[i] = '\0';

    return (int)i;
}

/**
 * @brief Convert ASCII uppercase letters to lowercase in-place, up to a limit
 *        or until a sentinel character is encountered.
 *
 * Iterates over @p s and converts each ASCII uppercase letter ('A'–'Z') to its
 * lowercase form. The loop stops when either:
 *  - @p len bytes have been processed, or
 *  - the character '\0' is encountered in @p s.
 *
 * @param s   Buffer to modify in-place.
 * @param len Maximum number of bytes to examine/modify.
 *
 * @note ASCII-only; not locale aware.
 */
void asm_tolower(char * const s, const size_t len)
{
    for (size_t i = 0; i < len && s[i] != '\0'; i++) {
        if (asm_isupper(s[i])) {
            s[i] += 'a' - 'A';
        }
    }
}

/**
 * @brief Convert ASCII lowercase letters to uppercase in-place, up to a limit
 *        or until a sentinel character is encountered.
 *
 * Iterates over @p s and converts each ASCII lowercase letter ('a'–'z') to its
 * uppercase form. The loop stops when either:
 *  - @p len bytes have been processed, or
 *  - the character '\0' is encountered in @p s.
 *
 * @param s   Buffer to modify in-place.
 * @param len Maximum number of bytes to examine/modify.
 *
 * @note ASCII-only; not locale aware.
 */
void asm_toupper(char * const s, const size_t len)
{
    for (size_t i = 0; i < len && s[i] != '\0'; i++) {
        if (asm_islower(s[i])) {
            s[i] += 'A' - 'a';
        }
    }
}

/**
 * @brief Remove leading ASCII whitespace from a string in-place.
 *
 * Finds the first character in @p s for which asm_isspace() is false and
 * left-shifts the string so that character becomes the first character.
 *
 * @param s String to modify in-place. Must be null-terminated.
 */
void asm_trim_left_whitespace(char * const s)
{
    size_t len = asm_length(s);

    if (len == 0) return;
    size_t i;
    for (i = 0; i < len; i++) {
        if (!asm_isspace(s[i])) {
            break;
        } 
    }
    asm_shift_left(s, i);
}

#ifdef ASM_NO_ERRORS
#undef ASM_NO_ERRORS
#endif

#endif /*ALMOG_STRING_MANIPULATION_IMPLEMENTATION*/

