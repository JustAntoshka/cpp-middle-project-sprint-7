#include "headers.h"

#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

using Callback = std::function<void(std::string_view, std::string_view)>;


void iterHeaders(std::string_view req, Callback&& callback) {
  size_t pos = 0;
  while (true) {
    auto end = req.find("\r\n", pos);
    if (end == std::string_view::npos || end == pos) {
      break;
    }
    
    std::string_view line = req.substr(pos, end - pos);
    pos = end + 2;

    if(line.ends_with("HTTP/1.1"sv)) {
      continue;
    }
    
    auto colon = line.find(':');
    if (colon == std::string_view::npos) {
      continue;
    }
    
    std::string_view name = line.substr(0, colon);
    std::string_view value = line.substr(colon + 1, end - colon - 1);

    while (!value.empty() && value.front() == ' ') {
      value.remove_prefix(1);
    }

    callback(name, value);
  }
}

std::pair<std::string, std::string> findHostPort(std::string_view req) {
  std::pair<std::string, std::string> result;
  std::string_view header_value;

  iterHeaders(req, [&header_value](std::string_view name, std::string_view value) {
    if(name == "Host") {
      header_value = value;
    }
  });

  auto colon = header_value.find(':');
  if (colon == std::string_view::npos) {
    result.first = std::string(header_value);
    result.second = "80";
  } else {
    result.first = std::string(header_value.substr(0, colon));
    result.second = std::string(header_value.substr(colon + 1, header_value.size() - colon - 1));
  }
  
  return result;
}

std::optional<size_t> findContentLength(std::string_view rsp) {
  std::optional<size_t> result;
  iterHeaders(rsp, [&result](std::string_view name, std::string_view value){
    if (name == "Content-Length") {
      try {
        result = std::stoul(std::string(value));
      } catch (...) {
        // некорректное значение — игнорируем
      }
    }
  });
  return result;
}
