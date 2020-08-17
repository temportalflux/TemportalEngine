# Temportal Engine
-----

This is a "fun" side-project which explores various areas of game and game-engine development.

## Rendering

### Vulkan 1.1.x (Apache License Version 2.0)

The first and largest hurdle of any good game is what the user can interact with and see. That means we definitely need some kind of graphics.
I decided to poke around at using Vulkan, as this projects intent was and continues to be to push myself to learn more.
Here are some resources for you (and myself) to get started with Vulkan!
- Vulkan SDK: https://vulkan.lunarg.com/sdk/home#windows (this will set the `VULKAN_SDK` system environment path, which is required for setup)
- LunarG Vulkan SDK Docs: https://vulkan.lunarg.com/doc/sdk/1.2.135.0/windows/getting_started.html
- Vulkan-HPP: https://github.com/KhronosGroup/Vulkan-Hpp
- Vulkan SDK tutorial: https://vulkan-tutorial.com/en/Overview
- Vulkan Dos & Don'ts: https://developer.nvidia.com/blog/vulkan-dos-donts/
- Efficient rendering: https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

### Shader Compilation

To compile shaders, the editor uses libshaderc

1. Download the Debug and Release build zip files from https://github.com/google/shaderc/blob/master/downloads.md
2. Create `libs\shaderc`
3. Copy the contents of (Debug build zip) `install\include\shaderc\` to `Editor\libs\shaderc\include\Debug\shaderc`
4. Copy (Debug build zip) `install\lib\shaderc_combined.lib` to ``Editor\libs\shaderc\lib\Debug\shaderc_combined.lib`
5. Copy the contents of (Release build zip) `install\include\shaderc\` to ``Editor\libs\shaderc\include\Release\shaderc`
6. Copy (Release build zip) `install\lib\shaderc_combined.lib` to ``Editor\libs\shaderc\lib\Release\shaderc_combined.lib`
