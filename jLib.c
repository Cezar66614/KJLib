#include "jLib.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

char *_parseJsonString(const char *data, const char *finalPosition, json *output);

void freeEntries(jsonEntry *input, const size_t entriesCount) {
  size_t i = 0;
  for (i = 0; i < entriesCount; ++i)
    if ( ((input + i)->type == JSON_OBJECT) || ((input + i)->type == JSON_ARRAY) ) { freeJsonContents((input + i)->value.object); free((input + i)->value.object); }
}
void freeJsonContents(json *input) {
  if (input == NULL) return;
  if (input->entries == NULL) return;

  freeEntries(input->entries, input->entriesCount);
  free(input->entries);
}

char *skipWhitespace(char *data, const char *finalPosition) {
  if (data == NULL || finalPosition == NULL) return NULL;

  bool ok = 1;
  while (ok) {
    if (data > finalPosition) return NULL;

    switch (*data) {
    case 32: // SPACE
    case 10: // LINE FEED
    case 13: // CARRIAGE RETURN
    case  9: // TAB
    case  0: // END OF FILE
      ++data;
      break;

    default:
      ok = 0;
      break;
    }
  }

  return data;
}

char *getString(char *data, const char *finalPosition, char *output) {
  if (data == NULL || finalPosition == NULL || output == NULL) return NULL;
  if (*(data++) != '\"') return NULL;

  size_t outputSize = 0;
  while (*data != '\"') {
    if ( (data >= finalPosition) || (outputSize >= MAX_STRING_SIZE) ) return NULL;

    *output = *data;

    if (*data == '\\') {
      ++data;
      if (data >= finalPosition) return NULL;

      switch (*data) {
      case 'b':  // backspace
        *output = '\b';
        break;
      case 'f':  // formfeed
        *output = '\f';
        break;
      case 'n':  // linefeed
        *output = '\n';
        break;
      case 'r':  // carriage return
        *output = '\r';
        break;
      case 't':  // horizontal tab
        *output = '\t';
        break;
      case '\\': // reverse solidus
        *output = '\\';
        break;
      case '/':  // solidus
        *output = '/';
        break;
      case 'u':  // TODO: Handle 4 hex digits
        break;

      default:
        *(++output) = *data;
        ++outputSize;
        break;
      }
    }

    ++output; ++outputSize;
    ++data;
  }
  ++data;

  if (outputSize >= MAX_STRING_SIZE) return NULL;
  *output = '\0';

  if (data >= finalPosition) return NULL;

  return data;
}

char *getInteger(char *data, const char *finalPosition, int64_t *output) {
  if (data == NULL || finalPosition == NULL || output == NULL) return NULL;

  *output = 0;
  while ( (*data >= '0') && (*data <= '9') ) {
    if (data >= finalPosition) return NULL;

    *output *= 10;
    *output += (*data - '0');
    ++data;
  }

  return data;
}

char *getValue(char *data, const char *finalPosition, jsonEntry *output) {
  if (data == NULL || finalPosition == NULL || output == NULL) return NULL;

  if (*data == '{') {
    output->type = JSON_OBJECT;

    output->value.object = NULL;
    output->value.object = (json *)malloc(sizeof(json));
    if (output->value.object == NULL) { fprintf(stderr, "Error: `getValue` allocating object.\n"); return NULL; }

    data = _parseJsonString(data, finalPosition, output->value.object);
    if ( (data == NULL) || (data >= finalPosition) ) data = NULL;

  } else if (*data == '[') {
    output->type = JSON_ARRAY;
    
    output->value.object = NULL;
    output->value.object = (json *)malloc(sizeof(json));
    if (output->value.object == NULL) { fprintf(stderr, "Error: `getValue` allocating object.\n"); return NULL; }

    data = _parseJsonString(data, finalPosition, output->value.object);
    if ( (data == NULL) || (data >= finalPosition) ) data = NULL;

  } else if (*data == '\"') {
    output->type = JSON_STRING;
    data = getString(data, finalPosition, output->value.string);
  } else if ( (*data >= '0') && (*data <= '9') ) {
    output->type = JSON_INT;
    data = getInteger(data, finalPosition, &output->value.longInteger);
    if (output->value.longInteger >= INT_LEAST32_MAX) output->type = JSON_INT_LONG;
  } else if ( (finalPosition - data > 4) && (strncmp(data, "true", 4) == 0) ) {
    output->type = JSON_BOOL;
    output->value.boolean = 1;
    data += 4;
  } else if ( (finalPosition - data > 5) && (strncmp(data, "false", 5) == 0) ) {
    output->type = JSON_BOOL;
    output->value.boolean = 0;
    data += 5;
  } else data = NULL;

  return data;
}

