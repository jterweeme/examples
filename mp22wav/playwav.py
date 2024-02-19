#!/usr/bin/python3

from collections import namedtuple
import warnings
import builtins
import struct
import pyaudio
import sys

class Error(Exception):
    pass

WAVE_FORMAT_PCM = 0x0001
WAVE_FORMAT_EXTENSIBLE = 0xFFFE
KSDATAFORMAT_SUBTYPE_PCM = b'\x01\x00\x00\x00\x00\x00\x10\x00\x80\x00\x00\xaa\x008\x9bq'

_array_fmts = None, 'b', 'h', None, 'i'

_wave_params = namedtuple('_wave_params',
    'nchannels sampwidth framerate nframes comptype compname')

def _byteswap(data, width):
    swapped_data = bytearray(len(data))
    for i in range(0, len(data), width):
        for j in range(width):
            swapped_data[i + width - 1 - j] = data[i + j]
    return bytes(swapped_data)

class _Chunk:
    def __init__(self, file, align=True, bigendian=True, inclheader=False):
        self.closed = False
        self.align = align      # whether to align to word (2-byte) boundaries
        if bigendian:
            strflag = '>'
        else:
            strflag = '<'
        self.file = file
        self.chunkname = file.read(4)
        if len(self.chunkname) < 4:
            raise EOFError
        try:
            self.chunksize = struct.unpack_from(strflag+'L', file.read(4))[0]
        except struct.error:
            raise EOFError from None
        if inclheader:
            self.chunksize = self.chunksize - 8 # subtract header
        self.size_read = 0
        try:
            self.offset = self.file.tell()
        except (AttributeError, OSError):
            self.seekable = False
        else:
            self.seekable = True

    def getname(self):
        return self.chunkname

    def close(self):
        if not self.closed:
            try:
                self.skip()
            finally:
                self.closed = True

    def seek(self, pos, whence=0):
        if self.closed:
            raise ValueError("I/O operation on closed file")
        if not self.seekable:
            raise OSError("cannot seek")
        if whence == 1:
            pos = pos + self.size_read
        elif whence == 2:
            pos = pos + self.chunksize
        if pos < 0 or pos > self.chunksize:
            raise RuntimeError
        self.file.seek(self.offset + pos, 0)
        self.size_read = pos

    def tell(self):
        if self.closed:
            raise ValueError("I/O operation on closed file")
        return self.size_read

    def read(self, size=-1):
        if self.closed:
            raise ValueError("I/O operation on closed file")
        if self.size_read >= self.chunksize:
            return b''
        if size < 0:
            size = self.chunksize - self.size_read
        if size > self.chunksize - self.size_read:
            size = self.chunksize - self.size_read
        data = self.file.read(size)
        self.size_read = self.size_read + len(data)
        if self.size_read == self.chunksize and \
           self.align and \
           (self.chunksize & 1):
            dummy = self.file.read(1)
            self.size_read = self.size_read + len(dummy)
        return data

    def skip(self):
        if self.closed:
            raise ValueError("I/O operation on closed file")
        if self.seekable:
            try:
                n = self.chunksize - self.size_read
                # maybe fix alignment
                if self.align and (self.chunksize & 1):
                    n = n + 1
                self.file.seek(n, 1)
                self.size_read = self.size_read + n
                return
            except OSError:
                pass
        while self.size_read < self.chunksize:
            n = min(8192, self.chunksize - self.size_read)
            dummy = self.read(n)
            if not dummy:
                raise EOFError

