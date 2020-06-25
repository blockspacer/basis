#pragma once

#include <basis/boost_command_line.hpp>

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

// converts command-line argument to ABSOLUTE path
/// \note returns empty base::FilePath{} if path
/// is NOT valid directory
[[nodiscard]] /* do not ignore return value */
base::FilePath cmdKeyToDirectory(
  const char key[],
  cmd::BoostCmdParser& boostCmdParser);

// converts command-line argument to ABSOLUTE path
/// \note returns empty base::FilePath{} if path
/// is NOT valid file
[[nodiscard]] /* do not ignore return value */
base::FilePath cmdKeyToFile(
  const char key[],
  cmd::BoostCmdParser& boostCmdParser);

/// \note returns std::numeric_limits<int>::max() if
/// command-line argument is NOT specified or NOT convertable
/// to int
[[nodiscard]] /* do not ignore return value */
int cmdKeyToInt(
  const base::StringPiece& key,
  cmd::BoostCmdParser& boostCmdParser);

// converts command-line argument to path
// path may be NOT absolute
/// \note returns empty base::FilePath{}
/// if command-line argument
/// is NOT specified or NOT valid
[[nodiscard]] /* do not ignore return value */
base::FilePath getAsPath(
  const base::StringPiece& key,
  cmd::BoostCmdParser& boostCmdParser);

// calls |base::MakeAbsoluteFilePath| for each string in vector
/// \note On POSIX, |MakeAbsoluteFilePath| fails
/// if the path does not exist
std::vector<base::FilePath>
toFilePaths(const std::vector<std::string>& paths);

}  // namespace basis
