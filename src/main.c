#include <stdio.h>
#include <ctype.h>

const int PAGE_SIZE = 4096;

enum ParserState {
    OBJ_START,
    OBJ_END,
    KEY,
    VALUE,
    OBJ_VALUE_END,
};

char * parser_state_str(enum ParserState state) {
    switch (state) {
        case OBJ_START:
            return "OBJ_START";
        case OBJ_END:
            return "OBJ_END";
        case KEY:
            return "KEY";
        case VALUE:
            return "VALUE";
        case OBJ_VALUE_END:
            return "OBJ_VALUE_END";
        default:
            fprintf(stderr, "Asked to print unhandled state: %d\n", state);
            return "UNKNOWN";
    }
}


char parse_eat_whitespace() {
    char c = getchar();
    while (isspace(c)) {
        c=getchar();
    }
    return c;
}

void parse_string() {
    char buf[100];
    int i = 0;
    char c;
    while ((c=getchar()) != '"') {
        buf[i] = c;
        i++;
    }
    buf[i] = '\0';
    printf("found str: %s\n", buf);
}

int main(int argc, char**argv) {
    char c;
    char buf[100];
    int i;
    enum ParserState state = OBJ_START;

    while (1) {
        switch (state) {
            case OBJ_START:
                c = parse_eat_whitespace();
                switch (c) {
                    case '{':
                        state = KEY;
                        break;
                    default:
                        fprintf(stderr, "PARSE ERROR: Expected object start ({) but found: %c\n", c);
                        return -1;
                }
                break;
            case OBJ_END:
                c = parse_eat_whitespace();
                if (!isspace(c) && c != EOF) {
                    fprintf(stderr, "PARSE ERROR: Expected EOF but found: %c\n", c);
                }
                else {
                    return 0;
                }
                break;
            case KEY:
                c = parse_eat_whitespace();
                if (c == '"') {
                    parse_string();
                    c = parse_eat_whitespace();
                    if (c != ':') {
                        fprintf(stderr, "PARSE ERROR: Expected ':' but found: %c\n", c);
                        return -1;
                    }
                    state = VALUE;
                } else {
                    fprintf(stderr, "PARSE ERROR: Expected key but found: %c\n", c);
                    return -1;
                }
                break;
            case VALUE:
                c = parse_eat_whitespace();
                switch (c) {
                    case '"':
                        parse_string();
                        state = OBJ_VALUE_END;
                        break;
                    default:
                        fprintf(stderr, "PARSE ERROR: Expected value but found: %c\n", c);
                        return -1;
                }
                break;
            case OBJ_VALUE_END:
                c = parse_eat_whitespace();
                switch (c) {
                    case ',':
                        state = KEY;
                        break;
                    case '}':
                        state = OBJ_END;
                        break;
                    default:
                        fprintf(stderr, "PARSE ERROR: Expected end of object or comma but found: %c\n", c);
                        return -1;
                }
        }
        fprintf(stderr, "char: '%c' State: %s\n", c, parser_state_str(state));
    }
}
