# TemportalEngine

## Setup

1. Install [cmake 3.14](https://cmake.org/download/)
2. Clone recursive modules
```
git clone --recurse-submodules --single-branch -branch master git@github.com:temportalflux/TemportalEngine.git
```
3. (Optional) Install Clion

## Requirements

1. SDL2
	Source: https://www.libsdl.org/download-2.0.php
	Version: Windows Development Library `SDL2-devel-2.0.12-VC`
	Directory: `lib/SDL2`
2. Vulkan-HPP
	Source: https://github.com/KhronosGroup/Vulkan-Headers/releases
	Version: `Vulkan-Headers-sdk-1.2.135.0`
	Directory: `lib/Vulkan`
3. RakNet: Binaries/Lib files saved from old work in networking classes.
	Should be replaced with a more active environment (RakNet was discontinued many years ago.)
	[SLikeNet](https://www.slikesoft.com/?page_id=1224&lang=en) seems to be a decent replacement,
	and has a moderately active [GitHub repository](https://github.com/SLikeSoft/SLikeNet).
