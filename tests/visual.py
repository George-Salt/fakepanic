#!/usr/bin/env python3
"""Check real rendered pixels across resizing, fullscreen and language changes."""
import os
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile

binary, probe = map(str, map(Path.resolve, map(Path, sys.argv[1:3])))
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    local_probe = root / 'probe.so'
    shutil.copy2(probe, local_probe)
    clip = root / 'white.mp4'
    subprocess.run(['ffmpeg', '-v', 'error', '-f', 'lavfi', '-i',
                    'color=white:size=320x240:rate=24', '-t', '2', '-c:v', 'mpeg4', str(clip)], check=True)
    images = {}
    for language in ('en', 'ru'):
        output = root / language
        output.mkdir()
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software',
                   LD_PRELOAD=str(local_probe), FAKEPANIC_VISUAL_OUTPUT=str(output),
                   SDL_RENDER_SCALE_QUALITY='linear', LANG='ru_RU.UTF-8')
        subprocess.run([binary, '--video', str(clip), '--mute', '--windowed', '--size',
                        '640x480', '--lang', language], env=env, check=True, timeout=10)
        for frame, expected in [(3, (640, 480)), (10, (960, 540)), (18, (1024, 768))]:
            data = (output / f'{frame}.ppm').read_bytes()
            header, dimensions, maximum, pixels = data.split(b'\n', 3)
            w, h = map(int, dimensions.split())
            assert (w, h) == expected, (frame, w, h)
            assert header == b'P6' and maximum == b'255'
            colors = {pixels[i:i + 3] for i in range(0, len(pixels), 3)}
            assert colors == {b'\x00\x00\xaa', b'\xff\xff\xff'}, (frame, colors)
            borders = []
            for y in range(h):
                row = pixels[y * w * 3:(y + 1) * w * 3]
                runs = row.split(b'\x00\x00\xaa')
                longest = max(runs, key=len)
                if len(longest) >= w * 3 * 0.4 and longest == b'\xff\xff\xff' * (len(longest) // 3):
                    left = row.index(longest) // 3
                    borders.append((y, left, len(longest) // 3))
            assert len(borders) == 2, (frame, borders)
            top, bottom = borders
            assert abs(top[0] + bottom[0] - (h - 1)) <= 1, (frame, borders)
            assert abs(2 * top[1] + top[2] - w) <= 1, (frame, borders)
            if language == 'en':
                images[frame] = data
            else:
                assert data == images[frame], f'Panic screen depends on language at frame {frame}'
    print('Centered, pixel-sharp English panic screen verified after resize and fullscreen.')
