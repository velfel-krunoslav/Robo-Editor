#include <cglm/cglm.h>
#include "queue_utils.h"

#ifndef _SHAPES_H
#define _SHAPES_H

extern void queue_head_type_0(struct queue_element **);
extern void queue_head_type_1(struct queue_element **);
extern void queue_head_type_2(struct queue_element **);

extern void queue_neck(struct queue_element **);

extern void queue_wings_type_0(struct queue_element **);
extern void queue_wings_type_1(struct queue_element **);
extern void queue_wings_type_2(struct queue_element **);

extern void queue_torso_type_0(struct queue_element **);
extern void queue_torso_type_1(struct queue_element **);

extern void queue_feet_type_0(int, struct queue_element **);
extern void queue_feet_type_1(int, struct queue_element **);

extern void queue_coord_x(struct queue_element **);
extern void queue_coord_y(struct queue_element **);
extern void queue_coord_z(struct queue_element **);
#endif