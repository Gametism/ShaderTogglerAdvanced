#pragma once
#include <SDL.h>
#include <SDL_gamecontroller.h>

class GamepadInput {
public:
    static void Initialize();
    static void Update();
    static bool IsButtonPressed(SDL_GameControllerButton button);
    static void Cleanup();

private:
    static SDL_GameController* controller;
    static Uint8 buttonStates[SDL_CONTROLLER_BUTTON_MAX];
};