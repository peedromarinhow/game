//note: not going to do gamepad output for now

#include "lingo.h"

#include <xinput.h>

#define X_INPUT_GET_STATE(name) \
    DWORD WINAPI name(DWORD DwUserIndex, XINPUT_STATE *PState)
typedef X_INPUT_GET_STATE(xinput_get_state_callback);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global xinput_get_state_callback *XInputGetStatePtr = XInputGetStateStub;
#define XInputGetState XInputGetStatePtr

// for XInputSetState
#define X_INPUT_SET_STATE(name) \
    DWORD WINAPI name(DWORD DwUserIndex, XINPUT_VIBRATION *PVibration)
typedef X_INPUT_SET_STATE(xinput_set_state_callback);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global xinput_set_state_callback *XInputSetStatePtr = XInputSetStateStub;
#define XInputSetState XInputSetStatePtr

internal void Win32InitXInput(void) {

    XInputGetStatePtr = XInputGetStateStub;
    XInputSetStatePtr = XInputSetStateStub;

    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary) {
        XInputLibrary = LoadLibrary("xinput9_1_0.dll");
        //todo: logging
    }
    if (!XInputLibrary) {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
        //todo: logging
    }
    if (XInputLibrary) {
        XInputGetStatePtr =
            (xinput_get_state_callback *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetStatePtr =
            (xinput_set_state_callback *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else {
        //todo: logging
    }
}

// pass some state
internal void Win32UpdateXInput(platform_state *State) {
    for(u32 i = 0; i < W32_MAX_GAMEPADS; ++i) {
        if(i < XUSER_MAX_COUNT) {
            XINPUT_STATE controller_state = {0};
            if(XInputGetStateProcPtr(0, &controller_state) == ERROR_SUCCESS) {
                // NOTE(rjf): Controller connected.
                global_gamepads[i].connected = 1;
                XINPUT_GAMEPAD *pad = &controller_state.Gamepad;
                
                for(u32 j = 0; j < W32_MAX_GAMEPADS; ++j)
                {
                    global_gamepads[i].button_states[j] = 0;
                }
                
                global_gamepads[i].button_states[GamepadButton_DPadUp] |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                global_gamepads[i].button_states[GamepadButton_DPadDown] |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                global_gamepads[i].button_states[GamepadButton_DPadLeft] |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                global_gamepads[i].button_states[GamepadButton_DPadRight] |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                global_gamepads[i].button_states[GamepadButton_Start] |= !!(pad->wButtons & XINPUT_GAMEPAD_START);
                global_gamepads[i].button_states[GamepadButton_Back] |= !!(pad->wButtons & XINPUT_GAMEPAD_BACK);
                global_gamepads[i].button_states[GamepadButton_LeftThumb] |= !!(pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
                global_gamepads[i].button_states[GamepadButton_RightThumb] |= !!(pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
                global_gamepads[i].button_states[GamepadButton_LeftShoulder] |= !!(pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                global_gamepads[i].button_states[GamepadButton_RightShoulder] |= !!(pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                global_gamepads[i].button_states[GamepadButton_A] |= !!(pad->wButtons & XINPUT_GAMEPAD_A);
                global_gamepads[i].button_states[GamepadButton_B] |= !!(pad->wButtons & XINPUT_GAMEPAD_B);
                global_gamepads[i].button_states[GamepadButton_X] |= !!(pad->wButtons & XINPUT_GAMEPAD_X);
                global_gamepads[i].button_states[GamepadButton_Y] |= !!(pad->wButtons & XINPUT_GAMEPAD_Y);
                
                global_gamepads[i].joystick_1.x = pad->sThumbLX / 32768.f;
                global_gamepads[i].joystick_1.y = pad->sThumbLY / 32768.f;
                global_gamepads[i].joystick_2.x = pad->sThumbRX / 32768.f;
                global_gamepads[i].joystick_2.y = pad->sThumbRY / 32768.f;
            }
            else
            {
                // NOTE(rjf): Controller is not connected
                global_gamepads[i].connected = 0;
            }
        }
    }
}
