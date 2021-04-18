#ifndef STRETCHY_BUFFER_H
#define STRETCHY_BUFFER_H

//note:
//  Stretchy buffers.
//  Invented by Sean Barret?
//  Interface:
//    i32 *Buffer = NULL;
//    BufferPush(Buffer, 42);
//    BufferPush(Buffer, 21);
//    for (i32 i = 0, i < BufferLen(Buffer), i++) {...}
//

typedef struct BuffHdr {
    size_t len;
    size_t cap;
    char buff[0];
} BuffHdr;

// annotation
#define BUFF(x) x

/* internal functions
    */
#define _buff_hdr(b) ((BuffHdr *)((char *)b - offsetof(BuffHdr, buff)))
#define _buff_fits(b, n) (buff_len(b) + (n) <= buff_cap(b))
#define _buff_fit(b, n) (_buff_fits(b, n) ? 0 : ((b) = _buff_grow((b), buff_len(b) + (n), sizeof(*(b)))))

/* public functions
    */
#define buff_len(b) ((b) ? _buff_hdr(b)->len : 0)
#define buff_cap(b) ((b) ? _buff_hdr(b)->cap : 0)
#define buff_end(b) ((b) + buff_len(b))
#define buff_free(b) ((b) ? (free(_buff_hdr(b)), (b) = NULL) : 0)
#define buff_push(b, ...) (_buff_fit((b), 1), (b)[_buff_hdr(b)->len++] = (__VA_ARGS__))
#define buff_printf(b, ...) ((b) = _buff_printf((b), __VA_ARGS__))
#define buff_clear(b) ((b) ? _buff_hdr(b)->len = 0 : 0)

void *_buff_grow(const void *buff, size_t new_len, size_t elm_size) {
    assert(buff_cap(buff) <= (SIZE_MAX - 1)/2);
    size_t new_cap = MAX(1 + 2 * buff_cap(buff), new_len);
    assert(new_len <= new_cap);
    assert(new_cap <= (SIZE_MAX - offsetof(BuffHdr, buff))/elm_size);
    size_t new_size = offsetof(BuffHdr, buff) + new_cap * elm_size;

    BuffHdr *new_hdr;
    if (buff) {
        new_hdr = xrealloc(_buff_hdr(buff), new_size);
    } else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buff;
}

char *_buff_printf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (buff_len(buf) == 0) {
        n++;
    }
    _buff_fit(buf, n + buff_len(buf));
    char *dest = buff_len(buf) == 0 ? buf : buf + buff_len(buf) - 1;
    va_start(args, fmt);
    vsnprintf(dest, buf + buff_cap(buf) - dest, fmt, args);
    va_end(args);
    _buff_hdr(buf)->len += n;
    return buf;
}

#ifndef STRETCHY_BUFFER_H