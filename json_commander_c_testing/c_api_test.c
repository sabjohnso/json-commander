#include <json_commander.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Minimal test harness (pure C)
 * -------------------------------------------------------------------------- */

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                             \
  static void name(void);                                                      \
  static void run_##name(void) {                                               \
    ++tests_run;                                                               \
    printf("  %s ... ", #name);                                                \
    name();                                                                    \
    ++tests_passed;                                                            \
    printf("ok\n");                                                            \
  }                                                                            \
  static void name(void)

#define ASSERT(expr)                                                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      printf(                                                                  \
        "FAIL\n    assertion failed: %s\n    at %s:%d\n",                      \
        #expr,                                                                 \
        __FILE__,                                                              \
        __LINE__);                                                             \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_EQ_INT(a, b)                                                    \
  do {                                                                         \
    int _a = (a), _b = (b);                                                    \
    if (_a != _b) {                                                            \
      printf(                                                                  \
        "FAIL\n    expected %d == %d\n    at %s:%d\n",                         \
        _a,                                                                    \
        _b,                                                                    \
        __FILE__,                                                              \
        __LINE__);                                                             \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_STR_CONTAINS(haystack, needle)                                  \
  do {                                                                         \
    if (strstr((haystack), (needle)) == NULL) {                                \
      printf(                                                                  \
        "FAIL\n    \"%s\" not found in \"%s\"\n    at %s:%d\n",                \
        (needle),                                                              \
        (haystack),                                                            \
        __FILE__,                                                              \
        __LINE__);                                                             \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define RUN(name) run_##name()

/* --------------------------------------------------------------------------
 * Test schema and callback state
 * -------------------------------------------------------------------------- */

static const char* SCHEMA_WITH_OPTION =
  "{\"name\":\"app\",\"doc\":[\"A test app\"],\"version\":\"1.0.0\","
  "\"args\":[{\"kind\":\"option\",\"names\":[\"output\",\"o\"],"
  "\"doc\":[\"Output file\"],\"type\":\"string\",\"default\":\"out.txt\"}]}";

static char last_config[4096];
static int callback_called;

static void
reset_state(void) {
  callback_called = 0;
  last_config[0] = '\0';
}

static int
capture_callback(const char* config_json) {
  callback_called = 1;
  strncpy(last_config, config_json, sizeof(last_config) - 1);
  last_config[sizeof(last_config) - 1] = '\0';
  return 0;
}

static int
return42_callback(const char* config_json) {
  (void)config_json;
  callback_called = 1;
  return 42;
}

/* --------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

TEST(test_parse_ok) {
  reset_state();
  char* argv[] = {"app", "--output", "foo.txt"};
  int rc = jcmd_run(SCHEMA_WITH_OPTION, 3, argv, capture_callback);
  ASSERT_EQ_INT(rc, 0);
  ASSERT_EQ_INT(callback_called, 1);
  ASSERT_STR_CONTAINS(last_config, "\"output\":\"foo.txt\"");
}

TEST(test_parse_defaults) {
  reset_state();
  char* argv[] = {"app"};
  int rc = jcmd_run(SCHEMA_WITH_OPTION, 1, argv, capture_callback);
  ASSERT_EQ_INT(rc, 0);
  ASSERT_EQ_INT(callback_called, 1);
  ASSERT_STR_CONTAINS(last_config, "\"output\":\"out.txt\"");
}

TEST(test_callback_return_value) {
  reset_state();
  char* argv[] = {"app"};
  int rc = jcmd_run(SCHEMA_WITH_OPTION, 1, argv, return42_callback);
  ASSERT_EQ_INT(rc, 42);
  ASSERT_EQ_INT(callback_called, 1);
}

TEST(test_help) {
  reset_state();
  char* argv[] = {"app", "--help"};
  int rc = jcmd_run(SCHEMA_WITH_OPTION, 2, argv, capture_callback);
  ASSERT_EQ_INT(rc, 0);
  ASSERT_EQ_INT(callback_called, 0);
}

TEST(test_version) {
  reset_state();
  char* argv[] = {"app", "--version"};
  int rc = jcmd_run(SCHEMA_WITH_OPTION, 2, argv, capture_callback);
  ASSERT_EQ_INT(rc, 0);
  ASSERT_EQ_INT(callback_called, 0);
}

TEST(test_manpage) {
  reset_state();
  char* argv[] = {"app", "--help-man"};
  int rc = jcmd_run(SCHEMA_WITH_OPTION, 2, argv, capture_callback);
  ASSERT_EQ_INT(rc, 0);
  ASSERT_EQ_INT(callback_called, 0);
}

TEST(test_parse_error) {
  reset_state();
  char* argv[] = {"app", "--unknown"};
  int rc = jcmd_run(SCHEMA_WITH_OPTION, 2, argv, capture_callback);
  ASSERT_EQ_INT(rc, 1);
  ASSERT_EQ_INT(callback_called, 0);
}

TEST(test_invalid_schema) {
  reset_state();
  char* argv[] = {"app"};
  int rc = jcmd_run("{}", 1, argv, capture_callback);
  ASSERT_EQ_INT(rc, 1);
  ASSERT_EQ_INT(callback_called, 0);
}

TEST(test_bad_json_schema) {
  reset_state();
  char* argv[] = {"app"};
  int rc = jcmd_run("not json", 1, argv, capture_callback);
  ASSERT_EQ_INT(rc, 1);
  ASSERT_EQ_INT(callback_called, 0);
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int
main(void) {
  printf("json-commander C API tests\n");

  printf("\n[jcmd_run]\n");
  RUN(test_parse_ok);
  RUN(test_parse_defaults);
  RUN(test_callback_return_value);
  RUN(test_help);
  RUN(test_version);
  RUN(test_manpage);
  RUN(test_parse_error);
  RUN(test_invalid_schema);
  RUN(test_bad_json_schema);

  printf("\n%d/%d tests passed\n", tests_passed, tests_run);
  return (tests_passed == tests_run) ? 0 : 1;
}
