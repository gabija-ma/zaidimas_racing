#include "SFMLGame.h"

#define NOMINMAX
#include <Windows.h>
#include <exception>

int main()
{
    try
    {
        SFMLGame game;
        game.Run();
    }
    catch (const std::exception& error)
    {
        MessageBoxA(nullptr, error.what(), "Racing - klaida", MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}
