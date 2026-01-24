#include "headers.h"

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <string_view>
#include <iostream>

using boost::asio::io_service;
using boost::asio::co_spawn;
using boost::asio::async_read_until;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::system::error_code;
using boost::asio::buffer;
using boost::asio::dynamic_buffer;
using boost::asio::transfer_at_least;
using boost::asio::ip::tcp;


constexpr std::string_view delimiter = "\r\n\r\n";


awaitable<void> session(tcp::socket client_socket, io_service& io_service)
{
  try {
    std::string client_buffer;
    co_await async_read_until(client_socket, dynamic_buffer(client_buffer), delimiter, use_awaitable);
    
    auto [host, port] = findHostPort(client_buffer);
    if(host.empty()) {
      co_return;
    }
    
    tcp::resolver resolver(io_service);
    auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);
    
    tcp::socket server_socket(io_service);
    co_await boost::asio::async_connect(server_socket, endpoints, use_awaitable);
    
    co_await boost::asio::async_write(server_socket, buffer(client_buffer), use_awaitable);
    
    std::string server_buffer;
    co_await boost::asio::async_read_until(server_socket, dynamic_buffer(server_buffer), delimiter, use_awaitable);
    
    co_await boost::asio::async_write(client_socket, buffer(server_buffer), use_awaitable);
    
    auto content_length = findContentLength(server_buffer);
    if(!content_length.has_value()) {
      co_return;
    }
    
    auto header_end = server_buffer.find(delimiter);
    auto body_start = header_end + delimiter.size();
    
    size_t already_received = server_buffer.size() - body_start;
    
    size_t remaining = (*content_length > already_received) ? (*content_length - already_received) : 0;
    
    std::array<char, 8192> buf;
    
    while(remaining > 0) {
      size_t to_read = std::min(buf.size(), remaining);
      size_t n = co_await server_socket.async_read_some(buffer(buf.data(), to_read), use_awaitable);
      remaining -= n;
      co_await boost::asio::async_write(client_socket, buffer(buf.data(), n), use_awaitable);
    }

  } catch (...) {
    co_return;
  }
}

class Server
{
public:
  Server(io_service& io_service, short port)
    : io_service_(io_service)
    , acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    auto client_socket = std::make_shared<tcp::socket>(io_service_);
    acceptor_.async_accept(*client_socket, [this, client_socket](error_code ec){
        if(!ec) {
          co_spawn(io_service_, session(std::move(*client_socket), io_service_), boost::asio::detached);
        }
        do_accept();
    });
  }

  io_service& io_service_;
  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: proxy_server";
      std::cerr << " <listen_port>\n";
      return 1;
    }
    io_service io_service(1);
    Server server(io_service, std::atoi(argv[1]));
    io_service.run();

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}
