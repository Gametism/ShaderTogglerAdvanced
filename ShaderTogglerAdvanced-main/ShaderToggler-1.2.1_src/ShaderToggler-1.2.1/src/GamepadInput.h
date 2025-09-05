#pragma once
#include <windows.h>
#include <XInput.h>

#pragma comment(lib, "XInput9_1_0.lib")

class GamepadInput
{
public:
    static void Update();
    static bool IsButtonPressed(WORD button);

private:
    static XINPUT_STATE state;
};