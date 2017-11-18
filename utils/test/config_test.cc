#include <gtest/gtest.h>

#include <config.h>

namespace {
const char SIMPLE_CONFIG_FILE[] = "/data/local/tmp/test_config.conf";
const char SIMPLE_CONFIG[] =
    "# Simple config file test\n\
STRING_VALUE=\"Hello World!\"\n\
#COMMENTED_OUT_VALUE=1\n\
NUM_VALUE=42\n\
BYTES_VALUE={0A:0b:0C:fF:00}\n";

const char INVALID_CONFIG1[] =
    "# This is an invalid config\n\
# Config values must contain an = sign\n\
TEST:1";

const char INVALID_CONFIG2[] =
    "# This is an invalid config\n\
# Byte arrays must contain at least one value\n\
TEST={}";

const char INVALID_CONFIG3[] =
    "# This is an invalid config\n\
# String values cannot be empty\n\
TEST="
    "";

const char INVALID_CONFIG4[] =
    "# This is an invalid config\n\
# Multiple config entries with the same key\n\
TEST=1\n\
TEST=2";
}  // namespace

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FILE* fp = fopen(SIMPLE_CONFIG_FILE, "wt");
    fwrite(SIMPLE_CONFIG, 1, sizeof(SIMPLE_CONFIG), fp);
    fclose(fp);
  }
};

TEST_F(ConfigTest, test_simple_config) {
  ConfigFile config;
  config.parseFromString(SIMPLE_CONFIG);
  ASSERT_FALSE(config.hasValue("UNKNOWN_VALUE"));
  ASSERT_FALSE(config.hasValue("COMMENTED_OUT_VALUE"));
  ASSERT_TRUE(config.hasValue("NUM_VALUE"));
  ASSERT_TRUE(config.hasValue("STRING_VALUE"));
  ASSERT_TRUE(config.hasValue("BYTES_VALUE"));
}

TEST_F(ConfigTest, test_simple_values) {
  ConfigFile config;
  config.parseFromString(SIMPLE_CONFIG);
  ASSERT_TRUE(config.getUnsignedValue("NUM_VALUE") == 42);
  ASSERT_TRUE(config.getStringValue("STRING_VALUE") == "Hello World!");
  auto bytes = config.getBytesValue("BYTES_VALUE");
  ASSERT_TRUE(bytes.size() == 5);
  ASSERT_TRUE(bytes[0] == 10);
  ASSERT_TRUE(bytes[1] == 11);
  ASSERT_TRUE(bytes[2] == 12);
  ASSERT_TRUE(bytes[3] == 255);
  ASSERT_TRUE(bytes[4] == 0);
}

TEST_F(ConfigTest, test_invalid_configs) {
  ConfigFile config1;
  EXPECT_DEATH(config1.parseFromString(INVALID_CONFIG1), "");
  ConfigFile config2;
  EXPECT_DEATH(config2.parseFromString(INVALID_CONFIG2), "");
  ConfigFile config3;
  EXPECT_DEATH(config3.parseFromString(INVALID_CONFIG3), "");
  ConfigFile config4;
  EXPECT_DEATH(config4.parseFromString(INVALID_CONFIG4), "");
}

TEST_F(ConfigTest, test_file_based_config) {
  ConfigFile config;
  config.parseFromFile(SIMPLE_CONFIG_FILE);
  ASSERT_FALSE(config.hasValue("UNKNOWN_VALUE"));
  ASSERT_TRUE(config.getUnsignedValue("NUM_VALUE") == 42);
  ASSERT_TRUE(config.getStringValue("STRING_VALUE") == "Hello World!");
  auto bytes = config.getBytesValue("BYTES_VALUE");
  ASSERT_TRUE(bytes.size() == 5);
  ASSERT_TRUE(bytes[0] == 10);
  ASSERT_TRUE(bytes[1] == 11);
  ASSERT_TRUE(bytes[2] == 12);
  ASSERT_TRUE(bytes[3] == 255);
  ASSERT_TRUE(bytes[4] == 0);
}
