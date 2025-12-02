#include <catch2/catch_test_macros.hpp>
#include <svarog/network/ip/address.hpp>

using namespace svarog::network::ip;

TEST_CASE("address_v4 construction", "[ip][address_v4]") {
    SECTION("default is 0.0.0.0") {
        address_v4 addr;
        REQUIRE(addr.is_unspecified());
        REQUIRE(addr.to_string() == "0.0.0.0");
    }

    SECTION("from uint") {
        address_v4 addr{0x7F000001};  // 127.0.0.1
        REQUIRE(addr.is_loopback());
        REQUIRE(addr.to_string() == "127.0.0.1");
    }

    SECTION("from bytes") {
        address_v4 addr{
            address_v4::bytes_type{192, 168, 1, 1}
        };
        REQUIRE(addr.to_string() == "192.168.1.1");
        REQUIRE(addr.is_private());
    }
}

TEST_CASE("address_v4 parsing", "[ip][address_v4]") {
    SECTION("valid addresses") {
        REQUIRE(address_v4::from_string("0.0.0.0").has_value());
        REQUIRE(address_v4::from_string("127.0.0.1").has_value());
        REQUIRE(address_v4::from_string("255.255.255.255").has_value());
    }

    SECTION("invalid addresses") {
        REQUIRE_FALSE(address_v4::from_string("256.0.0.0").has_value());
        REQUIRE_FALSE(address_v4::from_string("1.2.3").has_value());
        REQUIRE_FALSE(address_v4::from_string("::1").has_value());
        REQUIRE_FALSE(address_v4::from_string("invalid").has_value());
    }
}

TEST_CASE("address_v4 classification", "[ip][address_v4]") {
    REQUIRE(address_v4::any().is_unspecified());
    REQUIRE(address_v4::loopback().is_loopback());
    REQUIRE(address_v4::broadcast().to_uint() == 0xFFFFFFFF);

    auto multicast = address_v4::from_string("224.0.0.1");
    REQUIRE(multicast->is_multicast());

    auto link_local = address_v4::from_string("169.254.1.1");
    REQUIRE(link_local->is_link_local());
}

TEST_CASE("address_v6 construction", "[ip][address_v6]") {
    SECTION("default is ::") {
        address_v6 addr;
        REQUIRE(addr.is_unspecified());
        REQUIRE(addr.to_string() == "::");
    }

    SECTION("loopback") {
        auto addr = address_v6::loopback();
        REQUIRE(addr.is_loopback());
        REQUIRE(addr.to_string() == "::1");
    }
}

TEST_CASE("address_v6 parsing", "[ip][address_v6]") {
    SECTION("valid addresses") {
        REQUIRE(address_v6::from_string("::").has_value());
        REQUIRE(address_v6::from_string("::1").has_value());
        REQUIRE(address_v6::from_string("fe80::1").has_value());
        REQUIRE(address_v6::from_string("2001:db8::1").has_value());
    }

    SECTION("with scope id") {
        auto addr = address_v6::from_string("fe80::1%5");
        REQUIRE(addr.has_value());
        REQUIRE(addr->scope_id() == 5);
    }

    SECTION("v4-mapped") {
        auto addr = address_v6::from_string("::ffff:192.168.1.1");
        REQUIRE(addr.has_value());
        REQUIRE(addr->is_v4_mapped());
        REQUIRE(addr->to_v4().to_string() == "192.168.1.1");
    }
}

TEST_CASE("unified address", "[ip][address]") {
    SECTION("from_string auto-detect") {
        auto v4 = address::from_string("192.168.1.1");
        REQUIRE(v4.has_value());
        REQUIRE(v4->is_v4());

        auto v6 = address::from_string("::1");
        REQUIRE(v6.has_value());
        REQUIRE(v6->is_v6());
    }

    SECTION("make_address throwing") {
        REQUIRE_NOTHROW(make_address("192.168.1.1"));
        REQUIRE_THROWS_AS(make_address("invalid"), std::invalid_argument);
    }

    SECTION("comparison") {
        address a1{address_v4::from_string("192.168.1.1").value()};
        address a2{address_v4::from_string("192.168.1.2").value()};
        address a3{address_v6::loopback()};

        REQUIRE(a1 < a2);  // Same type, compare values
        REQUIRE(a1 < a3);  // v4 < v6
    }
}

TEST_CASE("address formatting", "[ip][address]") {
    auto v4 = address_v4::from_string("10.0.0.1").value();
    REQUIRE(std::format("{}", v4) == "10.0.0.1");

    auto v6 = address_v6::loopback();
    REQUIRE(std::format("{}", v6) == "::1");

    address addr{v4};
    REQUIRE(std::format("{}", addr) == "10.0.0.1");
}
