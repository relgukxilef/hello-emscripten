#pragma once

struct message {
    enum class type {
        // sent by client
        login = 1, 

        // message contains a (partial) (delta) update of instance data
        // sent either by server or client, also used between servers
        update = 2, 

        // sent by server, asking a client to connect to a more suitable server
        reconnect_request = 4,

        // list of subscribed chunks, sent between servers
        subscription_update = 8,
    };
};