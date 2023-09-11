#include "render_pipeline.h"
#include "constants/general.h"
#include "constants/screen.h"
#include "gamestate.h"
#include "threadutils.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

// useful for screen scaling
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
	unsigned int id;
	int width;
	int height;
	// additional textures used for deferred rendering
	Texture position;
	Texture normals;
	// rgb = albedo, a = specular
	Texture albedo_specular;
	// handle for depth
	unsigned int depth;
} DeferredFBO;

// window scaling and splitscreen
static RenderTexture2D main_target;
static DeferredFBO fbos[NUM_PLANES];
static Shader gather_shader;
static Shader screen_shader;

// make sure to take absolute values when using height...
static const Rectangle splitScreenRect = {
	.x = 0,
	.y = 0,
	.width = GAME_WIDTH,
	.height = -(GAME_HEIGHT / 2.0f),
};

static void init_rendertextures();
static void window_draw(float screen_scale);
/// Load a texture with parameters given by the params texture.
static void init_texture(Texture* tex, const Texture* params);
static void BeginTextureModeDeferred(const DeferredFBO* fbo);
static void BeginMode3DCustom(const Camera* camera, const DeferredFBO* target);

void render(void (*game_draw)())
{
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		// enable the framebuffer
		BeginTextureModeDeferred(&fbos[i]);
		{
			ClearBackground(RAYWHITE);
			const FullCamera* player = &gamestate_get_cameras()[i];
			// push matrices to rlgl state
			BeginMode3DCustom(&player->camera, &fbos[i]);
			{
				// use the deferred rendering shader to draw position, normals,
				// albedo, etc to the textures
				BeginShaderMode(gather_shader);
				{
                    // TODO: fix position buffer not being drawn to
					// draw the actual in game objects
					game_draw();
				}
				EndShaderMode();
			}
			EndMode3D();
		}
		EndTextureMode();
	}

	// set draw target to the rendertexture, dont actually draw to window
	BeginTextureMode(main_target);
	{
		BeginShaderMode(screen_shader);
		// assign the screen shader to sample from the correct textures
		rlEnableShader(screen_shader.id);
		rlSetUniformSampler(GetShaderLocation(screen_shader, "gPosition"),
							fbos[0].position.id);
		rlSetUniformSampler(GetShaderLocation(screen_shader, "gNormal"),
							fbos[0].normals.id);
		rlSetUniformSampler(GetShaderLocation(screen_shader, "gAlbedoSpec"),
							fbos[0].albedo_specular.id);
		{
			ClearBackground(BLACK);
			DrawTextureRec(fbos[0].position, splitScreenRect, (Vector2){0, 0},
						   WHITE);
		}
		EndShaderMode();

		BeginShaderMode(screen_shader);
		// assign the screen shader to sample from the correct textures
		rlEnableShader(screen_shader.id);
		rlSetUniformSampler(GetShaderLocation(screen_shader, "gPosition"),
							fbos[1].position.id);
		rlSetUniformSampler(GetShaderLocation(screen_shader, "gNormal"),
							fbos[1].normals.id);
		rlSetUniformSampler(GetShaderLocation(screen_shader, "gAlbedoSpec"),
							fbos[1].albedo_specular.id);
		{
			DrawTextureRec(fbos[1].position, splitScreenRect,
						   (Vector2){
							   0,
							   fabsf(splitScreenRect.height),
						   },
						   WHITE);
		}
		EndShaderMode();
	}
	EndTextureMode();

	// draw the game to the window at the correct size
	BeginDrawing();
	{
		window_draw(gamestate_get_screen_scale());
	}
	EndDrawing();
}

void render_pipeline_init()
{
	init_rendertextures();
	gather_shader = LoadShader("assets/postprocessing/g_buffer.vert",
							   "assets/postprocessing/g_buffer.frag");
	screen_shader = LoadShader("assets/postprocessing/lighting_deferred.vert",
							   "assets/postprocessing/lighting_deferred.frag");
	gather_shader.locs[SHADER_LOC_MATRIX_VIEW] =
		GetShaderLocation(gather_shader, "view");
	gather_shader.locs[SHADER_LOC_MATRIX_MODEL] =
		GetShaderLocation(gather_shader, "model");
	gather_shader.locs[SHADER_LOC_MATRIX_PROJECTION] =
		GetShaderLocation(gather_shader, "proj");
}

void render_pipeline_cleanup()
{
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		UnloadTexture(fbos[i].position);
		UnloadTexture(fbos[i].normals);
		UnloadTexture(fbos[i].albedo_specular);
	}
	UnloadRenderTexture(main_target);
	UnloadShader(gather_shader);
	UnloadShader(screen_shader);
}

/// Populate gamestate with information about the screen.
void render_pipeline_gather_screen_info()
{
	// fraction of window resize that will occur this frame, basically the
	// difference between current width/height ratio to target width/height
	gamestate_set_screen_scale(MIN((float)GetScreenWidth() / GAME_WIDTH,
								   (float)GetScreenHeight() / GAME_HEIGHT));
}

