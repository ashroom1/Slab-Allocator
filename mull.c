#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define DEFAULT_SIZE 4096

struct slab_meta
{
    void **pointer_array;
    unsigned long long **block_free;
    unsigned long n_avail;
    unsigned long n_free_total;
    //unsigned short *n_free;
    int size;
}**meta;

typedef unsigned long long int ullint;


//pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
int init_count = 0;
int n_stypes = 5;
size_t temp_count = 0;
char *curr_top = 0;
char *curr_limit = 0;

void *request_memory(int size)
{
    if ((curr_top + size) >= curr_limit)
    {
        //printf("%lu\n", temp_count++);
        curr_top = mmap(0, 50000000, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        curr_limit = curr_top + 50000000;
    }
    void *temp = curr_top;
    curr_top += size;
    return temp;    //Return -1 if fails
}


int mullinit(size_t size, int count) {

    //   pthread_mutex_lock(&mx);
    if (size < 1 || (int)size > INT32_MAX)
        return -1;

    struct slab_meta **meta_arr;
    if (!init_count) {
        if ((meta_arr = sbrk(n_stypes * sizeof(struct slab_meta *))) == (void *) -1) {
            errno = ENOMEM;
            return -1;
        }
        meta = meta_arr;
    }
    else if (init_count == n_stypes) {
        if ((meta_arr = sbrk((n_stypes += 5) * sizeof(struct slab_meta *))) == (void *) -1) {
            errno = ENOMEM;
            return -1;
        }
        for (int_fast32_t i = 0; i < n_stypes - 5; ++i)
            meta_arr[i] = meta[i];
        meta = meta_arr;
    }
    int to_alloc = count < 1 ? DEFAULT_SIZE : count;
    to_alloc = (((to_alloc + 4095) >> 12) << 12);

    struct slab_meta *slabMeta;
    if ((slabMeta = sbrk(sizeof(struct slab_meta))) == (void *) -1) {
        errno = ENOMEM;
        return -1;
    }

    meta[init_count] = slabMeta;
    meta[init_count]->n_free_total = to_alloc;
    meta[init_count]->n_avail = to_alloc;
    meta[init_count]->size = size;
    if ((meta[init_count]->block_free = sbrk((to_alloc >> 12) * sizeof(ullint))) == (void *) -1) {
        errno = ENOMEM;
        return -1;
    }

    for (int_fast32_t k = 0; k < to_alloc >> 12; ++k) {
        meta[init_count]->block_free[k] = request_memory(64 * sizeof(ullint));
        memset(meta[init_count]->block_free[k], 255, 64 * sizeof(ullint));
    }

    if ((meta[init_count]->pointer_array = sbrk((to_alloc >> 12) * sizeof(void *))) == (void *) -1) {
        errno = ENOMEM;
        return -1;
    }

    //if ((meta[init_count]->n_free = request_memory(to_alloc >> 12 * sizeof(unsigned short))) == (void *) -1) {
    //    errno = ENOMEM;
    //    return -1;
    //}
//
    //for (int_fast32_t j = 0; j < to_alloc >> 12; ++j)
    //    meta[init_count]->n_free[j] = 4096;

    char *top;
    if((top = request_memory(size * to_alloc)) == (void *) -1) {
        errno = ENOMEM;
        return -1;
    }

    for (int_fast32_t i = 0; i < (to_alloc >> 12); ++i)
        meta[init_count]->pointer_array[i] = top + ((i * size) << 12);         //Put a pointer to -
    // each of the size 8 blocks in the pointer array
//    pthread_mutex_unlock(&mx);
    return init_count++;
}

void* mulloc(int SlabID)
{

    register struct slab_meta *slab = meta[SlabID];

//    pthread_mutex_lock(&mx);
    if (SlabID < -1 || SlabID >= init_count) {
        //fprintf(stderr, "Invalid SlabID received by mulloc\n");
        return NULL;
    }


    if (slab->n_free_total) {      //There is an empty slot
        for (int_fast32_t i = 0; i < (slab->n_avail >> 12); ++i)    //Check each 64, ullints
            for (int_fast32_t j = 0; j < 64; ++j) {
                if (slab->block_free[i][j]) {
                    --slab->n_free_total;
                    int_fast32_t k;
                    for (k = 0; k < 64; ++k)
                        if (slab->block_free[i][j] & (1ULL << k))
                        {
                            slab->block_free[i][j] &= ~(1ULL << k);
                            return (char *) slab->pointer_array[i] + ((j << 6) + k) * slab->size;
                        }
//                pthread_mutex_unlock(&mx);
                }
            }
    }
    else        //Make 64 * 64 new slots and give one
    {
        void **new_top;
        if (!(new_top =  request_memory(((slab->n_avail >> 12) + 1) * sizeof(void *)))) {
            errno = ENOMEM;
            return NULL;
        }
        //for (int_fast32_t i = 0; i < slab->n_avail >> 3; ++i)
        //    new_top[i] = slab->pointer_array[i];
        memcpy(new_top, slab->pointer_array, (slab->n_avail >> 12) * sizeof(void *));

        slab->pointer_array = new_top;
        if (!(slab->pointer_array[slab->n_avail >> 12] = request_memory(slab->size << 12))) {   //  12 was 3
            errno = ENOMEM;
            return NULL;
        }
        //printf("Prev: %p\n", (char *)slab->pointer_array[slab->n_avail >> 3] + 192);
        //printf("Now: %p\n", sbrk(0));

        _Bool *new_free;
        if (!(new_free = request_memory(((slab->n_avail >> 12) + 1) * sizeof(void *)))) {
            errno = ENOMEM;
            return NULL;
        }

        //for (int_fast32_t j = 0; j < slab->n_avail; ++j)
        //    new_free[j] = slab->block_free[j];
        memcpy(new_free, slab->block_free, (slab->n_avail >> 12) * sizeof(void *));
        slab->block_free = (ullint **) new_free;

        //for (int_fast32_t k = slab->n_avail; k < slab->n_avail + 8; ++k)
        //    new_free[k] = 1;
        if (!(slab->block_free[slab->n_avail >> 12] = request_memory(64 * sizeof(ullint)))) {
            errno = ENOMEM;
            return NULL;
        }

        memset(slab->block_free[slab->n_avail >> 12], 255, 64 * sizeof(ullint));

        slab->n_avail += 4096;
        slab->n_free_total += 4096;

//        pthread_mutex_unlock(&mx);
        return mulloc(SlabID);
    }
    return NULL;    //Never going to happen
}

void mullfree(void * restrict ptr, int SlabID)      //SlabID optional parameter, < 0 to ignore
{
    if (!ptr)
        return;

//    pthread_mutex_lock(&mx);
    int_fast32_t i;
    int_fast32_t comp;

    if (SlabID >= init_count)
    {
        return;
    }
    else if (SlabID < 0) {
        i = 0;
        comp = init_count;
    }
    else
    {
        i = SlabID;
        comp = SlabID + 1;
    }

    for (; i < comp; ++i)
        for (int_fast32_t j = 0; j < meta[i]->n_avail >> 12; ++j)
            if ((size_t)((char *)meta[i]->pointer_array[j] + 4096 * meta[i]->size) > (size_t)ptr && (size_t)(meta[i]->pointer_array[j]) <= (size_t) ptr)
            {
                if (meta[i]->block_free[j][(((size_t)ptr - (size_t)meta[i]->pointer_array[j]) / meta[i]->size) / 64] & (1UL << (((size_t)ptr - (size_t)meta[i]->pointer_array[j]) / meta[i]->size) % 64)) {
                    //fprintf(stderr, "Pointer %p being freed was not allocated\n", ptr);
                    return;
                }
                else {
                    meta[i]->block_free[j][(((size_t)ptr - (size_t)meta[i]->pointer_array[j]) / meta[i]->size) / 64] |= 1UL << ((((size_t)ptr - (size_t)meta[i]->pointer_array[j]) / meta[i]->size) % 64);
                    ++meta[i]->n_free_total;
//                    pthread_mutex_unlock(&mx);
                    return;
                }
            }

    //fprintf(stderr, "Pointer %p being freed was not allocated using mulloc\n", ptr);
}

void freeall(int SlabID)        //Negative value frees everything
{
    if (SlabID >= init_count)
    {
        //fprintf(stderr, "Invalid SlabID received by mullfree\n");
        return;
    }

    //   pthread_mutex_lock(&mx);
    int_fast32_t i;
    int_fast32_t comp;

    if (SlabID < 0)
    {
        i = 0;
        comp = init_count;
    }
    else
    {
        i = SlabID;
        comp = SlabID + 1;
    }

    for (; i < comp; ++i)
        for (int_fast32_t j = 0; j < meta[i]->n_avail >> 12; ++j)
            memset(meta[i]->block_free[j], 255, 64 * sizeof(ullint));
    //pthread_mutex_unlock(&mx);
}