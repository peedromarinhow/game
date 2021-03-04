#include "lingo.h"

//todo: better names for these two

inline void Win32ProcessButtonMessage(/* button_state */ b32 *EndedDown, b32 IsDown) {
    // if (State->EndedDown != IsDown) {
    //     State->EndedDown =  IsDown;
    //     ++State->HalfTransitionCount;
    // }
    if (*EndedDown != IsDown)
        *EndedDown =  IsDown;
}

inline void Win32ProcessEventMessage(/* event_state */ b32 *EndedHappening, b32 IsHappening) {
    // if (State->EndedDown != IsDown) {
    //     State->EndedDown =  IsDown;
    //     ++State->HalfTransitionCount;
    // }
    if (*EndedHappening != IsHappening)
        *EndedHappening =  IsHappening;
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
