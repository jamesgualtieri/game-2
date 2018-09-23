#pragma once

#include <glm/glm.hpp>

struct Game {
	glm::vec2 paddle = glm::vec2(0.0f,-3.0f);
	glm::vec2 ball = glm::vec2(0.0f, 0.0f);
	glm::vec2 ball_velocity = glm::vec2(0.0f,-2.0f);

    glm::vec2 player_1 = glm::vec2(0.0f, -3.0f); //boat x and hook y coords
    glm::vec2 player_2 = glm::vec2(0.0f, -3.0f);
    glm::vec2 crab = glm::vec2(0.0f, 0.0f);

    bool p1_connect = false;

	void update(float time);

	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float PaddleWidth = 2.0f;
	static constexpr const float PaddleHeight = 0.4f;
	static constexpr const float BallRadius = 0.5f;
};
