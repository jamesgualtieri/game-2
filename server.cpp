#include "Connection.hpp"
#include "Game.hpp"

#include <iostream>
#include <set>
#include <chrono>

bool intersect(float x1, float y1, float x2, float y2) {
	float eps = 0.5f;
	if (std::abs(std::sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2))) < eps) {
		return true;
	} else {
		return false;
	}
}

void new_fish(Game *state) {
	if (state->dir == 1) state->dir = -1;
	else state->dir = 1;
	state->fish.x = -state->dir * 10.0f; 
	state->fish.y = (rand()%20 - 10)*0.1f - 2.0f;
	state->fish_owner = 0;
}

int main(int argc, char **argv) {
	srand(time(NULL));
	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}
	
	Server server(argv[1]);

	Game state;

	while (1) {
		server.poll([&](Connection *c, Connection::Event evt){
			if (evt == Connection::OnOpen) {
			} else if (evt == Connection::OnClose) {
			} else { assert(evt == Connection::OnRecv);
				if (c->recv_buffer[0] == 'h') {
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
					std::cout << c << ": Got hello." << std::endl;
					c->recv_buffer.clear();
					//only a 2 player game so foh if theres more than 2 players.
					if (!state.p1_connect) {
						//init game state as player 1
						state.p1_connect = true;
						c->send_raw("b", 1);
						c->send_raw(&state.p1_connect, sizeof(bool));
						std::cout << "Waiting on player 2 to connect...\n";
					} else {
						//otherwise init as player 2
						bool p1 = false;
						c->send_raw("b", 1);
						c->send_raw(&p1, sizeof(bool));
						std::cout << "Player 2 connected. Game start\n";
					}
				} else if (c->recv_buffer[0] == 's') {
					//player 1 position data
					if (c->recv_buffer.size() < 1 + 2 * sizeof(float) ) {
						return; //wait for more data
					} else {
						memcpy(&state.player_1.x, c->recv_buffer.data() + 1, 						sizeof(float));
						memcpy(&state.player_1.y, c->recv_buffer.data() + 1 + sizeof(float), 		sizeof(float));
						c->recv_buffer.clear();
					}
				} else if (c->recv_buffer[0] == 't') {
					//player two position data ('t' for two I guess)
					if (c->recv_buffer.size() < 1 + 2 * sizeof(float) ) {
						return; //wait for more data
					} else {
						memcpy(&state.player_2.x, c->recv_buffer.data() + 1, 						sizeof(float));
						memcpy(&state.player_2.y, c->recv_buffer.data() + 1 + sizeof(float), 		sizeof(float));
						c->recv_buffer.clear();
					}
				} 
			}
			if (intersect(state.fish.x, state.fish.y, state.player_1.x, state.player_1.y)) {
				state.fish_owner = 1;
			} else if (intersect(state.fish.x, state.fish.y, state.player_2.x, state.player_2.y)) {
				state.fish_owner = 2;
			} 

			if (state.fish_owner == 1){
				state.fish.x = state.player_1.x;
				state.fish.y = state.player_1.y;

				if (state.fish.y == 1.0f){
					std::cout << "before " << state.fish.x << std::endl;
					state.p1_score += 10;
					new_fish(&state);
					state.fish_owner = 0;
					std::cout << "after " << state.fish.x << std::endl;
				}
			} else if (state.fish_owner == 2){
				state.fish.x = state.player_2.x;
				state.fish.y = state.player_2.y;

				if (state.fish.y == 1.0f){
					//top of the boat i guess
					state.p2_score += 10;
					std::cout << "before " << state.fish_owner << std::endl;
					new_fish(&state);
					state.fish_owner = 0;
					std::cout << "after " << state.fish_owner << std::endl;
				}
			} else {
				state.fish.x += state.dir * 0.05f;
				if (state.fish.x > 10.0f || state.fish.x < -10.0f) {
					new_fish(&state);
				}
			}

			c->send_raw("g", 1);
			c->send_raw(&state.player_1.x, sizeof(float));
			c->send_raw(&state.player_1.y, sizeof(float));
			c->send_raw(&state.player_2.x, sizeof(float));
			c->send_raw(&state.player_2.y, sizeof(float));
			c->send_raw(&state.fish.x, sizeof(float));
			c->send_raw(&state.fish.y, sizeof(float));
			c->send_raw(&state.fish_owner, sizeof(int));
			c->send_raw(&state.p1_score, sizeof(int));
			c->send_raw(&state.p2_score, sizeof(int));
			c->send_raw(&state.dir, sizeof(int));
		}, 0.1);
		//every second or so, dump the current paddle position:
		static auto then = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		if (now > then + std::chrono::seconds(1)) {
			then = now;
			std::cout << "Player 1:" << state.player_1.x << " " << state.player_1.y  << std::endl;
			std::cout << "Player 2:" << state.player_2.x << " " << state.player_2.y << std::endl;
			std::cout << "(" << state.fish_owner << "," << state.p1_score << "," <<state.p2_score << ")\n";

		}
	}
}
