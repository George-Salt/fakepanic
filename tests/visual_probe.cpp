#include <SDL.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static SDL_Window* window = nullptr;

extern "C" SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags) {
    auto create = reinterpret_cast<decltype(&SDL_CreateWindow)>(dlsym(RTLD_NEXT, "SDL_CreateWindow"));
    window = create(title, x, y, w, h, flags);
    return window;
}

extern "C" void SDL_RenderPresent(SDL_Renderer* renderer) {
    auto present = reinterpret_cast<decltype(&SDL_RenderPresent)>(dlsym(RTLD_NEXT, "SDL_RenderPresent"));
    static int frame = 0;
    ++frame;
    if (frame == 3 || frame == 10 || frame == 18) {
        const char* directory = std::getenv("FAKEPANIC_VISUAL_OUTPUT");
        if (directory) {
            int w = 0, h = 0;
            SDL_GetRendererOutputSize(renderer, &w, &h);
            std::vector<unsigned char> pixels(size_t(w * h * 3));
            if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGB24, pixels.data(), w * 3) != 0) std::abort();
            std::string path = std::string(directory) + "/" + std::to_string(frame) + ".ppm";
            FILE* file = std::fopen(path.c_str(), "wb");
            if (!file) std::abort();
            std::fprintf(file, "P6\n%d %d\n255\n", w, h);
            std::fwrite(pixels.data(), 1, pixels.size(), file);
            std::fclose(file);
        }
    }
    present(renderer);
    if (frame == 4) SDL_SetWindowSize(window, 960, 540);
    if (frame == 12) SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
}