class Wave_read:
    def initfp(self, file):
        self._convert = None
        self._soundpos = 0
        self._file = _Chunk(file, bigendian = 0)
        if self._file.getname() != b'RIFF':
            raise Error('file does not start with RIFF id')
        if self._file.read(4) != b'WAVE':
            raise Error('not a WAVE file')
        self._fmt_chunk_read = 0
        self._data_chunk = None
        while 1:
            self._data_seek_needed = 1
            try:
                chunk = _Chunk(self._file, bigendian = 0)
            except EOFError:
                break
            chunkname = chunk.getname()
            if chunkname == b'fmt ':
                self._read_fmt_chunk(chunk)
                self._fmt_chunk_read = 1
            elif chunkname == b'data':
                if not self._fmt_chunk_read:
                    raise Error('data chunk before fmt chunk')
                self._data_chunk = chunk
                self._nframes = chunk.chunksize // self._framesize
                self._data_seek_needed = 0
                break
            chunk.skip()
        if not self._fmt_chunk_read or not self._data_chunk:
            raise Error('fmt chunk and/or data chunk missing')

    def __init__(self):
        self._i_opened_the_file = None
        f = sys.stdin.buffer
        self._i_opened_the_file = f
        self.initfp(f)

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def getnchannels(self): return self._nchannels
    def getnframes(self): return self._nframes
    def getsampwidth(self): return self._sampwidth
    def getframerate(self): return self._framerate
    def getcomptype(self): return self._comptype
    def getcompname(self): return self._compname

    def getparams(self):
        return _wave_params(self.getnchannels(), self.getsampwidth(),
                       self.getframerate(), self.getnframes(),
                       self.getcomptype(), self.getcompname())

    def getmarkers(self):
        warnings._deprecated("Wave_read.getmarkers", remove=(3, 15))
        return None

    def getmark(self, id):
        warnings._deprecated("Wave_read.getmark", remove=(3, 15))
        raise Error('no marks')

    def setpos(self, pos):
        if pos < 0 or pos > self._nframes:
            raise Error('position not in range')
        self._soundpos = pos
        self._data_seek_needed = 1

    def readframes(self, nframes):
        if self._data_seek_needed:
            self._data_chunk.seek(0, 0)
            pos = self._soundpos * self._framesize
            if pos:
                self._data_chunk.seek(pos, 0)
            self._data_seek_needed = 0
        if nframes == 0:
            return b''
        data = self._data_chunk.read(nframes * self._framesize)
        if self._sampwidth != 1 and sys.byteorder == 'big':
            data = _byteswap(data, self._sampwidth)
        if self._convert and data:
            data = self._convert(data)
        self._soundpos = self._soundpos + len(data) // (self._nchannels * self._sampwidth)
        return data

    def _read_fmt_chunk(self, chunk):
        try:
            wFormatTag, self._nchannels, self._framerate, dwAvgBytesPerSec, wBlockAlign = struct.unpack_from('<HHLLH', chunk.read(14))
        except struct.error:
            raise EOFError from None
        if wFormatTag != WAVE_FORMAT_PCM and wFormatTag != WAVE_FORMAT_EXTENSIBLE:
            raise Error('unknown format: %r' % (wFormatTag,))
        try:
            sampwidth = struct.unpack_from('<H', chunk.read(2))[0]
        except struct.error:
            raise EOFError from None
        if wFormatTag == WAVE_FORMAT_EXTENSIBLE:
            try:
                cbSize, wValidBitsPerSample, dwChannelMask = struct.unpack_from('<HHL', chunk.read(8))
                # Read the entire UUID from the chunk
                SubFormat = chunk.read(16)
                if len(SubFormat) < 16:
                    raise EOFError
            except struct.error:
                raise EOFError from None
            if SubFormat != KSDATAFORMAT_SUBTYPE_PCM:
                try:
                    import uuid
                    subformat_msg = f'unknown extended format: {uuid.UUID(bytes_le=SubFormat)}'
                except Exception:
                    subformat_msg = 'unknown extended format'
                raise Error(subformat_msg)
        self._sampwidth = (sampwidth + 7) // 8
        if not self._sampwidth:
            raise Error('bad sample width')
        if not self._nchannels:
            raise Error('bad # of channels')
        self._framesize = self._nchannels * self._sampwidth
        self._comptype = 'NONE'
        self._compname = 'not compressed'

f = Wave_read()
p = pyaudio.PyAudio()

stream = p.open(format = p.get_format_from_width(f.getsampwidth()),
                channels = f.getnchannels(),
                rate = f.getframerate(),
                output = True)

while (data := f.readframes(1024)):
    stream.write(data)

stream.stop_stream()
p.terminate()


