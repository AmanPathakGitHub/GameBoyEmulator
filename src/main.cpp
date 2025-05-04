#include "application.h"


int main(int argc, char* argv[])
{
	Application* app = new Application({"GameBoy Emulator", 1080, 720, true});
	app->Run();

	delete app;

	return 0;
}