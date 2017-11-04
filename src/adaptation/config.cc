/******************************************************************************
 *
 *  Copyright (C) 2011-2012 Broadcom Corporation
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
#include <stdio.h>
#include <sys/stat.h>
#include <map>
#include <string>
#include <vector>
#include "_OverrideLog.h"

const char* transport_config_paths[] = {"/vendor/etc/", "/odm/etc/", "/etc/"};
const int transport_config_path_size =
    (sizeof(transport_config_paths) / sizeof(transport_config_paths[0]));

#define config_name "libnfc-nci.conf"
#define IsStringValue 0x80000000

using namespace ::std;

class CNfcConfig {
 public:
  virtual ~CNfcConfig();
  static CNfcConfig& GetInstance();
  bool find(const char* name, vector<uint8_t>& vecValue);
  void clean();

 private:
  CNfcConfig();
  bool readConfig(const char* name);

  std::map<string, vector<uint8_t>> mParamMap;
  bool mValidFile;
  unsigned long state;

  inline bool Is(unsigned long f) { return (state & f) == f; }
  inline void Set(unsigned long f) { state |= f; }
};

/*******************************************************************************
**
** Function:    isPrintable()
**
** Description: detremine if a char is printable
**
** Returns:     none
**
*******************************************************************************/
inline bool isPrintable(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '/' || c == '_' || c == '-' || c == '.';
}

/*******************************************************************************
**
** Function:    isDigit()
**
** Description: detremine if a char is numeral digit
**
** Returns:     none
**
*******************************************************************************/
inline bool isDigit(char c, int base) {
  if ('0' <= c && c <= '9') return true;
  if (base == 16) {
    if (('A' <= c && c <= 'F') || ('a' <= c && c <= 'f')) return true;
  }
  return false;
}

/*******************************************************************************
**
** Function:    getDigitValue()
**
** Description: return numercal value of a char
**
** Returns:     none
**
*******************************************************************************/
inline int getDigitValue(char c, int base) {
  if ('0' <= c && c <= '9') return c - '0';
  if (base == 16) {
    if ('A' <= c && c <= 'F')
      return c - 'A' + 10;
    else if ('a' <= c && c <= 'f')
      return c - 'a' + 10;
  }
  return 0;
}

