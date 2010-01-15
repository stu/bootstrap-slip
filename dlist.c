#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dlist.h"


/**
  * pre: list != NULL
  */
void FreeDList(DList *list)
{
	if(list == NULL)
		return;

	dlist_destroy(list);
	free(list);
}


DList* NewDList(void (*destroy)(void *data))
{
	DList *x = malloc(sizeof(DList));
	dlist_init(x, destroy);

	return x;
}

/**
  * pre: list != NULL
  */
void dlist_init(DList *list, void (*destroy)(void *data))
{
	list->size = 0;
	list->destroy = destroy;
	list->head = NULL;
	list->tail = NULL;

	return;
}

/**
  * pre: list  != NULL
  */
void dlist_empty(DList *list)
{
	void *data;

	while (dlist_size(list) > 0)
	{
		if (dlist_remove(list, dlist_tail(list), (void **)&data) == 0 && list-> destroy != NULL)
		{
			list->destroy(data);
		}
	}

	return;
}

/**
  * pre: list  != NULL
  */
void dlist_destroy(DList *list)
{
	dlist_empty(list);
	memset(list, 0, sizeof(DList));

	return;
}

/**
  * pre: list != NULL
  */
int dlist_ins_next(DList *list, DLElement *element, const void *data)
{

	DLElement	*new_element;

	if (element == NULL && dlist_size(list) != 0)
		return -1;

	if ((new_element = (DLElement *)malloc(sizeof(DLElement))) == NULL)
		return -1;

	new_element->data = (void *)data;

	if (dlist_size(list) == 0)
	{
		list->head = new_element;
		list->head->prev = NULL;
		list->head->next = NULL;
		list->tail = new_element;
	}
	else
	{
		new_element->next = element->next;
		new_element->prev = element;

		if (element->next == NULL)
			list->tail = new_element;
		else
			element->next->prev = new_element;

		element->next = new_element;
	}

	list->size++;

	return 0;

}


/**
  * pre: list != NULL
  */
int dlist_ins_prev(DList *list, DLElement *element, const void *data)
{
	DLElement	*new_element;

	if (element == NULL && dlist_size(list) != 0)
		return -1;

	if ((new_element = (DLElement *)malloc(sizeof(DLElement))) == NULL)
		return -1;

	new_element->data = (void *)data;

	if (dlist_size(list) == 0)
	{
		list->head = new_element;
		list->head->prev = NULL;
		list->head->next = NULL;
		list->tail = new_element;
	}
	else
	{
		new_element->next = element;
		new_element->prev = element->prev;

		if (element->prev == NULL)
			list->head = new_element;
		else
			element->prev->next = new_element;

		element->prev = new_element;
	}

	list->size++;

	return 0;

}

/**
  * pre: list != NULL
  */
int dlist_remove(DList *list, DLElement *element, void **data)
{
	if (element == NULL || dlist_size(list) == 0)
		return -1;

	*data = element->data;

	if (element == list->head)
	{
		list->head = element->next;

		if (list->head == NULL)
			list->tail = NULL;
		else
			element->next->prev = NULL;
	}
	else
	{
		element->prev->next = element->next;

		if (element->next == NULL)
			list->tail = element->prev;
		else
			element->next->prev = element->prev;
	}

	free(element);
	list->size--;
	return 0;
}