void typeToString(jsonType type, char *output) {
  if (output == NULL) return;

  switch (type) {
  case JSON_NULL:
    strncpy(output, "JSON_NULL", MAX_STRING_SIZE);
    break;
  case JSON_OBJECT:
    strncpy(output, "JSON_OBJECT", MAX_STRING_SIZE);
    break;
  case JSON_ARRAY:
    strncpy(output, "JSON_ARRAY", MAX_STRING_SIZE);
    break;
  case JSON_STRING:
    strncpy(output, "JSON_STRING", MAX_STRING_SIZE);
    break;
  case JSON_INT:
    strncpy(output, "JSON_INT", MAX_STRING_SIZE);
    break;
  case JSON_INT_LONG:
    strncpy(output, "JSON_INT_LONG", MAX_STRING_SIZE);
    break;
  case JSON_BOOL:
    strncpy(output, "JSON_BOOL", MAX_STRING_SIZE);
    break;

  default:
    strncpy(output, "JSON_BAD_TYPE", MAX_STRING_SIZE);
    break;
  }
}

char *_parseJsonString(const char *data, const char *finalPosition, json *output) {
  /// Basic Checks
  if (data == NULL || finalPosition == NULL || output == NULL) { fprintf(stderr, "Error: `parseJsonString` bad parameters (NULL).\n"); return NULL; }

  // Error string for later output
  char errorLog[MAX_STRING_SIZE];

  /// Init
  size_t entriesSize = 1;
  output->entries = NULL;
  output->entriesCount = 0;

  // First allocation
  output->entries = (jsonEntry *)malloc(sizeof(jsonEntry) * entriesSize);
  if (output->entries == NULL) { strcpy(errorLog, "Error: `parseJsonString` allocating entries."); goto error; }


  /// Start JSON Parsing
  char *p = (char *)data; // for walking through the string


  p = skipWhitespace(p, finalPosition);
  if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 0"); goto error; }

  /// Start Object
  if (*p == '{') {
    ++p;

    p = skipWhitespace(p, finalPosition);
    if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 2"); goto error; }

    while (*p != '}') {

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 3"); goto error; }

      /// Get Key (string)
      p = getString(p, finalPosition, (output->entries + output->entriesCount)->key); // + to support looong entries
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 4"); goto error; }
      //printf("%s: ", (output->entries + output->entriesCount)->key);

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 5"); goto error; }

      /// Get ':'
      if (*(p++) != ':') { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 6"); goto error; }

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 7"); goto error; }

      /// Get Value
      p = getValue(p, finalPosition, (output->entries + output->entriesCount)); // + to support looong entries
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 8"); goto error; }
      //typeToString((output->entries + output->entriesCount)->type, errorLog);
      //printf("%s\n", errorLog);

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 9"); goto error; }

      /// Get next entry
      if (*p == ',') ++p;

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 10"); goto error; }


      /// Allocate space for new entries
      ++output->entriesCount;
      if (output->entriesCount >= output->entriesCount) {
        entriesSize += 10;
        output->entries = (jsonEntry *)realloc(output->entries, sizeof(jsonEntry) * entriesSize);
        if (output->entries == NULL) { strcpy(errorLog, "Error: `parseJsonString` re-allocating entries. ID: 0"); goto error; }
      }

    }

  } else if (*p == '[') {
    ++p;

    p = skipWhitespace(p, finalPosition);
    if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json array file. ID: 2"); goto error; }

    while (*p != ']') {

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json array file. ID: 3"); goto error; }

      /// Set Key (string)
      snprintf((output->entries + output->entriesCount)->key, MAX_STRING_SIZE, "%ld", output->entriesCount);
      //printf("%s: ", (output->entries + output->entriesCount)->key);

      /// Get Value
      p = getValue(p, finalPosition, (output->entries + output->entriesCount)); // + to support looong entries
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json array file. ID: 4"); goto error; }
      //typeToString((output->entries + output->entriesCount)->type, errorLog);
      //printf("%s\n", errorLog);

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json array file. ID: 5"); goto error; }

      /// Get next entry
      if (*p == ',') ++p;

      p = skipWhitespace(p, finalPosition);
      if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json array file. ID: 6"); goto error; }


      /// Allocate space for new entries
      ++output->entriesCount;
      if (output->entriesCount >= output->entriesCount) {
        entriesSize += 10;
        output->entries = (jsonEntry *)realloc(output->entries, sizeof(jsonEntry) * entriesSize);
        if (output->entries == NULL) { strcpy(errorLog, "Error: `parseJsonString` re-allocating entries. ID: 0"); goto error; }
      }

    }

  } else { strcpy(errorLog, "Error: `parseJsonString` invalid json / json array file. ID: 1"); goto error; }

  ++p; // skip '}' or ']'

  p = skipWhitespace(p, finalPosition);

  // not a problem if it find the end of the file
  // if (p == NULL) { strcpy(errorLog, "Error: `parseJsonString` invalid json file. ID: 10"); goto error; }

  if (p == NULL) p = (char *)finalPosition;

  /// Realloc to fit the data perfectly
  entriesSize = output->entriesCount;
  output->entries = (jsonEntry *)realloc(output->entries, sizeof(jsonEntry) * entriesSize);
  if (output->entries == NULL) { strcpy(errorLog, "Error: `parseJsonString` re-allocating entries. ID: 1"); goto error; }

  /// Leave and error handling
  goto leave;

  error:
    freeJsonContents(output);
    fprintf(stderr, "%s\n", errorLog);
    return NULL;

  leave:
    return p;
}

int parseJsonString(const char *data, json *output) {
  if (data == NULL || output == NULL) { fprintf(stderr, "Error: `parseJsonString` bad parameters (NULL).\n"); return 1; }

  size_t dataSize = strlen(data);
  if (dataSize < 2) { fprintf(stderr, "Error: `parseJsonString` bad data (size).\n"); return 1; }
  const char *finalPosition = (char *)data + dataSize;

  if (_parseJsonString(data, finalPosition, output) == NULL) return 1;

  return 0;
}

char *readFileAlloc(const char *filename, size_t *fileSize) { (*fileSize) = 0;
  FILE *fp = fopen(filename, "rb");

  if (fp == NULL) { perror("Error: Opening File"); return NULL; }

  if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); perror("Error: seeking to end of file"); return NULL; }
  (*fileSize) = ftell(fp); if ((*fileSize) == -1UL) { fclose(fp); perror("Error: getting file position"); return NULL; }
  if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); perror("Error: seeking to start of file"); return NULL; }

  char *buffer = (char *)malloc((*fileSize)+1);

  uint bytesRead = fread(buffer, 1, (*fileSize), fp);
  if (bytesRead != (*fileSize)) {
    free(buffer);
    if (ferror(fp)) perror("Error: Reading File");
    else if (feof(fp)) perror("Error: Unexpected end of file");
    else perror("Error: Unknown Read Error!");
  
    fclose(fp);
    return NULL;
  }

  if (ferror(fp)) { free(buffer); fclose(fp); perror("Error: Reading File"); }

  buffer[(*fileSize)] = '\0';
  fclose(fp);

  return buffer;
}

