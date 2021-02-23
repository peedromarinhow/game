#include "lingo.h"

//todo: better names for these two

internal void Win32ProcessButtonMessage(button_state *State, b32 IsDown) {
    if (State->EndedDown != IsDown) {
        State->EndedDown = IsDown;
        ++State->HalfTransitionCount;
    }
}

internal void Win32ProcessEventMessage(event_state *State, b32 IsDown) {
    if (State->EndedHappening != IsDown) {
        State->EndedHappening = IsDown;
        ++State->HalfTransitionCount;
    }
}
