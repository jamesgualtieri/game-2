#pragma once

#include <glm/glm.hpp>

struct Game {
	glm::vec2 paddle = glm::vec2(0.0f,1.0f);
	// glm::vec2 ball = glm::vec2(0.0f, 0.0f);
	// glm::vec2 ball_velocity = glm::vec2(0.0f,-2.0f);

    glm::vec2 player_1 = glm::vec2(0.0f, 1.0f); //boat x and hook y coords
    glm::vec2 player_2 = glm::vec2(0.0f, 1.0f);
    glm::vec2 crab = glm::vec2(0.0f, 0.0f);
    glm::vec2 fish = glm::vec2(-4.0f, -3.0f);


    bool p1_connect = false;
    int fish_owner = 0; // 1 for p1, 2 for p2, 0 for neither. Prevent from double fishing same fish
	
    int p1_score = 0;
    int p2_score = 0;
    int dir = 1;

    void update(float time);
	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float PaddleWidth = 2.0f;
	static constexpr const float PaddleHeight = 0.4f;
	static constexpr const float BallRadius = 0.5f;
};
