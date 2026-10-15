#!/usr/bin/env python3
"""Exercise actual decoding, validation and shutdown with SDL's dummy display."""
import os
from pathlib import Path
import signal
import shutil
import shlex
import subprocess
import sys
import tempfile
import time
import unittest

BINARY = Path(sys.argv.pop(1)).resolve()
ROOT = Path(__file__).resolve().parents[1]
ENV = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', LANG='C.UTF-8')


class Integration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.clip = Path(cls.temp.name) / 'clip with spaces.mp4'
        subprocess.run(['ffmpeg', '-v', 'error', '-f', 'lavfi', '-i',
                        'testsrc2=size=320x240:rate=24', '-t', '1', '-c:v', 'mpeg4',
                        str(cls.clip)], check=True)
        cls.bad = Path(cls.temp.name) / 'bad.mp4'
        cls.bad.write_text('invalid video')

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_app(self, *args, expected=0, env=None):
        result = subprocess.run([str(BINARY), *map(str, args)], env=env or ENV,
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, expected, result.stderr)
        return result.stdout + result.stderr

    def test_help_and_languages(self):
        self.assertIn('Usage:', self.run_app('--lang', 'en', '--help'))
        self.assertIn('Использование:', self.run_app('--lang', 'ru', '--help'))
        self.assertIn('1.0.0', self.run_app('--version'))

    def test_argument_validation(self):
        for args in [('--fps', '0'), ('--fps', '121'), ('--fps', '24x'),
                     ('--background', 'zzzzzz'), ('--threshold', '256'),
                     ('--size', '1x1'), ('--reason',), ('--lang', 'de'),
                     ('--duration', '-2'), ('--unknown',)]:
            with self.subTest(args=args):
                self.run_app(self.clip, *args, expected=1)
        self.run_app('/missing/video.mp4', expected=1)

    def test_decode_all_frames(self):
        result = self.run_app(self.clip, '--mute', '--windowed', '--size', '640x480', '--stats')
        self.assertIn('frames=24', result)

    def test_customization(self):
        self.run_app('--video', self.clip, '--mute', '--lang', 'ru', '--reason',
                     'Проверка', '--background', '#111827', '--foreground', 'A7F3D0',
                     '--fps', '12', '--threshold', '100')

    def test_decode_failure_is_reported(self):
        self.assertIn('failed', self.run_app('--lang', 'en', self.bad, '--mute', expected=1))

    def test_signal_cleans_up_stalled_decoder(self):
        fifo = Path(self.temp.name) / 'stream.nut'
        os.mkfifo(fifo)
        child_env = dict(ENV, FAKEPANIC_STREAM=str(fifo))
        proc = subprocess.Popen([str(BINARY), '--game-stream'], env=child_env,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            time.sleep(0.4)
            descendants = Path(f'/proc/{proc.pid}/task/{proc.pid}/children')
            pids = descendants.read_text().split() if descendants.exists() else []
            proc.send_signal(signal.SIGTERM)
            proc.communicate(timeout=3)
            self.assertEqual(proc.returncode, 143)
            for pid in pids:
                self.assertFalse(Path(f'/proc/{pid}').exists(), f'Orphan child {pid}')
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.communicate()

    def test_installed_launcher(self):
        prefix = Path(self.temp.name) / 'prefix with spaces'
        (prefix / 'share/fakepanic').mkdir(parents=True)
        (prefix / 'bin').mkdir()
        (prefix / 'share/fakepanic/fakepanic-renderer').symlink_to(BINARY)
        launcher = prefix / 'bin/fakepanic'
        launcher.write_bytes((ROOT / 'scripts/fakepanic').read_bytes())
        launcher.chmod(0o755)
        result = subprocess.run([str(launcher), '--video', str(self.clip), '--mute'],
                                env=ENV, capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(launcher), '--lutris', 'abc'], env=ENV,
                                capture_output=True, timeout=3)
        self.assertEqual(result.returncode, 2)
        for shell in ('bash', 'zsh', 'fish'):
            if not shutil.which(shell):
                continue
            with self.subTest(shell=shell):
                command = 'exec ' + ' '.join(map(shlex.quote, [str(launcher), '--video', str(self.clip), '--mute']))
                result = subprocess.run([shell, '-c', command], env=ENV, capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main(verbosity=2)
