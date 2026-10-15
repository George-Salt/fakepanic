#!/usr/bin/env python3
"""Check centering, aspect-aware bounds and crisp pixels through resize/fullscreen."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

binary, probe = map(str, map(Path.resolve, map(Path, sys.argv[1:3])))
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    local_probe = root / 'probe.so'
    shutil.copy2(probe, local_probe)
    images, heights = {}, {}
    for case, sar, aspect in [('standard', '1/1', 4/3), ('wide', '4/3', 16/9),
                              ('portrait', '27/64', 9/16), ('extreme', '15/1', 20), ('needle', '1/1', 1/1000)]:
        clip = root / f'{case}.nut'
        source_size = '2x2000' if case == 'needle' else '320x240'
        subprocess.run(['ffmpeg', '-v', 'error', '-f', 'lavfi', '-i',
                        f'color=white:size={source_size}:rate=24', '-vf', f'setsar={sar}:max=10000',
                        '-t', '2', '-c:v', 'rawvideo', '-pix_fmt', 'gray', '-threads', '1', str(clip)], check=True)
        for language in (('en', 'ru') if case == 'standard' else ('en',)):
            output = root / f'{case}-{language}'
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
                assert (w, h) == expected, (case, frame, w, h)
                assert header == b'P6' and maximum == b'255'
                colors = {pixels[i:i + 3] for i in range(0, len(pixels), 3)}
                assert colors == {b'\x00\x00\xaa', b'\xff\xff\xff'}, (case, frame, colors)
                borders = []
                for y in range(h):
                    row = pixels[y * w * 3:(y + 1) * w * 3]
                    longest = max(row.split(b'\x00\x00\xaa'), key=len)
                    if len(longest) < 12 * 3 or longest != b'\xff\xff\xff' * (len(longest) // 3):
                        continue
                    left, width = row.index(longest) // 3, len(longest) // 3
                    if abs(2 * left + width - w) <= 1:
                        borders.append((y, left, width))
                assert len(borders) == 2, (case, frame, borders)
                top, bottom = borders
                assert abs(top[0] + bottom[0] - (h - 1)) <= 1, (case, frame, borders)
                inner_h, inner_w = bottom[0] - top[0] + 1 - 12, top[2] - 12
                assert top[1] >= 20 and top[1] + top[2] <= w - 20
                assert abs(inner_w - min(round(inner_h * aspect), w - 52)) <= 1, (case, frame, inner_w, inner_h)
                if case == 'standard':
                    heights[frame] = inner_h
                    if language == 'en':
                        images[frame] = data
                    else:
                        assert data == images[frame], f'Panic screen depends on language at frame {frame}'
                else:
                    assert inner_h == heights[frame], f'Video height depends on aspect: {case}'
                if inner_h * aspect <= w - 52:
                    index = ((top[0] + 6) * w + top[1] + 6) * 3
                    assert pixels[index:index + 3] == b'\xff\xff\xff', f'Unnecessary padding: {case}'
    print('Aspect-aware width, fixed height, safe bounds and crisp English fullscreen verified.')
