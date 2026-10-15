<div align="center">

# Fake Panic

**Ваше видео. Фальшивая паника ядра. Тысячи маленьких точек.**

[![Релиз](https://img.shields.io/github/v/release/George-Salt/fakepanic?style=flat-square&color=2563eb)](https://github.com/George-Salt/fakepanic/releases/latest)
[![CI](https://github.com/George-Salt/fakepanic/actions/workflows/build.yml/badge.svg)](https://github.com/George-Salt/fakepanic/actions/workflows/build.yml)
[![Загрузки](https://img.shields.io/github/downloads/George-Salt/fakepanic/total?style=flat-square&color=7c3aed)](https://github.com/George-Salt/fakepanic/releases)
[![MIT](https://img.shields.io/badge/code-MIT-22c55e?style=flat-square)](LICENSE)
![Linux](https://img.shields.io/badge/platform-Linux-f59e0b?style=flat-square&logo=linux&logoColor=white)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599c?style=flat-square&logo=cplusplus)

**[English](README.md) · Русский**

[![Посмотреть Fake Panic](docs/media/preview.gif)](https://github.com/George-Salt/fakepanic/raw/refs/heads/main/docs/media/preview.mp4)

[▶ Запись работы — 10 секунд](https://github.com/George-Salt/fakepanic/raw/refs/heads/main/docs/media/preview.mp4) · [Скачать и установить](https://github.com/George-Salt/fakepanic/releases/latest) · [Настроить](#настройки)

</div>

Fake Panic (`fakepanic`) воспроизводит локальное видео внутри синего полноэкранного
экрана паники Linux. Изображение превращается в монохромную сетку точек в стиле Брайля,
рядом — ASCII-пингвин Tux и вымышленные сообщения об ошибках.
**Приложение работает в пользовательском пространстве и не обрушает ядро. Выход — Esc.**

Сверху — запись настоящего рендерера в отдельном виртуальном дисплее X11.
[Bad Apple](examples/bad-apple.mp4) включён в проект, чтобы эффект можно было повторить.
Видеорежим работает через SDL2 в X11 и Wayland. Экспериментальный игровой режим
захватывает Lutris или Prism Launcher через Gamescope в Hyprland.

## Установка

### Готовый релиз

Откройте [Releases](https://github.com/George-Salt/fakepanic/releases/latest).
Для локальной установки выберите **Linux x86_64 `.tar.gz`**, для Ubuntu/Debian — **`.deb`**.
Бинарники собираются в Ubuntu 22.04. Архив использует системные библиотеки;
это не AppImage. Для других архитектур доступна сборка из исходников.

Сначала установите зависимости:

```bash
# Ubuntu / Debian
sudo apt install libsdl2-2.0-0 libsdl2-ttf-2.0-0 ffmpeg fonts-dejavu-core python3

# Arch / CachyOS
sudo pacman -S --needed sdl2 sdl2_ttf ffmpeg ttf-dejavu python
```

Для архива выполните команды из папки с загруженными файлами:

```bash
sha256sum -c SHA256SUMS --ignore-missing  # также скачайте SHA256SUMS
mkdir -p fakepanic-release
tar -xzf fakepanic-1.0.0-linux-x86_64.tar.gz -C fakepanic-release
cd fakepanic-release/fakepanic-1.0.0-linux-x86_64
./install.sh
~/.local/bin/fakepanic --version
```

Установщик записывает файлы в `~/.local/bin` и `~/.local/share/fakepanic`; root не нужен.
При необходимости добавьте `~/.local/bin` в `PATH` (`fish_add_path ~/.local/bin` для Fish).
Другой путь: `FAKEPANIC_PREFIX=/ваш/путь ./install.sh`.
Можно запускать `./bin/fakepanic` прямо из распакованного архива.

Для `.deb` используйте APT, чтобы он установил зависимости:

```bash
sudo apt install ./fakepanic-1.0.0-linux-x86_64.deb
fakepanic --version
```

### Из исходников

```bash
# Ubuntu / Debian
sudo apt install build-essential cmake pkg-config libsdl2-dev libsdl2-ttf-dev ffmpeg fonts-dejavu-core python3

# Arch / CachyOS
sudo pacman -S --needed base-devel cmake pkgconf sdl2 sdl2_ttf ffmpeg ttf-dejavu python

git clone https://github.com/George-Salt/fakepanic.git
cd fakepanic
./install.sh
```

Ручная сборка и проверки:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
# Установка по желанию:
cmake --install build --prefix "$HOME/.local"
```

## Запуск видео

Из корня репозитория:

```bash
fakepanic --video examples/bad-apple.mp4
fakepanic --video "/путь/к моему видео.mp4" --mute
fakepanic --video examples/bad-apple.mp4 --lang en
```

Релизные пакеты тоже устанавливают пример:

```bash
# Локальная установка из архива или исходников
fakepanic --video ~/.local/share/fakepanic/examples/bad-apple.mp4
# Системная установка .deb
fakepanic --video /usr/share/fakepanic/examples/bad-apple.mp4
```

`fakepanic ФАЙЛ` — сокращение для `fakepanic --video ФАЙЛ`.
После окончания видео приложение закрывается. Esc немедленно завершает эффект.
Изображение декодирует FFmpeg, звук отдельно воспроизводит FFplay;
синхронизация звука приблизительная. Интернет для воспроизведения не нужен.

## Настройки

![Цвета](https://img.shields.io/badge/colors-RRGGBB-2563eb?style=flat-square)
![FPS](https://img.shields.io/badge/FPS-1–120-7c3aed?style=flat-square)
![Языки](https://img.shields.io/badge/language-EN%20%2F%20RU-22c55e?style=flat-square)
![Окно](https://img.shields.io/badge/window-fullscreen%20%2F%20windowed-f59e0b?style=flat-square)

| Параметр | По умолчанию | Действие |
| --- | --- | --- |
| `--lang en\|ru` | Язык системы (`ru*` → русский) | Справка, интерфейс и сообщения приложения |
| `--background RRGGBB` | `0000AA` | Цвет фона; `#RRGGBB` тоже поддерживается в кавычках |
| `--foreground RRGGBB` | `FFFFFF` | Цвет текста, рамки и точек |
| `--threshold 0…255` | `128` | Меньше значение — больше светящихся точек; дизеринг сохраняет детали |
| `--fps 1…120` | `24` | Частота преобразования и воспроизведения видео |
| `--windowed` | Полный экран | Фиксированное окно |
| `--size ШИРИНАxВЫСОТА` | `1280x720` | Размер окна, от `640x480` до `7680x4320` |
| `--reason ТЕКСТ` | Вымышленное критическое исключение | Свой текст ошибки, в том числе на русском |
| `--font ФАЙЛ` | DejaVu Sans Mono | Другой моноширинный TTF-шрифт |
| `--mute` | Со звуком | Выключить FFplay |
| `--duration СЕКУНДЫ` | Всё видео | Завершить через 1–86400 секунд с учётом запуска |
| `--stats` | Выключено | Выводить число кадров и средний FPS в stderr |

```bash
# Мятный цвет на тёмном фоне
fakepanic --video examples/bad-apple.mp4 --background 111827 --foreground A7F3D0

# В окне, без звука, со своим сообщением
fakepanic --video examples/bad-apple.mp4 --windowed --size 1280x720 \
  --mute --fps 30 --reason 'Kernel panic - not syncing: Bad Apple!!'

fakepanic --help
fakepanic --lang en --help
```

Пропорции видео сохраняются с помощью полей. Точки рисуются напрямую и не зависят
от наличия символов Брайля в шрифте. Сетка подстраивается под размер экрана.
Сторонние инструменты могут выводить собственные сообщения на английском.

## Экспериментальный игровой режим

Нужен активный сеанс **Hyprland 0.56+** с Lua API диспетчеров, Gamescope,
`wf-recorder`, `jq`, Python 3, `libX11`, `libXtst` и выбранный лаунчер.
Для обычного видео эти игровые зависимости не нужны.

```bash
# Arch / CachyOS: дополнительные зависимости
sudo pacman -S --needed gamescope wf-recorder jq libx11 libxtst
# Lutris и/или Prism Launcher установите отдельно.

fakepanic --lutris 42 --stats
fakepanic --prism "Моя сборка" --stats
fakepanic --prism "Моя сборка" --capture-size 960x540 --fps 30
```

В Lutris укажите локальный ID своей игры, в Prism — имя существующей сборки.
Лаунчер запускает новый процесс Gamescope и создаёт собственный временный виртуальный
монитор. `--monitor ИМЯ` задаёт имя нового монитора; существующий монитор не используется.
При выходе приложение удаляет этот монитор и завершает запущенные им захват и Gamescope.
Некоторые лаунчеры могут оставить игру отдельным процессом; при необходимости закройте её обычным способом.

**Ограничения:** режим экспериментальный, захват добавляет задержку. Через XTEST
передаются основные клавиши, кнопки мыши, колесо и абсолютные координаты курсора.
**Относительный ввод и raw mouse look не поддерживаются**, поэтому управление камерой
Minecraft ограничено. Esc закрывает Fake Panic, F1 отправляет Escape в игру.
Лаунчеры с повторным использованием уже работающего процесса могут оказаться вне Gamescope;
в таких случаях автоматическое определение дисплея тоже может не сработать.
Задавайте `FAKEPANIC_GAME_DISPLAY=:N` только для известного вложенного дисплея.
Передача ввода в основной Xwayland рабочего стола по умолчанию запрещена.

Автотесты проверяют рендерер, обработку аргументов лаунчера и завершение потока.
Коммерческие игры и личные сборки Prism в CI не запускаются.
Подробнее: [границы проверок](docs/VALIDATION.md).

## Решение проблем

| Симптом | Что проверить |
| --- | --- |
| `fakepanic: command not found` | Добавьте `~/.local/bin` в `PATH` или укажите полный путь |
| Нет рендерера или библиотеки | Установите зависимости; при несовместимом ABI пересоберите на своём дистрибутиве |
| Ошибка шрифта или нет русских букв | Установите DejaVu Sans Mono или задайте `--font /путь/к/шрифту.ttf` |
| Не декодируется видео | Выполните `ffmpeg -v error -i ФАЙЛ -f null -`; посмотрите stderr Fake Panic |
| Нет звука | Проверьте FFplay и аудиовыход: `ffplay -nodisp -autoexit ФАЙЛ` |
| Ожидание захвата игры | Смотрите `~/.local/state/fakepanic/` или `$XDG_STATE_HOME/fakepanic/` |
| Нет ввода в игре | Проверьте вложенный X11 и XTEST; относительный ввод мыши не поддерживается |
| Ошибка API Hyprland | Сверьте версию и [Lua-диспетчеры](https://wiki.hypr.land/configuring/core/dispatchers/) |

Ошибка декодирования или запуска возвращает ненулевой код завершения. Ожидание
первого кадра ограничено 20 секундами. Ctrl+C / SIGTERM завершают дочерние процессы.

## Устройство

```text
Видео → FFmpeg → оттенки серого → точки с дизерингом → SDL2
Игра  → Gamescope → виртуальный монитор Hyprland → wf-recorder → FFmpeg → SDL2
Ввод  ← вложенный X11 Gamescope ← мост XTEST ← события SDL2
```

Рендерер кэширует статичный текст, рисует точки одним пакетом и хранит в очереди не более
трёх видеокадров. Для живого захвата сохраняется только последний ожидающий кадр, чтобы
не накапливать задержку. Интерфейс обрабатывает события, пока декодер ждёт данные;
отрисовка выполняется при появлении нового кадра. Проект предназначен для визуального
эффекта и экспериментов; задержка захвата ограничивает игровые сценарии.

## Удаление

Для стандартной локальной установки:

```bash
rm -f ~/.local/bin/fakepanic
rm -rf ~/.local/share/fakepanic ~/.local/share/licenses/fakepanic
# По желанию: удалить логи игрового режима
rm -rf ~/.local/state/fakepanic
```

Пакет Debian: `sudo apt remove fakepanic`. При своём префиксе структура такая же.

## Проект

[Участие / Contributing](CONTRIBUTING.md) · [Безопасность / Security](SECURITY.md) · [История изменений](CHANGELOG.md)

Код — [MIT](LICENSE). Bad Apple и записи на его основе — сторонние медиа,
исключённые из MIT; подробности в [описании примера](examples/README.md).
Сообщения о перезагрузке и ошибках — часть эффекта. Запускайте его на своём экране
или с ведома владельца компьютера.
