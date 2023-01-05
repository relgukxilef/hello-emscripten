#pragma once

#include <memory>

#include "../state/client.h"

// called by network_backend
void put_message(::client& client, const char* buffer, size_t size);
void set_disconnected(::client& client);