int parseJsonFile(const char *file, json *output) {
  if (file == NULL || output == NULL) { fprintf(stderr, "Error: `parseJsonFile` bad parameters (NULL).\n"); return 1; }

  size_t fileSize;
  char *data = readFileAlloc(file, &fileSize);
  if (data == NULL) return 1;

  bool ok = parseJsonString(data, output);

  free(data);
  return ok;
}

void _printJson(const json *input, FILE *output, size_t tabCount, bool prevArray) {
  if (input == NULL) { fprintf(stderr, "Error: `printJson` bad parameters (NULL).\n"); return; }

  size_t i, tab;
  for (i = 0; i < input->entriesCount; ++i) {

    for (tab = 1; tab <= tabCount; ++tab) fprintf(output, "    ");

    if (!prevArray) fprintf(output, "\"%s\": ", (input->entries + i)->key);
    switch ((input->entries + i)->type) {
    case JSON_OBJECT:
      fprintf(output, "{\n");
      _printJson((input->entries + i)->value.object, output, tabCount + 1, 0);
      
      for (tab = 1; tab <= tabCount; ++tab) fprintf(output, "    ");
      fprintf(output, "}");

      break;
    case JSON_ARRAY:
      fprintf(output, "[\n");
      _printJson((input->entries + i)->value.object, output, tabCount + 1, 1);

      for (tab = 1; tab <= tabCount; ++tab) fprintf(output, "    ");
      fprintf(output, "]");

      break;
    
    case JSON_STRING:
      fprintf(output, "\"%s\"", (input->entries + i)->value.string);
      break;
    
    case JSON_INT:
      fprintf(output, "%d", (input->entries + i)->value.integer);
      break;
    case JSON_INT_LONG:
      fprintf(output, "%ld", (input->entries + i)->value.longInteger);
      break;
    
    case JSON_BOOL:
      fprintf(output, ((input->entries + i)->value.boolean) ? "true" : "false");
      break;
    
    case JSON_NULL:
      fprintf(output, "null");
      break;

    default:
      break;
    }
    if (i < input->entriesCount - 1) fprintf(output, ",");
    fprintf(output, "\n");
  }
}

void printJson(const json *input, FILE *output) {
  fprintf(output, "{\n");
  _printJson(input, output, 1, 0);
  fprintf(output, "}\n");
}
