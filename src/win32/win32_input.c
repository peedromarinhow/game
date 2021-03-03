#include "lingo.h"

//todo: better names for these two

inline void Win32ProcessButtonMessage(button_state *State, b32 IsDown) {
    if (State->EndedDown != IsDown) {
        State->EndedDown =  IsDown;
        ++State->HalfTransitionCount;
    }
}

inline void Win32ProcessEventMessage(event_state *State, b32 IsHappenning) {
    if (State->EndedHappening != IsHappenning) {
        State->EndedHappening =  IsHappenning;
        ++State->HalfTransitionCount;
    }
}

inline rv2 Win32GetMousePos(HWND Window) {
    rv2 Result = {0};
    POINT MousePoint;
    GetCursorPos(&MousePoint);
    ScreenToClient(Window, &MousePoint);
    Result.x = MousePoint.x;
    Result.y = MousePoint.y;
    return Result;
}
