#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <cmath>
#include <sstream>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <memory>
#include <poll.h>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <fcntl.h>

extern char** environ;
static volatile sig_atomic_t interrupted = 0;
static void onSignal(int signum) { interrupted = signum; }
static bool russian = false;
static const char* tr(const char* en, const char* ru) { return russian ? ru : en; }
static std::string env(const char* key) { const char* v = getenv(key); return v ? v : ""; }

static pid_t spawn(const std::vector<std::string>& args, int input = -1, int output = -1) {
    std::vector<char*> argv;
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    if (input >= 0) posix_spawn_file_actions_adddup2(&actions, input, STDIN_FILENO);
    if (output >= 0) posix_spawn_file_actions_adddup2(&actions, output, STDOUT_FILENO);
    pid_t pid = -1;
    int error = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    if (error) throw std::runtime_error(args[0] + ": " + std::string(strerror(error)));
    return pid;
}

struct Child {
    pid_t pid = -1;
    int finish(bool terminate = true) {
        if (pid <= 0) return 0;
        int status = 0;
        auto result = waitpid(pid, &status, WNOHANG);
        if (result == 0 && terminate) kill(pid, SIGTERM);
        for (int i = 0; result == 0 && i < 50; ++i) {
            usleep(10000);
            result = waitpid(pid, &status, WNOHANG);
        }
        if (result == 0) {
            kill(pid, SIGKILL);
            do { result = waitpid(pid, &status, 0); } while (result < 0 && errno == EINTR);
        }
        pid = -1;
        return result < 0 ? 1 : (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
    }
    ~Child() { finish(); }
};

static double videoAspect(const std::string& path) {
    int fds[2];
    if (pipe2(fds, O_CLOEXEC) != 0) throw std::runtime_error("pipe2");
    Child probe;
    try {
        probe.pid = spawn({"ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                           "stream=width,height,sample_aspect_ratio:side_data=rotation",
                           "-of", "default=noprint_wrappers=1", path}, -1, fds[1]);
    } catch (...) { close(fds[0]); close(fds[1]); throw; }
    close(fds[1]);
    std::string output;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!interrupted && std::chrono::steady_clock::now() < deadline && output.size() < 4096) {
        pollfd descriptor{fds[0], POLLIN | POLLHUP, 0};
        if (poll(&descriptor, 1, 50) <= 0) continue;
        char buffer[512];
        auto count = read(fds[0], buffer, sizeof(buffer));
        if (count <= 0) break;
        output.append(buffer, size_t(count));
    }
    close(fds[0]);
    if (probe.finish(false) != 0) throw std::runtime_error(tr("Video probing failed", "Не удалось определить параметры видео"));
    double width = 0, height = 0, sar = 1, rotation = 0;
    std::istringstream lines(output);
    std::string line;
    while (std::getline(lines, line)) {
        auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        auto key = line.substr(0, equals), value = line.substr(equals + 1);
        if (key == "width") width = std::stod(value);
        else if (key == "height") height = std::stod(value);
        else if (key == "rotation") rotation = std::stod(value);
        else if (key == "sample_aspect_ratio") {
            auto colon = value.find(':');
            if (colon != std::string::npos) {
                double numerator = std::stod(value.substr(0, colon)), denominator = std::stod(value.substr(colon + 1));
                if (numerator > 0 && denominator > 0) sar = numerator / denominator;
            }
        }
    }
    double aspect = width * sar / height;
    if (std::abs(std::remainder(rotation, 180.0)) > 45) aspect = 1.0 / aspect;
    if (!std::isfinite(aspect) || aspect <= 0) throw std::runtime_error(tr("Video probing failed: invalid dimensions", "Некорректные размеры видео"));
    return aspect;
}

struct Options {
    bool game = false, mute = false, windowed = false, stats = false;
    int width = 1280, height = 720, fps = 24, threshold = 128;
    double duration = 0, aspect = 16.0 / 9.0;
    SDL_Color background{0, 0, 170, 255}, foreground{255, 255, 255, 255};
    std::string path, font, reason;
};

