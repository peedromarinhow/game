#ifndef INPUT_H
#define INPUT_H

#include "lingo.h"

typedef enum _input_type {
    INPUT_PRESSED,
    INPUT_RELEASED
} input_event_type;

typedef struct _input_event {
    u16        KeyComb;
    u8         Char;
    input_event_type Type;
} input_event;

#endif//INPUT_H