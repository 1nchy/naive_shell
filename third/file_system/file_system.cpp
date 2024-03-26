#include "file_system.hpp"

namespace asp {
namespace filesystem {
void for_each_file(const std::filesystem::path& _path, file_handler _handler) {
    if (!std::filesystem::exists(_path)) return;
    std::filesystem::directory_entry _entry(_path);
    if (_entry.status().type() != std::filesystem::file_type::directory) {
        return;
    }
    std::filesystem::directory_iterator _list(_path);
    for (auto& _i : _list) {
        _handler(_i.path());
    }
}
}
}