static int number(const std::string& value, int lo, int hi) {
    size_t end = 0;
    int result = std::stoi(value, &end);
    if (end != value.size() || result < lo || result > hi) throw std::runtime_error(tr("Number out of range: ", "Число вне диапазона: ") + value);
    return result;
}

static SDL_Color color(std::string value) {
    if (!value.empty() && value[0] == '#') value.erase(0, 1);
    if (value.size() != 6 || value.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        throw std::runtime_error(tr("Expected RRGGBB color", "Ожидается цвет RRGGBB"));
    auto v = std::stoul(value, nullptr, 16);
    return SDL_Color{Uint8(v >> 16), Uint8(v >> 8), Uint8(v), 255};
}

static void help() {
    std::cout << "Fake Panic " << FAKEPANIC_VERSION << "\n"
        << tr("Usage: fakepanic --video FILE [options]\n", "Использование: fakepanic --video ФАЙЛ [параметры]\n")
        << "  --lang en|ru                 " << tr("Language (default: en)", "Язык (по умолчанию en)") << "\n"
        << "  --mute                       " << tr("Disable audio", "Без звука") << "\n"
        << "  --reason TEXT                " << tr("Panic message", "Текст ошибки") << "\n"
        << "  --windowed --size 1280x720    " << tr("Window instead of fullscreen", "Оконный режим") << "\n"
        << "  --fps 24                     " << tr("Frame limit (1–120)", "Частота кадров (1–120)") << "\n"
        << "  --background 0000AA          " << tr("Background color", "Цвет фона") << "\n"
        << "  --foreground FFFFFF          " << tr("Text and dots color", "Цвет текста и точек") << "\n"
        << "  --threshold 128              " << tr("Brightness threshold (0–255)", "Порог яркости (0–255)") << "\n"
        << "  --font FILE                  " << tr("Monospace TTF font", "Моноширинный шрифт TTF") << "\n"
        << "  --duration SECONDS --stats   " << tr("Time limit and FPS output", "Лимит времени и вывод FPS") << "\n"
        << "  --lutris ID | --prism NAME   " << tr("Experimental Gamescope/PipeWire game mode", "Экспериментальный режим Gamescope/PipeWire") << "\n"
        << "  --capture-size 1280x720      " << tr("Game capture size", "Размер захвата игры") << "\n"
        << tr("Esc: exit. F1: send Escape to game. --help, --version\n", "Esc: выход. F1: Escape в игру. --help, --version\n");
}

static Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error(tr("Missing value: ", "Не задано значение: ") + a);
            return argv[i];
        };
        if (a == "--lang") { auto v = value(); if (v != "en" && v != "ru") throw std::runtime_error("--lang: en|ru"); russian = v == "ru"; }
        else if (a == "--game-stream") o.game = true;
        else if (a == "--video") o.path = value();
        else if (a == "--mute") o.mute = true;
        else if (a == "--windowed") o.windowed = true;
        else if (a == "--stats") o.stats = true;
        else if (a == "--reason") o.reason = value();
        else if (a == "--font") o.font = value();
        else if (a == "--fps") o.fps = number(value(), 1, 120);
        else if (a == "--threshold") o.threshold = number(value(), 0, 255);
        else if (a == "--duration") o.duration = number(value(), 1, 86400);
        else if (a == "--background") o.background = color(value());
        else if (a == "--foreground") o.foreground = color(value());
        else if (a == "--size") {
            auto v = value(); auto x = v.find('x');
            if (x == std::string::npos) throw std::runtime_error("--size: WIDTHxHEIGHT");
            o.width = number(v.substr(0, x), 640, 7680); o.height = number(v.substr(x + 1), 480, 4320);
        } else if (a == "--help" || a == "-h") { help(); std::exit(0); }
        else if (a == "--version" || a == "-V") { std::cout << "fakepanic " << FAKEPANIC_VERSION << '\n'; std::exit(0); }
        else if (!a.empty() && a[0] != '-' && o.path.empty()) o.path = a;
        else throw std::runtime_error(tr("Unknown option: ", "Неизвестный параметр: ") + a);
    }
    if (o.game) {
        o.path = env("FAKEPANIC_STREAM");
        if (!env("FAKEPANIC_PIPEWIRE_NODE").empty()) o.path = "pipewire:" + env("FAKEPANIC_PIPEWIRE_NODE");
        if (o.path.empty()) throw std::runtime_error(tr("No game stream", "Не задан поток игры"));
    } else if (o.path.empty() || !std::filesystem::is_regular_file(o.path)) {
        throw std::runtime_error(tr("Video not found: ", "Видео не найдено: ") + o.path);
    } else o.path = std::filesystem::absolute(o.path).string();
    if (!o.game) o.aspect = videoAspect(o.path);
    else if (!env("FAKEPANIC_CAPTURE_WIDTH").empty() && !env("FAKEPANIC_CAPTURE_HEIGHT").empty())
        o.aspect = number(env("FAKEPANIC_CAPTURE_WIDTH"), 320, 3840) / double(number(env("FAKEPANIC_CAPTURE_HEIGHT"), 240, 2160));
    if (o.reason.empty()) o.reason = "Kernel panic - not syncing: Fatal exception";
    return o;
}

