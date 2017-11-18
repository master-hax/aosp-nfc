/******************************************************************************
 *
 *  Copyright (C) 2017 Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include "nfc_config.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include <config.h>

const char* transport_config_paths[] = {"/odm/etc/", "/vendor/etc/", "/etc/"};
const int transport_config_path_size =
    (sizeof(transport_config_paths) / sizeof(transport_config_paths[0]));

using namespace ::std;
using namespace ::android::base;

namespace {

std::string findConfigPath() {
  const vector<string> search_path = {"/odm/etc/", "/vendor/etc/", "/etc/"};
  const string file_name = "libnfc-nci.conf";

  for (string path : search_path) {
    path.append(file_name);
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) != 0) continue;
    if (S_ISREG(file_stat.st_mode)) return path;
  }
  return "";
}

}  // namespace

NfcConfig::NfcConfig() {
  string config_path = findConfigPath();
  CHECK(config_path != "");
  config_.parseFromFile(config_path);
}

NfcConfig& NfcConfig::getInstance() {
  static NfcConfig theInstance;
  return theInstance;
}

bool NfcConfig::hasValue(const std::string& key) {
  return getInstance().config_.hasValue(key);
}

std::string NfcConfig::getStringValue(const std::string& key) {
  return getInstance().config_.getStringValue(key);
}

unsigned NfcConfig::getUnsignedValue(const std::string& key) {
  return getInstance().config_.getUnsignedValue(key);
}

std::vector<uint8_t> NfcConfig::getBytesValue(const std::string& key) {
  return getInstance().config_.getBytesValue(key);
}
