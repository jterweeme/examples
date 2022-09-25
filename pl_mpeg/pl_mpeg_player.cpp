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

#include <stdlib.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

#include <SDL2/SDL.h>
#include <GL/glew.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define APP_SHADER_SOURCE(...) #__VA_ARGS__

const char * const APP_VERTEX_SHADER = APP_SHADER_SOURCE(
	attribute vec2 vertex;
	varying vec2 tex_coord;
	
	void main() {
		tex_coord = vertex;
		gl_Position = vec4((vertex * 2.0 - 1.0) * vec2(1, -1), 0.0, 1.0);
	}
);

const char * const APP_FRAGMENT_SHADER_YCRCB = APP_SHADER_SOURCE(
	uniform sampler2D texture_y;
	uniform sampler2D texture_cb;
	uniform sampler2D texture_cr;
	varying vec2 tex_coord;

	mat4 rec601 = mat4(
		1.16438,  0.00000,  1.59603, -0.87079,
		1.16438, -0.39176, -0.81297,  0.52959,
		1.16438,  2.01723,  0.00000, -1.08139,
		0, 0, 0, 1
	);
	  
	void main() {
		float y = texture2D(texture_y, tex_coord).r;
		float cb = texture2D(texture_cb, tex_coord).r;
		float cr = texture2D(texture_cr, tex_coord).r;

		gl_FragColor = vec4(y, cb, cr, 1.0) * rec601;
	}
);
#undef APP_SHADER_SOURCE

class CApp
{
private:
    static void app_on_video(plm_frame_t *frame, void *);
    static void app_on_audio(plm_samples_t *samples, void *);
    GLuint app_create_texture(GLuint index, const char *name);
    GLuint app_compile_shader(GLenum type, const char *source);
    static void app_update_texture(GLuint unit, GLuint texture, plm_plane_t *plane);
    static CApp *_inst;
    GLuint _texture_y;
    GLuint _texture_cb;
    GLuint _texture_cr;
    GLuint _texture_rgb;
    uint8_t *_rgb_data;
    SDL_Window *window;
    SDL_AudioDeviceID audio_device;
    SDL_GLContext gl;
    GLuint shader_program;
    double last_time = 0;
    PLM _plm;
public:
    int wants_to_quit = 0;
    void app_create(const char *filename);
    void app_destroy();
    void app_update();
};

CApp *CApp::_inst = nullptr;

void CApp::app_on_audio(plm_samples_t *samples, void *)
{
	int size = sizeof(float) * samples->count * 2;
	SDL_QueueAudio(_inst->audio_device, samples->interleaved, size);
}

void CApp::app_destroy()
{
    _plm.plm_destroy();
	
    if (audio_device)
        SDL_CloseAudioDevice(audio_device);
	
    SDL_GL_DeleteContext(gl);
    SDL_Quit();
}

void CApp::app_update()
{
    double seek_to = -1;
    SDL_Event ev;

    while (SDL_PollEvent(&ev))
    {
        if (ev.type == SDL_QUIT || (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE))
            wants_to_quit = TRUE;
		
		if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            glViewport(0, 0, ev.window.data1, ev.window.data2);

		// Seek 3sec forward/backward using arrow keys
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_RIGHT)
			seek_to = _plm.plm_get_time() + 3;
		else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_LEFT)
			seek_to = _plm.plm_get_time() - 3;
    }

    // Compute the delta time since the last app_update(), limit max step to 
    // 1/30th of a second
    double current_time = (double)SDL_GetTicks() / 1000.0;
    double elapsed_time = current_time - last_time;

    if (elapsed_time > 1.0 / 30.0)
        elapsed_time = 1.0 / 30.0;

    last_time = current_time;

	// Seek using mouse position
    int mouse_x, mouse_y;

    if (SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
        int sx, sy;
        SDL_GetWindowSize(window, &sx, &sy);
        seek_to = _plm.plm_get_duration() * ((float)mouse_x / (float)sx);
    }
	
    // Seek or advance decode
    if (seek_to != -1)
    {
        SDL_ClearQueuedAudio(audio_device);
        _plm.plm_seek(seek_to, FALSE);
	}
	else {
		_plm.plm_decode(elapsed_time);
	}

    if (_plm.plm_has_ended())
        wants_to_quit = TRUE;
	
    glClear(GL_COLOR_BUFFER_BIT);
    glRectf(0.0, 0.0, 1.0, 1.0);
    SDL_GL_SwapWindow(window);
}

