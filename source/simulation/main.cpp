#include <cstddef>
#include <stdio.h>
#include <vector>
#include <random>

#include <glm/glm.hpp>

#include "../state/client.h"
#include "../state/server.h"
#include "../state/instance.h"

size_t user_count = 10'000, server_count = 10, seed = 1;
float runtime = 60 * 10, tick = 0.1, move_speed = 0.1f;

int main()
{
    std::mt19937 random(seed);
    std::normal_distribution<float> positions(0.f, 5.f);
    std::normal_distribution<float> timeouts(150.f, 120.f);

    struct {
        std::vector<::client> client{user_count};
        std::vector<float> timeout = std::vector<float>(user_count);
        std::vector<glm::vec3> destination{user_count};
    } players;

    for (size_t i = 0; i < user_count; ++i) {
        players.timeout[i] = timeouts(random);
        players.client[i] = {
            .user_position = glm::vec3(
                positions(random), positions(random), positions(random)
            ),
            .identification = (unsigned)i,
        };
        players.destination[i] = players.client[i].user_position;
    }

    std::vector<server> servers(server_count);

    for (size_t i = 0; i < server_count; ++i) {
        servers[i] = {
            .identification = (unsigned)i,
        };
    }

    for (float t = 0; t < runtime; t += tick) {
        for (size_t i = 0; i < user_count; ++i) {
            players.timeout[i] -= tick;
            if (players.timeout[i] <= 0.f) {
                players.destination[i] = glm::vec3(
                    positions(random), positions(random), positions(random)
                );
                players.timeout[i] = timeouts(random);
            }
            auto direction = 
                players.destination[i] - players.client[i].user_position;
            players.client[i].user_position += 
                direction / 
                glm::max(glm::length(direction) / move_speed, 1.f);
        }

        printf(
            "%f %f %f\n", 
            players.client[0].user_position.x, 
            players.client[0].user_position.y, 
            players.client[0].user_position.z
        );
    }

	return 0;
}