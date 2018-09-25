#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>


Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("meshes.pnc"));
});

Load< GLuint > meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});

Scene::Transform *paddle_transform = nullptr;
Scene::Transform *paddle_2_transform = nullptr;
Scene::Transform *ball_transform = nullptr;
Scene::Transform *ball_transform_2 = nullptr;
Scene::Transform *fish_transform = nullptr;
Scene::Transform *crab_transform = nullptr;

Scene::Camera *camera = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//load transform hierarchy:
	ret->load(data_path("meshes.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->program = vertex_color_program->program;
		obj->program_mvp_mat4  = vertex_color_program->object_to_clip_mat4;
		obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

		MeshBuffer::Mesh const &mesh = meshes->lookup(m);
		obj->vao = *meshes_for_vertex_color_program;
		obj->start = mesh.start;
		obj->count = mesh.count;
	});

	//look up paddle and ball transforms:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "Player_1") {
			if (paddle_transform) throw std::runtime_error("Multiple 'Paddle' transforms in scene.");
			paddle_transform = t;
		}
		if (t->name == "Player_2") {
			if (paddle_2_transform) throw std::runtime_error("Multiple 'Paddle_2' transforms in scene.");
			paddle_2_transform = t;
		}
		if (t->name == "Hook") {
			if (ball_transform) throw std::runtime_error("Multiple 'Ball' transforms in scene.");
			ball_transform = t;
		}
		if (t->name == "Hook_2") {
			if (ball_transform) throw std::runtime_error("Multiple 'Ball' transforms in scene.");
			ball_transform_2 = t;
		}
		if (t->name == "Crab") {
			if (crab_transform) throw std::runtime_error("Multiple 'Crab' transforms in scene.");
			crab_transform = t;
		}
		if (t->name == "Fish") {
			if (fish_transform) throw std::runtime_error("Multiple 'Fish' transforms in scene.");
			fish_transform = t;
		}
	}
	if (!paddle_transform) throw std::runtime_error("No 'Paddle' transform in scene.");
	if (!paddle_2_transform) throw std::runtime_error("No 'Paddle_2' transform in scene.");
	if (!ball_transform) throw std::runtime_error("No 'Ball' transform in scene.");
	if (!ball_transform_2) throw std::runtime_error("No 'Ball 2' transform in scene.");
	if (!crab_transform) throw std::runtime_error("No 'Crab' transform in scene.");
	if (!fish_transform) throw std::runtime_error("No 'Fish' transform in scene.");

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
	return ret;
});


GameMode::GameMode(Client &client_) : client(client_) {
	client.connection.send_raw("h", 1); //send a 'hello' to the server

	paddle_transform -> position.y = 1.0f;
	paddle_2_transform -> position.y = 1.0f;
}

GameMode::~GameMode() {
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

	if (evt.type == SDL_MOUSEMOTION) {
		state.paddle.x = (evt.motion.x - 0.5f * window_size.x) / (0.5f * window_size.x) * Game::FrameWidth;
		state.paddle.x = std::max(state.paddle.x, -0.7f * Game::FrameWidth + 0.5f * Game::PaddleWidth);
		state.paddle.x = std::min(state.paddle.x,  0.7f * Game::FrameWidth - 0.5f * Game::PaddleWidth);
	}

	if (evt.key.keysym.scancode == SDL_SCANCODE_W && state.paddle.y < 1.0f) {
		state.paddle.y += 0.5f * (evt.type == SDL_KEYDOWN);
		return true;
	} else if (evt.key.keysym.scancode == SDL_SCANCODE_S && state.paddle.y > -4.0f) {
		state.paddle.y -= 0.5f * (evt.type == SDL_KEYDOWN);
		return true;
	} 
	return false;
}

