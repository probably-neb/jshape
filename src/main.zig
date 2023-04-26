const std = @import("std");
const log = std.log;
const ascii = std.ascii;
const assert = std.debug.assert;
const eprint = std.debug.print;

const PAGE_SIZE = 4096;
const FILE_BUF_SIZE = PAGE_SIZE * 4;
const MAX_DEPTH = 512;

const ParseError = error{
    UnexpectedChar,
    InvalidKey,
};

pub fn expect(expected: u8, got: u8, msg: []const u8) !void {
    if (expected != got) {
        log.err("{s}: Expected '{c}' but found '{c}'", .{ msg, expected, got });
        return ParseError.UnexpectedChar;
    }
}

const ParserState = enum {
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
const CollectionType = enum {
    OBJ,
    ARR,
};

const JsonReader = struct {
    i: u32 = 0,
    len: usize = 0,
    buf: []u8 = undefined,
    d: u32 = 0,
    ds: [MAX_DEPTH]CollectionType = undefined,
};

pub fn main() !void {
    const stdin = std.io.getStdIn().reader();
    var r = JsonReader{};
    var read_buf: [FILE_BUF_SIZE]u8 = undefined;
    r.buf = (try stdin.readUntilDelimiterOrEof(&read_buf, 0)).?;
    // eprint("{s}", .{r.buf});
    var state = ParserState.PARSE_JSON_START;
    var next_state = ParserState.PARSE_JSON_END;

    while (r.i < r.buf.len) {
        const c: u8 = r.buf[r.i];
        defer r.i += 1;

        if (state != .PARSE_STRING and ascii.isWhitespace(c)) {
            continue;
        }
        eprint("char: '{c}' | depth: {d} | type: {s} | state: {s}\n", .{ c, r.d, @tagName(r.ds[r.d]), @tagName(state) });
        switch (state) {
            .PARSE_JSON_START => {
                state = switch (c) {
                    '{' => .PARSE_KEY,
                    '[' => .PARSE_VALUE,
                    else => {
                        log.err("expected '{{' or '[' but found: '{c}'", .{c});
                        return ParseError.UnexpectedChar;
                    },
                };
                r.ds[r.d] = switch (c) {
                    '{' => .OBJ,
                    '[' => .ARR,
                    else => unreachable,
                };
            },
            .PARSE_KEY => {
                if (c != '"') {
                    log.err("Keys must be strings", .{});
                    return ParseError.InvalidKey;
                }
                state = .PARSE_STRING;
                next_state = .PARSE_KEY_VALUE_SEP;
            },
            .PARSE_STRING => blk: {
                if (c == '"') {
                    state = next_state;
                    break :blk;
                }
            },
            .PARSE_KEY_VALUE_SEP => {
                try expect(':', c, "Key,value in object must be separated by ':'");
                state = .PARSE_VALUE;
            },
            .PARSE_VALUE => {
                state = switch (c) {
                    '"' => str: {
                        next_state = .PARSE_VALUE_END;
                        break :str .PARSE_STRING;
                    },
                    '[' => arr: {
                        r.d += 1;
                        r.ds[r.d] = .ARR;
                        break :arr .PARSE_VALUE;
                    },
                    '{' => obj: {
                        r.d += 1;
                        r.ds[r.d] = .OBJ;
                        break :obj .PARSE_KEY;
                    },
                    else => esle: {
                        if (ascii.isDigit(c)) {
                            break :esle .PARSE_NUMBER;
                        }
                        log.err("Unsupported value starting with '{c}'", .{c});
                        return ParseError.UnexpectedChar;
                    },
                };
            },
            .PARSE_VALUE_END => {
                if (c == ',') {
                    state = switch (r.ds[r.d]) {
                        .ARR => .PARSE_VALUE,
                        .OBJ => .PARSE_KEY,
                    };
                } else if ((r.ds[r.d] == .ARR and c == ']') or (r.ds[r.d] == .OBJ and c == '}')) {
                    if (r.d == 0) {
                        state = .PARSE_JSON_END;
                    } else {
                        // keep state
                        r.d -= 1;
                    }
                } else {
                    log.err("Expected ',' or end of {s} but found '{c}'", .{ @tagName(r.ds[r.d]), c });
                    return ParseError.UnexpectedChar;
                }
            },
            .PARSE_NUMBER => {
                // TODO: parse floats
                if (!ascii.isDigit(c)) {
                    r.i -= 1;
                    state = .PARSE_VALUE_END;
                    // log.err("Expected digit but found '{c}'", .{c});
                    // return ParseError.UnexpectedChar;
                }
            },
            else => {
                log.err("{s} state not handled!\n", .{@tagName(state)});
                return error.Unreachable;
            },
        }
    }
}
