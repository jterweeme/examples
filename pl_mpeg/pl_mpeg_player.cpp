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

const char * const APP_FRAGMENT_SHADER_RGB = APP_SHADER_SOURCE(
	uniform sampler2D texture_rgb;
	varying vec2 tex_coord;

	void main() {
		gl_FragColor = vec4(texture2D(texture_rgb, tex_coord).rgb, 1.0);
	}
);

#undef APP_SHADER_SOURCE

#define APP_TEXTURE_MODE_YCRCB 1
#define APP_TEXTURE_MODE_RGB 2

struct app_t {
	plm_t *plm;
	double last_time;
	int wants_to_quit;
	SDL_Window *window;
	SDL_AudioDeviceID audio_device;
    SDL_GLContext gl;
    GLuint shader_program;
    GLuint vertex_shader;
    GLuint fragment_shader;

};

class CApp
{
private:
    static void app_on_video(plm_t *mpeg, plm_frame_t *frame, void *);
    static void app_on_audio(plm_t *mpeg, plm_samples_t *samples, void *);
    GLuint app_create_texture(GLuint index, const char *name);
    GLuint app_compile_shader(GLenum type, const char *source);
    static void app_update_texture(GLuint unit, GLuint texture, plm_plane_t *plane);
    static CApp *_inst;
    int _texture_mode;
    GLuint _texture_y;
    GLuint _texture_cb;
    GLuint _texture_cr;
    GLuint _texture_rgb;
    uint8_t *_rgb_data;
public:
    app_t *g_instance = nullptr;
    void app_create(const char *filename, int texture_mode);
    void app_destroy();
    void app_update();
};

CApp *CApp::_inst = nullptr;

void CApp::app_on_audio(plm_t *mpeg, plm_samples_t *samples, void *)
{
    app_t *inst = _inst->g_instance;
	// Hand the decoded samples over to SDL
	int size = sizeof(float) * samples->count * 2;
	SDL_QueueAudio(inst->audio_device, samples->interleaved, size);
}

void CApp::app_destroy()
{
    plm_destroy(g_instance->plm);
	
    if (_texture_mode == APP_TEXTURE_MODE_RGB)
        free(_rgb_data);

    if (g_instance->audio_device)
        SDL_CloseAudioDevice(g_instance->audio_device);
	
    SDL_GL_DeleteContext(g_instance->gl);
    SDL_Quit();
    delete g_instance;
}

void CApp::app_update()
{
    double seek_to = -1;
    SDL_Event ev;

    while (SDL_PollEvent(&ev))
    {
        if (ev.type == SDL_QUIT || (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE))
            g_instance->wants_to_quit = TRUE;
		
		if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            glViewport(0, 0, ev.window.data1, ev.window.data2);

		// Seek 3sec forward/backward using arrow keys
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_RIGHT)
			seek_to = plm_get_time(g_instance->plm) + 3;
		else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_LEFT)
			seek_to = plm_get_time(g_instance->plm) - 3;
    }

	// Compute the delta time since the last app_update(), limit max step to 
	// 1/30th of a second
	double current_time = (double)SDL_GetTicks() / 1000.0;
	double elapsed_time = current_time - g_instance->last_time;

    if (elapsed_time > 1.0 / 30.0)
        elapsed_time = 1.0 / 30.0;

    g_instance->last_time = current_time;

	// Seek using mouse position
    int mouse_x, mouse_y;

    if (SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
        int sx, sy;
        SDL_GetWindowSize(g_instance->window, &sx, &sy);
        seek_to = plm_get_duration(g_instance->plm) * ((float)mouse_x / (float)sx);
    }
	
    // Seek or advance decode
    if (seek_to != -1)
    {
        SDL_ClearQueuedAudio(g_instance->audio_device);
        plm_seek(g_instance->plm, seek_to, FALSE);
	}
	else {
		plm_decode(g_instance->plm, elapsed_time);
	}

    if (plm_has_ended(g_instance->plm))
        g_instance->wants_to_quit = TRUE;
	
    glClear(GL_COLOR_BUFFER_BIT);
    glRectf(0.0, 0.0, 1.0, 1.0);
    SDL_GL_SwapWindow(g_instance->window);
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
    glUniform1i(glGetUniformLocation(g_instance->shader_program, name), index);
    return texture;
}

void CApp::app_update_texture(GLuint unit, GLuint texture, plm_plane_t *plane)
{
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, plane->width, plane->height, 0,
        GL_LUMINANCE, GL_UNSIGNED_BYTE, plane->data);
}

