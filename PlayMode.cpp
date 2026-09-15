#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <array>

enum game_state {
	throwing,
	idle,
	demo,
	won
} state = demo;
int difficulty = 2;
int increment_difficulty_in = 2;
int score = 0;
void PlayMode::handle_win_stats() {
	increment_difficulty_in--;
	if (increment_difficulty_in <= 0) {
		difficulty++;
		increment_difficulty_in = difficulty - 1;
	}
	score++;
}
std::random_device rd;
std::mt19937 gen(rd());

std::vector<int> solution;
std::vector<int> answer;
int answer_index = 0;
std::vector<int> correct_sounds;
std::vector<std::array<int,4>> incorrect_sounds;
void PlayMode::generate_solution() {
	solution = std::vector<int>(difficulty);
	answer = std::vector<int>(difficulty);
	correct_sounds = std::vector<int>(difficulty);
	incorrect_sounds = std::vector<std::array<int,4>>(difficulty);
	answer_index = 0;
	for (int i = 0; i < difficulty; i++) {
		solution[i] = std::uniform_int_distribution<int>(0,3)(gen);
		correct_sounds[i] = std::uniform_int_distribution<int>(0,3)(gen);

		//more difficult: each ingredient can be a different sound depending on order
		// for (int option = 0; option < 4; option++) {
		// 	while (true) {
		// 		incorrect_sounds[i][option] = std::uniform_int_distribution<int>(0,3)(gen);
		// 		if (incorrect_sounds[i][option] != correct_sounds[i])
		// 			break;
		// 	}
		// }

		//easier: each ingredient has the same sound
		int s;
		while (true) {
			s = std::uniform_int_distribution<int>(0,3)(gen);
			if (s != correct_sounds[i])
				break;
		}
		for (int option = 0; option < 4; option++) {
			incorrect_sounds[i][option] = s;
		}
	}
}
void PlayMode::add_to_answer(int ingredient) {
	answer[answer_index] = ingredient;
	if (ingredient == solution[answer_index]) {
		play_fruit_sample(correct_sounds[answer_index]);
	} else {
		play_fruit_sample(incorrect_sounds[answer_index][ingredient]);
	}
	answer_index++;
}
float throw_progress = 0.0f;
int thrown = -1;

GLuint witch_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > witch_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("witch.pnct"));
	witch_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > witch_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("witch.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = witch_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = witch_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > background_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("background.wav"));
});

Load< Sound::Sample > sample1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit1.wav"));
});
Load< Sound::Sample > sample2(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit2.wav"));
});
Load< Sound::Sample > sample3(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit3.wav"));
});
Load< Sound::Sample > sample4(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit4.wav"));
});
Load< Sound::Sample > sample5(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit5.wav"));
});
Load< Sound::Sample > sample6(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit6.wav"));
});
Load< Sound::Sample > sample7(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit7.wav"));
});
Load< Sound::Sample > sample8(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit8.wav"));
});
Load< Sound::Sample > sample9(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit9.wav"));
});
Load< Sound::Sample > sample10(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/fruit10.wav"));
});
void PlayMode::play_fruit_sample(int num) {
	switch (num) {
		case 0: Sound::play(*sample1); break;
		case 1: Sound::play(*sample2); break;
		case 2: Sound::play(*sample3); break;
		case 3: Sound::play(*sample4); break;
		case 4: Sound::play(*sample5); break;
		case 5: Sound::play(*sample6); break;
		case 6: Sound::play(*sample7); break;
		case 7: Sound::play(*sample8); break;
		case 8: Sound::play(*sample9); break;
		case 9: Sound::play(*sample10); break;
	}
}

std::array<Scene::Transform *, 16> bubbles{};
std::array<Scene::Transform *, 4> ingredients{};
std::array<glm::vec3, 4> ingredients_initial_pos{};
glm::vec3 water_waypoint;
float bubble_scale;

glm::vec3 PlayMode::random_bubble_position() {
	static float water_radius = 1.1f;

	float angle = std::uniform_real_distribution<float>(0.0f, 2.0f *glm::pi<float>())(gen);

	float radius = water_radius * std::sqrt(std::uniform_real_distribution<float>(0.0f, 1.0f)(gen));

	return glm::vec3 {
		water_waypoint.x + radius * glm::cos(angle),
		water_waypoint.y + radius * glm::sin(angle),
		water_waypoint.z
	};
}

glm::vec3 PlayMode::thrown_ingredient_position(float progress, int ingredient) {
	static float arc_height = 2.0f;
	glm::vec3 start = ingredients_initial_pos[ingredient];
	glm::vec3 end = glm::vec3 {
		water_waypoint.x,
		water_waypoint.y,
		water_waypoint.z - 0.1f
	};
	glm::vec3 xy = glm::mix(start, end, progress);
	return glm::vec3 {
		xy.x,
		xy.y,
		arc_height * glm::sin(progress * glm::pi<float>()) + xy.z
	};
}

int demo_index = 0;
float demo_timer = 1.0f;

