// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

#include "base/io_buf.h"
#include "facade/facade_types.h"
#include "io/io.h"
#include "server/common.h"
#include "server/journal/types.h"

namespace dfly {

// JournalWriter serializes journal entries to a sink.
// It automatically keeps track of the current database index.
class JournalWriter {
 public:
  JournalWriter(io::Sink* sink);

  // Write single entry to sink.
  void Write(const journal::Entry& entry);

 private:
  void Write(uint64_t v);           // Write packed unsigned integer.
  void Write(std::string_view sv);  // Write string.

  template <typename C>  // CmdArgList or ArgSlice.
  void Write(std::pair<std::string_view, C> args);

  void Write(std::monostate);  // Overload for empty std::variant

 private:
  io::Sink* sink_;
  std::optional<DbIndex> cur_dbid_{};
};

// JournalReader allows deserializing journal entries from a source.
// Like the writer, it automatically keeps track of the database index.
struct JournalReader {
 public:
  // Initialize start database index.
  JournalReader(io::Source* source, DbIndex dbid);

  // Overwrite current source and ensure there is no leftover from previous.
  void SetSource(io::Source* source);

  // Try reading entry from source.
  io::Result<journal::ParsedEntry> ReadEntry();

 private:
  // Read from source until buffer contains at least num bytes.
  std::error_code EnsureRead(size_t num);

  // Read unsigned integer in packed encoding.
  template <typename UT> io::Result<UT> ReadUInt();

  // Read and copy to buffer, return size.
  io::Result<size_t> ReadString(MutableSlice buffer);

  // Read argument array into string buffer.
  std::error_code ReadCommand(journal::ParsedEntry::CmdData* entry);

 private:
  io::Source* source_;
  base::IoBuf buf_;
  DbIndex dbid_;
};

}  // namespace dfly