static std::string findFont() {
    for (const char* p : {"/usr/share/fonts/TTF/DejaVuSansMono.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                         "/usr/share/fonts/TTF/FreeMono.ttf", "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
                         "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf"})
        if (std::filesystem::is_regular_file(p)) return p;
    throw std::runtime_error(tr("Install DejaVu Sans Mono or use --font", "Установите DejaVu Sans Mono или задайте --font"));
}

struct Video {
    Child child;
    int fd = -1;
    std::atomic<bool> running{true}, ended{false};
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<std::vector<Uint8>> frames;
    std::thread reader;
    void start(const Options& o, int w, int h, int displayW, int displayH, double offset = 0) {
        int fds[2];
        if (pipe2(fds, O_CLOEXEC) != 0) throw std::runtime_error("pipe2");
        const double viewportAspect = double(displayW) / displayH;
        int contentW = w, contentH = h;
        if (o.aspect > viewportAspect) contentH = std::max(1, int(std::lround(h * viewportAspect / o.aspect)));
        else contentW = std::max(1, int(std::lround(w * o.aspect / viewportAspect)));
        std::string filter = "scale=" + std::to_string(contentW) + ":" + std::to_string(contentH) +
            ":flags=area,format=gray,pad=" + std::to_string(w) + ":" + std::to_string(h) + ":(ow-iw)/2:(oh-ih)/2";
        if (!o.game) filter = "fps=" + std::to_string(o.fps) + "," + filter;
        std::vector<std::string> args{"ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin"};
        if (o.game && !env("FAKEPANIC_PIPEWIRE_NODE").empty()) {
            auto serial = env("FAKEPANIC_PIPEWIRE_NODE");
            if (serial.find_first_not_of("0123456789") != std::string::npos) {
                close(fds[0]); close(fds[1]); throw std::runtime_error("Invalid PipeWire serial");
            }
            args = {"gst-launch-1.0", "-q", "pipewiresrc", "target-object=" + serial, "do-timestamp=true",
                    "!", "queue", "leaky=downstream", "max-size-buffers=2", "max-size-bytes=0", "max-size-time=0",
                    "!", "videoscale", "method=1", "add-borders=true", "!", "videoconvert", "!",
                    "video/x-raw,format=GRAY8,width=" + std::to_string(w) + ",height=" + std::to_string(h) + ",pixel-aspect-ratio=" + std::to_string(displayW * h) + "/" + std::to_string(displayH * w),
                    "!", "fdsink", "fd=1", "sync=false"};
        } else {
            if (o.game) args.insert(args.end(), {"-fflags", "nobuffer", "-flags", "low_delay", "-probesize", "32768", "-analyzeduration", "100000"});
            if (!o.game && offset > 0) args.insert(args.end(), {"-ss", std::to_string(offset)});
            args.insert(args.end(), {"-i", o.path, "-filter_threads", "1", "-vf", filter, "-threads", "1", "-f", "rawvideo", "-pix_fmt", "gray", "-an", "-sn", "-"});
        }
        try { child.pid = spawn(args, -1, fds[1]); }
        catch (...) { close(fds[0]); close(fds[1]); throw; }
        close(fds[1]); fd = fds[0];
        reader = std::thread([this, size = size_t(w * h), live = o.game] {
            std::vector<Uint8> buffer(size);
            size_t offset = 0;
            while (running) {
                pollfd p{fd, POLLIN | POLLHUP, 0};
                int ready = poll(&p, 1, 50);
                if (ready < 0) { if (errno == EINTR) continue; break; }
                if (ready == 0) continue;
                ssize_t got = read(fd, buffer.data() + offset, size - offset);
                if (got <= 0) { if (got < 0 && errno == EINTR) continue; break; }
                offset += size_t(got);
                if (offset == size) {
                    std::unique_lock<std::mutex> lock(mutex);
                    if (live) frames.clear();
                    else changed.wait(lock, [this] { return !running || frames.size() < 3; });
                    if (!running) break;
                    frames.push_back(buffer);
                    offset = 0;
                }
            }
            ended = true;
        });
    }
    bool next(std::vector<Uint8>& frame) {
        std::lock_guard<std::mutex> lock(mutex);
        if (frames.empty()) return false;
        frame.swap(frames.front()); frames.pop_front(); changed.notify_one(); return true;
    }
    ~Video() {
        running = false; changed.notify_all();
        child.finish();
        if (reader.joinable()) reader.join();
        if (fd >= 0) close(fd);
    }
};

