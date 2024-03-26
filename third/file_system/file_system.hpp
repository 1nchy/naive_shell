#ifndef _ASP_FILE_HANDLER_HPP_
#define _ASP_FILE_HANDLER_HPP_

#include <filesystem>
#include <functional>

namespace asp {
namespace filesystem {
typedef std::function<void(const std::filesystem::path&)> file_handler;
void for_each_file(const std::filesystem::path&, file_handler);
}
}

#endif // _ASP_FILE_HANDLER_HPP_