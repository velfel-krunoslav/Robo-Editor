#include <stdio.h>

#ifndef _QUEUE_UTILS_H
#define _QUEUE_UTILS_H

enum part {
	PART_HEAD = 0,
	PART_WINGS = 1,
	PART_TORSO = 2,
	PART_FEET = 3,
	PART_COORD_X = 4,
	PART_COORD_Y = 5,
	PART_COORD_Z = 6
};

struct queue_element
{
	float *data;
	enum part p;
	size_t size;
	int gl_mode;
	vec3 center_position;
	int _id;
	struct queue_element *_next;
};

struct style {
	int is_solid;
	vec3 color;
	char *filepath;
};

extern struct queue_element *queue;
extern int queue_id;

extern struct queue_element *coord_queue;
extern int queue_id;

extern struct style head_style;
extern struct style wings_style;
extern struct style torso_style;
extern struct style feet_style;

extern struct queue_element *new_queue_element();
extern int push_to_rendering_queue(struct queue_element **q, struct queue_element *e);
extern void remove_from_rendering_queue(int id, struct queue_element **q);
extern void destroy_queue(struct queue_element **q);

#endif