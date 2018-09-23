#include "Connection.hpp"
#include "Game.hpp"

#include <iostream>
#include <set>
#include <chrono>

int main(int argc, char **argv) {
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
					if (c->recv_buffer.size() < 1 + sizeof(float)) {
						return; //wait for more data
					} else {
						memcpy(&state.player_1.x, c->recv_buffer.data() + 1, sizeof(float));
						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(float));
					}
				} else if (c->recv_buffer[0] == 't') {
					//player two position data ('t' for two I guess)
					if (c->recv_buffer.size() < 1 + sizeof(float)) {
						return; //wait for more data
					} else {
						memcpy(&state.player_2.x, c->recv_buffer.data() + 1, sizeof(float));
						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(float));
					}
				}
				c->send_raw("g", 1);
				c->send_raw(&state.player_1.x, sizeof(float));
				c->send_raw(&state.player_2.x, sizeof(float));
			}
		}, 0.01);
		//every second or so, dump the current paddle position:
		static auto then = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		if (now > then + std::chrono::seconds(1)) {
			then = now;
			std::cout << "Player 1:" << state.player_1.x << std::endl;
			std::cout << "Player 2:" << state.player_2.x << std::endl;
		}
	}
}
