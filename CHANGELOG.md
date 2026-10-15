# Changelog / История изменений

## Unreleased / Не выпущено

- Keep two timestamped diagnostic lines, set the CPU timestamp to `142.881093`, and show a reboot instruction after a blank line.
- Оставлены две диагностические строки; время строки CPU — `142.881093`, после пустой строки добавлена просьба перезагрузить компьютер.

## 1.0.3 — 2026-10-15

### English

- Replace the reboot instructions with an English fatal-interrupt panic headline and timestamped kernel-style diagnostics.
- Shift the video and text together slightly above the center of the screen.
- Re-record the presentation near the middle of Bad Apple with a constant blue background.

### Русский

- Вместо инструкций перезагрузки — английский заголовок ошибки прерывания и строки в стиле ядра с временными метками.
- Видео и текст вместе подняты немного выше центра экрана.
- Презентация перезаписана примерно на середине Bad Apple, с постоянным синим фоном.

## 1.0.2 — 2026-10-15

### English

- Adapt the video frame width to its display aspect ratio while preserving its height.
- Cap width within the window, with 20-pixel side margins and no cropping.
- Respect sample aspect ratio and rotation metadata; remove unnecessary 16:9 padding for Bad Apple.
- Default to English console output; select Russian explicitly with `--lang ru`, without duplicate translations.
- Standardize future commit messages with Conventional Commits.
- Refresh the presentation with native 4:3 framing and blue/mint themes.

### Русский

- Ширина рамки подстраивается под пропорции ролика при прежней высоте.
- Ширина ограничена окном с отступами 20 пикселей, без обрезания содержимого.
- Учитываются пропорции пикселя и поворот; у Bad Apple нет лишних полей от рамки 16:9.
- Консоль по умолчанию на английском; русский включается через `--lang ru`, без дублирования переводов.
- Для новых коммитов принят формат Conventional Commits.
- Обновлена презентация: рамка 4:3, синее и мятное оформление.

## 1.0.1 — 2026-10-15

### English

- Keep the video centered when resizing or entering fullscreen.
- Rebuild text and dot geometry at the actual output resolution instead of stretching the cached scene.
- Keep the panic screen in English regardless of locale or `--lang`.
- Add pixel-level regression checks for resizing, fullscreen, sharpness and language independence.

### Русский

- Видео остаётся по центру при изменении размера окна и переходе в полный экран.
- Текст и сетка пересоздаются в реальном разрешении вместо растягивания готовой сцены.
- Экран паники всегда на английском независимо от локали и `--lang`.
- Добавлены проверки реальных пикселей: размер, центрирование, чёткость и независимость от языка.

## 1.0.0 — 2026-10-15

### English

- First public release of Fake Panic for Linux.
- Local video playback in a simulated kernel panic screen with Braille-style dots.
- English/Russian interface, color and threshold settings, frame limit, windowed mode and custom messages.
- Cached static scene, batched dot drawing, bounded decoding queue and signal cleanup.
- Desktop-independent headless Gamescope/PipeWire capture for Lutris and Prism Launcher.
- Bash, Zsh and Fish support, with shell-specific PATH instructions.
- Included Bad Apple sample, short renderer recording, bilingual documentation and issue forms.
- Integration tests, Ubuntu 22.04/24.04 CI, installable x86_64 archive and Debian package.

### Русский

- Первый публичный релиз Fake Panic для Linux.
- Локальное видео в имитации паники ядра с точками в стиле Брайля.
- Интерфейс EN/RU, цвета, порог яркости, лимит FPS, оконный режим и свой текст ошибки.
- Кэширование статичной сцены, пакетная отрисовка точек, ограниченная очередь кадров и обработка сигналов.
- Независимый от рабочего стола захват Gamescope/PipeWire для Lutris и Prism Launcher.
- Поддержка Bash, Zsh и Fish с инструкциями настройки PATH.
- Пример Bad Apple, короткая запись рендерера, двуязычная документация и формы issue.
- Интеграционные тесты, CI Ubuntu 22.04/24.04, установочный архив x86_64 и пакет Debian.
