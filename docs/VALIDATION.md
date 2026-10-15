# Validation / Проверки

## Automated / Автоматические

Every push and pull request runs `.github/workflows/build.yml` on Ubuntu 22.04 and 24.04:

- C++17 Release build with compiler warnings treated as errors.
- Video frame tests for 4:3, anamorphic 16:9, portrait, extreme wide and extremely narrow files: constant height, adaptive width, safe bounds and no unnecessary padding.
- English remains the console default under a Russian locale; explicit Russian launcher errors contain one translation.
- Pixel-level checks after window resizing and fullscreen: horizontally centered video and a raised composition, exact two-color rendering and identical English panic screens for EN/RU locales.
- Real FFmpeg decoding of synthetic video: exact frame count, custom colors and Russian text.
- Invalid arguments, unavailable files and corrupt media return errors.
- SIGTERM interrupts a blocked live stream and leaves no decoder child process.
- Installed launcher works with paths containing spaces, rejects invalid Lutris IDs, and plays video from Bash, Zsh and Fish.
- ShellCheck, Bash syntax checks and Python compilation.
- Ubuntu 22.04 builds `.tar.gz` and `.deb`, checks archive installation, plays two seconds
  of the bundled video with SDL's dummy driver, and installs the Debian package.

Каждый push и pull request проверяется на Ubuntu 22.04 и 24.04:

- Release-сборка C++17; предупреждения компилятора считаются ошибками.
- Проверки видео 4:3, анаморфного 16:9, вертикального, очень широкого и очень узкого: прежняя высота, адаптивная ширина, безопасные границы и отсутствие лишних полей.
- При русской локали консоль по умолчанию на английском; при явном выборе русского ошибки лаунчера содержат один перевод.
- Проверка пикселей после изменения окна и перехода в полный экран: центрирование по горизонтали и поднятая композиция, отсутствие размытых цветов и одинаковый английский экран при EN/RU.
- Реальное декодирование синтетического видео FFmpeg: число кадров, цвета, русский текст.
- Ошибки аргументов, отсутствующие и повреждённые файлы возвращают ошибку.
- SIGTERM прерывает зависший поток без оставшегося дочернего декодера.
- Лаунчер работает с пробелами в путях, отклоняет неверный ID Lutris и воспроизводит видео из Bash, Zsh и Fish.
- ShellCheck, синтаксис Bash и компиляция Python.
- Ubuntu 22.04 собирает `.tar.gz` и `.deb`, проверяет установку архива, воспроизводит
  две секунды Bad Apple через dummy-драйвер SDL и устанавливает пакет Debian.

## Local release review / Локальная проверка релиза

The release is also checked locally on CachyOS x86_64 with SDL2 and FFmpeg.
The README recording captures the real SDL renderer at 1280×720 in an isolated X11 display.
The sample is decoded at the default 24 FPS. The local integration suite also runs
under AddressSanitizer and UndefinedBehaviorSanitizer.

Релиз дополнительно проверяется локально на CachyOS x86_64 с SDL2 и FFmpeg.
Запись в README получена настоящим SDL-рендерером в отдельном X11-дисплее 1280×720.
Пример декодируется со стандартной частотой 24 FPS. Локальные интеграционные тесты
дополнительно выполняются с AddressSanitizer и UndefinedBehaviorSanitizer.

`tests/game-smoke.sh` exercises the full game capture path with a synthetic application
and a temporary Prism-compatible launcher: headless Gamescope → PipeWire → GStreamer → SDL.
It unsets `DISPLAY`, `WAYLAND_DISPLAY` and `HYPRLAND_INSTANCE_SIGNATURE`, so the capture
cannot use the host compositor. About 24 frames per second are displayed; the nested
X11 display is identified for input, and the owned processes are stopped on exit.
This checks capture plumbing, not Minecraft or a commercial game.
The integration suite also launches video playback from Bash, Zsh and Fish.

`tests/game-smoke.sh` проверяет полный тракт игрового захвата с синтетическим приложением
и временным Prism-совместимым лаунчером: Gamescope без окна → PipeWire → GStreamer → SDL.
Переменные `DISPLAY`, `WAYLAND_DISPLAY` и `HYPRLAND_INSTANCE_SIGNATURE` удаляются;
захват не может использовать основной композитор. Выводится около 24 кадров в секунду,
вложенный X11 определяется для ввода, запущенные процессы завершаются при выходе.
Это проверка захвата, а не Minecraft или коммерческой игры.
Интеграционные тесты также воспроизводят видео через Bash, Zsh и Fish.

## Scope / Границы

The suite does not certify every Linux distribution, graphics driver, codec or game.
Game mode depends on Gamescope/Vulkan, PipeWire, GStreamer, launcher process behavior and XTEST.
Personal game installations are not CI fixtures. Raw/relative mouse look is unsupported.
Audio uses a separate FFplay process and is approximately synchronized.
Performance depends on display size, decoder and video codec; no universal FPS or CPU guarantee is made.

Проверки не охватывают все дистрибутивы, драйверы, кодеки и игры.
Игровой режим зависит от Gamescope/Vulkan, PipeWire, GStreamer, поведения лаунчера и XTEST.
Личные игровые установки не используются в CI. Относительное управление мышью не поддерживается.
Звук воспроизводится отдельным FFplay с приблизительной синхронизацией.
Производительность зависит от разрешения, декодера и кодека; универсальной гарантии FPS и нагрузки нет.

Panic text is checked at render time: the fatal-interrupt headline and timestamped
diagnostics must be present; obsolete reboot instructions must be absent.

Текст проверяется при отрисовке: заголовок ошибки прерывания и диагностика с временными
метками присутствуют, старые инструкции о перезагрузке отсутствуют.
