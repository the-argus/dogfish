#include "raylib.h"

#include "render_pipeline.h"
#include "constants.h"

// useful for screen scaling
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// window scaling and splitscreen
static RenderTexture2D main_target;
static RenderTexture2D rt1;
static RenderTexture2D rt2;
static Shader shader;

// make sure to take absolute values when using height...
static const Rectangle splitScreenRect = {
	.x = 0, .y = 0, .width = GAME_WIDTH, .height = (int)-(GAME_HEIGHT / 2)};

static void init_rendertextures();
static void window_draw(float screen_scale);

void render(Gamestate gamestate, void (*game_draw)())
{
	// Render Camera 1
	BeginTextureMode(rt1);
	// clang-format off
		    ClearBackground(RAYWHITE);
            BeginMode3D(*gamestate.p1_camera);
                // draw in-game objects
                game_draw();

            EndMode3D();
	// clang-format on
	EndTextureMode();

	// Render Camera 2
	BeginTextureMode(rt2);
	// clang-format off
		    ClearBackground(RAYWHITE);
            BeginMode3D(*gamestate.p2_camera);
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
				   (Vector2){0, abs((int)splitScreenRect.height)}, WHITE);
	EndTextureMode();

	// draw the game to the window at the correct size
	BeginDrawing();
	// BeginShaderMode(shader);
	window_draw(gamestate.screen_scale);
	// EndShaderMode();
	EndDrawing();
}

void init_render_pipeline()
{
	init_rendertextures();
	shader = LoadShader(0, "assets/postprocessing/edges.fs");
	int resolution = GetShaderLocation(shader, "resolution");
	float resolution_vec2[2] = {GAME_WIDTH, GAME_HEIGHT};
	SetShaderValue(shader, resolution, resolution_vec2, SHADER_UNIFORM_VEC2);
}

void cleanup_render_pipeline()
{
	UnloadShader(shader);
	UnloadRenderTexture(rt1);
	UnloadRenderTexture(rt2);
	UnloadRenderTexture(main_target);
}

///
/// Populate a gamestate with information about the screen.
///
void gather_screen_info(Gamestate *gamestate)
{
	// fraction of window resize that will occur this frame, basically the
	// difference between current width/height ratio to target width/height
	gamestate->screen_scale = MIN((float)GetScreenWidth() / GAME_WIDTH,
								  (float)GetScreenHeight() / GAME_HEIGHT);
}

/// Initialize the main rendertexture to which the actual game elements are
/// drawn. Determines the universal texture filter for scaling.
static void init_rendertextures()
{
	// variable width screen
	main_target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);

	rt1 = LoadRenderTexture(abs((int)splitScreenRect.width),
							abs((int)splitScreenRect.height));
	rt2 = LoadRenderTexture(abs((int)splitScreenRect.width),
							abs((int)splitScreenRect.height));

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
		(Rectangle){0.0f, 0.0f, (float)main_target.texture.width,
					(float)-main_target.texture.height},
		(Rectangle){
			(GetScreenWidth() - ((float)GAME_WIDTH * screen_scale)) * 0.5f,
			(GetScreenHeight() - ((float)GAME_HEIGHT * screen_scale)) * 0.5f,
			(float)GAME_WIDTH * screen_scale,
			(float)GAME_HEIGHT * screen_scale},
		(Vector2){0, 0}, 0.0f, WHITE);
}