/// Initialize the main rendertexture to which the actual game elements are
/// drawn. Determines the universal texture filter for scaling.
static void init_rendertextures()
{
	// variable width screen
	main_target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
	int h = abs((int)splitScreenRect.height);
	int w = abs((int)splitScreenRect.width);
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		fbos[i].id = rlLoadFramebuffer(w, h);

		// TODO: should we fail here or just return and warn
		if (fbos[i].id == 0) {
			TraceLog(LOG_FATAL, "Unable to initialize framebuffer object");
			threadutils_exit(EXIT_FAILURE);
		}

		// describe how we want all the textures to be initialized
		Texture params = {
			.height = h,
			.width = w,
			.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
			.mipmaps = 1,
		};

		init_texture(&fbos[i].position, &params);
		init_texture(&fbos[i].normals, &params);
		init_texture(&fbos[i].albedo_specular, &params);
		fbos[i].depth = rlLoadTextureDepth(w, h, true);

		rlFramebufferAttach(fbos[i].id, fbos[i].position.id,
							RL_ATTACHMENT_COLOR_CHANNEL0,
							RL_ATTACHMENT_TEXTURE2D, 0);
		rlFramebufferAttach(fbos[i].id, fbos[i].normals.id,
							RL_ATTACHMENT_COLOR_CHANNEL1,
							RL_ATTACHMENT_TEXTURE2D, 0);
		rlFramebufferAttach(fbos[i].id, fbos[i].albedo_specular.id,
							RL_ATTACHMENT_COLOR_CHANNEL2,
							RL_ATTACHMENT_TEXTURE2D, 0);
		// attach depth also
		rlFramebufferAttach(fbos[i].id, fbos[i].depth, RL_ATTACHMENT_DEPTH,
							RL_ATTACHMENT_RENDERBUFFER, 0);

		if (!rlFramebufferComplete(fbos[i].id)) {
			TraceLog(LOG_FATAL, "unable to complete framebuffer object");
			threadutils_exit(EXIT_FAILURE);
		}
	}

	// set all to pixelated looking point filter
	for (uint8_t i = 0; i < NUM_PLANES; ++i) {
		if (rlFramebufferComplete(fbos[i].id)) {
			TraceLog(LOG_INFO, "Framebuffer %d loaded successfully.", i);
		}

		SetTextureFilter(fbos[i].position, TEXTURE_FILTER_POINT);
		SetTextureFilter(fbos[i].albedo_specular, TEXTURE_FILTER_POINT);
		SetTextureFilter(fbos[i].normals, TEXTURE_FILTER_POINT);
	}
	SetTextureFilter(main_target.texture, TEXTURE_FILTER_POINT);
}

static void init_texture(Texture* tex, const Texture* params)
{
	*tex = (Texture){
		.mipmaps = params->mipmaps,
		.format = params->format,
		.width = params->width,
		.height = params->height,
	};
	tex->id = rlLoadTexture(NULL, params->width, params->height, params->format,
							params->mipmaps);
}

/// Resize the game's main render texture and draw it to the window.
static void window_draw(float screen_scale)
{
	// color of the bars around the rendertexture
	ClearBackground(BLACK);
	// draw the render texture scaled
	DrawTexturePro(
		main_target.texture,
		(Rectangle){
			0.0f,
			0.0f,
			(float)main_target.texture.width,
			(float)-main_target.texture.height,
		},
		(Rectangle){
			((float)GetScreenWidth() - ((float)GAME_WIDTH * screen_scale)) *
				0.5f,
			((float)GetScreenHeight() - ((float)GAME_HEIGHT * screen_scale)) *
				0.5f,
			(float)GAME_WIDTH * screen_scale,
			(float)GAME_HEIGHT * screen_scale,
		},
		(Vector2){0, 0}, 0.0f, WHITE);
}

static void BeginTextureModeDeferred(const DeferredFBO* fbo)
{
	rlDrawRenderBatchActive(); // Update and draw internal render batch

	rlEnableFramebuffer(fbo->id); // Enable render target

	// Set viewport and RLGL internal framebuffer size
	rlViewport(0, 0, fbo->width, fbo->height);
	rlSetFramebufferWidth(fbo->width);
	rlSetFramebufferHeight(fbo->height);

	rlMatrixMode(RL_PROJECTION); // Switch to projection matrix
	rlLoadIdentity();			 // Reset current matrix (projection)

	// Set orthographic projection to current framebuffer size
	// NOTE: Configured top-left corner as (0, 0)
	rlOrtho(0, fbo->width, fbo->height, 0, 0.0f, 1.0f);

	rlMatrixMode(RL_MODELVIEW); // Switch back to modelview matrix
	rlLoadIdentity();			// Reset current matrix (modelview)
}

static void BeginMode3DCustom(const Camera* camera, const DeferredFBO* target)
{
	rlDrawRenderBatchActive(); // Update and draw internal render batch

	rlMatrixMode(RL_PROJECTION); // Switch to projection matrix
	rlPushMatrix(); // Save previous matrix, which contains the settings for the
					// 2d ortho projection
	rlLoadIdentity(); // Reset current matrix (projection)

	float aspect = (float)target->width / (float)target->height;

	// NOTE: zNear and zFar values are important when computing depth buffer
	// values
	if (camera->projection == CAMERA_PERSPECTIVE) {
		// Setup perspective projection
		double top =
			RL_CULL_DISTANCE_NEAR * tanf(camera->fovy * 0.5f * DEG2RAD);
		double right = top * aspect;

		rlFrustum(-right, right, -top, top, RL_CULL_DISTANCE_NEAR,
				  RL_CULL_DISTANCE_FAR);
	} else if (camera->projection == CAMERA_ORTHOGRAPHIC) {
		// Setup orthographic projection
		double top = camera->fovy / 2;
		double right = top * aspect;

		rlOrtho(-right, right, -top, top, RL_CULL_DISTANCE_NEAR,
				RL_CULL_DISTANCE_FAR);
	}

	rlMatrixMode(RL_MODELVIEW); // Switch back to modelview matrix
	rlLoadIdentity();			// Reset current matrix (modelview)

	// Setup Camera view
	Matrix matView = MatrixLookAt(camera->position, camera->target, camera->up);
	// Multiply modelview matrix by view matrix (camera)
	rlMultMatrixf(MatrixToFloat(matView));

	rlEnableDepthTest(); // Enable DEPTH_TEST for 3D
}
