#ifndef OFFSETSNIPPET_H
#define OFFSETSNIPPET_H

#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

int sqlite3_extension_init(sqlite3* db, char** errMsg, const sqlite3_api_routines* pApi);

#ifdef __cplusplus
}
#endif

#endif // OFFSETSNIPPET_H 