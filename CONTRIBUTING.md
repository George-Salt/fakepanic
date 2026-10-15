# Contributing / Участие в разработке

**English.** Open a bug report or feature request in English or Russian. For code changes,
fork the repository, create a branch, and submit a pull request. Keep changes focused.
Update both README versions when behavior changes. Comments should explain decisions,
not restate the code. Never commit credentials, personal paths, logs or build output.
The supplied Bad Apple video is an explicit media exception; do not add unrelated assets.

**Русский.** Сообщения об ошибках и предложения принимаются на русском и английском.
Для изменений сделайте fork, создайте ветку и откройте pull request. Один PR — одна задача.
При изменении поведения обновите обе версии README. Комментарии должны объяснять
решения, а не пересказывать код. Не добавляйте пароли, личные пути, логи и сборочные файлы.
Bad Apple включён отдельно как пример; посторонние медиафайлы не нужны.

## Checks / Проверки

Install dependencies from the README, then run / Установите зависимости из README и выполните:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
shellcheck install.sh scripts/fakepanic scripts/start-game.sh packaging/package.sh
python3 -m py_compile scripts/input-bridge.py tests/integration.py
```

Tests use synthetic video and SDL's dummy display; no desktop or game is needed.
For renderer changes also check a real graphical session. Game changes need Hyprland,
Gamescope and an actual launcher; mention untested combinations explicitly.

Тесты используют синтетическое видео и виртуальный драйвер SDL; рабочий стол и игра не нужны.
Изменения отрисовки проверяйте также в графическом сеансе. Изменения игрового режима
проверяйте с Hyprland, Gamescope и настоящим лаунчером; указывайте непроверенные сочетания.
