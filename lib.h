#ifndef LIB_H
#define LIB_H

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define POCKET_LENGTH 100
#define UdpPocketLength (POCKET_LENGTH + 8)
#define BYTE char
#define BOOL unsigned int
#define TRUE 1
#define FALSE 0
//Setting '$' symbol as an data end symbol
#define STOP_DATA '$'

/**
* Defining Errors
*/
#define SUCCESS 1;
#define ERROR_CONNECTING 2;
#define ERROR_CREATE_SOCKET 3;
#define ERROR_SENDING_UDP 4;
#define ERROR_SENDING_TCP 5;


// MapData implementation for only FNTP needs

// Struct for data MAP
typedef struct map {
    int key; // index for DataStack
    BYTE value[UdpPocketLength]; // Byte array for data
    BOOL created; // will be 0 by default and in program should be set 1 if it will be created
} MapData;

#define MAP_SIZEOF(meta) (meta->PocketCount * sizeof(MapData))

// FNTP metadata
typedef struct _metaData {
    int DataId;
    int Length;
    int PocketCount;
    MapData * DataStack;
    int currentPocketCount;
    BOOL StopUDP;
    struct _metaData * next;
} MetaData;

typedef struct {
    int code;
    char * message;
} error;

// Convert int to 4 bytes
unsigned int int_to_bytes(int number, char * bytes);
int bytes_to_int(char * bytes);
int calc_pocket_count(int length, int pocket_length);
void combine_map_stack(MetaData *meta, BYTE *ret);


// Returns pointer to map which is contains that key, or pointer to last element with NULL value if key doesn't exists
MetaData * meta_search_point(MetaData *mapData, int key);
MetaData * meta_search_point_prev(MetaData *mapData, int dataId, MetaData **prevMeta);

// MetaData contains ... TRUE if data exists FALSE if not
BOOL meta_contains_key(MetaData *mapData, int key);

// MetaData Delete ... Returns Pointer to Map after deleting
MetaData * meta_delete_key(MetaData **mapData, int key);

void delete_meta(MetaData *mapData);
void meta_copy(MetaData *f, MetaData *s);
// Sets value by key, and if key doesn't exists it will add it ... TRUE if value inserted, otherwise false
BOOL meta_set_data(MetaData **mapData, int key, MetaData * setData);

// Run Threads
pthread_t fntp_thread_start(void *(*start_routine) (void *), void *arg);
void fntp_thread_stop(pthread_t thread);

#endif