/*******************************************************************************
**
** Function:    findConfigFilePathFromTransportConfigPaths()
**
** Description: find a config file path with a given config name from transport
**              config paths
**
** Returns:     none
**
*******************************************************************************/
void findConfigFilePathFromTransportConfigPaths(const string& configName,
                                                string& filePath) {
  for (int i = 0; i < transport_config_path_size - 1; i++) {
    filePath.assign(transport_config_paths[i]);
    filePath += configName;
    struct stat file_stat;
    if (stat(filePath.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
      return;
    }
  }
  filePath.assign(transport_config_paths[transport_config_path_size - 1]);
  filePath += configName;
}

/*******************************************************************************
**
** Function:    CNfcConfig::readConfig()
**
** Description: read Config settings and parse them into a linked list
**              move the element from linked list to a array at the end
**
** Returns:     none
**
*******************************************************************************/
bool CNfcConfig::readConfig(const char* name) {
  enum {
    BEGIN_LINE = 1,
    TOKEN,
    STR_VALUE,
    NUM_VALUE,
    BEGIN_HEX,
    BEGIN_QUOTE,
    END_LINE
  };

  FILE* fd = NULL;
  string token;
  vector<uint8_t> strValue;
  unsigned long numValue = 0;
  int i = 0;
  int base = 0;
  char c = 0;

  state = BEGIN_LINE;
  if ((fd = fopen(name, "rb")) == NULL) {
    DLOG_IF(INFO, nfc_debug_enabled)
        << StringPrintf("%s Cannot open config file %s", __func__, name);
    mValidFile = false;
    return false;
  }
  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("%s Opened config %s", __func__, name);

  mValidFile = true;
  clean();

  for (;;) {
    if (feof(fd) || fread(&c, 1, 1, fd) != 1) {
      if (state == BEGIN_LINE) break;

      // got to the EOF but not in BEGIN_LINE state so the file
      // probably does not end with a newline, so the parser has
      // not processed current line, simulate a newline in the file
      c = '\n';
    }

    switch (state & 0xff) {
      case BEGIN_LINE:
        if (c == '#')
          state = END_LINE;
        else if (isPrintable(c)) {
          i = 0;
          token.erase();
          strValue.clear();
          state = TOKEN;
          token.push_back(c);
        }
        break;
      case TOKEN:
        if (c == '=') {
          state = BEGIN_QUOTE;
        } else if (isPrintable(c))
          token.push_back(c);
        else
          state = END_LINE;
        break;
      case BEGIN_QUOTE:
        if (c == '"') {
          state = STR_VALUE;
          base = 0;
        } else if (c == '0')
          state = BEGIN_HEX;
        else if (isDigit(c, 10)) {
          state = NUM_VALUE;
          base = 10;
          numValue = getDigitValue(c, base);
          i = 0;
        } else if (c == '{') {
          state = NUM_VALUE;
          base = 16;
          i = 0;
          Set(IsStringValue);
        } else
          state = END_LINE;
        break;
      case BEGIN_HEX:
        if (c == 'x' || c == 'X') {
          state = NUM_VALUE;
          base = 16;
          numValue = 0;
          i = 0;
          break;
        } else if (isDigit(c, 10)) {
          state = NUM_VALUE;
          base = 10;
          numValue = getDigitValue(c, base);
          break;
        } else if (c != '\n' && c != '\r') {
          state = END_LINE;
          break;
        }
      // fal through to numValue to handle numValue

      case NUM_VALUE:
        if (isDigit(c, base)) {
          numValue *= base;
          numValue += getDigitValue(c, base);
          ++i;
        } else if (base == 16 &&
                   (c == ':' || c == '-' || c == ' ' || c == '}')) {
          if (i > 0) {
            int n = (i + 1) / 2;
            while (n-- > 0) {
              uint8_t c = (numValue >> (n * 8)) & 0xFF;
              strValue.push_back(c);
            }
          }
          Set(IsStringValue);
          numValue = 0;
          i = 0;
        } else {
          if (c == '\n' || c == '\r')
            state = BEGIN_LINE;
          else
            state = END_LINE;
          if (Is(IsStringValue) && base == 16 && i > 0) {
            int n = (i + 1) / 2;
            while (n-- > 0) strValue.push_back(((numValue >> (n * 8)) & 0xFF));
          }
          if (strValue.size() == 0) {
            while (numValue != 0) {
              strValue.push_back(numValue & 0xFF);
              numValue >>= 8;
            }
          }
          mParamMap[token] = strValue;
          strValue.clear();
          numValue = 0;
        }
        break;
      case STR_VALUE:
        if (c == '"') {
          strValue.push_back('\0');
          mParamMap[token] = strValue;
          state = END_LINE;
        } else if (isPrintable(c))
          strValue.push_back(c);
        break;
      case END_LINE:
        if (c == '\n' || c == '\r') state = BEGIN_LINE;
        break;
      default:
        break;
    }

    if (feof(fd)) break;
  }

  fclose(fd);

  return (mParamMap.size() > 0);
}

/*******************************************************************************
**
** Function:    CNfcConfig::CNfcConfig()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig::CNfcConfig() : mValidFile(true), state(0) {}

/*******************************************************************************
**
** Function:    CNfcConfig::~CNfcConfig()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig::~CNfcConfig() {}

/*******************************************************************************
**
** Function:    CNfcConfig::GetInstance()
**
** Description: get class singleton object
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig& CNfcConfig::GetInstance() {
  static CNfcConfig theInstance;

  if (theInstance.mParamMap.empty() && theInstance.mValidFile) {
    string strPath;
    findConfigFilePathFromTransportConfigPaths(config_name, strPath);
    theInstance.readConfig(strPath.c_str());
  }
  return theInstance;
}

/*******************************************************************************
**
** Function:    CNfcConfig::clean()
**
** Description: reset the setting array
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::clean() { mParamMap.clear(); }

/*******************************************************************************
**
** Function:    CNfcConfig::find()
**
** Description: find and return if the config is present
**
** Returns:     True/False if the config was found
**
*******************************************************************************/
bool CNfcConfig::find(const char* name, vector<uint8_t>& vectorValue) {
  auto value = mParamMap.find(name);
  if (value != mParamMap.end()) {
    vectorValue = mParamMap[name];
    return true;
  }
  return false;
}

/*******************************************************************************
**
** Function:    GetStrValue
**
** Description: API function for getting a string value of a setting
**
** Returns:     size of the string
**
*******************************************************************************/
extern int GetStrValue(const char* name, char* pValue, unsigned long l) {
  size_t len = l;
  CNfcConfig& rConfig = CNfcConfig::GetInstance();
  vector<uint8_t> vecValue;
  if (!rConfig.find(name, vecValue)) return 0;

  unsigned long vecSize = vecValue.size();
  if (vecSize > 0) {
    std::string str(vecValue.begin(), vecValue.end());
    memset(pValue, 0, len);
    if (len > vecSize) len = vecSize;
    memcpy(pValue, str.c_str(), len);
    DLOG_IF(INFO, nfc_debug_enabled)
        << StringPrintf("%s found %s=%s", __func__, name, pValue);
    return len;
  }
  return 0;
}

/*******************************************************************************
**
** Function:    GetVecValue
**
** Description: API function for getting a vector value of a setting
**
** Returns:     boolean if the value is found
**
*******************************************************************************/
extern bool GetVecValue(const char* name, vector<uint8_t>& vecValue) {
  CNfcConfig& rConfig = CNfcConfig::GetInstance();
  if (!rConfig.find(name, vecValue)) return false;

  if (vecValue.size() > 0) {
    string stringFormat = "";
    for (unsigned long i = 0; i < vecValue.size(); i++) {
      char temp[3];
      sprintf(temp, "%02x", vecValue[i]);
      stringFormat = stringFormat + temp;
    }
    DLOG_IF(INFO, nfc_debug_enabled)
        << StringPrintf("%s found %s=%s", __func__, name, stringFormat.c_str());
    return true;
  }
  return false;
}

/*******************************************************************************
**
** Function:    GetNumValue
**
** Description: API function for getting a numerical value of a setting
**
** Returns:     boolean if the value is found
**
*******************************************************************************/
extern bool GetNumValue(const char* name, void* pValue, unsigned long len) {
  if (!pValue) return false;

  CNfcConfig& rConfig = CNfcConfig::GetInstance();
  vector<uint8_t> vecValue;
  if (!rConfig.find(name, vecValue)) return false;

  unsigned long value = 0;
  for (unsigned long i = vecValue.size(); i > 0; i--) {
    value <<= 8;
    value += vecValue[i - 1];
  }
  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("%s found %s=(0x%lX)", __func__, name, value);

  switch (len) {
    case sizeof(unsigned long):
      *(static_cast<unsigned long*>(pValue)) = (unsigned long)value;
      break;
    case sizeof(unsigned short):
      *(static_cast<unsigned short*>(pValue)) = (unsigned short)value;
      break;
    case sizeof(unsigned char):
      *(static_cast<unsigned char*>(pValue)) = (unsigned char)value;
      break;
    default:
      return false;
  }
  return true;
}

/*******************************************************************************
**
** Function:    resetConfig
**
** Description: reset settings array
**
** Returns:     none
**
*******************************************************************************/
extern void resetConfig() {
  CNfcConfig& rConfig = CNfcConfig::GetInstance();
  rConfig.clean();
}
