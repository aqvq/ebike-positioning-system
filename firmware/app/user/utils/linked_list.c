
#include "FreeRTOS.h"
#include "stdio.h"
#include "linked_list.h"

/*
 * Allocate a new linked_list_t. NULL on failure.
 */

linked_list_t *linked_list_new(void)
{
    linked_list_t *self;
    if (!(self = LIST_MALLOC(sizeof(linked_list_t))))
        return NULL;
    self->head  = NULL;
    self->tail  = NULL;
    self->free  = NULL;
    self->match = NULL;
    self->len   = 0;
    return self;
}

/*
 * Free the list.
 */

void linked_list_destroy(linked_list_t *self)
{
    unsigned int len = self->len;
    linked_list_node_t *next;
    linked_list_node_t *curr = self->head;

    while (len--) {
        next = curr->next;
        if (self->free)
            self->free(curr->val);
        LIST_FREE(curr);
        curr = next;
    }

    LIST_FREE(self);
}

/*
 * Append the given node to the list
 * and return the node, NULL on failure.
 */

linked_list_node_t *linked_list_rpush(linked_list_t *self, linked_list_node_t *node)
{
    if (!node)
        return NULL;

    if (self->len) {
        node->prev       = self->tail;
        node->next       = NULL;
        self->tail->next = node;
        self->tail       = node;
    } else {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;
    return node;
}

/*
 * Return / detach the last node in the list, or NULL.
 */

linked_list_node_t *linked_list_rpop(linked_list_t *self)
{
    if (!self->len)
        return NULL;

    linked_list_node_t *node = self->tail;

    if (--self->len) {
        (self->tail = node->prev)->next = NULL;
    } else {
        self->tail = self->head = NULL;
    }

    node->next = node->prev = NULL;
    return node;
}

/*
 * Return / detach the first node in the list, or NULL.
 */

linked_list_node_t *linked_list_lpop(linked_list_t *self)
{
    if (!self->len)
        return NULL;

    linked_list_node_t *node = self->head;

    if (--self->len) {
        (self->head = node->next)->prev = NULL;
    } else {
        self->head = self->tail = NULL;
    }

    node->next = node->prev = NULL;
    return node;
}

/*
 * Prepend the given node to the list
 * and return the node, NULL on failure.
 */

linked_list_node_t *linked_list_lpush(linked_list_t *self, linked_list_node_t *node)
{
    if (!node)
        return NULL;

    if (self->len) {
        node->next       = self->head;
        node->prev       = NULL;
        self->head->prev = node;
        self->head       = node;
    } else {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;
    return node;
}

/*
 * Return the node associated to val or NULL.
 */

linked_list_node_t *linked_list_find(linked_list_t *self, void *val)
{
    list_iterator_t *it = linked_list_iterator_new(self, LIST_HEAD);
    linked_list_node_t *node;

    while ((node = linked_list_iterator_next(it))) {
        if (self->match) {
            if (self->match(val, node->val)) {
                linked_list_iterator_destroy(it);
                return node;
            }
        } else {
            if (val == node->val) {
                linked_list_iterator_destroy(it);
                return node;
            }
        }
    }

    linked_list_iterator_destroy(it);
    return NULL;
}

/*
 * Return the node at the given index or NULL.
 */

linked_list_node_t *linked_list_at(linked_list_t *self, int index)
{
    linked_list_direction_t direction = LIST_HEAD;

    if (index < 0) {
        direction = LIST_TAIL;
        index     = ~index;
    }

    if ((unsigned)index < self->len) {
        list_iterator_t *it      = linked_list_iterator_new(self, direction);
        linked_list_node_t *node = linked_list_iterator_next(it);
        while (index--)
            node = linked_list_iterator_next(it);
        linked_list_iterator_destroy(it);
        return node;
    }

    return NULL;
}

/*
 * Remove the given node from the list, freeing it and it's value.
 */

void linked_list_remove(linked_list_t *self, linked_list_node_t *node)
{
    node->prev
        ? (node->prev->next = node->next)
        : (self->head = node->next);

    node->next
        ? (node->next->prev = node->prev)
        : (self->tail = node->prev);

    if (self->free)
        self->free(node->val);

    LIST_FREE(node);
    --self->len;
}

/*
 * Allocate a new list_iterator_t. NULL on failure.
 * Accepts a direction, which may be LIST_HEAD or linked_list_tAIL.
 */

list_iterator_t *linked_list_iterator_new(linked_list_t *list, linked_list_direction_t direction)
{
    linked_list_node_t *node = direction == LIST_HEAD
                                   ? list->head
                                   : list->tail;
    return linked_list_iterator_new_from_node(node, direction);
}

/*
 * Allocate a new list_iterator_t with the given start
 * node. NULL on failure.
 */

list_iterator_t *linked_list_iterator_new_from_node(linked_list_node_t *node, linked_list_direction_t direction)
{
    list_iterator_t *self;
    if (!(self = LIST_MALLOC(sizeof(list_iterator_t))))
        return NULL;
    self->next      = node;
    self->direction = direction;
    return self;
}

/*
 * Return the next linked_list_node_t or NULL when no more
 * nodes remain in the list.
 */

linked_list_node_t *linked_list_iterator_next(list_iterator_t *self)
{
    linked_list_node_t *curr = self->next;
    if (curr) {
        self->next = self->direction == LIST_HEAD
                         ? curr->next
                         : curr->prev;
    }
    return curr;
}

/*
 * Free the list iterator.
 */

void linked_list_iterator_destroy(list_iterator_t *self)
{
    LIST_FREE(self);
    self = NULL;
}

/*
 * Allocates a new linked_list_node_t. NULL on failure.
 */

linked_list_node_t *linked_list_node_new(void *val)
{
    linked_list_node_t *self;
    if (!(self = LIST_MALLOC(sizeof(linked_list_node_t))))
        return NULL;
    self->prev = NULL;
    self->next = NULL;
    self->val  = val;
    return self;
}