struct Input {
    Child child;
    FILE* sink = nullptr;
    std::vector<std::string> held;
    void start() {
        auto script = env("FAKEPANIC_INPUT_HELPER");
        if (script.empty()) return;
        int fds[2];
        if (pipe2(fds, O_CLOEXEC) != 0) throw std::runtime_error("pipe2");
        try { child.pid = spawn({"python3", "-u", script}, fds[0]); }
        catch (...) { close(fds[0]); close(fds[1]); throw; }
        close(fds[0]); sink = fdopen(fds[1], "w");
        if (!sink) { close(fds[1]); throw std::runtime_error("fdopen"); }
        setvbuf(sink, nullptr, _IOLBF, 0);
    }
    void send(const std::string& message) { if (sink) fprintf(sink, "%s\n", message.c_str()); }
    void toggle(const std::string& id, bool down) {
        if (!sink || id.empty()) return;
        auto it = std::find(held.begin(), held.end(), id);
        if (down == (it != held.end())) return;
        send(id + (down ? " down" : " up"));
        if (down) held.push_back(id); else held.erase(it);
    }
    void release() { for (const auto& id : held) send(id + " up"); held.clear(); }
    ~Input() { release(); if (sink) fclose(sink); child.finish(false); }
};

static std::string keyName(SDL_Keycode code) {
    if (code >= SDLK_a && code <= SDLK_z) return std::string(1, char(code));
    if (code >= SDLK_0 && code <= SDLK_9) return std::string(1, char(code));
    switch (code) {
        case SDLK_F1: return "escape";
        case SDLK_MINUS: return "minus";
        case SDLK_EQUALS: return "equal";
        case SDLK_PERIOD: return "period";
        case SDLK_COMMA: return "comma";
        case SDLK_SLASH: return "slash";
        case SDLK_BACKSLASH: return "backslash";
        case SDLK_SEMICOLON: return "semicolon";
        case SDLK_QUOTE: return "apostrophe";
        case SDLK_LEFTBRACKET: return "bracketleft";
        case SDLK_RIGHTBRACKET: return "bracketright";
        case SDLK_BACKQUOTE: return "grave";
        case SDLK_LSHIFT: case SDLK_RSHIFT: return "shift";
        case SDLK_LCTRL: case SDLK_RCTRL: return "ctrl";
        case SDLK_LALT: case SDLK_RALT: return "alt";
        case SDLK_RETURN: case SDLK_KP_ENTER: return "enter";
        default: break;
    }
    auto name = std::string(SDL_GetKeyName(code));
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (name.find(' ') != std::string::npos || name.empty()) return {};
    return name;
}

