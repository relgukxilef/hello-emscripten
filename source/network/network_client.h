#pragma once

#include <memory>

#include "../state/client.h"

// called by network_backend
// TODO: remove these in favor of a polling API
// the main thread can only use polling anyway
void put_message(::client& client, const char* buffer, size_t size);
void set_disconnected(::client& client);
