
#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include <stdlib.h>

// Memory management macros
#ifdef LIST_CONFIG_H
#define _STR(x) #x
#define STR(x)  _STR(x)
#include STR(LIST_CONFIG_H)
#undef _STR
#undef STR
#endif

#ifndef LIST_MALLOC
#define LIST_MALLOC pvPortMalloc
#endif

#ifndef LIST_FREE
#define LIST_FREE vPortFree
#endif

/*
 * linked_list_t iterator direction.
 */

typedef enum {
    LIST_HEAD,
    LIST_TAIL
} linked_list_direction_t;

/*
 * linked_list_t node struct.
 */
typedef struct linked_list_node {
    struct linked_list_node *prev;
    struct linked_list_node *next;
    void *val;
} linked_list_node_t;

/*
 * linked_list_t struct.
 */
typedef struct
{
    linked_list_node_t *head;
    linked_list_node_t *tail;
    unsigned int len;
    void (*free)(void *val);
    int (*match)(void *a, void *b);
} linked_list_t;

/*
 * linked_list_t iterator struct.
 */

typedef struct
{
    linked_list_node_t *next;
    linked_list_direction_t direction;
} list_iterator_t;

// Node prototypes.

linked_list_node_t *linked_list_node_new(void *val);

// linked_list_t prototypes.

linked_list_t *linked_list_new(void);

linked_list_node_t *linked_list_rpush(linked_list_t *self, linked_list_node_t *node);

linked_list_node_t *linked_list_lpush(linked_list_t *self, linked_list_node_t *node);

linked_list_node_t *linked_list_find(linked_list_t *self, void *val);

linked_list_node_t *linked_list_at(linked_list_t *self, int index);

linked_list_node_t *linked_list_rpop(linked_list_t *self);

linked_list_node_t *linked_list_lpop(linked_list_t *self);

void linked_list_remove(linked_list_t *self, linked_list_node_t *node);

void linked_list_destroy(linked_list_t *self);

// linked_list_t iterator prototypes.

list_iterator_t *linked_list_iterator_new(linked_list_t *list, linked_list_direction_t direction);

list_iterator_t *linked_list_iterator_new_from_node(linked_list_node_t *node, linked_list_direction_t direction);

linked_list_node_t *linked_list_iterator_next(list_iterator_t *self);

void linked_list_iterator_destroy(list_iterator_t *self);

#endif // _LINKED_LIST_H_