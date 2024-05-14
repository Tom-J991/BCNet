#include "Game.h"

#include "Common.h"

#include <iostream>
#include <string>

int main()
{
	// allocation
	Game *app = new Game();

	// initialise and loop
	app->Run("Networking App Client", SCREEN_WIDTH, SCREEN_HEIGHT);

	// deallocation
	if (app != nullptr)
	{
		delete app;
		app = nullptr;
	}

	return 0;
}
