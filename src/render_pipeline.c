#include "render_pipeline.h"
#include "gamestate.h"
#include "constants.h"
#include <raylib.h>
#include <rlgl.h>

// useful for screen scaling
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// window scaling and splitscreen
static RenderTexture2D main_target;
static RenderTexture2D rt1;
static RenderTexture2D rt2;
static Texture normals;
static Texture depth;
static Shader shader;
static Shader gather_shader;

// make sure to take absolute values when using height...
static const Rectangle splitScreenRect = {
	.x = 0,
	.y = 0,
	.width = GAME_WIDTH,
	.height = -(GAME_HEIGHT / 2.0f),
};

static void init_rendertextures();
static void window_draw(float screen_scale);

void render(void (*game_draw)())
{
	// Render Camera 1
	BeginTextureMode(rt1);
	// clang-format off
		ClearBackground(RAYWHITE);
        const FullCamera *player_one = gamestate_get_p1_camera();
        BeginMode3D(player_one->camera);
            // draw in-game objects
	        BeginShaderMode(gather_shader);
            game_draw();
            EndShaderMode();

        EndMode3D();
	// clang-format on
	EndTextureMode();

	// Render Camera 2
	BeginTextureMode(rt2);
	// clang-format off
		ClearBackground(RAYWHITE);
        const FullCamera *player_two = gamestate_get_p2_camera();
        BeginMode3D(player_two->camera);
            // draw in-game objects
            game_draw();

        EndMode3D();
	// clang-format on
	EndTextureMode();

	// set draw target to the rendertexture, dont actually draw to window
	BeginTextureMode(main_target);
	ClearBackground(BLACK);
	DrawTextureRec(rt1.texture, splitScreenRect, (Vector2){0, 0}, WHITE);
	DrawTextureRec(rt2.texture, splitScreenRect,
				   (Vector2){
					   0,
					   fabsf(splitScreenRect.height),
				   },
				   WHITE);
	EndTextureMode();

	// draw the game to the window at the correct size
	BeginDrawing();
	window_draw(gamestate_get_screen_scale());
	EndDrawing();
}

void render_pipeline_init()
{
	init_rendertextures();
	shader = LoadShader(0, "assets/postprocessing/edges.fs");
	gather_shader = LoadShader("assets/postprocessing/gather.vs",
							   "assets/postprocessing/gather.fs");

	// add additional textures to rt1
	rlFramebufferAttach(rt1.id, rt1.texture.id, 0, RL_ATTACHMENT_TEXTURE2D, 0);
	rlFramebufferAttach(rt1.id, normals.id, 1, RL_ATTACHMENT_TEXTURE2D, 0);
	rlFramebufferAttach(rt1.id, depth.id, 2, RL_ATTACHMENT_TEXTURE2D, 0);

	int resolution = GetShaderLocation(shader, "resolution");
	float resolution_vec2[2] = {GAME_WIDTH, GAME_HEIGHT};
	SetShaderValue(shader, resolution, resolution_vec2, SHADER_UNIFORM_VEC2);
}

void render_pipeline_cleanup()
{
	UnloadShader(shader);
	UnloadRenderTexture(rt1);
	UnloadRenderTexture(rt2);
	rlUnloadTexture(normals.id);
	rlUnloadTexture(depth.id);
	UnloadRenderTexture(main_target);
}

///
/// Populate a gamestate with information about the screen.
///
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
	rt1 = LoadRenderTexture(w, h);
	rt2 = LoadRenderTexture(w, h);
	normals = (Texture){0};
	depth = (Texture){0};
	normals.height = h;
	normals.width = w;
	normals.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	normals.mipmaps = 1;
	normals.id = rlLoadTexture(NULL, normals.width, normals.height,
							   normals.format, normals.mipmaps);
	depth.height = h;
	depth.width = w;
	depth.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	depth.mipmaps = 1;
	depth.id = rlLoadTexture(NULL, depth.width, depth.height, depth.format,
							 depth.mipmaps);

	// set all to bilinear
	SetTextureFilter(main_target.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(rt1.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(rt2.texture, TEXTURE_FILTER_BILINEAR);
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