void CApp::app_on_video(plm_t *mpeg, plm_frame_t *frame, void *)
{
    app_t *inst = _inst->g_instance;

    // Hand the decoded data over to OpenGL. For the RGB texture mode, the
    // YCrCb->RGB conversion is done on the CPU.
    if (_inst->_texture_mode == APP_TEXTURE_MODE_YCRCB)
    {
        app_update_texture(GL_TEXTURE0, _inst->_texture_y, &frame->y);
        app_update_texture(GL_TEXTURE1, _inst->_texture_cb, &frame->cb);
        app_update_texture(GL_TEXTURE2, _inst->_texture_cr, &frame->cr);
    }
    else
    {
        plm_frame_to_rgb(frame, _inst->_rgb_data, frame->width * 3);
        glBindTexture(GL_TEXTURE_2D, _inst->_texture_rgb);

        glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, _inst->_rgb_data);
	}
}

void CApp::app_create(const char *filename, int texture_mode)
{
    _inst = this;
    app_t *self = new app_t;
    g_instance = self;
    _texture_mode = texture_mode;
	
	// Initialize plmpeg, load the video file, install decode callbacks
	g_instance->plm = plm_create_with_filename(filename);

	if (!g_instance->plm)
    {
		SDL_Log("Couldn't open %s", filename);
		exit(1);
	}

	int samplerate = plm_get_samplerate(g_instance->plm);

	SDL_Log(
		"Opened %s - framerate: %f, samplerate: %d, duration: %f",
		filename, 
		plm_get_framerate(g_instance->plm),
		plm_get_samplerate(g_instance->plm),
		plm_get_duration(g_instance->plm)
	);
	
    plm_set_video_decode_callback(g_instance->plm, app_on_video, nullptr);
    plm_set_audio_decode_callback(g_instance->plm, app_on_audio, nullptr);
	
	plm_set_loop(g_instance->plm, TRUE);
	plm_set_audio_enabled(g_instance->plm, TRUE);
	plm_set_audio_stream(g_instance->plm, 0);

    if (plm_get_num_audio_streams(g_instance->plm) > 0)
    {
        // Initialize SDL Audio
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        SDL_AudioSpec audio_spec;
        SDL_memset(&audio_spec, 0, sizeof(audio_spec));
        audio_spec.freq = samplerate;
        audio_spec.format = AUDIO_F32;
        audio_spec.channels = 2;
        audio_spec.samples = 4096;
        g_instance->audio_device = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);

        if (g_instance->audio_device == 0)
            SDL_Log("Failed to open audio device: %s", SDL_GetError());

		SDL_PauseAudioDevice(g_instance->audio_device, 0);

		// Adjust the audio lead time according to the audio_spec buffer size
		plm_set_audio_lead_time(g_instance->plm, (double)audio_spec.samples / (double)samplerate);
	}
	
	// Create SDL Window
    g_instance->window = SDL_CreateWindow(
		"pl_mpeg",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		plm_get_width(g_instance->plm), plm_get_height(g_instance->plm),
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
    g_instance->gl = SDL_GL_CreateContext(g_instance->window);
	
	SDL_GL_SetSwapInterval(1);

	#if defined(__APPLE__) && defined(__MACH__)
		// OSX
		// (nothing to do here)
	#else
		// Windows, Linux
		glewExperimental = GL_TRUE;
		glewInit();
	#endif
	
	
	// Setup OpenGL shaders and textures
    const char *fsh = _texture_mode == APP_TEXTURE_MODE_YCRCB
		? APP_FRAGMENT_SHADER_YCRCB
		: APP_FRAGMENT_SHADER_RGB;
	
    g_instance->fragment_shader = app_compile_shader(GL_FRAGMENT_SHADER, fsh);
    g_instance->vertex_shader = app_compile_shader(GL_VERTEX_SHADER, APP_VERTEX_SHADER);
    g_instance->shader_program = glCreateProgram();
    glAttachShader(g_instance->shader_program, g_instance->vertex_shader);
    glAttachShader(g_instance->shader_program, g_instance->fragment_shader);
    glLinkProgram(g_instance->shader_program);
    glUseProgram(g_instance->shader_program);
	
    // Create textures for YCrCb or RGB rendering
    if (_texture_mode == APP_TEXTURE_MODE_YCRCB)
    {
		_texture_y  = app_create_texture(0, "texture_y");
		_texture_cb = app_create_texture(1, "texture_cb");
		_texture_cr = app_create_texture(2, "texture_cr");
	}
	else
    {
        _texture_rgb = app_create_texture(0, "texture_rgb");
		int num_pixels = plm_get_width(g_instance->plm) * plm_get_height(g_instance->plm);
		_inst->_rgb_data = (uint8_t*)malloc(num_pixels * 3);
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		SDL_Log("Usage: pl_mpeg_player <file.mpg>");
		exit(1);
	}
	
    CApp app;
	app.app_create(argv[1], APP_TEXTURE_MODE_YCRCB);

	while (!app.g_instance->wants_to_quit)
        app.app_update();
	
	app.app_destroy();
	
	return EXIT_SUCCESS;
}


