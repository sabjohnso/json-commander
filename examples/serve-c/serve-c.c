// serve-c — C port of the serve example using the json-commander C API.
//
// Demonstrates:
//   - Loading a CLI definition from a JSON schema file at runtime
//   - Typed options (int, string) with default values
//   - Environment variable bindings (SERVE_PORT, SERVE_HOST, SERVE_VERBOSE)
//
// Reuses the same serve.json schema as the C++ serve example.

#include <json_commander.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Schema loading
// ---------------------------------------------------------------------------

static char*
read_file(const char* path) {
  FILE* f = fopen(path, "r");
  if (!f) { return NULL; }

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  char* buf = malloc((size_t)len + 1);
  if (buf) {
    size_t n = fread(buf, 1, (size_t)len, f);
    buf[n] = '\0';
  }

  fclose(f);
  return buf;
}

// ---------------------------------------------------------------------------
// JSON helpers (minimal, for example purposes only)
// ---------------------------------------------------------------------------

static const char*
json_string(const char* json, const char* key, char* buf, size_t bufsz) {
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

  const char* start = strstr(json, pattern);
  if (!start) { return NULL; }
  start += strlen(pattern);

  const char* end = strchr(start, '"');
  if (!end) { return NULL; }

  size_t len = (size_t)(end - start);
  if (len >= bufsz) { len = bufsz - 1; }
  memcpy(buf, start, len);
  buf[len] = '\0';
  return buf;
}

static int
json_int(const char* json, const char* key, int fallback) {
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);

  const char* start = strstr(json, pattern);
  if (!start) { return fallback; }
  start += strlen(pattern);
  return atoi(start);
}

static int
json_bool(const char* json, const char* key) {
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\":true", key);
  return strstr(json, pattern) != NULL;
}

// ---------------------------------------------------------------------------
// Application logic
// ---------------------------------------------------------------------------

static int
serve_main(const char* config_json) {
  char host[256];
  char dir[256];

  json_string(config_json, "host", host, sizeof(host));
  json_string(config_json, "dir", dir, sizeof(dir));
  int port = json_int(config_json, "port", 8080);
  int verbose = json_bool(config_json, "verbose");

  printf("Serving %s on %s:%d", dir, host, port);
  if (verbose) { printf(" (verbose)"); }
  printf("\n");
  return 0;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int
main(int argc, char* argv[]) {
  char* schema = read_file(SERVE_C_SCHEMA);
  if (!schema) {
    fprintf(
      stderr, "%s: cannot read schema file: %s\n", argv[0], SERVE_C_SCHEMA);
    return 1;
  }

  int rc = jcmd_run(schema, argc, argv, serve_main);
  free(schema);
  return rc;
}
