#include <catch2/catch_test_macros.hpp>
#include <svarog/io/io_context.hpp>
#include <svarog/network/basic_socket.hpp>
#include <svarog/network/ip/tcp.hpp>
#include <svarog/network/ip/udp.hpp>

using namespace svarog;
using namespace svarog::network;

TEST_CASE("basic_socket open/close", "[socket]") {
    io::io_context ctx;

    SECTION("TCP v4") {
        network::basic_socket<ip::tcp> sock(ctx);
        REQUIRE_FALSE(sock.is_open());

        sock.open(ip::tcp::v4());
        REQUIRE(sock.is_open());
        REQUIRE(sock.native_handle() != network::detail::invalid_socket);

        sock.close();
        REQUIRE_FALSE(sock.is_open());
    }

    SECTION("UDP v6") {
        network::basic_socket<ip::udp> sock(ctx, ip::udp::v6());
        REQUIRE(sock.is_open());
    }
}

TEST_CASE("socket options", "[socket]") {
    io::io_context ctx;
    network::basic_socket<ip::tcp> sock(ctx, ip::tcp::v4());

    SECTION("reuse_address") {
        sock.set_option(network::socket_base::reuse_address{true});

        network::socket_base::reuse_address opt;
        sock.get_option(opt);
        REQUIRE(opt.value() == true);
    }

    SECTION("receive_buffer_size") {
        sock.set_option(network::socket_base::receive_buffer_size{65536});

        network::socket_base::receive_buffer_size opt;
        sock.get_option(opt);
        REQUIRE(opt.value() >= 65536);  // OS may adjust
    }
}

TEST_CASE("socket bind", "[socket]") {
    io::io_context ctx;
    network::basic_socket<ip::tcp> sock(ctx, ip::tcp::v4());

    sock.set_option(network::socket_base::reuse_address{true});
    sock.bind(ip::tcp::endpoint{ip::address_v4::loopback(), 0});  // Any port

    auto local_ep = sock.local_endpoint();
    REQUIRE(local_ep.get_address() == ip::address{ip::address_v4::loopback()});
    REQUIRE(local_ep.get_port() != 0);  // OS assigned port
}
