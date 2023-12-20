#ifndef JLIB_H
#define JLIB_H 1

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_STRING_SIZE 256

typedef enum {
  JSON_NULL,
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_INT,
  JSON_INT_LONG,
  JSON_BOOL
} jsonType;

typedef union {
  int32_t integer;
  int64_t longInteger;
  char string[MAX_STRING_SIZE];
  bool boolean;
  void *object;
} jsonValue;

typedef struct {
  char key[MAX_STRING_SIZE];
  jsonType type;
  jsonValue value;
} jsonEntry;

typedef struct {
  jsonEntry *entries;
  uint64_t entriesCount;
} json;


int parseJsonFile(const char *file, json *output);
int parseJsonString(const char *data, json *output);
void printJson(const json *input, FILE *output);
void freeJsonContents(json *input);

#endif // JLIB_H
