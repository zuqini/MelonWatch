#include "A3.hpp"

#include <iostream>
using namespace std;

int main( int argc, char **argv ) 
{
	std::string luaSceneFile("Assets/rifleScene.lua");
	CS488Window::launch(argc, argv, new A3(luaSceneFile), 1024, 768, "MelonWatch");

	return 0;
}
