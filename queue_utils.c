#include <stdio.h>
#include <cglm/cglm.h>
#include <assert.h>
#include "queue_utils.h"

struct queue_element *queue = NULL;
struct queue_element *coord_queue = NULL;

int queue_id = 0;

struct style head_style;
struct style wings_style;
struct style torso_style;
struct style feet_style;

struct queue_element *new_queue_element()
{
	struct queue_element *t = (struct queue_element *)malloc(sizeof(struct queue_element));
	assert(t);

	t->data = NULL;
	t->_next = NULL;
	return t;
}

int push_to_rendering_queue(struct queue_element **q, struct queue_element *e)
{
	e->_next = NULL;
	if (*q == NULL)
	{
		*q = e;
		goto set_id;
	}

	struct queue_element *t;
	for (t = *q; t->_next; t = t->_next)
		;
	t->_next = e;

set_id:

	e->_id = queue_id++;
	return e->_id;
}

void remove_from_rendering_queue(int id, struct queue_element **q)
{
	if (*q == NULL)
		return;

	struct queue_element *t, *u;

	for (u = NULL, t = *q; t && (t->_id != id); u = t, t = t->_next)
		;

	if (t->_id == id)
	{
		if (u)
			u->_next = t->_next;
		else
			*q = t->_next;
		free(t->data);
		free(t);
	}
	if(t == NULL) {
		printf("ELEMENT NOT FOUND IN QUEUE\n");
	}
}

void destroy_queue(struct queue_element **q)
{
	while (*q != NULL)
	{
		remove_from_rendering_queue((*q)->_id, q);
	}
	*q = NULL;
}
