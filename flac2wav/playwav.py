#!/usr/bin/python3

import struct, pyaudio, sys

f = sys.stdin.buffer
riff = struct.unpack("<IIIIIHHIIHHII", f.read(44))
assert riff[0] == 0x46464952
assert riff[2] == 0x45564157
assert riff[3] == 0x20746d66
p = pyaudio.PyAudio()
fmt = p.get_format_from_width(riff[10] / 8)
stream = p.open(format = fmt, channels = riff[6], rate = riff[7], output = True)
while (data := f.read(riff[9] * 100)): stream.write(data)
stream.stop_stream()
p.terminate()


