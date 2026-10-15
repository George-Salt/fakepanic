#!/usr/bin/env python3
"""Forward fakepanic input to the game's nested Gamescope Xwayland display."""
import ctypes
import ctypes.util
import os
import sys
from pathlib import Path


def game_display():
    if os.getenv('FAKEPANIC_GAME_DISPLAY'):
        return os.environ['FAKEPANIC_GAME_DISPLAY']
    root = int(os.environ.get('FAKEPANIC_GAMESCOPE_PID', '0'))
    if root <= 1:
        raise RuntimeError('Gamescope PID required / Требуется PID Gamescope')
    candidates = []
    host_display = os.getenv('DISPLAY', '')
    for proc in Path('/proc').iterdir():
        if not proc.name.isdigit():
            continue
        try:
            ancestor = int(proc.name)
            seen = set()
            while ancestor > 1 and ancestor != root and ancestor not in seen:
                seen.add(ancestor)
                status = Path(f'/proc/{ancestor}/status').read_text()
                ancestor = int(next(line.split()[1] for line in status.splitlines()
                                    if line.startswith('PPid:')))
            if ancestor != root or int(proc.name) == root:
                continue
            entries = (proc / 'environ').read_bytes().split(b'\0')
            process_env = dict(v.split(b'=', 1) for v in entries if b'=' in v)
            display = process_env.get(b'DISPLAY', b'').decode()
            if display and display != host_display:
                candidates.append(display)
        except (OSError, ValueError, StopIteration):
            continue
    displays = set(candidates)
    if len(displays) != 1:
        raise RuntimeError('Cannot identify one nested DISPLAY; set FAKEPANIC_GAME_DISPLAY / '
                           'Не удалось определить DISPLAY; задайте FAKEPANIC_GAME_DISPLAY')
    return displays.pop()


def main():
    display = game_display()
    if display == os.getenv('DISPLAY') and not os.getenv('FAKEPANIC_ALLOW_HOST_X11'):
        raise RuntimeError('Refusing desktop input / Ввод в основной рабочий стол запрещён')
    x11 = ctypes.CDLL(ctypes.util.find_library('X11') or 'libX11.so.6')
    xtst = ctypes.CDLL(ctypes.util.find_library('Xtst') or 'libXtst.so.6')
    x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
    x11.XOpenDisplay.restype = ctypes.c_void_p
    x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
    x11.XFlush.argtypes = [ctypes.c_void_p]
    x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
    x11.XStringToKeysym.restype = ctypes.c_ulong
    x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
    x11.XKeysymToKeycode.restype = ctypes.c_uint
    xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
    xtst.XTestFakeButtonEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
    xtst.XTestFakeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_ulong]
    connection = x11.XOpenDisplay(display.encode())
    if not connection:
        raise RuntimeError(f'Cannot connect to nested display {display}')
    held = set()
    aliases = {'up': 'Up', 'down': 'Down', 'left': 'Left', 'right': 'Right',
               'enter': 'Return', 'backspace': 'BackSpace', 'tab': 'Tab',
               'shift': 'Shift_L', 'ctrl': 'Control_L', 'alt': 'Alt_L',
               'escape': 'Escape', 'space': 'space', 'delete': 'Delete',
               'home': 'Home', 'end': 'End', 'minus': 'minus',
               'period': 'period', 'comma': 'comma', 'slash': 'slash'}
    def keycode(name):
        symbol = aliases.get(name, name)
        return x11.XKeysymToKeycode(connection, x11.XStringToKeysym(symbol.encode()))
    print(f'[input] ready on {display}; mouse positioning and keyboard forwarding enabled', file=sys.stderr, flush=True)
    try:
        for line in sys.stdin:
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == 'move' and len(parts) == 3:
                x, y = int(parts[1]), int(parts[2])
                xtst.XTestFakeMotionEvent(connection, -1, x, y, 0)
            elif parts[0] == 'wheel' and len(parts) == 2:
                button = 4 if parts[1] == 'up' else 5
                xtst.XTestFakeButtonEvent(connection, button, 1, 0)
                xtst.XTestFakeButtonEvent(connection, button, 0, 0)
            elif len(parts) == 3 and parts[0] == 'key':
                name, action = parts[1:]
                code = keycode(name)
                if not code:
                    continue
                ident = ('key', name)
                down = action == 'down'
                if down == (ident in held):
                    continue
                xtst.XTestFakeKeyEvent(connection, code, int(down), 0)
                (held.add if down else held.discard)(ident)
            elif len(parts) == 3 and parts[0] == 'button':
                button = int(parts[1])
                if button not in (1, 2, 3):
                    continue
                ident = ('button', button)
                down = parts[2] == 'down'
                if down == (ident in held):
                    continue
                xtst.XTestFakeButtonEvent(connection, button, int(down), 0)
                (held.add if down else held.discard)(ident)
            x11.XFlush(connection)
    finally:
        for kind, name in held:
            if kind == 'key':
                code = keycode(name)
                if code:
                    xtst.XTestFakeKeyEvent(connection, code, 0, 0)
            else:
                xtst.XTestFakeButtonEvent(connection, name, 0, 0)
        x11.XFlush(connection)
        x11.XCloseDisplay(connection)


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print(f'[input] ERROR: {exc}', file=sys.stderr, flush=True)
        sys.exit(1)
