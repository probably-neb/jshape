#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

#define PAGE_SIZE 4096
#define MAX_DEPTH 100
#define CUR(r) (r->buf[r->i])

enum ParserState {
    PARSE_JSON_START,
    PARSE_JSON_END,
    PARSE_KEY,
    PARSE_VALUE,
    PARSE_KEY_VALUE_SEP,
    PARSE_STRING,
    PARSE_VALUE_END,
    PARSE_NUMBER,
    PARSE_FLOAT,
};

enum CollectionType {
    COLLECTION_ARR,
    COLLECTION_OBJ,
};

/// Specifies the beginning and end of an item
/// in the json buffer
typedef struct {
    uint32_t start;
    uint32_t end;
} ItemRange;

typedef struct {
    uint32_t i;
    size_t len;
    char * buf;
    // depth in depth stack
    uint32_t d; 
    // depth stack: trace of type of collections we have recursed into
    enum CollectionType ds[MAX_DEPTH];
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

typedef struct {
    uint32_t i;
    char buf[PAGE_SIZE];
    ItemRange indices[PAGE_SIZE];
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
        case PARSE_KEY_VALUE_SEP:
            return "PARSE_KEY_VALUE_SEP";
        case PARSE_STRING:
            return "PARSE_STRING";
        case PARSE_VALUE_END:
            return "PARSE_VALUE_END";
        default:
            fprintf(stderr, "Asked to print unhandled state: %d\n", state);
            return "UNKNOWN";
    }
}

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
    JsonReader r = {.i=0, .buf=json, .len=bytes_read, .d=0};
    // TODO: handle empty file
    size_t i;
    unsigned char c;
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
                        r.ds[r.d] = COLLECTION_OBJ;
                        state = PARSE_KEY;
                        break;
                    case '[':
                        r.ds[r.d] = COLLECTION_ARR;
                        state = PARSE_VALUE;
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
                if (c == '"') {
                    state = PARSE_STRING;
                    next_state = PARSE_VALUE_END;
                } else if (c == '[') {
                    state = PARSE_VALUE;
                    r.d++;
                    r.ds[r.d] = COLLECTION_ARR;
                } else if (c == '{') {
                    state = PARSE_KEY;
                    r.d++;
                    r.ds[r.d] = COLLECTION_OBJ;
                } else if (isdigit(c)) {
                    // TODO: test for negatives
                    state = PARSE_NUMBER;
                    next_state = PARSE_VALUE_END;
                }
                else {
                    fprintf(stderr, "PARSE ERROR: Expected value but found: %c\n", c);
                    return -1;
                }
            case PARSE_VALUE_END:
                if (c == ',') {
                    state = r.ds[r.d] == COLLECTION_ARR ? PARSE_VALUE : PARSE_KEY;
                    break;
                }
                bool end_of_arr = r.ds[r.d] == COLLECTION_ARR && c == ']';
                bool end_of_obj = r.ds[r.d] == COLLECTION_OBJ && c == '}';

                if (end_of_arr || end_of_obj) {
                    if (r.d==0) {
                        state = PARSE_JSON_END;
                    } else {
                        r.d--;
                    }
                } else {
                    fprintf(stderr, "PARSE_ERROR: expected ',' or ']' after array value but found '%c'\n", c);
                }
                break;
            case PARSE_STRING:
                if (c == '"') {
                    state = next_state;
                    break;
                } else {
                    // TODO: store strings
                    break;
                }
            case PARSE_NUMBER:
                if (c == '.' || c == 'e' || c=='E') {
                    state = PARSE_FLOAT;
                }
        }
        fprintf(stderr, "char: '%c' | depth: %d | type: %s | state: %s\n", c, r.d,r.ds[r.d] == COLLECTION_ARR ? "arr" : "obj", parser_state_str(state));
    }
}

#ifdef TEST
#define assert(message, test) do {if (!(test)) fprintf(stderr, message)} while(0);
#endif
