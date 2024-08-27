#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdlib.h>

#define CONFIG_FILE_NAME "partymod.ini"
#define ORIG_CONFIG_FILE_NAME "bmx.ini"

void initConfig();
int getConfigBool(char *section, char *key, int def);
int getConfigInt(char *section, char *key, int def);
int getConfigString(char *section, char *key, char *dst, size_t sz);

int getBMXConfigInt(char *section, char *key, int def);
void writeBMXConfigInt(char *section, char *key, int val);

#endif