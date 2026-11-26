# Decoding untrusted input

CFL provides two small, header-only interfaces for code that parses data it
does not control, such as network payloads, files, or serialized records:

- [`cfl_checked.h`](include/cfl/cfl_checked.h): overflow-checked arithmetic
  for sizes and counters, and a length-aware string comparison.
- [`cfl_cursor.h`](include/cfl/cfl_cursor.h): a bounds-checked reader over a
  byte buffer.

Both are plain C99 `static inline` functions. They allocate nothing, keep no
global state, and add no symbols to the library.

## Why they exist

Binary decoders written with a raw pointer and an offset repeat the same three
steps for every field: check that enough bytes remain, read, advance. Each step
is easy to write and easy to get subtly wrong:

- A bounds check is forgotten for one field, or added for the header but not
  for the body it describes.
- The check itself overflows. `offset + length > size` is false when `length`
  is close to `SIZE_MAX`, so a hostile length passes it.
- A declared length is smaller than the header that carries it, and
  `length - header_size` wraps around to a huge value.
- A hand-written overflow guard uses a threshold that is off by one.
- A length-delimited string is compared with `strncmp()` or `strlen()` as if
  it were NUL terminated.
- A function reports failure and the caller does not look at the result.

These interfaces put each check in one place, so a decoder states what it
wants to read and cannot skip the validation.

## Conventions

- Functions return `CFL_TRUE` on success and `CFL_FALSE` on failure, so calls
  can be chained with `&&` and `||`.
- On failure the output argument is left untouched.
- Functions that can fail are marked with `CFL_MUST_USE`. GCC and Clang warn
  when the result is ignored; the macro expands to nothing elsewhere.

## Checked arithmetic

| Function | Result | Fails when |
| --- | --- | --- |
| `cfl_checked_size_add(a, b, &out)` | `a + b` | the sum exceeds `SIZE_MAX` |
| `cfl_checked_size_sub(a, b, &out)` | `a - b` | `b` is greater than `a` |
| `cfl_checked_size_mul(a, b, &out)` | `a * b` | the product exceeds `SIZE_MAX` |
| `cfl_checked_size_mul_add(a, b, c, &out)` | `a * b + c` | either step overflows |
| `cfl_checked_u64_add(a, b, &out)` | `a + b` | the sum exceeds `UINT64_MAX` |
| `cfl_checked_u64_sub(a, b, &out)` | `a - b` | `b` is greater than `a` |
| `cfl_checked_u64_mul(a, b, &out)` | `a * b` | the product exceeds `UINT64_MAX` |
| `cfl_checked_u64_mul_add(a, b, c, &out)` | `a * b + c` | either step overflows |
| `cfl_checked_u64_to_size(value, &out)` | `value` as `size_t` | the value does not fit in `size_t` |

Compute an allocation size from a count found in the input:

```c
size_t bytes;

if (!cfl_checked_size_mul(count, sizeof(struct entry), &bytes)) {
    return -1;
}

entries = malloc(bytes);
```

Accumulate a decimal number, for example a length prefix, without deriving an
overflow threshold by hand:

```c
size_t value = 0;

while (*text >= '0' && *text <= '9') {
    if (!cfl_checked_size_mul_add(value, 10, (size_t) (*text - '0'), &value)) {
        return -1;
    }
    text++;
}
```

GCC 5 or later and Clang use the compiler overflow builtins. Other compilers
use portable C99 comparisons with the same semantics. Define
`CFL_CHECKED_NO_BUILTINS` before including the header to force the portable
path.

### Length-aware string comparison

`cfl_checked_str_equals(data, length, str)` compares a length-delimited string
against a C string and returns `CFL_TRUE` only on an exact match. The lengths
are compared first, so it never reads more than `length` bytes from `data`, and
a string that merely starts with `str` does not match.

```c
/* 'key' and 'key_length' come from a decoded record, with no terminator */
if (cfl_checked_str_equals(key, key_length, "namespace")) {
    ...
}
```

## Byte cursor

A `struct cfl_cursor` is a read-only view over a buffer. It borrows the bytes:
it does not copy or free them, so the buffer must outlive the cursor and every
pointer the cursor hands out.

```c
struct cfl_cursor cursor;

cfl_cursor_init(&cursor, buffer, size);
```

