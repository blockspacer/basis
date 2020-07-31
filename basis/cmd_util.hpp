#pragma once

#include <base/files/file_path.h>
#include <base/strings/string_piece_forward.h>

#include <vector>
#include <string>

namespace basis {

extern
const char DEFAULT_EVENT_CATEGORIES[];

// calls |base::CommandLine::Init|
// and sets default command-line switches
void initCommandLine(int argc, char* argv[]);

}  // namespace basis
