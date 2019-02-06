//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2019
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/telegram/files/FileData.h"

#include "td/telegram/DialogId.h"
#include "td/telegram/FileReferenceManager.h"
#include "td/telegram/FileReferenceManager.hpp"
#include "td/telegram/files/FileEncryptionKey.h"
#include "td/telegram/files/FileLocation.h"
#include "td/telegram/files/FileSourceId.h"
#include "td/telegram/Global.h"
#include "td/telegram/Td.h"

#include "td/utils/common.h"
#include "td/utils/misc.h"
#include "td/utils/tl_helpers.h"

namespace td {

template <class StorerT>
void FileData::store(StorerT &storer) const {
  using ::td::store;
  bool has_owner_dialog_id = owner_dialog_id_.is_valid();
  bool has_expected_size = size_ == 0 && expected_size_ != 0;
  bool encryption_key_is_secure = encryption_key_.is_secure();
  bool has_sources = !file_source_ids_.empty();
  BEGIN_STORE_FLAGS();
  STORE_FLAG(has_owner_dialog_id);
  STORE_FLAG(has_expected_size);
  STORE_FLAG(encryption_key_is_secure);
  STORE_FLAG(has_sources);
  END_STORE_FLAGS();

  if (has_owner_dialog_id) {
    store(owner_dialog_id_, storer);
  }
  store(pmc_id_, storer);
  store(remote_, storer);
  store(local_, storer);
  auto generate = generate_ == nullptr ? GenerateFileLocation() : GenerateFileLocation(*generate_);
  store(generate, storer);
  if (has_expected_size) {
    store(expected_size_, storer);
  } else {
    store(size_, storer);
  }
  store(remote_name_, storer);
  store(url_, storer);
  store(encryption_key_, storer);
  if (has_sources) {
    auto td = G()->td().get_actor_unsafe();
    store(narrow_cast<int32>(file_source_ids_.size()), storer);
    for (auto file_source_id : file_source_ids_) {
      td->file_reference_manager_->store_file_source(file_source_id, storer);
    }
  }
}
template <class ParserT>
void FileData::parse(ParserT &parser) {
  using ::td::parse;
  bool has_owner_dialog_id;
  bool has_expected_size;
  bool encryption_key_is_secure;
  bool has_sources;
  BEGIN_PARSE_FLAGS();
  PARSE_FLAG(has_owner_dialog_id);
  PARSE_FLAG(has_expected_size);
  PARSE_FLAG(encryption_key_is_secure);
  PARSE_FLAG(has_sources);
  END_PARSE_FLAGS_GENERIC();

  if (has_owner_dialog_id) {
    parse(owner_dialog_id_, parser);
  }
  parse(pmc_id_, parser);
  parse(remote_, parser);
  parse(local_, parser);
  GenerateFileLocation generate;
  parse(generate, parser);
  if (generate.type() == GenerateFileLocation::Type::Full) {
    generate_ = make_unique<FullGenerateFileLocation>(generate.full());
  } else {
    generate_ = nullptr;
  }
  if (has_expected_size) {
    parse(expected_size_, parser);
  } else {
    parse(size_, parser);
  }
  parse(remote_name_, parser);
  parse(url_, parser);
  encryption_key_.parse(encryption_key_is_secure ? FileEncryptionKey::Type::Secure : FileEncryptionKey::Type::Secret,
                        parser);
  if (has_sources) {
    auto td = G()->td().get_actor_unsafe();
    int32 size;
    parse(size, parser);
    if (0 < size && size < 5) {
      for (int i = 0; i < size; i++) {
        file_source_ids_.push_back(td->file_reference_manager_->parse_file_source(td, parser));
      }
    } else {
      parser.set_error("Wrong number of file source ids");
    }
  }
}

}  // namespace td
