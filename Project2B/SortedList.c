/*
	NAME: MICHAEL ZHOU
	UID: 804663317
	EMAIL: MZJS96@GMAIL.COM
*/

#include "SortedList.h"

#include <stdlib.h>
#include <string.h>
#include <sched.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	SortedListElement_t *curr = list->next;
	//the next pointer in the head points at the first element in the list.
	while ((curr->key != NULL) && strcmp (element->key, curr->key))
	{
		if (opt_yield & INSERT_YIELD)
		{
			sched_yield();
		}
		curr = curr->next;
	}
	if (opt_yield & INSERT_YIELD)
	{
		sched_yield();
	}
	element->next = curr;
	element->prev = curr-> prev;
	curr->prev->next = element;
	curr->prev = element;
	return;
}

int SortedList_delete(SortedListElement_t *element)
{
	if((element->prev->next == element) && (element->next->prev == element))
	{
		if (opt_yield & DELETE_YIELD)
		{
			sched_yield();
		}
		element->prev->next = element->next;
		element->next->prev = element->prev;
		return 0;
	}
	return 1;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
	SortedListElement_t *curr = list->next;
	while(curr->key != NULL)
	{
		if (strcmp(key, curr->key) ==0)
		{
			return curr;
		}
		else if(opt_yield & LOOKUP_YIELD)
		{
			sched_yield();
		}
		curr = curr-> next;
	}
	return NULL;
}
/*
int SortedList_length(SortedList_t *list)
{
	if (list == NULL || list->key != NULL)
        return -1;
	SortedListElement_t* curr = list->next;

	int counter = 0;
	while(curr!=list)
	{
		if (opt_yield & LOOKUP_YIELD)
		{
			sched_yield();
		}
		counter += 1;
		if ((curr->next->prev != curr)||(curr->prev->next != curr))
		{
			return -1;
		}
		curr = curr->next;
	}
	return counter;
}
*/


int SortedList_length(SortedList_t *list){
	int count = 0;
	SortedListElement_t *cur = list->next;
	while(cur->key != NULL) {
		count = count + 1;
		if (opt_yield & LOOKUP_YIELD) {
			sched_yield();
		}
		if ((cur->next->prev != cur) || (cur->prev->next != cur))
			return -1;
		cur = cur->next;
	}
	return count;
}

