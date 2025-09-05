#include "GamepadInput.h"

XINPUT_STATE GamepadInput::state = {};

void GamepadInput::Update()
{
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    XInputGetState(0, &state);
}

bool GamepadInput::IsButtonPressed(WORD button)
{
    return (state.Gamepad.wButtons & button) != 0;
}