void GameMode::update(float elapsed) {
	timer += elapsed;
	state.update(elapsed);

	if (client.connection) {
		//send game state to server:

		//arbitrary choice of 2 players i guess idk servers
		if (p1) {
			client.connection.send_raw("s", 1);
		} else  {
			client.connection.send_raw("t", 1);
		}
		client.connection.send_raw(&state.paddle.x, sizeof(float));
		client.connection.send_raw(&state.paddle.y, sizeof(float));
	}

	client.poll([&](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			//probably won't get this.
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
		} else { 
			assert(event == Connection::OnRecv);
			if (c->recv_buffer[0] == 'b') {
				memcpy(&p1, c->recv_buffer.data() + 1, sizeof(bool));
				if (p1) std::cout << "Player 1" << std::endl;
				else std::cout << "Player 2" << std::endl;
				c->recv_buffer.clear();
			} else if (c->recv_buffer[0] == 'g'){
				if (c->recv_buffer.size() < (1 + 6 * sizeof(float) + 3 * sizeof(int) )) {
					return; //wait for more data
				} else {
					memcpy(&state.player_1.x, 	c->recv_buffer.data() + 1, 						 					sizeof(float));
					memcpy(&state.player_1.y, 	c->recv_buffer.data() + 1 + sizeof(float), 		 					sizeof(float));
					memcpy(&state.player_2.x, 	c->recv_buffer.data() + 1 + (2 * sizeof(float)), 					sizeof(float));
					memcpy(&state.player_2.y, 	c->recv_buffer.data() + 1 + (3 * sizeof(float)),		 			sizeof(float));
					memcpy(&state.fish.x, 		c->recv_buffer.data() + 1 + (4 * sizeof(float)), 					sizeof(float));
					memcpy(&state.fish.y, 		c->recv_buffer.data() + 1 + (5 * sizeof(float)), 					sizeof(float));
					memcpy(&state.fish_owner,	c->recv_buffer.data() + 1 + (6 * sizeof(float)), 					sizeof(int));
					memcpy(&state.p1_score, 	c->recv_buffer.data() + 1 + (6 * sizeof(float)) + sizeof(int),		sizeof(int));
					memcpy(&state.p2_score, 	c->recv_buffer.data() + 1 + (6 * sizeof(float)) + 2 * sizeof(int), 	sizeof(int));
					c->recv_buffer.clear();
				}
			} else {
				std::cerr << "Ignoring " << c->recv_buffer.size() << " bytes from server." << std::endl;
			}
		}
	}, 0.1);
	//copy game state to scene positions:

	if (p1) {
		paddle_transform->position.x = state.player_1.x;
		ball_transform->position.x = state.player_1.x;
		ball_transform->position.y = state.player_1.y;

		paddle_2_transform->position.x = state.player_2.x;
		ball_transform_2->position.x = state.player_2.x;
		ball_transform_2->position.y = state.player_2.y;
	} else {
		paddle_transform->position.x = state.player_2.x;
		ball_transform->position.x = state.player_2.x;
		ball_transform->position.y = state.player_2.y;


		paddle_2_transform->position.x = state.player_1.x;
		ball_transform_2->position.x = state.player_1.x;
		ball_transform_2->position.y = state.player_1.y;
	}

	fish_transform->position.x = state.fish.x;
	fish_transform->position.y = state.fish.y;
}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	camera->aspect = drawable_size.x / float(drawable_size.y);

	glClearColor(0.95f, 0.66f, 0.25f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light positions:
	glUseProgram(vertex_color_program->program);

	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.99f, 0.99f, 0.99f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.95f, 0.66f, 0.25f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

	scene->draw(camera);


	std::string p1_name = "PLAYER 1";
	float height = 0.05f;
	draw_text(p1_name, glm::vec2(-1.4f,0.92f), height, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	draw_text(p1_name, glm::vec2(-1.39f,0.91f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	std::string p2_name = "PLAYER 2";
	height = 0.05f;
	draw_text(p2_name, glm::vec2(1.0f,0.92f), height, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
	draw_text(p2_name, glm::vec2(1.01f,0.91f), height, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

	if (p1) {
		//i was running out of time give me a break
		std::string p1_str = std::to_string(state.p1_score);
		height = 0.1f;
		draw_text(p1_str, glm::vec2(-1.4f,0.8f), height, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

		std::string p2_str = std::to_string(state.p2_score);
		height = 0.1f;
		draw_text(p2_str, glm::vec2(1.0f,0.8f), height, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
	} else {
		std::string p1_str = std::to_string(state.p2_score);
		height = 0.1f;
		draw_text(p1_str, glm::vec2(-1.4f,0.8f), height, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

		std::string p2_str = std::to_string(state.p1_score);
		height = 0.1f;
		draw_text(p2_str, glm::vec2(1.0f,0.8f), height, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
	}

	GL_ERRORS();
}
