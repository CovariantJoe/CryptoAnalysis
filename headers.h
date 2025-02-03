#ifndef headers_h
#define headers_h
#include <vector>
#include <string>
#include <sqlite3.h>

int Alert(std::vector<std::string> alerts, const char* mode);
int API(sqlite3 *db);
int database();
std::string UNIX(std::string unix_time);
#endif
