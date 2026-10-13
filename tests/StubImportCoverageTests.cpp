// Boot-resolves every LIB_FUNC registration against the real game imports recorded
// in capture logs, so a regression that sends a previously reported import back to
// an unresolved stub fails the suite instead of shipping silently.
//
// Logs come from runtimeLinker.cpp, which emits either
//   "Relocate: unresolved PLT import patched to stub [i] [a] <- b, NID[Lib_vN][Mod_vN.M][Func]"
// or
//   "Stubbed: [a] <- 0, NID[Lib_vN][Mod_vN.M][Func]"
// for every import that found no registration at link time.
//
// The set of known unresolved imports is checked in alongside as
// tests/stubImportBaseline.txt ("<library> <nid>" pairs). The test fails only for
// unresolved pairs that are NOT in the baseline (a new gap) and reports baseline
// entries the current code already resolves so the baseline can be pruned. With no
// capture logs present the check degrades to a no-op pass, keeping CI without
// capture data green.

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "libs/libs.h"
#include "loader/symbolDatabase.h"

#include <fmt/format.h>
#include <magic_enum.hpp>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#ifndef KYTY_SOURCE_ROOT
#define KYTY_SOURCE_ROOT "."
#endif

namespace {

// "NID[Library_vN][Module_vN.M][Func]" -> ("<library>", NID, SymbolType).
// The _vN suffix is storage for the version the game linked against; the baseline
// keys on the bare library name, matching what LIB_NAME registers.
struct StubImport {
	std::string        library;
	std::string        nid;
	Loader::SymbolType type = Loader::SymbolType::Unknown;
};

std::string Trim(const std::string& s) {
	size_t begin = 0;
	size_t end   = s.size();
	while (begin < end && std::isspace(static_cast<unsigned char>(s[begin])) != 0) {
		begin++;
	}
	while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
		end--;
	}
	return s.substr(begin, end - begin);
}

std::string StripVersionSuffix(const std::string& library) {
	// runtimeLinker prints registry names like "WebBrowserDialog_v1"; the registered
	// library-side name is just "WebBrowserDialog".
	auto without_version = library.substr(0, library.rfind("_v"));
	return without_version.empty() ? library : without_version;
}

std::set<std::string> LoadBaseline(const std::string& path) {
	std::set<std::string> baseline;
	std::ifstream         file(path);
	if (!file.is_open()) {
		return baseline;
	}
	std::string line;
	while (std::getline(file, line)) {
		line = Trim(line);
		if (line.empty() || line.starts_with('#')) {
			continue;
		}
		baseline.insert(line);
	}
	return baseline;
}

std::map<std::string, StubImport> CollectStubImports(const std::string& log_dir,
                                                     std::size_t*       files_parsed) {
// Capture the 11-char NID token right before "[Lib_vN]...". No \\b anchor: NIDs
// may start with '+'/'-' (e.g. "+u6dKSLWM2o"), which are non-word chars, so a word
// boundary never fires for them and those imports would be silently skipped.
const std::regex token("(?:^|[^A-Za-z0-9+/_=-])([A-Za-z0-9+/_=-]{11})\\[([^\\]]+)_v([0-9]+)\\]"
                       "\\[([^\\]]+)_v([0-9]+)\\.([0-9]+)\\]\\[(Func|Object|TlsModule|NoType)\\]");

	std::map<std::string, StubImport> imports;
	for (const auto& entry: std::filesystem::directory_iterator(log_dir)) {
		if (!entry.is_regular_file() || entry.path().extension() != ".log") {
			continue;
		}
		std::ifstream in(entry.path());
		if (!in.is_open()) {
			continue;
		}
		(*files_parsed)++;
		std::string line;
		while (std::getline(in, line)) {
			const bool stubbed =
			    line.find("unresolved PLT import patched to stub") != std::string::npos ||
			    line.starts_with("Stubbed:");
			if (!stubbed) {
				continue;
			}
			std::smatch match;
			if (!std::regex_search(line, match, token)) {
				continue;
			}const auto type = magic_enum::enum_cast<Loader::SymbolType>(match[7].str()).value_or(
				    Loader::SymbolType::Unknown);
			if (type != Loader::SymbolType::Func) {
				continue;
			}
			StubImport import {
			    .library = StripVersionSuffix(match[2].str()),
			    .nid     = match[1].str(),
			    .type    = type,
			};
			imports.insert_or_assign(import.library + " " + import.nid, std::move(import));
		}
	}
	return imports;
}

int RunCoverage() {
	Loader::SymbolDatabase symbols;
	Libs::InitAll(&symbols);

	const char*                 log_dir_env = std::getenv("KYTY_IMPORT_LOG_DIR");
	const std::filesystem::path log_dir =
	    log_dir_env != nullptr ? std::filesystem::path(log_dir_env)
	                           : std::filesystem::path(KYTY_SOURCE_ROOT) / "_tmp_runs";
	if (!std::filesystem::exists(log_dir)) {
		fmt::println("stub_import_coverage: log dir '{}' not found, nothing to check",
		             log_dir.string());
		return 0;
	}

	std::size_t files_parsed = 0;
	const auto  stub_imports = CollectStubImports(log_dir.string(), &files_parsed);
	if (files_parsed == 0 || stub_imports.empty()) {
		fmt::println("stub_import_coverage: no stub imports in '{}' ({} files parsed)",
		             log_dir.string(), files_parsed);
		return 0;
	}

	const auto baseline = LoadBaseline((std::filesystem::path(KYTY_SOURCE_ROOT) / "tests" /
	                                    "stubImportBaseline.txt")
	                                       .string());

	std::vector<std::string> new_gaps;
	std::vector<std::string> resolved_by_baseline;
	for (const auto& [key, import]: stub_imports) {
		// A registration under a different library/version does not satisfy the guest's
		// exact GenerateName lookup, so only the same library counts as coverage here.
		const bool resolved =
		    symbols.FindByNidLib(import.nid, import.library, import.type) != nullptr;
		if (resolved) {
			if (baseline.contains(key)) {
				// The logs predate an implementation; keep coverage tight by pruning.
				resolved_by_baseline.push_back(key);
			}
		} else if (!baseline.contains(key)) {
			new_gaps.push_back(key);
		}
	}

	for (const auto& key: resolved_by_baseline) {
		fmt::println("stub_import_coverage: baseline entry now resolves, prune me: {}", key);
	}
	for (const auto& key: new_gaps) {
		fmt::println("stub_import_coverage: NEW unresolved stub import (not in baseline): {}",
		             key);
	}
	fmt::println("stub_import_coverage: {} files, {} unresolved imports, {} baseline entries, "
	             "{} new gaps",
	             files_parsed, stub_imports.size(), baseline.size(), new_gaps.size());
	return new_gaps.empty() ? 0 : 1;
}

} // namespace

int main(int, char**) {
	return RunCoverage();
}