### Reading

| Function | Reads |
| --- | --- |
| `cfl_cursor_read_u8()` | one byte |
| `cfl_cursor_read_u16_be()`, `_le()` | 16-bit unsigned integer |
| `cfl_cursor_read_u24_be()`, `_le()` | 24-bit unsigned integer, zero extended to `uint32_t` |
| `cfl_cursor_read_u32_be()`, `_le()` | 32-bit unsigned integer |
| `cfl_cursor_read_u64_be()`, `_le()` | 64-bit unsigned integer |
| `cfl_cursor_read_bytes()` | `length` bytes, returned as a pointer into the buffer |
| `cfl_cursor_copy_bytes()` | `length` bytes, copied to a caller buffer |
| `cfl_cursor_skip()` | nothing; advances over `length` bytes |
| `cfl_cursor_peek_u8()` | the next byte, without advancing |
| `cfl_cursor_sub()` | `length` bytes, returned as an independent cursor |

`_be` functions decode big-endian (network order) values and `_le` functions
little-endian ones. Integers are assembled byte by byte, so the input does not
need to be aligned and the result does not depend on the host byte order.

Every read performs the same single comparison against the bytes that remain,
written so that it cannot overflow for any `length`.

### Sticky error

When a read does not fit, it returns `CFL_FALSE`, the cursor stays where it
was, and the cursor enters an error state in which every later read also
fails. A decoder can therefore chain reads and test the outcome once:

```c
uint16_t type;
uint16_t length;
const unsigned char *body;

if (!cfl_cursor_read_u16_be(&cursor, &type) ||
    !cfl_cursor_read_u16_be(&cursor, &length) ||
    !cfl_cursor_read_bytes(&cursor, length, &body)) {
    return -1;
}
```

Call `cfl_cursor_init()` again to start over on a new buffer.

### Inspecting the state

| Function | Returns |
| --- | --- |
| `cfl_cursor_remaining()` | bytes not read yet |
| `cfl_cursor_offset()` | bytes consumed since `cfl_cursor_init()` |
| `cfl_cursor_is_done()` | `CFL_TRUE` when every byte was consumed and nothing failed |
| `cfl_cursor_failed()` | `CFL_TRUE` once any read has failed |

These never change the cursor. A streaming parser that may have to wait for
more data should test `cfl_cursor_remaining()` before reading, and use
`cfl_cursor_offset()` to learn how many bytes it consumed.

Use `cfl_cursor_is_done()` at the end of a decoder to reject trailing bytes
that the format does not allow.

### Nested content

`cfl_cursor_sub()` splits off the next `length` bytes as a new cursor and
advances the parent past them. The decoder that receives the sub-cursor cannot
read beyond its own content, and a failure inside it does not affect the
parent.

The following decodes a stream of parts, each with a two-byte type, a two-byte
length that includes the four-byte header, and a value:

```c
struct cfl_cursor cursor;
struct cfl_cursor part;
uint16_t type;
uint16_t length;
uint64_t number;
size_t body;

cfl_cursor_init(&cursor, buffer, size);

while (cfl_cursor_remaining(&cursor) > 0) {
    if (!cfl_cursor_read_u16_be(&cursor, &type) ||
        !cfl_cursor_read_u16_be(&cursor, &length) ||
        !cfl_checked_size_sub(length, 4, &body) ||
        !cfl_cursor_sub(&cursor, body, &part)) {
        return -1;
    }

    if (type == PART_NUMBER) {
        if (!cfl_cursor_read_u64_be(&part, &number) ||
            !cfl_cursor_is_done(&part)) {
            return -1;
        }
    }
}
```

This rejects a truncated header, a length smaller than the header, a length
that points past the input, and a number part of the wrong size, without an
offset variable or a hand-written comparison.

## Limits

- The interfaces validate reads, not lifetimes. A pointer returned by
  `cfl_cursor_read_bytes()` is only valid while the underlying buffer is.
- A cursor is not thread-safe. Give each thread its own cursor; the buffer
  itself may be shared for reading.
- `CFL_MUST_USE` has no effect on compilers without an equivalent attribute,
  such as MSVC. Rely on a GCC or Clang build to enforce it.
- Recursive formats still need their own depth limit. A cursor bounds how many
  bytes a decoder reads, not how deep it recurses.
