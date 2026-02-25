#ifndef JSON_COMMANDER_H
#define JSON_COMMANDER_H

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Visibility
 * -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef JCMD_BUILDING
#define JCMD_API __declspec(dllexport)
#else
#define JCMD_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define JCMD_API __attribute__((visibility("default")))
#else
#define JCMD_API
#endif

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

typedef int (*jcmd_main_fn)(const char* config_json);

JCMD_API int
jcmd_run(const char* cli_json, int argc, char* argv[], jcmd_main_fn main_fn);

#ifdef __cplusplus
}
#endif

#endif /* JSON_COMMANDER_H */
