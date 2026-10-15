# Validation / Проверки

## Automated / Автоматические

Every push and pull request runs `.github/workflows/build.yml` on Ubuntu 22.04 and 24.04:

- C++17 Release build with compiler warnings treated as errors.
- Real FFmpeg decoding of synthetic video: exact frame count, custom colors and Russian text.
- Invalid arguments, unavailable files and corrupt media return errors.
- SIGTERM interrupts a blocked live stream and leaves no decoder child process.
- Installed launcher works with paths containing spaces and rejects invalid Lutris IDs.
- ShellCheck, Bash syntax checks and Python compilation.
- Ubuntu 22.04 builds `.tar.gz` and `.deb`, checks archive installation, plays two seconds
  of the bundled video with SDL's dummy driver, and installs the Debian package.

Каждый push и pull request проверяется на Ubuntu 22.04 и 24.04:

- Release-сборка C++17; предупреждения компилятора считаются ошибками.
- Реальное декодирование синтетического видео FFmpeg: число кадров, цвета, русский текст.
- Ошибки аргументов, отсутствующие и повреждённые файлы возвращают ошибку.
- SIGTERM прерывает зависший поток без оставшегося дочернего декодера.
- Установленный лаунчер работает с пробелами в путях и отклоняет неверный ID Lutris.
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

## Scope / Границы

The suite does not certify every Linux distribution, graphics driver, codec or game.
Game mode depends on Hyprland's Lua API, Gamescope, launcher process behavior and XTEST.
Personal game installations are not CI fixtures. Raw/relative mouse look is unsupported.
Audio uses a separate FFplay process and is approximately synchronized.
Performance depends on display size, decoder and video codec; no universal FPS or CPU guarantee is made.

Проверки не охватывают все дистрибутивы, драйверы, кодеки и игры.
Игровой режим зависит от Lua API Hyprland, Gamescope, поведения процессов лаунчера и XTEST.
Личные игровые установки не используются в CI. Относительное управление мышью не поддерживается.
Звук воспроизводится отдельным FFplay с приблизительной синхронизацией.
Производительность зависит от разрешения, декодера и кодека; универсальной гарантии FPS и нагрузки нет.
