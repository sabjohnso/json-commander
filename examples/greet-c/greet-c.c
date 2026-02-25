// greet-c — C port of the greet example using the json-commander C API.
//
// Demonstrates:
//   - Inline JSON schema definition
//   - Single-function API via jcmd_run
//
// Compare with greet.cpp (97 lines) to see how jcmd_run eliminates
// boilerplate: no manual ParseResult dispatch, no error handling,
// no help/version/manpage plumbing.

#include <json_commander.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char* GREET_SCHEMA =
  "{"
  "  \"name\": \"greet\","
  "  \"doc\": [\"A friendly greeting tool.\"],"
  "  \"version\": \"1.0.0\","
  "  \"args\": ["
  "    {"
  "      \"kind\": \"flag\","
  "      \"names\": [\"loud\", \"l\"],"
  "      \"doc\": [\"Print the greeting in uppercase.\"]"
  "    },"
  "    {"
  "      \"kind\": \"positional\","
  "      \"name\": \"name\","
  "      \"doc\": [\"The name to greet.\"],"
  "      \"type\": \"string\","
  "      \"required\": true"
  "    }"
  "  ]"
  "}";

// ---------------------------------------------------------------------------
// Application logic
// ---------------------------------------------------------------------------

static int
greet_main(const char* config_json) {
  int loud = strstr(config_json, "\"loud\":true") != NULL;

  const char* key = "\"name\":\"";
  const char* start = strstr(config_json, key);
  if (!start) {
    fprintf(stderr, "greet: missing name in config\n");
    return 1;
  }
  start += strlen(key);

  const char* end = strchr(start, '"');
  if (!end) {
    fprintf(stderr, "greet: malformed config\n");
    return 1;
  }

  char greeting[256];
  snprintf(
    greeting, sizeof(greeting), "Hello, %.*s!", (int)(end - start), start);

  if (loud) {
    for (char* p = greeting; *p; ++p) {
      *p = (char)toupper((unsigned char)*p);
    }
  }

  printf("%s\n", greeting);
  return 0;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int
main(int argc, char* argv[]) {
  return jcmd_run(GREET_SCHEMA, argc, argv, greet_main);
}
