#include "Application2D.h"

#include "Common.h"

#include <iostream>
#include <string>

int main()
{
	// allocation
	Application2D *app = new Application2D();

	// initialise and loop
	app->run("Networking App Client", SCREEN_WIDTH, SCREEN_HEIGHT, false);

	// deallocation
	if (app != nullptr)
	{
		delete app;
		app = nullptr;
	}

	return 0;
}
