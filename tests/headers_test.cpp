#include <gtest/gtest.h>
#include "headers.h"

using namespace std::string_view_literals;

TEST(iterHeaders, Empty) {
  bool called = false;
  iterHeaders(""sv, [&](auto, auto){ called = true; });
  EXPECT_FALSE(called);
}

TEST(iterHeaders, SkipRequestLine) {
  std::vector<std::pair<std::string_view, std::string_view>> headers;

  constexpr auto req =
    "GET / HTTP/1.1\r\n"
    "\r\n"sv;

  iterHeaders(req, [&](auto name, auto value){ headers.emplace_back(name, value); });

  EXPECT_TRUE(headers.empty());
}

TEST(iterHeaders, SingleHeader) {
  std::vector<std::pair<std::string_view, std::string_view>> headers;

  constexpr auto req =
    "GET / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "\r\n"sv;

  iterHeaders(req, [&](auto name, auto value){ headers.emplace_back(name, value); });

  ASSERT_EQ(headers.size(), 1);
  EXPECT_EQ(headers[0].first, "Host");
  EXPECT_EQ(headers[0].second, "example.com");
}

TEST(iterHeaders, MultipleHeaders) {
  std::vector<std::pair<std::string_view, std::string_view>> headers;

  constexpr auto req =
    "GET / HTTP/1.1\r\n"
    "Header-1: value-1\r\n"
    "Header-2: value-2\r\n"
    "\r\n"sv;

  iterHeaders(req, [&](auto name, auto value){ headers.emplace_back(name, value); });

  ASSERT_EQ(headers.size(), 2);
  EXPECT_EQ(headers[0], std::make_pair("Header-1"sv, "value-1"sv));
  EXPECT_EQ(headers[1], std::make_pair("Header-2"sv, "value-2"sv));
}

TEST(iterHeaders, MultipleSameHeaders) {
  std::vector<std::pair<std::string_view, std::string_view>> headers;

  constexpr auto req =
    "GET / HTTP/1.1\r\n"
    "Header: value-1\r\n"
    "Header: value-2\r\n"
    "\r\n"sv;

  iterHeaders(req, [&](auto name, auto value){ headers.emplace_back(name, value); });
  
  ASSERT_EQ(headers.size(), 2);
  EXPECT_EQ(headers[0], std::make_pair("Header"sv, "value-1"sv));
  EXPECT_EQ(headers[1], std::make_pair("Header"sv, "value-2"sv));
}

TEST(findHostPort, Simple) {
  constexpr auto req_default =
    "GET / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "\r\n"sv;

    auto [host_default, port_default] = findHostPort(req_default);
    EXPECT_EQ(host_default, "example.com");
    EXPECT_EQ(port_default, "80");

    constexpr auto req_8080 =
      "GET / HTTP/1.1\r\n"
      "Host: example8080.com:8080\r\n"
      "\r\n"sv;

    auto [host_8080, port_8080] = findHostPort(req_8080);
    EXPECT_EQ(host_8080, "example8080.com");
    EXPECT_EQ(port_8080, "8080");

}

TEST(findHostPort, NoHost) {
  constexpr auto req =
    "GET / HTTP/1.1\r\n"
    "NoHostHeader: example.com\r\n"
    "Not-a-Host: example.com\r\n"
    "HostNot: example.com\r\n"
    "\r\n"sv;

    auto [host, port] = findHostPort(req);
    EXPECT_EQ(host, "");
    EXPECT_EQ(port, "80");
}

TEST(findContentLength, Simple) {
  constexpr auto req =
    "GET / HTTP/1.1\r\n"
    "Content-Length: 42\r\n"
    "\r\n"sv;

    auto len = findContentLength(req);
    ASSERT_TRUE(len.has_value());
    EXPECT_EQ(len.value(), 42);
}

TEST(findContentLength, NoContentLength) {
  constexpr auto req =
    "GET / HTTP/1.1\r\n"
    "No-Content-Length: 42\r\n"
    "\r\n"sv;

    auto len = findContentLength(req);
    EXPECT_FALSE(len.has_value());
}
