#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

#define PAGE_SIZE 4096

enum ParserState {
    PARSE_JSON_START,
    PARSE_JSON_END,
    PARSE_KEY,
    PARSE_VALUE,
    PARSE_OBJ_VALUE_END,       // ',' or '}'
    PARSE_KEY_VALUE_SEP,  // ':'
    PARSE_STRING,
    PARSE_ARRAY_VALUE,
    PARSE_ARRAY_VALUE_END, // ',' or ']'
};

typedef struct {
    unsigned int i;
    size_t len;
    char * buf;
} JsonReader;

enum JsonValueType {
    VALUE_ARRAY,
    VALUE_OBJECT,
    VALUE_STRING,
};

typedef struct {
    enum JsonValueType type;
    // the index for accessing the value in data store
    uint32_t i;
} JsonValue;

/// Specifies the beginning and end of a string
/// in the char[] of all strings
typedef struct {
    uint32_t start;
    uint32_t end;
} StringIdx;

typedef struct {
    uint32_t i;
    char buf[PAGE_SIZE];
    StringIdx indices[PAGE_SIZE];
} StringsBuf;

typedef struct {
    uint32_t length;
    JsonValue values[512] ;
} JsonArray;

typedef struct {
    StringsBuf strings;
} Parser;

StringsBuf new_strings_buf() {
    StringsBuf sb;
    sb.i = 0;
    return sb;
}

char * parser_state_str(enum ParserState state) {
    switch (state) {
        case PARSE_JSON_START:
            return "PARSE_JSON_START";
        case PARSE_JSON_END:
            return "PARSE_JSON_END";
        case PARSE_KEY:
            return "PARSE_KEY";
        case PARSE_VALUE:
            return "PARSE_VALUE";
        case PARSE_OBJ_VALUE_END:
            return "PARSE_OBJ_VALUE_END";
        case PARSE_KEY_VALUE_SEP:
            return "PARSE_KEY_VALUE_SEP";
        case PARSE_STRING:
            return "PARSE_STRING";
        default:
            fprintf(stderr, "Asked to print unhandled state: %d\n", state);
            return "UNKNOWN";
    }
}


StringIdx parse_string(JsonReader * r, StringsBuf * sb) {
    char c;
    StringIdx i = {.start=r->i};
    do {
        c = r->buf[r->i];
        r->i++;
    } while (c != '"');
    return i;
    // int i = sb->i;
    // char c;
    // while ((c=getchar()) != '"') {
    //     sb->buf[i] = c;
    //     i++;
    // }
    // sb->buf[i] = '\0';
    // printf("found str: %s\n", sb->buf + sb->i);
    // StringIdx ends = {.start=sb->i, .end=i};
    // sb->i = i+1;
    // return ends;
}

/// returns false if finds end of array
// bool parse_array()

// https://www.json.org/json-en.html
int main(int argc, char**argv) {
    StringsBuf sb = new_strings_buf();
    enum ParserState state = PARSE_JSON_START;
    enum ParserState next_state;

    char json[PAGE_SIZE * 4];
    ssize_t bytes_read = read(STDIN_FILENO, json, PAGE_SIZE * 4);
    if (bytes_read < 0) {
        perror("jasn");   
        exit(errno);
    }
    JsonReader r = {.i=0, .buf=json, .len=bytes_read};
    // TODO: handle empty file
    size_t i;
    char c;
    // TODO: int depth = 0; expect end when reaching } or ] at depth 0

    for (r.i = 0; r.i < r.len; r.i++) {
        c = r.buf[r.i];
        if (state != PARSE_STRING && isspace(c)) {
            continue;
        }
        switch (state) {
            case PARSE_JSON_START:
                switch (c) {
                    case '{':
                        state = PARSE_KEY;
                        break;
                    case '[':
                        state = PARSE_ARRAY_VALUE;
                        break;
                    default:
                        fprintf(stderr, "PARSE ERROR: Expected object start '{' but found: %c\n", c);
                        return -1;
                }
                break;
            case PARSE_JSON_END:
                if (!isspace(c) && c != EOF) {
                    fprintf(stderr, "PARSE ERROR: Expected EOF but found: %c\n", c);
                }
                else {
                    return 0;
                }
                break;
            case PARSE_KEY:
                if (c == '"') {
                    state = PARSE_STRING;
                    next_state = PARSE_KEY_VALUE_SEP;
                } else {
                    fprintf(stderr, "PARSE ERROR: Expected key but found: %c\n", c);
                    return -1;
                }
                break;
            case PARSE_KEY_VALUE_SEP:
                if (c != ':') { fprintf(stderr, "PARSE ERROR: Expected ':' but found: %c\n", c);
                    return -1;
                }
                state = PARSE_VALUE;
                break;
            case PARSE_VALUE:
                switch (c) {
                    case '"':
                        state = PARSE_STRING;
                        next_state = PARSE_OBJ_VALUE_END;
                        break;
                    default:
                        fprintf(stderr, "PARSE ERROR: Expected value but found: %c\n", c);
                        return -1;
                }
                break;
            case PARSE_OBJ_VALUE_END:
                switch (c) {
                    case ',':
                        state = PARSE_KEY;
                        break;
                    case '}':
                        state = PARSE_JSON_START;
                        break;
                    default:
                        fprintf(stderr, "PARSE ERROR: Expected end of object or comma but found: %c\n", c);
                        return -1;
                }
                break;
            case PARSE_ARRAY_VALUE:
                switch (c) {
                    case '"':
                        state = PARSE_STRING;
                        next_state = PARSE_ARRAY_VALUE_END;
                        break;
                    default:
                        fprintf(stderr, "PARSE ERROR: Expected array value but found item beginning with '%c'\n", c);
                        return -1;
                }
                break;
            case PARSE_ARRAY_VALUE_END:
                switch (c) {
                    case ',':
                        state = PARSE_ARRAY_VALUE;
                        break;
                    case ']':
                        state = PARSE_JSON_END;
                        break;
                    default:
                        fprintf(stderr, "PARSE_ERROR: expected ',' or ']' after array value but found '%c'\n", c);
                }
                break;

            case PARSE_STRING:
                if (c == '"') {
                    state = next_state;
                    break;
                } else {
                    printf("%c", c);
                    break;
                }
        }
        fprintf(stderr, "char: '%c' state: %s\n", c, parser_state_str(state));
    }
}

#ifdef TEST
#define assert(message, test) do {if (!(test)) fprintf(stderr, message)} while(0);
#endif
