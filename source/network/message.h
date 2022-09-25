#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct message {
    enum class type {
        // sent by client
        login = 1, 

        // message contains a (partial) (delta) update of instance data
        // sent either by server or client, also used between servers
        world_update = 2, 

        // sent by server, asking a client to connect to a more suitable server
        reconnect_request = 4,

        // list of subscribed chunks, sent between servers
        subscription_update = 8,

        // sent by player, contains input of local player
        player_update = 16,
    };

    struct {
        glm::vec3 position;
        glm::vec3 color;
        glm::quat orientation;
        std::vector<std::int16_t> samples;
    } player_update;

    struct {
        struct {
            unsigned size;
            struct{
                std::vector<unsigned> indices;
                std::vector<glm::vec3> positions;
                std::vector<glm::vec3> colors;
            } low;
            struct {
                std::vector<unsigned> indices;
                struct {
                    std::vector<unsigned> size;
                    std::vector<glm::vec4> positions;
                    std::vector<glm::quat> orientations;
                } bones;
            } high;
            struct {
                std::vector<unsigned> indices;
                // fixed number of samples per player
                // no date for a player if all samples are zero 
                std::vector<std::int16_t> samples;
            } audio;
        } players;

        struct {
            // block of aggregate information
            std::vector<glm::ivec4> position;
            std::vector<unsigned> density;
            std::vector<glm::vec3> color_average;
            std::vector<glm::vec3> color_variance;
        } nodes;

    } world_update;

    struct {
        std::vector<glm::ivec4> added;
        std::vector<glm::ivec4> removed;
    } subscription_update;
};