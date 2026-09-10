#include "GamepadInput.h"
#include <windows.h>

SDL_GameController* GamepadInput::controller = nullptr;
Uint8 GamepadInput::buttonStates[SDL_CONTROLLER_BUTTON_MAX] = {};

void GamepadInput::Initialize() {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        OutputDebugStringA("SDL_Init failed\n");
        return;
    }

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) {
                OutputDebugStringA("Gamepad connected via SDL\n");
                break;
            }
        }
    }

    if (!controller) {
        OutputDebugStringA("No SDL-compatible gamepad found\n");
    }
}

void GamepadInput::Update() {
    SDL_GameControllerUpdate();

    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        buttonStates[i] = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(i));
    }
}

bool GamepadInput::IsButtonPressed(SDL_GameControllerButton button) {
    return buttonStates[button] != 0;
}

void GamepadInput::Cleanup() {
    if (controller) {
        SDL_GameControllerClose(controller);
        controller = nullptr;
    }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}