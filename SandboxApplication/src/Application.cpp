#include "Rose/Core/Application.h"


int main()
{
	using namespace Rose;


	Application* app = new Application;
	app->Run();

	delete app;
}