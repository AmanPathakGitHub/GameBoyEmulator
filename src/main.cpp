#include "application.h"



int main(int argc, char* argv[])
{
	Application app = Application({"GameBoy Emulator", 1080, 720, true});
	app.Run();

	return 0;
}