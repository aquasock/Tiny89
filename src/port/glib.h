/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_PORT_GLIB_H
#define TINY89_PORT_GLIB_H

/* Minimal single-threaded GLib compatibility used by the headless TiEmu
 * runtime. This is deliberately limited to the list and scalar facilities
 * referenced by the retained CPU/hardware modules. */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef void *gpointer;
typedef const void *gconstpointer;
typedef int gboolean;
typedef int gint;
typedef unsigned int guint;
typedef char gchar;

typedef struct _GList {
    gpointer data;
    struct _GList *next;
    struct _GList *prev;
} GList;

typedef struct _GNode {
    gpointer data;
    struct _GNode *next;
    struct _GNode *prev;
    struct _GNode *parent;
    struct _GNode *children;
} GNode;

typedef gint (*GCompareFunc)(gconstpointer a, gconstpointer b);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define GINT_TO_POINTER(value) ((gpointer)(intptr_t)(value))
#define GPOINTER_TO_INT(value) ((gint)(intptr_t)(value))
#define g_list_next(list) ((list) ? (list)->next : NULL)

#define G_LOCK_DEFINE(name)
#define G_LOCK(name) ((void)0)
#define G_UNLOCK(name) ((void)0)

#ifdef __cplusplus
#define G_BEGIN_DECLS extern "C" {
#define G_END_DECLS }
#else
#define G_BEGIN_DECLS
#define G_END_DECLS
#endif

static inline gpointer g_malloc(size_t size) { return malloc(size); }
static inline gpointer g_malloc0(size_t size) { return calloc(1, size); }
static inline gpointer g_realloc(gpointer data, size_t size) { return realloc(data, size); }
static inline void g_free(gpointer data) { free(data); }
#define g_new0(type, count) ((type *)calloc((count), sizeof(type)))

static inline GList *g_list_append(GList *list, gpointer data)
{
    GList *node = (GList *)calloc(1, sizeof(*node));
    GList *tail;
    if (!node) return list;
    node->data = data;
    if (!list) return node;
    for (tail = list; tail->next; tail = tail->next) {}
    tail->next = node;
    node->prev = tail;
    return list;
}

static inline guint g_list_length(GList *list)
{
    guint length = 0;
    for (; list; list = list->next) length++;
    return length;
}

static inline GList *g_list_nth(GList *list, guint n)
{
    while (list && n--) list = list->next;
    return list;
}

static inline GList *g_list_find_custom(GList *list, gconstpointer data, GCompareFunc compare)
{
    for (; list; list = list->next)
        if (compare(list->data, data) == 0) return list;
    return NULL;
}

static inline GList *g_list_delete_link(GList *list, GList *link)
{
    if (!link) return list;
    if (link->prev) link->prev->next = link->next;
    else list = link->next;
    if (link->next) link->next->prev = link->prev;
    free(link);
    return list;
}

static inline GList *g_list_remove(GList *list, gconstpointer data)
{
    GList *node;
    for (node = list; node; node = node->next)
        if (node->data == data) return g_list_delete_link(list, node);
    return list;
}

static inline void g_list_free(GList *list)
{
    while (list) {
        GList *next = list->next;
        free(list);
        list = next;
    }
}

#endif
