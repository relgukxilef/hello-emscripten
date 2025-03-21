#pragma once

#include <vector>
#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/utility.hpp>
#include <boost/beast/ssl.hpp>

#include "websocket.h"

struct boost_insecure_websocket;

// TODO: move to utility file
struct callable {
    virtual ~callable() = default;
    virtual void operator()(std::error_code e) = 0;
};

auto make_callable(auto f) {
    struct f_type : public callable {
        ~f_type() override = default;
        f_type(decltype(f) f_object) : f_object(std::move(f_object)) {}
        void operator()(std::error_code e) override {
            std::move(f_object)(e);
        }
        decltype(f) f_object;
    };
    return std::make_unique<f_type>(std::move(f));
}

struct fence {
    ~fence() {
        for (auto &t : wait_list)
            (*t)(std::make_error_code(std::errc::operation_canceled));
    }

    template <typename CompletionToken> 
    auto async_wait(CompletionToken&& token) {
        return async_initiate<
            CompletionToken,
            void(std::error_code)
        >(
            [](auto completion_handler, fence *s) {
                s->wait_list.push_back(
                    make_callable(std::move(completion_handler))
                );
            }, 
            token, 
            this
        );
    }

    void signal() {
        auto list = std::move(wait_list);
        for (auto &t : list)
            (*t)({});
    }

    std::vector<std::unique_ptr<callable>> wait_list;
};

struct boost_websockets {
    boost_websockets();
    ~boost_websockets();
    void update();
    websockets get_websockets();

    websockets websockets;
    boost::asio::io_context context;
    std::shared_ptr<boost::asio::ip::tcp::resolver> resolver;
    std::vector<std::shared_ptr<boost_insecure_websocket>> sockets;
    fence update_fence;
};
