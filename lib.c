#include "lib.h"

unsigned int int_to_bytes(int number, char * bytes)
{
    memcpy(bytes, &number, 4);
    return 1;
}

int bytes_to_int(char * bytes)
{
    int ret = 0;
    memcpy(&ret, bytes, 4);
    return ret;
}

int calc_pocket_count(int length, int pocket_length)
{
    return ((length / pocket_length) + 1);
}

void combine_map_stack(MetaData *meta, BYTE *ret)
{
    BYTE *stackLen;
    stackLen = malloc(sizeof(BYTE) * meta->PocketCount * POCKET_LENGTH);
    int i, index=0;
    for (i=0; i < meta->PocketCount; i++)
    {
        memcpy(&stackLen[index], meta->DataStack[i].value, POCKET_LENGTH);
        index += POCKET_LENGTH;
    }
    memcpy(ret, stackLen, meta->Length);
    free(stackLen);
}

MetaData * meta_search_point_prev(MetaData *mapData, int dataId, MetaData **prevMeta)
{
    MetaData * tmp;
    tmp = mapData;
    if(tmp == NULL) return NULL;
    if(tmp->DataId == dataId)
    {
        return tmp;
    }
    (*prevMeta) = tmp;
    tmp = tmp->next;
    while (tmp != NULL)
    {
        if(tmp->DataId == dataId)
        {
            return tmp;
        }
        (*prevMeta) = tmp;
        tmp = tmp->next;
    }
    return tmp;
}

MetaData * meta_search_point(MetaData *mapData, int dataId)
{
    MetaData *prev;
    return meta_search_point_prev(mapData, dataId, &prev);
}

BOOL meta_contains_key(MetaData *mapData, int dataId)
{
    MetaData *tmp = meta_search_point(mapData, dataId);
    if(tmp != NULL)
    {
        return TRUE;
    }
    return FALSE;
}

void meta_copy(MetaData *f, MetaData *s)
{
    f->DataId = s->DataId;
    f->Length = s->Length;
    f->PocketCount = s->PocketCount;
    f->StopUDP = s->StopUDP;
    f->DataStack = (MapData *) malloc(MAP_SIZEOF(s));
    memcpy(f->DataStack, s->DataStack, MAP_SIZEOF(s));
}

BOOL meta_set_data(MetaData **mapData, int dataId, MetaData * setData)
{
    MetaData *tmp, *prev;
    tmp = meta_search_point_prev((*mapData), dataId, &prev);
    if(tmp != NULL)
    {
        if(prev != NULL)
            prev->next = setData;
        else
            (*mapData) = setData;
        setData->next = tmp->next;
        free(tmp->DataStack);
        free(tmp);
        return FALSE;
    }
    // If prev is null and tmp is null then something wrong
    if(prev == NULL)
    {
        return FALSE;
    }
    prev->next = setData;
    meta_copy(prev->next, setData);

    return TRUE;
}

//TODO: TEST THIS CODE !!!!
MetaData * meta_delete_key(MetaData **mapData, int dataId)
{
    MetaData *tmp;
    tmp = (*mapData);
    if(tmp == NULL) return (*mapData);
    if(tmp->DataId == dataId)
    {
        (*mapData) = tmp->next;
        free(tmp->DataStack);
        free(tmp);
    }
    else
    {
        MetaData *temp_next;
        temp_next = tmp->next;
        while (temp_next != NULL)
        {
            if(temp_next->DataId == dataId)
            {
                tmp->next = temp_next->next;
                free(temp_next->DataStack);
                free(temp_next);
            }
            tmp = tmp->next;
            temp_next = temp_next->next;
        }
    }

    return (*mapData);
}

void delete_meta(MetaData *mapData)
{
    MetaData *tmp, *tmp2;
    tmp = mapData;
    while (tmp != NULL)
    {
        tmp2 = tmp;
        tmp = tmp->next;
        free(tmp2->DataStack);
        free(tmp2);
    }
}

pthread_t fntp_thread_start(void *(*start_routine) (void *), void *arg)
{
    pthread_t ret_thread;
    if(pthread_create(&ret_thread, NULL, start_routine, arg)) {
        return 0;
    }
    return ret_thread;
}

void fntp_thread_stop(pthread_t pthread)
{
    pthread_cancel(pthread);
}