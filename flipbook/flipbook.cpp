#include <stdio.h>

#define TRUE 1
#define FALSE 0

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <iostream>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define APP_SHADER_SOURCE(...) #__VA_ARGS__

static unsigned int g_nr = 1;
static unsigned int g_audio = 1;

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

void CApp::app_on_audio(plm_samples_t *samples, void *)
{
	int size = sizeof(float) * samples->count * 2;
	SDL_QueueAudio(_inst->audio_device, samples->interleaved, size);
    ++g_audio;
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
	plm_frame_t *frame = _plm.plm_decode_video();
    app_on_video(frame, nullptr);
    //glClear(GL_COLOR_BUFFER_BIT);
    glRectf(0.0, 0.0, 1.0, 1.0);
    SDL_GL_SwapWindow(_window);
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
    ++g_nr;
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
	
    _plm.plm_set_loop(FALSE);
    _plm.plm_set_audio_enabled(FALSE);

    _window = SDL_CreateWindow(
        "pl_mpeg",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        _plm.plm_get_width(), _plm.plm_get_height(),
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    gl = SDL_GL_CreateContext(_window);
	
	SDL_GL_SetSwapInterval(0);

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