GLuint CApp::app_compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        int log_written;
        char log[256];
        glGetShaderInfoLog(shader, 256, &log_written, log);
        SDL_Log("Error compiling shader: %s.\n", log);
    }
    return shader;
}

GLuint CApp::app_create_texture(GLuint index, const char *name)
{
    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glUniform1i(glGetUniformLocation(shader_program, name), index);
    return texture;
}

void CApp::app_update_texture(GLuint unit, GLuint texture, plm_plane_t *plane)
{
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, plane->width, plane->height, 0,
        GL_LUMINANCE, GL_UNSIGNED_BYTE, plane->data);
}

void CApp::app_on_video(plm_frame_t *frame, void *)
{
    app_update_texture(GL_TEXTURE0, _inst->_texture_y, &frame->y);
    app_update_texture(GL_TEXTURE1, _inst->_texture_cb, &frame->cb);
    app_update_texture(GL_TEXTURE2, _inst->_texture_cr, &frame->cr);
}

void CApp::app_create(const char *filename)
{
    _inst = this;
    
    // Initialize plmpeg, load the video file, install decode callbacks
    _plm.plm_create_with_filename(filename);
    int samplerate = _plm.plm_get_samplerate();

    SDL_Log(
        "Opened %s - framerate: %f, samplerate: %d, duration: %f",
        filename, 
        _plm.plm_get_framerate(),
        _plm.plm_get_samplerate(),
        _plm.plm_get_duration());
	
    _plm.plm_set_video_decode_callback(app_on_video, nullptr);
    _plm.plm_set_audio_decode_callback(app_on_audio, nullptr);
	
    _plm.plm_set_loop(TRUE);
    _plm.plm_set_audio_enabled(TRUE);
    _plm.plm_set_audio_stream(0);

    if (_plm.plm_get_num_audio_streams() > 0)
    {
        // Initialize SDL Audio
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        SDL_AudioSpec audio_spec;
        SDL_memset(&audio_spec, 0, sizeof(audio_spec));
        audio_spec.freq = samplerate;
        audio_spec.format = AUDIO_F32;
        audio_spec.channels = 2;
        audio_spec.samples = 4096;
        audio_device = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);

        if (audio_device == 0)
            SDL_Log("Failed to open audio device: %s", SDL_GetError());

        SDL_PauseAudioDevice(audio_device, 0);

        // Adjust the audio lead time according to the audio_spec buffer size
        _plm.plm_set_audio_lead_time((double)audio_spec.samples / (double)samplerate);
    }
	
    // Create SDL Window
    window = SDL_CreateWindow(
        "pl_mpeg",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        _plm.plm_get_width(), _plm.plm_get_height(),
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    gl = SDL_GL_CreateContext(window);
	
	SDL_GL_SetSwapInterval(1);

#if defined(__APPLE__) && defined(__MACH__)
    // OSX
    // (nothing to do here)
#else
    // Windows, Linux
    glewExperimental = GL_TRUE;
    glewInit();
#endif
    const char *fsh = APP_FRAGMENT_SHADER_YCRCB;
    GLuint fragment_shader = app_compile_shader(GL_FRAGMENT_SHADER, fsh);
    GLuint vertex_shader = app_compile_shader(GL_VERTEX_SHADER, APP_VERTEX_SHADER);
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glUseProgram(shader_program);
    _texture_y  = app_create_texture(0, "texture_y");
    _texture_cb = app_create_texture(1, "texture_cb");
    _texture_cr = app_create_texture(2, "texture_cr");
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		SDL_Log("Usage: pl_mpeg_player <file.mpg>");
		exit(1);
	}
	
    CApp app;
    app.app_create(argv[1]);

    while (!app.wants_to_quit)
        app.app_update();
	
    app.app_destroy();
    return EXIT_SUCCESS;
}