PlayMode::PlayMode() : scene(*witch_scene) {
	int bubbles_i = 0;
	for (auto &transform : scene.transforms) {
		if (transform.name == "CauldronWater") {
			water_waypoint = transform.make_world_from_local() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	for (auto &transform : scene.transforms) {
		if (transform.name.starts_with("Bubble")) {
			bubbles[bubbles_i] = &transform;
			bubble_scale = transform.scale.x;
			transform.position = random_bubble_position();
			bubbles_i++;
		}
		if (transform.name == "Aloe") {
			ingredients[3] = &transform;
			ingredients_initial_pos[3] = transform.position;
		} else if (transform.name == "Prune") {
			ingredients[2] = &transform;
			ingredients_initial_pos[2] = transform.position;
		} else if (transform.name == "Xylitol") {
			ingredients[1] = &transform;
			ingredients_initial_pos[1] = transform.position;
		} else if (transform.name == "Rhubarb") {
			ingredients[0] = &transform;
			ingredients_initial_pos[0] = transform.position;
		}
	}
	generate_solution();
	demo_index = 0;
	demo_timer = 2.0f;

	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;

	// //get pointer to camera for convenience:
	// if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));

	// //start music loop playing:
	// // (note: position will be over-ridden in update())
	// leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);

	Sound::loop(*background_sample);

	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_1) {
			one.downs += 1;
			one.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_2) {
			two.downs += 1;
			two.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_3) {
			three.downs += 1;
			three.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_4) {
			four.downs += 1;
			four.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_1) {
			one.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_2) {
			two.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_3) {
			three.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_4) {
			four.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	}

	return false;
}

float time_elapsed = 0.0f;
float won_timer = 3.0f;

void PlayMode::update(float elapsed) {
	time_elapsed += elapsed;

	{ //update bubbles
		for (int i = 0; i < bubbles.size(); i++) {
			float bubbles_z_add = 0.15f * glm::sin(time_elapsed / 1.0f + float(i)) - 0.1f;
			glm::vec3 bubble_position = bubbles[i]->position;
			bubble_position.z = water_waypoint.z;
			bubble_position.z += bubbles_z_add;
			bubbles[i]->position = bubble_position;
			if (bubbles_z_add < -0.2f) {
				glm::vec3 bubble_pos = bubbles[i]->position;
				glm::vec3 bubble_random = random_bubble_position();
				bubbles[i]->position = glm::vec3(
					bubble_random.x,
					bubble_random.y,
					bubble_pos.z
				);
				bubbles[i]->scale = glm::vec3(bubble_scale);
			} else if (bubbles_z_add > 0.0f) {
				if (std::uniform_real_distribution<float>(0.0f, 1.0f)(gen) < 0.005f) {
					bubbles[i]->scale = glm::vec3(0.0f);
				}
			}
		}
	}

	if (state == idle) {
		if (one.pressed) {
			state = throwing;
			thrown = 0;
		} else if (two.pressed) {
			state = throwing;
			thrown = 1;
		} else if (three.pressed) {
			state = throwing;
			thrown = 2;
		} else if (four.pressed) {
			state = throwing;
			thrown = 3;
		}
	} else if (state == throwing) {
		ingredients[thrown]->position = thrown_ingredient_position(throw_progress, thrown);
		throw_progress += elapsed * 0.9f;
		if (throw_progress > 1.0f) {
			ingredients[thrown]->position = ingredients_initial_pos[thrown];
			add_to_answer(thrown);
			throw_progress = 0.0f;
			if (answer_index >= solution.size()) {
				if (solution == answer) {
					//won
					handle_win_stats();
					generate_solution();
					state = won;
					won_timer = 3.0f;
				} else {
					answer_index = 0;
					state = demo;
					demo_index = 0;
					demo_timer = 2.0f;
				}
			} else {
				state = idle;
			}
		}
	} else if (state == demo) {
		demo_timer -= elapsed;
		if (demo_timer <= 0.0f) {
			play_fruit_sample(correct_sounds[demo_index]);
			demo_index++;
			demo_timer = 1.0f;
			if (demo_index > difficulty - 1)
				state = idle;
		}
	} else if (state == won) {
		won_timer -= elapsed;
		if (won_timer <= 0.0f) {
			answer_index = 0;
			state = demo;
			demo_index = 0;
			demo_timer = 2.0f;
		}
	}

	// //slowly rotates through [0,1):
	// wobble += elapsed / 10.0f;
	// wobble -= std::floor(wobble);

	// hip->rotation = hip_base_rotation * glm::angleAxis(
	// 	glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 1.0f, 0.0f)
	// );
	// upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );
	// lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );

	// //move sound to follow leg tip position:
	// leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	//move camera:
	// {

	// 	//combine inputs into a move:
	// 	constexpr float PlayerSpeed = 30.0f;
	// 	glm::vec2 move = glm::vec2(0.0f);
	// 	if (left.pressed && !right.pressed) move.x =-1.0f;
	// 	if (!left.pressed && right.pressed) move.x = 1.0f;
	// 	if (down.pressed && !up.pressed) move.y =-1.0f;
	// 	if (!down.pressed && up.pressed) move.y = 1.0f;

	// 	//make it so that moving diagonally doesn't go faster:
	// 	if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

	// 	glm::mat4x3 frame = camera->transform->make_parent_from_local();
	// 	glm::vec3 frame_right = frame[0];
	// 	//glm::vec3 up = frame[1];
	// 	glm::vec3 frame_forward = -frame[2];

	// 	camera->transform->position += move.x * frame_right + move.y * frame_forward;
	// }

	// { //update listener to camera position:
	// 	glm::mat4x3 frame = camera->transform->make_parent_from_local();
	// 	glm::vec3 frame_right = frame[0];
	// 	glm::vec3 frame_at = frame[3];
	// 	Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	// }

	//reset button press counters:
	one.downs = 0;
	two.downs = 0;
	three.downs = 0;
	four.downs = 0;
	space.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	if (state == won)
		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	else if (state == demo)
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	else
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

// glm::vec3 PlayMode::get_leg_tip_position() {
// 	//the vertex position here was read from the model in blender:
// 	return lower_leg->make_world_from_local() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
// }
