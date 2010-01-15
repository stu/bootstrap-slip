#ifndef DLIST_H
#define DLIST_H

#ifdef __cplusplus
extern "C"{
#endif

/* double linked list */
typedef struct DLElement
{
	struct DLElement	*prev;
	struct DLElement	*next;
	void				*data;
} DLElement;

typedef struct DList
{
	DLElement		*head;
	DLElement		*tail;

	int				size;

	void			(*destroy)(void *data);
} DList;

extern void FreeDList(DList *lst);
extern DList* NewDList(void (*destroy)(void *data));
extern void dlist_init(DList *list, void (*destroy)(void *data));
extern void dlist_destroy(DList *list);
extern int dlist_ins_next(DList *list, DLElement *element, const void *data);
extern int dlist_ins_prev(DList *list, DLElement *element, const void *data);
extern int dlist_remove(DList *list, DLElement *element, void **data);
extern void dlist_empty(DList *list);

#define dlist_size(list) ((list)->size)
#define dlist_head(list) ((list)->head)
#define dlist_tail(list) ((list)->tail)
#define dlist_is_head(element) ((element)->prev == NULL ? 1 : 0)
#define dlist_is_tail(element) ((element)->next == NULL ? 1 : 0)
#define dlist_data(element) ((element)->data)
#define dlist_next(element) ((element)->next)
#define dlist_prev(element) ((element)->prev)
#define dlist_ins(list, data) (dlist_ins_next((list), dlist_tail((list)), data))

#ifdef __cplusplus
};
#endif

#endif

