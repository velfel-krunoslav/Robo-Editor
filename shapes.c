#include <stdio.h>
#include <cglm/cglm.h>
#include <epoxy/gl.h>
#include <assert.h>
#include <string.h>

#include "shapes.h"
#include "queue_utils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void generate_rotational_body(float **base,
                              float **normals,
                              unsigned int number_of_sectors,
                              unsigned int number_of_stacks,
                              unsigned int closure,
                              float **dest,
                              size_t *size)
{

    mat4 rotate, rotate_result;
    vec4 vertex;

    float ***gen;
    int i, j;

    gen = (float ***)malloc(number_of_stacks * sizeof(float **));
    assert(gen);
    for (i = 0; i < number_of_stacks; i++)
    {
        gen[i] = (float **)malloc(number_of_sectors * sizeof(float *));
        assert(gen[i]);
        for (j = 0; j < number_of_sectors; j++)
        {
            gen[i][j] = (float *)malloc(6 * sizeof(float));
            assert(gen[i][j]);
        }
    }

    glm_mat4_identity(rotate);

    for (i = 0; i < number_of_stacks; i++)
    {
        glm_rotate_y(rotate, i * (2 * M_PI / (number_of_stacks - 1)), rotate_result);

        for (j = 0; j < number_of_sectors; j++)
        {
            vec4 v = {base[j][0], base[j][1], base[j][2], 1.0f};
            glm_mat4_mulv(rotate_result, v, vertex);

            memcpy(gen[i][j], vertex, 3 * sizeof(float));
        }

        for (j = 0; j < number_of_sectors; j++)
        {
            vec4 v = {normals[j][0], normals[j][1], normals[j][2], 1.0f};
            glm_mat4_mulv(rotate_result, v, vertex);
            vertex[3] = 0.0f;
            glm_normalize(vertex);
            memcpy(gen[i][j] + 3, vertex, 3 * sizeof(float));
        }
    }

    float *data = (float *)malloc((number_of_stacks - closure) * number_of_sectors * 6 * 2 * sizeof(float));
    assert(data);

    int index = 0;
    for (i = 0; i < (number_of_stacks - closure - 1); i++)
    {
        if (i % 2 == 0)
        {
            for (j = 0; j < number_of_sectors; j++)
            {
                memcpy(data + index, gen[i + 1][j], 6 * sizeof(float));
                index += 6;
                memcpy(data + index, gen[i][j], 6 * sizeof(float));
                index += 6;
            }
        }
        else
        {
            for (j = number_of_sectors - 1; j >= 0; j--)
            {
                memcpy(data + index, gen[i][j], 6 * sizeof(float));
                index += 6;
                memcpy(data + index, gen[i + 1][j], 6 * sizeof(float));
                index += 6;
            }
        }
    }

    if (!closure)
    {
        if (i % 2 == 0)
        {
            for (j = 0; j < number_of_sectors; j++)
            {
                memcpy(data + index, gen[i][j], 6 * sizeof(float));
                index += 6;
                memcpy(data + index, gen[0][j], 6 * sizeof(float));
                index += 6;
            }
        }
        else
        {
            for (j = number_of_sectors - 1; j >= 0; j--)
            {
                memcpy(data + index, gen[i][j], 6 * sizeof(float));
                index += 6;
                memcpy(data + index, gen[0][j], 6 * sizeof(float));
                index += 6;
            }
        }
    }
    else
    {
        mat4 rotation;
        vec3 v3;
        vec4 v4;
        glm_mat4_identity(rotation);
        glm_rotate_y(rotation, -(2 * M_PI * closure / number_of_stacks), rotation);

        for (int i = 0; i < index; i += 3)
        {
            v4[0] = data[i];
            v4[1] = data[i + 1];
            v4[2] = data[i + 2];
            v4[3] = 1.0f;

            glm_mat4_mulv(rotation, v4, v4);
            data[i] = v4[0];
            data[i + 1] = v4[1];
            data[i + 2] = v4[2];
        }
    }

    for (i = 0; i < number_of_stacks; i++)
    {
        for (int j = 0; j < number_of_sectors; j++)
        {
            free(gen[i][j]);
        }
        free(gen[i]);
    }
    free(gen);
    *dest = data;
    *size = (index * sizeof(float));
}

void generate_sphere(float r,
                     unsigned int closure,
                     float **data,
                     size_t *data_size)
{
    float **sphere_base, **normals, step;

    int i, number_of_sectors, number_of_stacks, _closure;

    number_of_sectors = r * 12;
    number_of_stacks = number_of_sectors * 2;

    step = M_PI / number_of_sectors;

    _closure = (int)(number_of_sectors * closure / 100.0f);

    sphere_base = (float **)malloc(number_of_sectors * sizeof(float *));
    assert(sphere_base);

    normals = (float **)malloc(number_of_sectors * sizeof(float *));
    assert(normals);

    for (i = 0; i < number_of_sectors; i++)
    {
        sphere_base[i] = (float *)malloc(3 * sizeof(float));
        assert(sphere_base[i]);
        normals[i] = (float *)malloc(3 * sizeof(float));
        assert(normals[i]);
    }

    for (i = 0; i < number_of_sectors; i++)
    {

        sphere_base[i][0] = cos(i * step + (M_PI / 2.0));
        sphere_base[i][1] = sin(i * step + (M_PI / 2.0));
        sphere_base[i][2] = 0;

        glm_vec3_scale(sphere_base[i], r, sphere_base[i]);

        normals[i][0] = cos(i * step + (M_PI / 2.0));
        normals[i][1] = sin(i * step + (M_PI / 2.0));
        normals[i][2] = 0;
    }

    generate_rotational_body(sphere_base, normals, number_of_sectors, number_of_stacks, _closure, data, data_size);

    for (i = 0; i < number_of_sectors; i++)
    {
        free(sphere_base[i]);
        free(normals[i]);
    }
    free(sphere_base);
    free(normals);
}

static void allocate_generators(float ***_g, float ***_n, int n)
{
    float **generator = (float **)malloc(n * sizeof(float *));
    float **normals = (float **)malloc(n * sizeof(float *));

    assert(generator);
    assert(normals);

    for (int i = 0; i < n; i++)
    {
        generator[i] = (float *)malloc(3 * sizeof(float));
        assert(generator[i]);
        generator[i][0] = generator[i][1] = generator[i][2] = 0.0f;

        normals[i] = (float *)malloc(3 * sizeof(float));
        assert(normals[i]);
        normals[i][0] = normals[i][1] = normals[i][2] = 0.0f;
    }

    *_g = generator;
    *_n = normals;
}

static void free_generators(float ***_g, float ***_n, int n)
{

    float **generator = *_g, **normals = *_n;
    for (int i = 0; i < n; i++)
    {
        free(generator[i]);

        free(normals[i]);
    }
    free(generator);
    free(normals);
}

