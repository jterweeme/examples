/*
PL_MPEG Example - Video player using SDL2/OpenGL for rendering

Dominic Szablewski - https://phoboslab.org


-- LICENSE: The MIT License(MIT)

Copyright(c) 2019 Dominic Szablewski

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


-- Usage

plmpeg-player <video-file.mpg>

Use the arrow keys to seek forward/backward by 3 seconds. Click anywhere on the
window to seek to seek through the whole file.


-- About

This program demonstrates a simple video/audio player using plmpeg for decoding
and SDL2 with OpenGL for rendering and sound output. It was tested on Windows
using Microsoft Visual Studio 2015 and on macOS using XCode 10.2

This program can be configured to either convert the raw YCrCb data to RGB on
the GPU (default), or to do it on CPU. Just pass APP_TEXTURE_MODE_RGB to
app_create() to switch to do the conversion on the CPU.

YCrCb->RGB conversion on the CPU is a very costly operation and should be
avoided if possible. It easily takes as much time as all other mpeg1 decoding
steps combined.

*/

#include <stdio.h>

#define TRUE 1
#define FALSE 0

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <iostream>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"


static unsigned int g_nr = 1;
static unsigned int g_audio = 1;

class CApp
{
private:
    static void app_on_video(plm_frame_t *frame, void *);
    static void app_on_audio(plm_samples_t *samples, void *);
    GLuint app_create_texture(GLuint index, const char *name);
    GLuint app_compile_shader(GLenum type, const char *source);
    static void app_update_texture(GLuint unit, GLuint texture, plm_plane_t *plane);
    static CApp *_inst;
    uint8_t *_rgb_data;
    SDL_Window *_window;
    SDL_AudioDeviceID audio_device;
    SDL_GLContext gl;
    GLuint shader_program;
    double _last_time = 0;
    PLM _plm;
public:
    int wants_to_quit = 0;
    void app_create(const char *filename);
    void app_destroy();
    void app_update();
};

CApp *CApp::_inst = nullptr;

void CApp::app_destroy()
{
    _plm.plm_destroy();
}

void CApp::app_update()
{
    double seek_to = -1;
    double current_time = (double)SDL_GetTicks() / 1000.0;
    double elapsed_time = current_time - _last_time;

    if (elapsed_time > 1.0 / 30.0)
        elapsed_time = 1.0 / 30.0;

    _last_time = current_time;

    _plm.plm_decode(elapsed_time);

    if (_plm.plm_has_ended())
        wants_to_quit = TRUE;
}

void CApp::app_on_video(plm_frame_t *frame, void *)
{
    std::cout << "FRAME\n";
    std::cout.write((const char *)(frame->y.data), frame->y.width * frame->y.height);
    std::cout.write((const char *)(frame->cr.data), frame->cr.width * frame->cr.height);
    std::cout.write((const char *)(frame->cb.data), frame->cb.width * frame->cb.height);
    ++g_nr;
}

void CApp::app_create(const char *filename)
{
    _inst = this;
    _plm.plm_create_with_filename(filename);
    int samplerate = _plm.plm_get_samplerate();
    std::cout << "YUV4MPEG2 ";
    std::cout << "W320 ";
    std::cout << "H240\n";
    _plm.plm_set_video_decode_callback(app_on_video, nullptr);
    _plm.plm_set_loop(FALSE);
}

int main(int argc, char **argv)
{
    CApp app;
    app.app_create(argv[1]);

    while (!app.wants_to_quit)
        app.app_update();
	
    app.app_destroy();
    return EXIT_SUCCESS;
}


