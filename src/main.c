#include "defs.h"
#include "linalg.h"
#include "draw.h"
#include "import.h"
#include "input.h"
#include "timing.h"
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <sys/time.h>

SDL_Window *window;
struct Vec4 inp_rots = {0};
struct Vec4 cam_trans = {0};
struct Vec4 delta_trans = {0};

struct Timer *delta;

void err_if_null(void *ptr, char *str) {
	if (!ptr) {
		fprintf(stderr, "%s", str);
		exit(EXIT_FAILURE);
	}
}

void cam_transform(double delta) {
	delta_trans = (struct Vec4) {0,0,0,0};
	if (key_held(SDL_SCANCODE_W)) delta_trans.z -= 3 * delta;
	if (key_held(SDL_SCANCODE_S)) delta_trans.z += 3 * delta;
	if (key_held(SDL_SCANCODE_A)) delta_trans.x -= 3 * delta;
	if (key_held(SDL_SCANCODE_D)) delta_trans.x += 3 * delta;
	if (key_held(SDL_SCANCODE_SPACE)) delta_trans.y -= 3 * delta;
	if (key_held(SDL_SCANCODE_LSHIFT)) delta_trans.y += 3 * delta;

	if (key_held(SDL_SCANCODE_LEFT)) inp_rots.y += 3 * delta;
	if (key_held(SDL_SCANCODE_RIGHT)) inp_rots.y -= 3 * delta;
	if (key_held(SDL_SCANCODE_UP)) inp_rots.x -= 3 * delta;
	if (key_held(SDL_SCANCODE_DOWN)) inp_rots.x += 3 * delta;
}

void draw_cube(SDL_Surface *surface, double delta) {
	struct Mesh m = parse_obj("bin/cube.obj");

	// clang-format off
	struct Triangle cube[12] = {
		(struct Triangle) {(struct Vec4){0, 0, 0, 1}, (struct Vec4){0, 1, 0, 1}, (struct Vec4){1, 1, 0, 1}},
		(struct Triangle) {(struct Vec4){0, 0, 0, 1}, (struct Vec4){1, 1, 0, 1}, (struct Vec4){1, 0, 0, 1}},

		(struct Triangle) {(struct Vec4){1, 0, 0, 1}, (struct Vec4){1, 1, 0, 1}, (struct Vec4){1, 1, 1, 1}},
		(struct Triangle) {(struct Vec4){1, 0, 0, 1}, (struct Vec4){1, 1, 1, 1}, (struct Vec4){1, 0, 1, 1}},

		(struct Triangle) {(struct Vec4){1, 0, 1, 1}, (struct Vec4){1, 1, 1, 1}, (struct Vec4){0, 1, 1, 1}},
		(struct Triangle) {(struct Vec4){1, 0, 1, 1}, (struct Vec4){0, 1, 1, 1}, (struct Vec4){0, 0, 1, 1}},

		(struct Triangle) {(struct Vec4){0, 0, 1, 1}, (struct Vec4){0, 1, 1, 1}, (struct Vec4){0, 1, 0, 1}},
		(struct Triangle) {(struct Vec4){0, 0, 1, 1}, (struct Vec4){0, 1, 0, 1}, (struct Vec4){0, 0, 0, 1}},

		(struct Triangle) {(struct Vec4){0, 1, 0, 1}, (struct Vec4){0, 1, 1, 1}, (struct Vec4){1, 1, 1, 1}},
		(struct Triangle) {(struct Vec4){0, 1, 0, 1}, (struct Vec4){1, 1, 1, 1}, (struct Vec4){1, 1, 0, 1}},

		(struct Triangle) {(struct Vec4){1, 0, 1, 1}, (struct Vec4){0, 0, 1, 1}, (struct Vec4){0, 0, 0, 1}},
		(struct Triangle) {(struct Vec4){1, 0, 1, 1}, (struct Vec4){0, 0, 0, 1}, (struct Vec4){1, 0, 0, 1}},
	};
	// clang-format on

	cam_transform(delta);

	struct Mat4x4 proj = get_proj_matrix(0.1f, 1000.0f, M_PI/4, 
									  (float)SCREEN_HEIGHT/(float)SCREEN_WIDTH);

	struct Mat4x4 model = get_model_matrix(&(struct Vec4){0.0f, 0.0f,  0.0f},
										   &(struct Vec4){0.0f, 0.0f, -3.0f},
										   &(struct Vec4){1.0f, 1.0f,  1.0f});


	// Transform the input absolute position to camera relative position.
	struct Mat4x4 cam_rot = get_rot_matrix(&inp_rots);
	struct Vec4 rel_trans = matrix_vec_mul(&cam_rot, &delta_trans);

	cam_trans = vector_add(&cam_trans, &rel_trans);

	struct Mat4x4 cam_m = get_model_matrix(&inp_rots,
										   &cam_trans,
										   &(struct Vec4){1.0f, 1.0f, 1.0f});

	struct Mat4x4 view = get_view_matrix(cam_m);

	struct Mat4x4 model_view = matrix_matrix_mul(model, view);

	for (int i = 0; i < 12; i++) {
		struct Triangle t;

		// First half of processing till clip space
		for (int j = 0; j < 3; j++) {
			t.verts[j] = cube[i].verts[j];

			// C = P * V * M * v_model
			// Here C is clipspace, P is projection matrix, V is view matrix,
			// M is model matrix. v_model is vertex in model's object space.

			// This can be done with a model matrix.
			t.verts[j] = matrix_vec_mul(&model_view, &t.verts[j]);

			// Projection matrix step.
			t.verts[j] = matrix_vec_mul(&proj, &t.verts[j]);
		}

		// TODO: If triangle is not within Camera frustrum, don't render.
		// Culled tris completely out.
		// Still need to remove vertices and re-triangulate the broken tris.

		uint8_t outcodes[3] = {};
		for (int j = 0; j < 3; j++) {
			struct Vec4 v = t.verts[j];

			// If the vertex is clipped at all, the bitwise operations will make
			// the outcode non-zero. In the future, if needed, we can also get
			// which plane it is out of bounds with a simple &.
			outcodes[j] = 0
				| ((v.x < -v.w) << 0)	// Left plane
				| ((v.x >  v.w) << 1)	// Right plane
				| ((v.y < -v.w) << 2)	// Bottom plane
				| ((v.y >  v.w) << 3)	// Top plane
				| ((v.z < -v.w) << 4)	// Near plane
				| ((v.z >  v.w) << 5);	// Far plane
		}

		// Culling ONLY if tri is completely out of ONE plane.
		// If one vertex is outside left plane, while other is out right plane
		// and the third is out the top plane, this WON'T cull it.
		// We'll need to do fancy clipping for that.
		//
		// Also has edge case where if tri is in position where verts are out of
		// the left, back and top planes, for example, or left-left-back, while
		// being completely outside the frustrum, we can't detect and cull it yet
		// Likely will need to do a test to see if edge (or plane once we
		// rasterize tris) clips into the frustrum and clip the tri accordingly
		// This only concerns corners and edges of view frustrum.
		//
		// Actually, Raster clipping should take care of parts that clip into
		// frustrum (except on z-axis, we clip normally there) by only drawing
		// frustrum, but I'd still like to cull the tris we can.
		if ((outcodes[0] & outcodes[1] & outcodes[2]) > 0) {
			print_vec4(t.verts[0], "Vec0");
			print_vec4(t.verts[1], "Vec1");
			print_vec4(t.verts[2], "Vec2");
			for (int j = 0; j < 3; j++) {
				printf("%d ", outcodes[j]);
			}
			printf("\n");
			continue; // entire triangle is out of frustrum, ignore.
		}

		// Clip z-axis. We do raster clipping for x and y (i.e., clip at render)
		if ((outcodes[0] | outcodes[1] | outcodes[2]) >= (1<<4)) {
			// TODO
		}

		// Second half of processing till screen space
		for (int j = 0; j < 3; j++) {
			// Projection division step. Converts from Clip-space -> NDC.
			t.verts[j] = vector_div(&t.verts[j], t.verts[j].w);

			// Viewport transformation steps. Should be the last thing done.
			// converts from NDC -> screen space.
			t.verts[j] = vector_add(&t.verts[j], &((struct Vec4){1, 1, 0}));
			t.verts[j].x *= 0.5f * (float)SCREEN_WIDTH;
			t.verts[j].y *= 0.5f * (float)SCREEN_HEIGHT;
		}

		struct Vec4 a = vector_sub(&t.verts[0], &t.verts[1]);
		struct Vec4 b = vector_sub(&t.verts[0], &t.verts[2]);

		// Positive if CCW, negative if CW. Only render CCW faces.
		// 2D cross prod (determinant formula).
		float winding_order = (a.x * b.y) - (b.x * a.y);

		// Only draw if winding in CCW. CW means backface, so this just
		// defines normals.
		if (winding_order > 0) draw_triangle(surface, t);
	}
}

