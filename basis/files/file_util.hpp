#pragma once

#include <string>

#include <base/files/file_path.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/location.h>
#include <base/rvalue_cast.h>

namespace basis {

// Returns the provided MIME type without the subtype component.
//
// EXAMPLE
//
// "video/mp4" -> "video"
// "video/" -> ""
// "/abc/xyz" -> ""
std::string StripMimeSubType(const std::string& mime_type);

} // namespace basis