void queue_neck(struct queue_element **q)
{
    struct queue_element *neck = new_queue_element();
    float **generator, **normals;

    allocate_generators(&generator, &normals, 4);

    memcpy(generator[0], (vec3){0.0f, 0.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.5f, 0.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){0.75f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[3], (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.0f, 1.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){-0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){-0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[3], (vec3){0.0f, -1.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 4, 20, 0, &(neck->data), &(neck->size));

    memcpy(&(neck->center_position), (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));

    neck->gl_mode = GL_TRIANGLE_STRIP;
    neck->p = PART_HEAD;
    push_to_rendering_queue(q,neck);

    free_generators(&generator, &normals, 4);
}

void queue_head_type_0(struct queue_element **q)
{
    struct queue_element *head_type_0_part_0 = new_queue_element();
    struct queue_element *head_type_0_part_1 = new_queue_element();
    struct queue_element *head_type_0_part_2 = new_queue_element();

    float **generator, **normals;
    allocate_generators(&generator, &normals, 11);

    for (int i = 0; i < 11; i++)
    {
        generator[i] = (float *)malloc(3 * sizeof(float));
        assert(generator[i]);
        generator[i][0] = generator[i][1] = generator[i][2] = 0.0f;

        normals[i] = (float *)malloc(3 * sizeof(float));
        assert(normals[i]);
        normals[i][0] = normals[i][1] = normals[i][2] = 0.0f;
    }

    for (int i = 0; i < 8; i++)
    {
        generator[i + 2][0] = cosf((M_PI / 16.0f) * i) * 0.5;
        generator[i + 2][1] = sinf((M_PI / 16.0f) * i) * 0.5;
        generator[i + 2][2] = 0;

        normals[i + 2][0] = cosf((M_PI / 16.0f) * i);
        normals[i + 2][1] = sinf((M_PI / 16.0f) * i);
        normals[i + 2][2] = 0;

        glm_vec3_add(generator[i + 2], (vec3){0.5f, 1.5f, 0.0f}, generator[i + 2]);
        glm_vec3_add(normals[i + 2], (vec3){0.5f, 1.5f, 0.0f}, normals[i + 2]);
    }

    memcpy(generator[0], (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.5f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[10], (vec3){0.0f, 2.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){-1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){-1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[10], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 11, 40, 0, &(head_type_0_part_0->data), &(head_type_0_part_0->size));

    memcpy(&(head_type_0_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    head_type_0_part_0->gl_mode = GL_TRIANGLE_STRIP;
    head_type_0_part_0->p = PART_HEAD;
    push_to_rendering_queue(q,head_type_0_part_0);

    memcpy(generator[0], (vec3){1.34f, 1.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){2.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    generate_rotational_body(generator, normals, 2, 40, 6, &(head_type_0_part_1->data), &(head_type_0_part_1->size));

    memcpy(&(head_type_0_part_1->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    head_type_0_part_1->gl_mode = GL_TRIANGLE_STRIP;
    head_type_0_part_1->p = PART_HEAD;
    push_to_rendering_queue(q,head_type_0_part_1);

    memcpy(generator[0], (vec3){0.0f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){1.0f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){1.34f, 1.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.0f, 1.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 3, 40, 0, &(head_type_0_part_2->data), &(head_type_0_part_2->size));

    memcpy(&(head_type_0_part_2->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    head_type_0_part_2->gl_mode = GL_TRIANGLE_STRIP;
    head_type_0_part_2->p = PART_HEAD;
    push_to_rendering_queue(q,head_type_0_part_2);

    free_generators(&generator, &normals, 11);
}

void queue_head_type_1(struct queue_element **q)
{

    struct queue_element *head_type_1_part_0 = new_queue_element();
    struct queue_element *head_type_1_part_1 = new_queue_element();
    struct queue_element *head_type_1_part_2 = new_queue_element();

    float **generator, **normals;
    allocate_generators(&generator, &normals, 11);

    for (int i = 0; i < 8; i++)
    {
        generator[i + 2][0] = cosf((M_PI / 16.0f) * i) * 0.5;
        generator[i + 2][1] = sinf((M_PI / 16.0f) * i) * 0.5;
        generator[i + 2][2] = 0;

        normals[i + 2][0] = cosf((M_PI / 16.0f) * i);
        normals[i + 2][1] = sinf((M_PI / 16.0f) * i);
        normals[i + 2][2] = 0;

        glm_vec3_add(generator[i + 2], (vec3){0.5f, 1.5f, 0.0f}, generator[i + 2]);
        glm_vec3_add(normals[i + 2], (vec3){0.5f, 1.5f, 0.0f}, normals[i + 2]);
    }

    memcpy(generator[0], (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.5f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[10], (vec3){0.0f, 2.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){-1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){-1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[10], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 11, 40, 0, &(head_type_1_part_0->data), &(head_type_1_part_0->size));

    memcpy(&(head_type_1_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    head_type_1_part_0->gl_mode = GL_TRIANGLE_STRIP;
    head_type_1_part_0->p = PART_HEAD;
    push_to_rendering_queue(q,head_type_1_part_0);

    memcpy(generator[0], (vec3){1.2f, 2.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){1.0f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){1.2f, -0.2f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 3, 40, 6, &(head_type_1_part_1->data), &(head_type_1_part_1->size));

    memcpy(&(head_type_1_part_1->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    head_type_1_part_1->gl_mode = GL_TRIANGLE_STRIP;
    head_type_1_part_1->p = PART_HEAD;
    push_to_rendering_queue(q,head_type_1_part_1);

    free_generators(&generator, &normals, 11);
}
void queue_head_type_2(struct queue_element **q)
{

    struct queue_element *head_type_2_part_0 = new_queue_element();

    float **generator, **normals;
    allocate_generators(&generator, &normals, 16);

    for (int i = 0; i < 8; i++)
    {
        generator[i][0] = cos(M_PI / 2.0f - (M_PI / 16.0f) * i) * 0.25;
        generator[i][1] = sin(M_PI / 2.0f - (M_PI / 16.0f) * i) * 0.25 + 3.0f;
        generator[i][2] = 0;

        normals[i][0] = cos(M_PI / 2.0f - (M_PI / 16.0f) * i);
        normals[i][1] = sin(M_PI / 2.0f - (M_PI / 16.0f) * i);
        normals[i][2] = 0;
    }

    memcpy(generator[8], (vec3){0.25f, 2.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[9], (vec3){0.5f, 2.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[10], (vec3){0.5f, 2.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[11], (vec3){1.0f, 2.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[12], (vec3){1.0f, 0.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[13], (vec3){0.5f, 0.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[14], (vec3){0.5f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[15], (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[8], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[9], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[10], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[11], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[12], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[13], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[14], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[15], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 16, 40, 0, &(head_type_2_part_0->data), &(head_type_2_part_0->size));

    memcpy(&(head_type_2_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    head_type_2_part_0->gl_mode = GL_TRIANGLE_STRIP;
    head_type_2_part_0->p = PART_HEAD;
    push_to_rendering_queue(q,head_type_2_part_0);
    free_generators(&generator, &normals, 16);
}

void queue_wings_type_0(struct queue_element **q)
{

    struct queue_element *wings_type_0_part_0 = new_queue_element();
    struct queue_element *wings_type_0_part_1 = new_queue_element();
    struct queue_element *wings_type_0_part_2_wrapper = new_queue_element();

    struct queue_element *wings_type_0_part_0_mirror = new_queue_element();
    struct queue_element *wings_type_0_part_1_mirror = new_queue_element();
    struct queue_element *wings_type_0_part_2_wrapper_mirror = new_queue_element();

    float *plane = (float *)malloc(6 * 3 * sizeof(float));
    assert(plane);

    memcpy(plane, (vec3){-3.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 3, (vec3){-7.0f, -6.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 6, (vec3){-6.5f, -8.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 9, (vec3){-1.5f, -4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 12, (vec3){0.0f, -4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 15, (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(&(wings_type_0_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_0_part_0->gl_mode = GL_TRIANGLE_FAN;

    wings_type_0_part_0->data = (float *)calloc(6 * 6, sizeof(float));
    assert(wings_type_0_part_0->data);

    wings_type_0_part_0->size = 6 * 6 * sizeof(float);

    wings_type_0_part_0_mirror->gl_mode = GL_TRIANGLE_FAN;

    wings_type_0_part_0_mirror->data = (float *)calloc(6 * 6, sizeof(float));
    assert(wings_type_0_part_0_mirror->data);

    wings_type_0_part_0_mirror->size = 6 * 6 * sizeof(float);

    memcpy(&(wings_type_0_part_0_mirror->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_0_part_1->gl_mode = GL_TRIANGLE_FAN;
    wings_type_0_part_1->data = (float *)calloc(6 * 6, sizeof(float));
    assert(wings_type_0_part_1->data);
    wings_type_0_part_1->size = 6 * 6 * sizeof(float);

    memcpy(&(wings_type_0_part_1->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_0_part_1_mirror->gl_mode = GL_TRIANGLE_FAN;
    wings_type_0_part_1_mirror->data = (float *)calloc(6 * 6, sizeof(float));
    assert(wings_type_0_part_1_mirror->data);
    wings_type_0_part_1_mirror->size = 6 * 6 * sizeof(float);

    memcpy(&(wings_type_0_part_1_mirror->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 6; i++)
    {
        vec3 t;
        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -1.1f}, t);
        memcpy(wings_type_0_part_0->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_0_part_0->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -0.9f}, t);
        memcpy(wings_type_0_part_1->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_0_part_1->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -1.1f}, t);
        glm_vec3_mul(t, (vec3){-1.0f, 1.0f, 1.0f}, t);
        memcpy(wings_type_0_part_0_mirror->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_0_part_0_mirror->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -0.9f}, t);
        glm_vec3_mul(t, (vec3){-1.0f, 1.0f, 1.0f}, t);
        memcpy(wings_type_0_part_1_mirror->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_0_part_1_mirror->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));
    }

    wings_type_0_part_2_wrapper->data = (float *)calloc(6 * 6 * 2, sizeof(float));
    wings_type_0_part_2_wrapper->size = 6 * 6 * 2 * sizeof(float);
    wings_type_0_part_2_wrapper->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(wings_type_0_part_2_wrapper->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_0_part_2_wrapper_mirror->data = (float *)calloc(6 * 6 * 2, sizeof(float));
    wings_type_0_part_2_wrapper_mirror->size = 6 * 6 * 2 * sizeof(float);
    wings_type_0_part_2_wrapper_mirror->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(wings_type_0_part_2_wrapper_mirror->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    int index = 0, i;
    for (i = 0; i < 6; i++)
    {
        memcpy(wings_type_0_part_2_wrapper->data + index, wings_type_0_part_0->data + i * 6, 3 * sizeof(float));
        memcpy(wings_type_0_part_2_wrapper_mirror->data + index, wings_type_0_part_0_mirror->data + i * 6, 3 * sizeof(float));

        index += 6;
        memcpy(wings_type_0_part_2_wrapper->data + index, wings_type_0_part_1->data + i * 6, 3 * sizeof(float));
        memcpy(wings_type_0_part_2_wrapper_mirror->data + index, wings_type_0_part_1_mirror->data + i * 6, 3 * sizeof(float));
        index += 6;
    }
    wings_type_0_part_0->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_0_part_0);
    wings_type_0_part_1->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_0_part_1);
    wings_type_0_part_2_wrapper->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_0_part_2_wrapper);
    wings_type_0_part_0_mirror->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_0_part_0_mirror);
    wings_type_0_part_1_mirror->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_0_part_1_mirror);
    wings_type_0_part_2_wrapper_mirror->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_0_part_2_wrapper_mirror);

    free(plane);
}
void queue_wings_type_1(struct queue_element **q)
{

    struct queue_element *wings_type_1_part_0 = new_queue_element();
    struct queue_element *wings_type_1_part_1 = new_queue_element();
    struct queue_element *wings_type_1_part_2_wrapper = new_queue_element();

    struct queue_element *wings_type_1_part_0_mirror = new_queue_element();
    struct queue_element *wings_type_1_part_1_mirror = new_queue_element();
    struct queue_element *wings_type_1_part_2_wrapper_mirror = new_queue_element();

    float *plane = (float *)malloc(40 * 3 * sizeof(float));
    assert(plane);

    for (int i = 0; i < 10; i++)
    {
        plane[27 - i * 3] = cos((2 * M_PI / 3.0f) + ((i + 1) * (M_PI / 22.6f))) * 5;
        plane[28 - i * 3] = sin((2 * M_PI / 3.0f) + ((i + 1) * (M_PI / 22.6f))) * 5;
        plane[29 - i * 3] = 0.0f;
    }

    for (int i = 0; i < 20; i++)
    {
        plane[30 + i * 3] = cos(M_PI + (i * (M_PI / 38.0f))) * 3;
        plane[31 + i * 3] = sin(M_PI + (i * (M_PI / 38.0f))) * 3 + 3;
        plane[32 + i * 3] = 0.0f;
    }

    for (int i = 10; i < 20; i++)
    {
        plane[117 - (i - 10) * 3] = cos((2 * M_PI / 3.0f) + (i * (M_PI / 22.6f))) * 5;
        plane[118 - (i - 10) * 3] = sin((2 * M_PI / 3.0f) + (i * (M_PI / 22.6f))) * 5;
        plane[119 - (i - 10) * 3] = 0.0f;
    }

    memcpy(&(wings_type_1_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_1_part_0->gl_mode = GL_TRIANGLE_FAN;

    wings_type_1_part_0->data = (float *)calloc(40 * 6, sizeof(float));
    assert(wings_type_1_part_0->data);

    wings_type_1_part_0->size = 40 * 6 * sizeof(float);

    wings_type_1_part_0_mirror->gl_mode = GL_TRIANGLE_FAN;

    wings_type_1_part_0_mirror->data = (float *)calloc(40 * 6, sizeof(float));
    assert(wings_type_1_part_0_mirror->data);

    wings_type_1_part_0_mirror->size = 40 * 6 * sizeof(float);

    memcpy(&(wings_type_1_part_0_mirror->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_1_part_1->gl_mode = GL_TRIANGLE_FAN;
    wings_type_1_part_1->data = (float *)calloc(40 * 6, sizeof(float));
    assert(wings_type_1_part_1->data);
    wings_type_1_part_1->size = 40 * 6 * sizeof(float);

    memcpy(&(wings_type_1_part_1->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_1_part_1_mirror->gl_mode = GL_TRIANGLE_FAN;
    wings_type_1_part_1_mirror->data = (float *)calloc(40 * 6, sizeof(float));
    assert(wings_type_1_part_1_mirror->data);
    wings_type_1_part_1_mirror->size = 40 * 6 * sizeof(float);

    memcpy(&(wings_type_1_part_1_mirror->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    mat4 scale;
    glm_mat4_identity(scale);
    glm_mat4_scale(scale, 0.8f);
    for (int i = 0; i < 40; i++)
    {
        vec3 t;
        vec4 t4;
        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -1.1}, t);
        memcpy(t4, t, sizeof(vec3));
        t4[3] = 1.0f;
        glm_mat4_mulv(scale, t4, t4);
        memcpy(wings_type_1_part_0->data + i * 6, t4, sizeof(vec3));
        memcpy(wings_type_1_part_0->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -2.1f}, t);
        memcpy(wings_type_1_part_1->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_1_part_1->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -1.1f}, t);
        memcpy(t4, t, sizeof(vec3));
        t4[3] = 1.0f;
        glm_mat4_mulv(scale, t4, t4);
        glm_vec3_mul(t4, (vec4){-1.0f, 1.0f, 1.0f, 1.0f}, t);
        memcpy(wings_type_1_part_0_mirror->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_1_part_0_mirror->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -2.1f}, t);
        glm_vec3_mul(t, (vec3){-1.0f, 1.0f, 1.0f}, t);
        memcpy(wings_type_1_part_1_mirror->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_1_part_1_mirror->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));
    }

    wings_type_1_part_2_wrapper->data = (float *)calloc(40 * 6 * 2, sizeof(float));
    wings_type_1_part_2_wrapper->size = 40 * 6 * 2 * sizeof(float);
    wings_type_1_part_2_wrapper->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(wings_type_1_part_2_wrapper->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_1_part_2_wrapper_mirror->data = (float *)calloc(40 * 6 * 2, sizeof(float));
    wings_type_1_part_2_wrapper_mirror->size = 40 * 6 * 2 * sizeof(float);
    wings_type_1_part_2_wrapper_mirror->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(wings_type_1_part_2_wrapper_mirror->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 20; i++)
    {
        plane[57 - i * 3] = cos((2 * M_PI / 3.0f) + ((i + 1) * (M_PI / 22.6f))) * 5;
        plane[58 - i * 3] = sin((2 * M_PI / 3.0f) + ((i + 1) * (M_PI / 22.6f))) * 5;
        plane[59 - i * 3] = 0.0f;
    }

    for (int i = 0; i < 20; i++)
    {
        plane[60 + i * 3] = cos(M_PI + (i * (M_PI / 38.0f))) * 3;
        plane[61 + i * 3] = sin(M_PI + (i * (M_PI / 38.0f))) * 3 + 3;
        plane[62 + i * 3] = 0.0f;
    }

    int index = 0, i;
    for (i = 0; i < 40; i++)
    {
        memcpy(wings_type_1_part_2_wrapper->data + index, wings_type_1_part_0->data + i * 6, 3 * sizeof(float));
        memcpy(wings_type_1_part_2_wrapper_mirror->data + index, wings_type_1_part_0_mirror->data + i * 6, 3 * sizeof(float));

        index += 6;
        memcpy(wings_type_1_part_2_wrapper->data + index, wings_type_1_part_1->data + i * 6, 3 * sizeof(float));
        memcpy(wings_type_1_part_2_wrapper_mirror->data + index, wings_type_1_part_1_mirror->data + i * 6, 3 * sizeof(float));
        index += 6;
    }
    wings_type_1_part_0->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_1_part_0);
    wings_type_1_part_1->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_1_part_1);
    wings_type_1_part_2_wrapper->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_1_part_2_wrapper);
    wings_type_1_part_0_mirror->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_1_part_0_mirror);
    wings_type_1_part_1_mirror->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_1_part_1_mirror);
    wings_type_1_part_2_wrapper_mirror->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_1_part_2_wrapper_mirror);

    free(plane);
}
void queue_wings_type_2(struct queue_element **q)
{

    struct queue_element *wings_type_2_part_0 = new_queue_element();
    struct queue_element *wings_type_2_part_1 = new_queue_element();
    struct queue_element *wings_type_2_part_2_wrapper = new_queue_element();
    struct queue_element *wings_type_2_part_3 = new_queue_element();
    struct queue_element *wings_type_2_part_4 = new_queue_element();

    float **generator, **normals;
    allocate_generators(&generator, &normals, 4);

    memcpy(generator[0], (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.5f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){0.25f, -2.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[3], (vec3){0.0f, -2.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.0f, 1.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[3], (vec3){0.0f, -1.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 4, 20, 0, &(wings_type_2_part_3->data), &(wings_type_2_part_3->size));

    memcpy(&(wings_type_2_part_3->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_2_part_3->gl_mode = GL_TRIANGLE_STRIP;

    generate_rotational_body(generator, normals, 4, 20, 0, &(wings_type_2_part_4->data), &(wings_type_2_part_4->size));

    memcpy(&(wings_type_2_part_4->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_2_part_4->gl_mode = GL_TRIANGLE_STRIP;

    for (int i = 0; i < 4 * 40; i++)
    {
        glm_vec3_add((wings_type_2_part_3->data) + i * 6, (vec3){-3, -4.0f, -2.0f}, (wings_type_2_part_3->data) + i * 6);
        glm_vec3_add((wings_type_2_part_4->data) + i * 6, (vec3){3, -4.0f, -2.0f}, (wings_type_2_part_4->data) + i * 6);
    }

    float *plane = (float *)malloc(40 * 3 * sizeof(float));
    assert(plane);

    for (int i = 0; i < 20; i++)
    {
        plane[i * 3] = cos(i * (M_PI / 19.0) + M_PI / 2.0f) - 3.0f;
        plane[i * 3 + 1] = 0.0f;
        plane[i * 3 + 2] = sin(i * (M_PI / 19.0) + M_PI / 2.0f);
    }

    for (int i = 0; i < 20; i++)
    {
        plane[(i + 20) * 3] = cos(3 * M_PI / 2.0f + i * (M_PI / 19.0)) + 3.0f;
        plane[(i + 20) * 3 + 1] = 0.0f;
        plane[(i + 20) * 3 + 2] = sin(3 * M_PI / 2.0f + i * (M_PI / 19.0));
    }

    memcpy(&(wings_type_2_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    wings_type_2_part_0->gl_mode = GL_TRIANGLE_FAN;

    wings_type_2_part_0->data = (float *)calloc(40 * 6, sizeof(float));
    assert(wings_type_2_part_0->data);

    wings_type_2_part_0->size = 40 * 6 * sizeof(float);

    wings_type_2_part_1->gl_mode = GL_TRIANGLE_FAN;
    wings_type_2_part_1->data = (float *)calloc(40 * 6, sizeof(float));
    assert(wings_type_2_part_1->data);
    wings_type_2_part_1->size = 40 * 6 * sizeof(float);

    memcpy(&(wings_type_2_part_1->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 40; i++)
    {
        vec3 t;
        glm_vec3_add(3 * i + plane, (vec3){0.0f, -4.0f, -2.0f}, t);
        memcpy(wings_type_2_part_0->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_2_part_0->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -2.0f}, t);
        memcpy(wings_type_2_part_1->data + i * 6, t, sizeof(vec3));
        memcpy(wings_type_2_part_1->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));
    }

    wings_type_2_part_2_wrapper->data = (float *)calloc(40 * 6 * 2, sizeof(float));
    wings_type_2_part_2_wrapper->size = 40 * 6 * 2 * sizeof(float);
    wings_type_2_part_2_wrapper->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(wings_type_2_part_2_wrapper->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    int index = 0, i;
    for (i = 0; i < 39; i++)
    {
        memcpy(wings_type_2_part_2_wrapper->data + index, wings_type_2_part_0->data + i * 6, 3 * sizeof(float));
        index += 6;
        memcpy(wings_type_2_part_2_wrapper->data + index, wings_type_2_part_1->data + i * 6, 3 * sizeof(float));
        index += 6;
    }

    memcpy(wings_type_2_part_2_wrapper->data + index, wings_type_2_part_0->data + 0, 3 * sizeof(float));
    index += 6;
    memcpy(wings_type_2_part_2_wrapper->data + index, wings_type_2_part_1->data + 0, 3 * sizeof(float));
    index += 6;
    wings_type_2_part_0->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_2_part_0);
    wings_type_2_part_1->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_2_part_1);
    wings_type_2_part_2_wrapper->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_2_part_2_wrapper);
    wings_type_2_part_3->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_2_part_3);
    wings_type_2_part_4->p = PART_WINGS;
    push_to_rendering_queue(q,wings_type_2_part_4);

    free_generators(&generator, &normals, 4);
    free(plane);
}

void queue_torso_type_0(struct queue_element **q)
{
    struct queue_element *torso_type_0_part_0 = new_queue_element();
    struct queue_element *torso_type_0_part_1 = new_queue_element();
    struct queue_element *torso_type_0_part_2_wrapper = new_queue_element();
    struct queue_element *torso_type_0_part_3 = new_queue_element();

    float **generator, **normals;
    allocate_generators(&generator, &normals, 11);

    for (int i = 0; i < 11; i++)
    {
        generator[10 - i][0] = cosf(M_PI - (M_PI / 33.0f) * i) * 3;
        generator[10 - i][1] = sinf(M_PI - (M_PI / 33.0f) * i) * 3;
        generator[10 - i][2] = 0;

        normals[10 - i][0] = cosf(M_PI - (M_PI / 33.0f) * i);
        normals[10 - i][1] = sinf(M_PI - (M_PI / 33.0f) * i);
        normals[10 - i][2] = 0;
    }

    float *plane = (float *)malloc(29 * 3 * sizeof(float));
    assert(plane);

    memcpy(plane, (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 3, (vec3){2.5f, 0.0f, 0.0f}, sizeof(vec3));
    for (int i = 0; i < 11; i++)
    {
        glm_vec3_add(generator[i], (vec3){4.5f, -3.5f, 0.0f}, 3 * i + plane + 6);
    }
    memcpy(plane + 39, (vec3){0.75f, -3.5f, 0.0f}, sizeof(vec3));
    memcpy(plane + 42, (vec3){0.5f, -4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 45, (vec3){-0.5f, -4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 48, (vec3){-0.75f, -3.5f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 11; i++)
    {
        glm_vec3_mul(generator[10 - i], (vec3){-1.0f, 1.0, 1.0f}, generator[10 - i]);
        glm_vec3_add(generator[10 - i], (vec3){-4.5f, -3.5f, 0.0f}, 3 * i + plane + 51);
    }

    memcpy(plane + 84, (vec3){-2.5f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(&(torso_type_0_part_0->center_position), (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));

    torso_type_0_part_0->gl_mode = GL_TRIANGLE_FAN;

    torso_type_0_part_0->data = (float *)malloc(29 * 6 * sizeof(float));
    assert(torso_type_0_part_0->data);

    torso_type_0_part_0->size = 29 * 6 * sizeof(float);

    memcpy(&(torso_type_0_part_1->center_position), (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));

    torso_type_0_part_1->gl_mode = GL_TRIANGLE_FAN;

    torso_type_0_part_1->data = (float *)malloc(29 * 6 * sizeof(float));
    assert(torso_type_0_part_1->data);

    torso_type_0_part_1->size = 29 * 6 * sizeof(float);

    for (int i = 0; i < 29; i++)
    {
        vec3 t;
        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, 1.0f}, t);
        memcpy(torso_type_0_part_0->data + i * 6, t, sizeof(vec3));
        memcpy(torso_type_0_part_0->data + i * 6 + 3, (vec3){0.0f, 0.0f, -1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -1.0f}, t);
        memcpy(torso_type_0_part_1->data + i * 6, t, sizeof(vec3));
        memcpy(torso_type_0_part_1->data + i * 6 + 3, (vec3){0.0f, 0.0f, -1.0f}, sizeof(vec3));
    }

    torso_type_0_part_2_wrapper->data = (float *)calloc(29 * 6 * 2, sizeof(float));
    torso_type_0_part_2_wrapper->size = 29 * 6 * 2 * sizeof(float);
    torso_type_0_part_2_wrapper->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(torso_type_0_part_2_wrapper->center_position), (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));

    int index = 0, i;
    for (i = 0; i < 29; i++)
    {
        memcpy(torso_type_0_part_2_wrapper->data + index, torso_type_0_part_0->data + i * 6, 3 * sizeof(float));
        index += 6;
        memcpy(torso_type_0_part_2_wrapper->data + index, torso_type_0_part_1->data + i * 6, 3 * sizeof(float));
        index += 6;
    }

    /* 
	NORMALS MISSING
	*/
    torso_type_0_part_0->p = PART_TORSO;

    push_to_rendering_queue(q,torso_type_0_part_0);
    torso_type_0_part_1->p = PART_TORSO;
    push_to_rendering_queue(q,torso_type_0_part_1);
    torso_type_0_part_2_wrapper->p = PART_TORSO;
    push_to_rendering_queue(q,torso_type_0_part_2_wrapper);

    generate_sphere(1.5, 0, &(torso_type_0_part_3->data), &(torso_type_0_part_3->size));
    torso_type_0_part_3->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(torso_type_0_part_3->center_position), (vec3){0.0f, -2.5f, 0.0f}, sizeof(vec3));
    torso_type_0_part_3->p = PART_TORSO;
    push_to_rendering_queue(q,torso_type_0_part_3);

    free_generators(&generator, &normals, 11);
    free(plane);
}
void queue_torso_type_1(struct queue_element **q)
{
    struct queue_element *torso_type_1_part_0 = new_queue_element();
    struct queue_element *torso_type_1_part_1 = new_queue_element();
    struct queue_element *torso_type_1_part_2_wrapper = new_queue_element();
    struct queue_element *torso_type_1_part_3 = new_queue_element();

    float **generator, **normals;
    allocate_generators(&generator, &normals, 11);

    memcpy(generator[0], (vec3){0.0f, 0.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.5f, 0.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){0.75f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[3], (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.0f, 1.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){-0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){-0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[3], (vec3){0.0f, -1.0f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 11; i++)
    {
        generator[10 - i][0] = cosf(M_PI - (M_PI / 33.0f) * i) * 3;
        generator[10 - i][1] = sinf(M_PI - (M_PI / 33.0f) * i) * 3;
        generator[10 - i][2] = 0;

        normals[10 - i][0] = cosf(M_PI - (M_PI / 33.0f) * i);
        normals[10 - i][1] = sinf(M_PI - (M_PI / 33.0f) * i);
        normals[10 - i][2] = 0;
    }

    float *plane = (float *)malloc(29 * 3 * sizeof(float));
    assert(plane);

    memcpy(plane, (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));
    memcpy(plane + 3, (vec3){2.5f, 0.0f, 0.0f}, sizeof(vec3));
    for (int i = 0; i < 11; i++)
    {
        glm_vec3_add(generator[i], (vec3){4.5f, -3.5f, 0.0f}, 3 * i + plane + 6);
    }
    memcpy(plane + 39, (vec3){0.75f, -3.5f, 0.0f}, sizeof(vec3));
    memcpy(plane + 42, (vec3){0.5f, -4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 45, (vec3){-0.5f, -4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 48, (vec3){-0.75f, -3.5f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 11; i++)
    {
        glm_vec3_mul(generator[10 - i], (vec3){-1.0f, 1.0, 1.0f}, generator[10 - i]);
        glm_vec3_add(generator[10 - i], (vec3){-4.5f, -3.5f, 0.0f}, 3 * i + plane + 51);
    }

    memcpy(plane + 84, (vec3){-2.5f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(&(torso_type_1_part_0->center_position), (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));

    torso_type_1_part_0->gl_mode = GL_TRIANGLE_FAN;

    torso_type_1_part_0->data = (float *)malloc(29 * 6 * sizeof(float));
    assert(torso_type_1_part_0->data);

    torso_type_1_part_0->size = 29 * 6 * sizeof(float);

    memcpy(&(torso_type_1_part_1->center_position), (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));

    torso_type_1_part_1->gl_mode = GL_TRIANGLE_FAN;

    torso_type_1_part_1->data = (float *)malloc(29 * 6 * sizeof(float));
    assert(torso_type_1_part_1->data);

    torso_type_1_part_1->size = 29 * 6 * sizeof(float);

    for (int i = 0; i < 29; i++)
    {
        vec3 t;
        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, 1.0f}, t);
        memcpy(torso_type_1_part_0->data + i * 6, t, sizeof(vec3));
        memcpy(torso_type_1_part_0->data + i * 6 + 3, (vec3){0.0f, 0.0f, -1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, -1.0f}, t);
        memcpy(torso_type_1_part_1->data + i * 6, t, sizeof(vec3));
        memcpy(torso_type_1_part_1->data + i * 6 + 3, (vec3){0.0f, 0.0f, -1.0f}, sizeof(vec3));
    }

    torso_type_1_part_2_wrapper->data = (float *)calloc(29 * 6 * 2, sizeof(float));
    torso_type_1_part_2_wrapper->size = 29 * 6 * 2 * sizeof(float);
    torso_type_1_part_2_wrapper->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(torso_type_1_part_2_wrapper->center_position), (vec3){0.0f, -0.5f, 0.0f}, sizeof(vec3));

    int index = 0, i;
    for (i = 0; i < 29; i++)
    {
        memcpy(torso_type_1_part_2_wrapper->data + index, torso_type_1_part_0->data + i * 6, 3 * sizeof(float));
        index += 6;
        memcpy(torso_type_1_part_2_wrapper->data + index, torso_type_1_part_1->data + i * 6, 3 * sizeof(float));
        index += 6;
    }
    torso_type_1_part_0->p = PART_TORSO;
    push_to_rendering_queue(q,torso_type_1_part_0);
    torso_type_1_part_1->p = PART_TORSO;
    push_to_rendering_queue(q,torso_type_1_part_1);
    torso_type_1_part_2_wrapper->p = PART_TORSO;
    push_to_rendering_queue(q,torso_type_1_part_2_wrapper);

    free_generators(&generator, &normals, 11);

    allocate_generators(&generator, &normals, 20);

    for (int i = 0; i < 10; i++)
    {
        generator[i][0] = cos((3 * M_PI / 2) + (i * (M_PI / 65))) * 7;
        generator[i][1] = sin((3 * M_PI / 2) + (i * (M_PI / 65))) * 7 + 7;
        normals[i][0] = 0.0f;
        normals[i][1] = 1.0f;
        glm_vec3_normalize(normals[i]);
    }

    for (int i = 0; i < 10; i++)
    {
        generator[19 - i][0] = cos((3 * M_PI / 2) + (i * (M_PI / 60))) * 7;
        generator[19 - i][1] = sin((3 * M_PI / 2) + (i * (M_PI / 60))) * 7 + 6.5;
        normals[19 - i][0] = 0.0f;
        normals[19 - i][1] = -1.0f;
        glm_vec3_normalize(normals[i]);
    }

    generate_rotational_body(generator, normals, 20, 40, 0, &(torso_type_1_part_3->data), &(torso_type_1_part_3->size));
    torso_type_1_part_3->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(torso_type_1_part_3->center_position), (vec3){0.0f, -0.6f, 0.0f}, sizeof(vec3));
    torso_type_1_part_3->p = PART_TORSO;
    push_to_rendering_queue(q,torso_type_1_part_3);

    free_generators(&generator, &normals, 20);
    free(plane);
}

void queue_feet_type_0(int flg, struct queue_element **q)
{
    struct queue_element *feet_type_0_part_0 = new_queue_element();
    struct queue_element *feet_type_0_part_1 = new_queue_element();
    struct queue_element *feet_type_0_part_2_wrapper = new_queue_element();

    float *plane = (float *)malloc(6 * 3 * sizeof(float));
    assert(plane);

    memcpy(plane, (vec3){-1.0, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 3, (vec3){-0.5, 4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 6, (vec3){1.5, 4.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 9, (vec3){2.0f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(plane + 12, (vec3){4.0f, 1.0f, 0.0f}, sizeof(vec3));
    memcpy(plane + 15, (vec3){4.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(&(feet_type_0_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    feet_type_0_part_0->gl_mode = GL_TRIANGLE_FAN;

    feet_type_0_part_0->data = (float *)calloc(6 * 6, sizeof(float));
    assert(feet_type_0_part_0->data);

    feet_type_0_part_0->size = 6 * 6 * sizeof(float);

    feet_type_0_part_1->gl_mode = GL_TRIANGLE_FAN;
    feet_type_0_part_1->data = (float *)calloc(6 * 6, sizeof(float));
    assert(feet_type_0_part_1->data);
    feet_type_0_part_1->size = 6 * 6 * sizeof(float);

    memcpy(&(feet_type_0_part_1->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 6; i++)
    {
        vec3 t;
        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, 1.0f}, t);
        memcpy(feet_type_0_part_0->data + i * 6, t, sizeof(vec3));
        memcpy(feet_type_0_part_0->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, 0.0f}, t);
        memcpy(feet_type_0_part_1->data + i * 6, t, sizeof(vec3));
        memcpy(feet_type_0_part_1->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));
    }

    feet_type_0_part_2_wrapper->data = (float *)calloc(6 * 6 * 2, sizeof(float));
    feet_type_0_part_2_wrapper->size = 6 * 6 * 2 * sizeof(float);
    feet_type_0_part_2_wrapper->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(feet_type_0_part_2_wrapper->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    int index = 0, i;
    for (i = 0; i < 6; i++)
    {
        memcpy(feet_type_0_part_2_wrapper->data + index, feet_type_0_part_0->data + i * 6, 3 * sizeof(float));
        index += 6;
        memcpy(feet_type_0_part_2_wrapper->data + index, feet_type_0_part_1->data + i * 6, 3 * sizeof(float));
        index += 6;
    }

    mat4 rotation;
    glm_mat4_identity(rotation);
    glm_rotate_y(rotation, -M_PI / 2.0, rotation);

    for (int i = 0; i < (feet_type_0_part_0->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_0_part_0->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_0_part_0->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_0_part_0->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_0_part_0->data) + (i * 6));
    }

    for (int i = 0; i < (feet_type_0_part_1->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_0_part_1->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_0_part_1->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_0_part_1->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_0_part_1->data) + (i * 6));
    }

    for (int i = 0; i < (feet_type_0_part_2_wrapper->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_0_part_2_wrapper->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_0_part_2_wrapper->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_0_part_2_wrapper->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_0_part_2_wrapper->data) + (i * 6));
    }

    feet_type_0_part_0->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_0_part_0);
    feet_type_0_part_1->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_0_part_1);
    feet_type_0_part_2_wrapper->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_0_part_2_wrapper);

    free(plane);
}
void queue_feet_type_1(int flg, struct queue_element **q)
{
    struct queue_element *feet_type_1_part_0 = new_queue_element();
    struct queue_element *feet_type_1_part_1 = new_queue_element();
    struct queue_element *feet_type_1_part_2_wrapper = new_queue_element();
    struct queue_element *feet_type_1_part_3 = new_queue_element();
    struct queue_element *feet_type_1_part_4 = new_queue_element();
    struct queue_element *feet_type_1_part_5 = new_queue_element();

    float *plane = (float *)malloc(6 * 3 * sizeof(float));
    assert(plane);

    memcpy(plane, (vec3){-1.0, 0.0f, -0.5f}, sizeof(vec3));
    memcpy(plane + 3, (vec3){-0.5, 4.0f, -0.5f}, sizeof(vec3));
    memcpy(plane + 6, (vec3){1.5, 4.0f, -0.5f}, sizeof(vec3));
    memcpy(plane + 9, (vec3){2.0f, 1.5f, -0.5f}, sizeof(vec3));
    memcpy(plane + 12, (vec3){4.0f, 1.0f, -0.5f}, sizeof(vec3));
    memcpy(plane + 15, (vec3){4.0f, 0.0f, -0.5f}, sizeof(vec3));

    memcpy(&(feet_type_1_part_0->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    feet_type_1_part_0->gl_mode = GL_TRIANGLE_FAN;

    feet_type_1_part_0->data = (float *)calloc(6 * 6, sizeof(float));
    assert(feet_type_1_part_0->data);

    feet_type_1_part_0->size = 6 * 6 * sizeof(float);

    feet_type_1_part_1->gl_mode = GL_TRIANGLE_FAN;
    feet_type_1_part_1->data = (float *)calloc(6 * 6, sizeof(float));
    assert(feet_type_1_part_1->data);
    feet_type_1_part_1->size = 6 * 6 * sizeof(float);

    memcpy(&(feet_type_1_part_1->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    for (int i = 0; i < 6; i++)
    {
        vec3 t;
        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, 2.0f}, t);
        memcpy(feet_type_1_part_0->data + i * 6, t, sizeof(vec3));
        memcpy(feet_type_1_part_0->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));

        glm_vec3_add(3 * i + plane, (vec3){0.0f, 0.0f, 0.0f}, t);
        memcpy(feet_type_1_part_1->data + i * 6, t, sizeof(vec3));
        memcpy(feet_type_1_part_1->data + i * 6 + 3, (vec3){0.0f, 0.0f, 1.0f}, sizeof(vec3));
    }

    feet_type_1_part_2_wrapper->data = (float *)calloc(6 * 6 * 2, sizeof(float));
    feet_type_1_part_2_wrapper->size = 6 * 6 * 2 * sizeof(float);
    feet_type_1_part_2_wrapper->gl_mode = GL_TRIANGLE_STRIP;

    memcpy(&(feet_type_1_part_2_wrapper->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    int index = 0, i;
    for (i = 0; i < 6; i++)
    {
        memcpy(feet_type_1_part_2_wrapper->data + index, feet_type_1_part_0->data + i * 6, 3 * sizeof(float));
        index += 6;
        memcpy(feet_type_1_part_2_wrapper->data + index, feet_type_1_part_1->data + i * 6, 3 * sizeof(float));
        index += 6;
    }

    generate_sphere(1, 0, &(feet_type_1_part_3->data), &(feet_type_1_part_3->size));
    generate_sphere(1, 0, &(feet_type_1_part_4->data), &(feet_type_1_part_4->size));
    generate_sphere(1, 0, &(feet_type_1_part_5->data), &(feet_type_1_part_5->size));

    feet_type_1_part_3->gl_mode =
        feet_type_1_part_4->gl_mode =
            feet_type_1_part_5->gl_mode =
                GL_TRIANGLE_STRIP;

    memcpy(&(feet_type_1_part_3->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(&(feet_type_1_part_4->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(&(feet_type_1_part_5->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    mat4 rotation;
    glm_mat4_identity(rotation);
    glm_rotate_y(rotation, -M_PI / 2.0, rotation);

    for (int i = 0; i < (feet_type_1_part_3->size) / (sizeof(float) * 6); i++)
    {
        glm_vec3_add((feet_type_1_part_3->data) + i * 6, (vec3){-0.5f, -1.0f, 0.5f}, (feet_type_1_part_3->data) + i * 6);
        glm_vec3_add((feet_type_1_part_4->data) + i * 6, (vec3){1.5f, -1.0f, 0.5f}, (feet_type_1_part_4->data) + i * 6);
        glm_vec3_add((feet_type_1_part_5->data) + i * 6, (vec3){3.5f, -1.0f, 0.5f}, (feet_type_1_part_5->data) + i * 6);
    }

    for (int i = 0; i < (feet_type_1_part_0->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_1_part_0->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_1_part_0->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_1_part_0->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_1_part_0->data) + (i * 6));
    }

    for (int i = 0; i < (feet_type_1_part_1->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_1_part_1->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_1_part_1->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_1_part_1->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_1_part_1->data) + (i * 6));
    }

    for (int i = 0; i < (feet_type_1_part_2_wrapper->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_1_part_2_wrapper->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_1_part_2_wrapper->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_1_part_2_wrapper->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_1_part_2_wrapper->data) + (i * 6));
    }

    for (int i = 0; i < (feet_type_1_part_3->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_1_part_3->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_1_part_3->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_1_part_3->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_1_part_3->data) + (i * 6));
    }

    for (int i = 0; i < (feet_type_1_part_4->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_1_part_4->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_1_part_4->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_1_part_4->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_1_part_4->data) + (i * 6));
    }

    for (int i = 0; i < (feet_type_1_part_5->size) / (6 * sizeof(float)); i++)
    {
        vec4 v4 = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(v4, (feet_type_1_part_5->data) + (i * 6), sizeof(vec3));
        glm_mat4_mulv(rotation, v4, v4);
        memcpy((feet_type_1_part_5->data) + (i * 6), v4, sizeof(vec3));
        glm_vec3_add((feet_type_1_part_5->data) + (i * 6), (vec3){flg * -0.5f, -8.0f, 0.0f}, (feet_type_1_part_5->data) + (i * 6));
    }

    feet_type_1_part_0->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_1_part_0);
    feet_type_1_part_1->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_1_part_1);
    feet_type_1_part_2_wrapper->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_1_part_2_wrapper);
    feet_type_1_part_3->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_1_part_3);
    feet_type_1_part_4->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_1_part_4);
    feet_type_1_part_5->p = PART_FEET;
    push_to_rendering_queue(q,feet_type_1_part_5);

    free(plane);
}

void queue_coord_x(struct queue_element **q)
{
    struct queue_element *coord_axis = new_queue_element();

    float **generator, **normals;

    allocate_generators(&generator, &normals, 4);

    memcpy(generator[0], (vec3){0.0f, 2.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.25f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){0.125f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[3], (vec3){0.125f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){0.0f, -1.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[3], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 4, 10, 0, &(coord_axis->data), &(coord_axis->size));

    for(int i = 0; i < (coord_axis->size) / (6 * sizeof(float)); i++) {
        glm_vec3_rotate((coord_axis->data) + (i * 6), -M_PI/2.0, (vec3){0.0f, 0.0f, 1.0f});
    }

    memcpy(&(coord_axis->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    coord_axis->gl_mode = GL_TRIANGLE_STRIP;
    coord_axis->p = PART_COORD_X;
    push_to_rendering_queue(q,coord_axis);
    free_generators(&generator, &normals, 4);
}

void queue_coord_y(struct queue_element **q)
{
    struct queue_element *coord_axis = new_queue_element();

    float **generator, **normals;

    allocate_generators(&generator, &normals, 4);

    memcpy(generator[0], (vec3){0.0f, 2.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.25f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){0.125f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[3], (vec3){0.125f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){0.0f, -1.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[3], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 4, 10, 0, &(coord_axis->data), &(coord_axis->size));

    memcpy(&(coord_axis->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    coord_axis->gl_mode = GL_TRIANGLE_STRIP;
    coord_axis->p = PART_COORD_Y;
    push_to_rendering_queue(q,coord_axis);
    free_generators(&generator, &normals, 4);
}


void queue_coord_z(struct queue_element **q)
{

    struct queue_element *coord_axis = new_queue_element();

    float **generator, **normals;

    allocate_generators(&generator, &normals, 4);

    memcpy(generator[0], (vec3){0.0f, 2.0f, 0.0f}, sizeof(vec3));
    memcpy(generator[1], (vec3){0.25f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[2], (vec3){0.125f, 1.5f, 0.0f}, sizeof(vec3));
    memcpy(generator[3], (vec3){0.125f, 0.0f, 0.0f}, sizeof(vec3));

    memcpy(normals[0], (vec3){0.7f, 0.7f, 0.0f}, sizeof(vec3));
    memcpy(normals[1], (vec3){0.0f, -1.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[2], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));
    memcpy(normals[3], (vec3){1.0f, 0.0f, 0.0f}, sizeof(vec3));

    generate_rotational_body(generator, normals, 4, 10, 0, &(coord_axis->data), &(coord_axis->size));

     for(int i = 0; i < (coord_axis->size) / (6 * sizeof(float)); i++) {
        glm_vec3_rotate((coord_axis->data) + (i * 6), M_PI/2.0, (vec3){1.0f, 0.0f, 0.0f});
    }


    memcpy(&(coord_axis->center_position), (vec3){0.0f, 0.0f, 0.0f}, sizeof(vec3));

    coord_axis->gl_mode = GL_TRIANGLE_STRIP;
    coord_axis->p = PART_COORD_Z;
    push_to_rendering_queue(q,coord_axis);
    free_generators(&generator, &normals, 4);
}
