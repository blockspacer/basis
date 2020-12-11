#include "basis/files/file_util.hpp" // IWYU pragma: associated

namespace basis {

std::string StripMimeSubType(const std::string& mime_type)
{
  if (mime_type.empty()) {
    return mime_type;
  }

  size_t index = mime_type.find_first_of('/', 0);

  if (index == 0 || index == mime_type.size() - 1 ||
      index == std::string::npos)
  {
    // This looks malformed, return an empty string.
    return std::string();
  }

  return mime_type.substr(0, index);
}

} // namespace basis
