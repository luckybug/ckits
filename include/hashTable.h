// hash table of string to string
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif

#define DEFINE_HASH_TABLE(name, row_num)  \
hashTable ##name; \
hashToken *##name_rows[row_num];

typedef struct _hashToken hashToken;
struct _hashToken
{
    hashToken *next;      // for hash table linkage
    const char *name;
    void *value;
};

typedef struct _tableInfo tableInfo;
typedef struct _tableInfo hashTable;

struct _tableInfo
{
    uint32_t reserved;
    uint32_t hashSize;      // DO NOT modify
#if 0
    void *(*xMalloc)(size_t);  /* malloc() function to use */
    void (*xFree)(void *);     /* free() function to use */

    void *(*tkMalloc)(size_t);
    void (*tkFree)(void *);
#endif
    hashToken *_table[0];
};

// NOTICE: use free() to de-allocate the object created by 'newHashTable'
int newHashTable(hashTable **pHashTable, uint32_t hashSize);
// 'initHashTable' is for initializing pre-allocated hast table
int initHashTable(hashTable *table, uint32_t hashSize);

int setHashValue(hashTable *table, const char *name, void *value, void **retval);
int getHashValue(hashTable *table, const char *name, void **retval);
int delHashValue(hashTable *table, const char *name, void **retval);

typedef enum _iterCtrl iterCtrl;
enum _iterCtrl
{
    ITERCTRL_CONTINUE,
    ITERCTRL_BREAK
};

typedef iterCtrl (*iterFunc)(const char *tokenName, void *tokenValue, void *param);
typedef int (*diffFunc)(void *value1, void *value2);
int iterHashTable(hashTable *table, iterFunc callback, void *param);

void dumpHashTable(hashTable *table);

#if defined(__cplusplus)
};
#endif

#endif