void init_sdl(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	// Setting x and y pos to SDL_WINDOW_POS_UNDEFINED leaves window positioning
	// upto the OS
	window = SDL_CreateWindow(
		"Renderer", 
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, 
		SCREEN_WIDTH, SCREEN_HEIGHT, 
		SDL_WINDOW_RESIZABLE
	);
	err_if_null(window, "Couldn't initialize SDL Window. Returned NULL!");
}

void draw_loop(double delta) {
	err_if_null(window, "Couldn't initialize SDL Window. Returned NULL!");

	SDL_Surface *surface = SDL_GetWindowSurface(window);
	err_if_null(surface, "Couldn't initialize SDL Surface. Returned NULL!");

	SDL_LockSurface(surface);
	SDL_FillRect(surface, NULL, 0x000000);

	draw_cube(surface, delta);
	
	SDL_UnlockSurface(surface);
	SDL_UpdateWindowSurface(window);
}

void cleanup(void) {
	destroy_timer(delta);
	// Handle application closing
	SDL_DestroyWindow(window);
	SDL_Quit();
}

double delta_time = 1;

const int target_fps = 60;
uint64_t us_per_frame = 1e6 / target_fps;

int main(int argc, char *argv[]) {
	// As name suggests, initialize SDL.
	init_sdl();

	atexit(cleanup);
	delta = create_timer();

	// Main "game" loop.
	while (1) {
		start_timer(delta);
		update_input();

		draw_loop(delta_time);

		mark_timer(delta);
		uint64_t d_draw = get_elapsed_time(delta) / 1e3;

		uint64_t sleep_us = (us_per_frame - d_draw);
		sleep_us = sleep_us >= 0 ? sleep_us : 0;

		us_sleep(sleep_us);

		mark_timer(delta);
		delta_time = (double)get_elapsed_time(delta) / 1e6;
		/* printf("FPS: %f\n", 1/delta_time); */
	}

	return EXIT_SUCCESS;
}