struct Screen {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_Texture* backdrop = nullptr;
    ~Screen() {
        if (backdrop) SDL_DestroyTexture(backdrop);
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit(); SDL_Quit();
    }
    void text(const std::string& s, int x, int y, SDL_Color c, bool centered = true) {
        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, s.c_str(), c);
        if (!surface) throw std::runtime_error(TTF_GetError());
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
        SDL_Rect r{x - (centered ? surface->w / 2 : 0), y, surface->w, surface->h};
        SDL_FreeSurface(surface);
        if (!texture) throw std::runtime_error(SDL_GetError());
        SDL_RenderCopy(renderer, texture, nullptr, &r); SDL_DestroyTexture(texture);
    }
};

static int run(const Options& o) {
    Screen s;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0 || TTF_Init() != 0) throw std::runtime_error(SDL_GetError());
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    int windowW = o.width, windowH = o.height;
    if (!o.windowed) {
        SDL_DisplayMode desktop{};
        if (SDL_GetCurrentDisplayMode(0, &desktop) != 0) throw std::runtime_error(SDL_GetError());
        windowW = desktop.w; windowH = desktop.h;
    }
    s.window = SDL_CreateWindow("Fake Panic", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowW, windowH,
                                (o.windowed ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!s.window) throw std::runtime_error(SDL_GetError());
    SDL_SetWindowMinimumSize(s.window, 640, 480);
    if (!o.game) SDL_ShowCursor(SDL_DISABLE);
    s.renderer = SDL_CreateRenderer(s.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (!s.renderer) s.renderer = SDL_CreateRenderer(s.window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (!s.renderer) throw std::runtime_error(SDL_GetError());
    int w = 0, h = 0, dot = 0, pitch = 0, cellW = 0, cellH = 0;
    int cellsX = 0, cellsY = 0, cols = 0, rows = 0, boxW = 0, boxH = 0, boxX = 0, boxY = 0, innerW = 0, innerH = 0;
    const int pad = 6;
    auto rebuild = [&] {
        SDL_GetRendererOutputSize(s.renderer, &w, &h);
        SDL_RenderSetViewport(s.renderer, nullptr);
        SDL_RenderSetScale(s.renderer, 1.0f, 1.0f);
        if (s.backdrop) { SDL_DestroyTexture(s.backdrop); s.backdrop = nullptr; }
        if (s.font) { TTF_CloseFont(s.font); s.font = nullptr; }
        s.font = TTF_OpenFont((o.font.empty() ? findFont() : o.font).c_str(), std::clamp(h * 12 / 1000, 12, 19));
        if (!s.font) throw std::runtime_error(TTF_GetError());
        TTF_SetFontHinting(s.font, TTF_HINTING_MONO);
        dot = std::clamp(h / 330, 2, 5); pitch = dot * 2;
        cellW = pitch * 2 + dot; cellH = pitch * 4 + dot;
        int referenceCellsX = std::max(18, std::min(int(w * 0.48), w - 40) / cellW / 2 * 2);
        cellsY = std::max(12, std::min(int(h * 0.49) / cellH, referenceCellsX * cellW * 9 / (16 * cellH)));
        innerH = cellsY * cellH;
        int widthLimit = std::max(1, w - 40 - 2 * pad);
        innerW = int(std::lround(std::clamp(innerH * o.aspect, 1.0, double(widthLimit))));
        cellsX = std::max(2, int(std::lround(double(innerW) / cellW / 2)) * 2);
        cols = cellsX * 2; rows = cellsY * 4;
        boxW = innerW + pad * 2; boxH = innerH + pad * 2;
        boxX = (w - boxW) / 2; boxY = (h - boxH) / 2;
        s.backdrop = SDL_CreateTexture(s.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if (!s.backdrop) throw std::runtime_error(SDL_GetError());
        SDL_SetTextureScaleMode(s.backdrop, SDL_ScaleModeNearest);
        SDL_SetRenderTarget(s.renderer, s.backdrop);
        SDL_SetRenderDrawColor(s.renderer, o.background.r, o.background.g, o.background.b, 255); SDL_RenderClear(s.renderer);
        const std::array<std::string, 7> tux{"    .--.        _", "   |o_o |      | |", "   |:_/ |      | |", "  //   \\ \\     | |", " (|     | )    |_|", " /'\\_   _/`\\   (_)", " \\___)=(___/"};
        int line = TTF_FontLineSkip(s.font);
        for (size_t i = 0; i < tux.size(); ++i) s.text(tux[i], 18, 10 + int(i) * line, o.foreground, false);
        SDL_SetRenderDrawColor(s.renderer, o.foreground.r, o.foreground.g, o.foreground.b, 255);
        SDL_Rect border{boxX, boxY, boxW, boxH}; SDL_RenderDrawRect(s.renderer, &border);
        int y = boxY + boxH + std::clamp(h / 38, 14, 34);
        for (const auto& text : {std::string("KERNEL PANIC!"), std::string("Please reboot your computer"), o.reason,
                                std::string("A fatal kernel error has occurred and the system cannot continue."),
                                std::string("Restart the computer to recover. Unsaved data may be lost.")}) {
            s.text(text, w / 2, y, o.foreground); y += line + 5;
        }
        SDL_SetRenderTarget(s.renderer, nullptr);
        SDL_RenderSetViewport(s.renderer, nullptr);
    };
    rebuild();
    auto video = std::make_unique<Video>(); video->start(o, cols, rows, innerW, innerH);
    Input input; if (o.game) input.start();
    Child audio;
    std::vector<Uint8> pixels(size_t(cols * rows), 0);
    std::vector<SDL_Rect> dots; dots.reserve(pixels.size());
    bool gotFrame = false, running = true, audioStarted = false, failed = false;
    uint64_t frames = 0;
    auto launched = std::chrono::steady_clock::now(), playback = launched, statsAt = launched;
    auto nextFrame = launched, awaitingSince = launched;
    bool painted = false;
    while (running && !interrupted) {
        auto now = std::chrono::steady_clock::now();
        SDL_Event e;
        bool resetScene = false;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_RENDER_TARGETS_RESET || e.type == SDL_RENDER_DEVICE_RESET) resetScene = true;
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) running = false;
            if (!o.game) continue;
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                auto name = keyName(e.key.keysym.sym);
                if (!e.key.repeat && e.key.keysym.sym != SDLK_ESCAPE && !name.empty()) input.toggle("key " + name, e.type == SDL_KEYDOWN);
            }
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) input.release();
            if (e.type == SDL_MOUSEMOTION) {
                int logicalW = 0, logicalH = 0; SDL_GetWindowSize(s.window, &logicalW, &logicalH);
                int mouseX = e.motion.x * w / std::max(1, logicalW), mouseY = e.motion.y * h / std::max(1, logicalH);
                int x = std::clamp((mouseX - boxX - pad) * 1280 / (boxW - pad * 2), 0, 1279);
                int yMouse = std::clamp((mouseY - boxY - pad) * 720 / (boxH - pad * 2), 0, 719);
                input.send("move " + std::to_string(x) + " " + std::to_string(yMouse));
            }
            if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                int b = e.button.button == SDL_BUTTON_LEFT ? 1 : e.button.button == SDL_BUTTON_MIDDLE ? 2 : e.button.button == SDL_BUTTON_RIGHT ? 3 : 0;
                if (b) input.toggle("button " + std::to_string(b), e.type == SDL_MOUSEBUTTONDOWN);
            }
            if (e.type == SDL_MOUSEWHEEL && e.wheel.y) input.send(e.wheel.y > 0 ? "wheel up" : "wheel down");
        }
        if (!running) break;
        int outputW = 0, outputH = 0;
        SDL_GetRendererOutputSize(s.renderer, &outputW, &outputH);
        if (outputW > 0 && outputH > 0 && (outputW != w || outputH != h || resetScene)) {
            video.reset();
            rebuild();
            pixels.assign(size_t(cols * rows), 0);
            dots.clear(); dots.reserve(pixels.size());
            video = std::make_unique<Video>(); video->start(o, cols, rows, innerW, innerH, frames / double(o.fps));
            painted = false; gotFrame = false; nextFrame = now; awaitingSince = now;
        }
        if (o.duration && std::chrono::duration<double>(now - launched).count() >= o.duration) break;
        bool updated = false;
        if (now >= nextFrame) {
            updated = video->next(pixels);
            if (updated) {
                if (!gotFrame) { if (frames == 0) playback = now; gotFrame = true; }
                if (!audioStarted && !o.game && !o.mute) {
                    audioStarted = true;
                    audio.pid = spawn({"ffplay", "-nodisp", "-autoexit", "-vn", "-loglevel", "error", "-nostats", "-i", o.path});
                }
                ++frames;
                nextFrame += std::chrono::microseconds(1000000 / o.fps);
                if (nextFrame < now) nextFrame = now;
            } else if (video->ended) { failed = video->child.finish(false) != 0 || !gotFrame; break; }
        }
        if (!gotFrame && std::chrono::duration<double>(now - awaitingSince).count() > 20) { failed = true; break; }
        if (updated || !painted) {
            painted = true;
            SDL_Rect screenRect{0, 0, w, h};
            SDL_RenderCopy(s.renderer, s.backdrop, nullptr, &screenRect);
            SDL_SetRenderDrawColor(s.renderer, o.foreground.r, o.foreground.g, o.foreground.b, 255);
            dots.clear();
            constexpr int pattern[4][2]{{-16, 16}, {32, -32}, {0, 48}, {24, -8}};
            for (int py = 0; py < rows; ++py) for (int px = 0; px < cols; ++px)
                if (int(pixels[size_t(py * cols + px)]) >= std::clamp(o.threshold + pattern[py % 4][px % 2], 0, 255))
                    dots.push_back({boxX + pad + px * std::max(0, innerW - dot) / std::max(1, cols - 1), boxY + pad + (py / 4) * cellH + (py % 4) * pitch, std::min(dot, innerW), dot});
            if (!dots.empty()) SDL_RenderFillRects(s.renderer, dots.data(), int(dots.size()));
            if (!gotFrame) s.text("Waiting for video...", w / 2, boxY + boxH / 2, o.foreground);
            SDL_RenderPresent(s.renderer);
        }
        if (o.stats && now - statsAt >= std::chrono::seconds(3)) {
            std::cerr << "[stats] frames=" << frames << " fps=" << frames / std::max(0.001, std::chrono::duration<double>(now - playback).count()) << '\n';
            statsAt = now;
        }
        SDL_Delay(2);
    }
    if (o.stats) std::cerr << "[stats] frames=" << frames << '\n';
    if (failed) std::cerr << tr("Video decoding/capture failed; see decoder output above.\n", "Ошибка декодирования/захвата; см. вывод декодера выше.\n");
    return interrupted ? 128 + interrupted : (failed ? 1 : 0);
}

int main(int argc, char** argv) {
    russian = false;
    for (int i = 1; i + 1 < argc; ++i) if (std::string(argv[i]) == "--lang") russian = std::string(argv[i + 1]) == "ru";
    signal(SIGPIPE, SIG_IGN); signal(SIGINT, onSignal); signal(SIGTERM, onSignal);
    try { if (argc == 1) { help(); return 0; } return run(parse(argc, argv)); }
    catch (const std::exception& e) { std::cerr << "fakepanic: " << e.what() << '\n'; return 1; }
}
