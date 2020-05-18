#include <SDL.h>
//#include <SDL_vulkan.h>

#include <imgui.h>
#include <examples/imgui_impl_sdl.h>
//#include <examples/imgui_impl_vulkan.h>

int main()
{

	auto window = SDL_CreateWindow("Editor IMGUI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO();

	//ImGui_ImplSDL2_InitForVulkan(window);
	//ImGui_ImplVulkan_Init()
	
	bool bShouldQuit = false;
	while (!bShouldQuit)
	{


		// Handle exiting the window

		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_QUIT)
			{
				bShouldQuit = true;
				break;
			}
		}

	}

	ImGui::DestroyContext();

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
