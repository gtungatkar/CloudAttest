/*
 * File Name : wcache.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 07-03-2011
 * Description :
 * Web cache
 * This includes generic linked list implementation as well as wrapper function
 * for managing cache.
 * Cache design : 
 * Each cache object is linked in a global double linked list "cache"
 * It is also part of the list pointed to by a HashTable entry.
 * Each cache object contains multiple "fragments" - nothing but data buffers,
 * linked together.
 * Hash table stores pointers to cache based on hash value of http GET reqest
 * line.
 *
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include "wcache.h"
struct wcache_list * wcache_hashtable[WCACHE_HASHTABLE_SIZE];
//extern struct wcache cache;
//extern struct http_server_config cfg; 
 pthread_mutex_t list_mutex;
 /* very rudimentary hash function */
unsigned int hash(char * buf)
{
        unsigned int hashv = 1298; //seed
        const char *s = buf;
        while (*s)
        {
                hashv = hashv * 101  +  *s++;
        }
        return hashv;
}

struct wcache_entry * wcache_entry_alloc()
{
        struct wcache_entry *e = (struct wcache_entry *)malloc(sizeof(struct wcache_entry));
        memset(e, 0, sizeof(struct wcache_entry));
        pthread_mutex_init(&(e->entry_lock), NULL);
        return e;
}
int lock_entry(struct wcache_entry *w)
{
        return pthread_mutex_lock(&(w->entry_lock));

}
int unlock_entry(struct wcache_entry *w)
{
        return pthread_mutex_unlock(&(w->entry_lock));

}
int list_empty(struct wcache_list *l)
{
        if(l == NULL)
                return 1;
        if((l->head.next == l->head.prev) &&
                        (l->head.next == &(l->head)))
                return 1;
        return 0;
}
void wcache_table_init()
{
        int i;
        for(i = 0; i < WCACHE_HASHTABLE_SIZE; i++)
        {
                wcache_hashtable[i] = NULL;
        }
}
struct wcache_list * wcache_list_alloc()
{
        struct wcache_list * l = (struct wcache_list*)malloc(sizeof(struct wcache_list));
        memset(l, 0, sizeof(struct wcache_list));
        return l;
}
void wcache_list_init(struct wcache_list *l)
{
        assert(l != NULL);
        l->head.next = l->head.prev = &(l->head);
}


/*add element to a list*/
int wcache_list_add(struct wcache_list *l, struct wcache_list_element *e)
{
        if(e == NULL) goto ERR;
        if(l == NULL) goto ERR;
        struct wcache_list_element * last;
        if(l->head.prev == NULL)
        {
                l->head.prev = l->head.next = e;
                e->prev = &(l->head);
                e->next = &(l->head);
                return OK;
        }
        last = l->head.prev;
        last->next = e;
        e->prev = last;
        e->next = &(l->head);
        l->head.prev = e;
        return OK;
ERR:
        //LOG
        return ERROR;

}

int wcache_list_del(struct wcache_list_element *e)
{
        if(e == NULL) goto ERR;
        e->prev->next = e->next;
        e->next->prev = e->prev;
        e->prev = NULL;
        e->next = NULL;
        return OK;    
ERR:
        return ERROR;

}
/*add a fragment to this entry*/

/*add an entry to cache. Handle cache full and replacement*/
int wcache_add(struct wcache *w, struct wcache_entry *entry)
{
        struct wcache_list * l = &(w->l);
        struct wcache_list *hte;
        w->curr_size++;
        if(pthread_mutex_lock(&list_mutex) != 0)
                return ERROR;
                
        if(wcache_list_add(l, &(entry->cache_elem)) == ERROR)
                goto ERR;
        /*
        hte = wcache_hashtable[entry->signature_hash % WCACHE_HASHTABLE_SIZE];
        if(hte == NULL)
        {
        hte = wcache_hashtable[entry->signature_hash % WCACHE_HASHTABLE_SIZE] =
                wcache_list_alloc();

        }
        if(wcache_list_add(hte, &(entry->hash_elem)) == ERROR)
                goto ERR;
         */
        pthread_mutex_unlock(&list_mutex);
        return OK;
ERR:
        pthread_mutex_unlock(&list_mutex);
        //LOG
        return ERROR;
}

#if 0
/*find the cache entry corresponding to this http signature*/
struct wcache_entry * wcache_find(char http_signature[HTTP_SIGNATURE_SIZE],
                time_t ts)
{
        /*
         * ts - either the if modified since time or the current time
         * */
        /* TODO */
        unsigned int hashv = hash(http_signature);
        struct wcache_list *l = NULL;
        struct wcache_list_element *e;
        struct wcache_entry * we;
        
        if(pthread_mutex_lock(&list_mutex) != 0)
                return NULL;
        l = wcache_hashtable[hashv % WCACHE_HASHTABLE_SIZE];
        if(l) 
                e = &(l->head);
        else 
                goto RET;
        for(;e->next != &(l->head); e = e->next)
        {
                //w = l;
                we = container_of((e->next), struct wcache_entry, hash_elem);
                if(!we->valid) 
                        goto RET;
                if(strcmp(http_signature, we->http_signature) == 0)
                {
                        goto RETWE;
                }

        }
RET:
        pthread_mutex_unlock(&list_mutex);
        return NULL;
RETWE:
        pthread_mutex_unlock(&list_mutex);
        return we;

}
#endif
struct wcache_entry*  wcache_remove_first(struct wcache *w)
{

        struct wcache_entry * entry = 
                container_of((w->l.head.next), struct wcache_entry, cache_elem);
        if(entry == NULL)
                printf("NULL entry\n");
     //   printf("entry = %s\n", entry->http_request);
        if(wcache_list_del(w->l.head.next) == ERROR)
        { 
                return NULL;
        }
        w->curr_size--;
        return entry;

}
int is_request_replicated(struct wcache *w)
{

        struct wcache_entry * entry;
        if(!list_empty(&(w->l)))
        {
                entry = 
                        container_of(w->l.head.next, struct wcache_entry, cache_elem);
                if(entry)
                        return entry->is_replicated;
        }
        return 0;

}
int wcache_remove_entry(struct wcache_entry *w)
{
      
        if(wcache_list_del(&(w->cache_elem)) == ERROR)
        { 
                return ERROR;
        }
        /*
        if(wcache_list_del(&(w->hash_elem)) == ERROR)
        { 
                                        printf(":3 check\n");
                return ERROR;
        }
         */                              printf(":4 check\n");
        //free every fragment
        pthread_mutex_destroy(&(w->entry_lock));
        return OK;
}
/*
int main()
{
        struct wcache cache;
        struct wcache_entry *entry = wcache_entry_alloc();
        entry->http_request = (char *)malloc(20);
        strcpy(entry->http_request, "This is test request");
        cache.curr_size = 0;
        wcache_list_init(&(cache.l));
        wcache_add(&cache, entry);
        printf("size of queue = %d\n", cache.curr_size);
        entry = NULL;
        entry = wcache_remove_first(&cache);
        if(list_empty(&(cache.l)))
                printf("list empty\n");
        printf("size of queue after remove = %d\n http req = %s\n",
                       cache.curr_size, entry->http_request);
        return 0;
}
 */
