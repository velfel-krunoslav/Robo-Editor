#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cglm/cglm.h>
#include <assert.h>
#include <sqlite3.h>

#include "shapes.h"
#include "queue_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEFAULT_BUFFER_SIZE 500000
#define COORD_DEAFULT_BUFFER_SIZE 10000

GtkWidget *window;
GtkWidget *gl_area;
GtkWidget *coord;
GtkWidget *sizer;
GtkWidget *fixed;
GtkWidget *quit;
GtkWidget *save;
GtkWidget *open;
GtkWidget *about;
GtkAboutDialog *about_dialog;
GtkBuilder *builder;

GtkButton *head_type_buttons[3];
GtkButton *wings_type_buttons[3];
GtkButton *torso_type_buttons[2];
GtkButton *feet_type_buttons[2];

GtkComboBoxText *body_parts;

GtkColorButton *color_button;
GtkImage *image;

GtkRadioButton *solid_color;
GtkRadioButton *png_texture;
GtkButton *pick_file;

char *active_combo_box_option = "Head";

int active_head_type = 0;
int active_wing_type = 0;
int active_torso_type = 0;
int active_feet_type = 0;

static GLuint position_buffer;
static GLuint program = 0;
static GLuint coord_position_buffer;
static GLuint coord_program = 0;

vec3 coord_eye = (vec3){0.0f, -1.0, 5.0f};
vec3 coord_center = (vec3){0.0, 0.0, 0.0f};
vec3 coord_lookup = (vec3){0.0f, 1.0f, 0.0f};

vec3 eye = (vec3){0.0f, -5.0f, 22.3f};
vec3 center = (vec3){0.0, -2.0, 0.0f};
vec3 lookup = (vec3){0.0f, 1.0f, 0.0f};
vec3 light_pos = {2.0, 2.0, 2.0};

vec2 prev_size = (vec2){800.0f, 740.0f};

void coord_update_buffer()
{
	if (coord_queue)
	{
		destroy_queue(&coord_queue);
	}

	struct queue_element *t;
	int offset;
	if (coord_queue)
	{
		glBindBuffer(GL_ARRAY_BUFFER, coord_position_buffer);
		float *ptr = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		for (offset = 0, t = coord_queue; t != NULL; offset += ((t->size) / sizeof(float)), t = t->_next)
		{

			memcpy(ptr + offset, t->data, t->size);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	gtk_widget_queue_draw(coord);
}

void update_buffer()
{
	if (queue)
	{
		destroy_queue(&queue);
	}

	switch (active_head_type)
	{
	case 0:
		queue_head_type_0(&queue);
		break;
	case 1:
		queue_head_type_1(&queue);
		break;
	case 2:
		queue_head_type_2(&queue);
		break;
	}

	queue_neck(&queue);

	switch (active_wing_type)
	{
	case 0:
		queue_wings_type_0(&queue);
		break;
	case 1:
		queue_wings_type_1(&queue);
		break;
	case 2:
		queue_wings_type_2(&queue);
		break;
	}

	switch (active_torso_type)
	{
	case 0:
		queue_torso_type_0(&queue);
		break;
	case 1:
		queue_torso_type_1(&queue);
		break;
	}

	switch (active_feet_type)
	{
	case 0:
		queue_feet_type_0(1, &queue);
		queue_feet_type_0(-3, &queue);
		break;
	case 1:
		queue_feet_type_1(1, &queue);
		queue_feet_type_1(-3, &queue);
		break;
	}

	queue_coord_x(&queue);
	queue_coord_y(&queue);
	queue_coord_z(&queue);

	struct queue_element *t;
	int offset;
	if (queue)
	{
		glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
		float *ptr = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		for (offset = 0, t = queue; t != NULL; offset += ((t->size) / sizeof(float)), t = t->_next)
		{
			memcpy(ptr + offset, t->data, t->size);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	gtk_widget_queue_draw(gl_area);
}

static void
coord_init_buffers(GLuint *vao_out,
				   GLuint *buffer_out)
{
	GLuint vao, buffer;

	float *r_data;
	size_t r_data_size;
	/* We only use one VAO, so we always keep it bound */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	/* This is the buffer that holds the vertices */
	glGenBuffers(1, &buffer);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	r_data = (float *)calloc(COORD_DEAFULT_BUFFER_SIZE, sizeof(float));
	r_data_size = COORD_DEAFULT_BUFFER_SIZE * sizeof(float);
	glBufferData(GL_ARRAY_BUFFER, r_data_size, r_data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	free(r_data);

	if (vao_out != NULL)
		*vao_out = vao;

	if (buffer_out != NULL)
		*buffer_out = buffer;
}

/* Initialize the GL buffers */
static void
init_buffers(GLuint *vao_out,
			 GLuint *buffer_out)
{
	GLuint vao, buffer;
	float *r_data;
	size_t r_data_size;
	/* We only use one VAO, so we always keep it bound */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	/* This is the buffer that holds the vertices */
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	r_data = (float *)calloc(DEFAULT_BUFFER_SIZE, sizeof(float));
	r_data_size = DEFAULT_BUFFER_SIZE * sizeof(float);
	glBufferData(GL_ARRAY_BUFFER, r_data_size, r_data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	free(r_data);

	if (vao_out != NULL)
		*vao_out = vao;

	if (buffer_out != NULL)
		*buffer_out = buffer;
}

GLuint coord_load_shader(const char *vertex_src, const char *fragment_src)
{
	GLint result = GL_FALSE;
	int info_log_length;

	// Create the shaders
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	// Compile Vertex Shader
	glShaderSource(vertex_shader_id, 1, &vertex_src, NULL);
	glCompileShader(vertex_shader_id);

	// Check Vertex Shader
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char vertex_shader_error_message[info_log_length + 1];
		glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, vertex_shader_error_message);
		printf("%s\n", &vertex_shader_error_message[0]);
	}

	// Compile Fragment Shader
	glShaderSource(fragment_shader_id, 1, &fragment_src, NULL);
	glCompileShader(fragment_shader_id);

	// Check Fragment Shader
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char fragment_shader_error_message[info_log_length + 1];
		glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, fragment_shader_error_message);
		printf("%s\n", &fragment_shader_error_message[0]);
	}

	// Link the coord_program
	GLuint coord_program_id = glCreateProgram();
	glAttachShader(coord_program_id, vertex_shader_id);
	glAttachShader(coord_program_id, fragment_shader_id);
	glLinkProgram(coord_program_id);

	// Check the coord_program
	glGetProgramiv(coord_program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(coord_program_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char coord_program_error_message[info_log_length + 1];
		glGetProgramInfoLog(coord_program_id, info_log_length, NULL, coord_program_error_message);
		printf("%s\n", coord_program_error_message);
	}

	glDetachShader(coord_program_id, vertex_shader_id);
	glDetachShader(coord_program_id, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	return coord_program_id;
}

GLuint load_shader(const char *vertex_src, const char *fragment_src)
{
	GLint result = GL_FALSE;
	int info_log_length;

	// Create the shaders
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	// Compile Vertex Shader
	glShaderSource(vertex_shader_id, 1, &vertex_src, NULL);
	glCompileShader(vertex_shader_id);

	// Check Vertex Shader
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char vertex_shader_error_message[info_log_length + 1];
		glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, vertex_shader_error_message);
		printf("%s\n", &vertex_shader_error_message[0]);
	}

	// Compile Fragment Shader
	glShaderSource(fragment_shader_id, 1, &fragment_src, NULL);
	glCompileShader(fragment_shader_id);

	// Check Fragment Shader
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char fragment_shader_error_message[info_log_length + 1];
		glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, fragment_shader_error_message);
		printf("%s\n", &fragment_shader_error_message[0]);
	}

	// Link the program
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);

	// Check the program
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char program_error_message[info_log_length + 1];
		glGetProgramInfoLog(program_id, info_log_length, NULL, program_error_message);
		printf("%s\n", program_error_message);
	}

	glDetachShader(program_id, vertex_shader_id);
	glDetachShader(program_id, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	return program_id;
}

static gboolean
coord_render(GtkGLArea *area,
			 GdkGLContext *context)
{

	if (gtk_gl_area_get_error(area) != NULL)
		return FALSE;

	gint width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
	gint height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

	glClearColor(0.8f, 0.8f, 0.8f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color and depth buffers

	/* DRAW FUNC BEGIN */

	/* Use our shaders */
	glUseProgram(coord_program);

	glBindBuffer(GL_ARRAY_BUFFER, coord_position_buffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	mat4 model_mat, view_mat, projection_mat;
	int model_loc;

	glm_lookat(coord_eye, coord_center, coord_lookup, view_mat);

	model_loc = glGetUniformLocation(coord_program, "view");
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *)view_mat);

	glm_perspective(45.0f * M_PI / 180.0f, (float)width / height, 0.1f, 100.0f, projection_mat);
	model_loc = glGetUniformLocation(coord_program, "projection");
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *)projection_mat);

	struct queue_element *t;
	size_t offset;
	int count;
	for (count = 0, t = queue; t != NULL; count++, t = t->_next)
		;

	for (offset = 0, t = queue; t != NULL; offset += t->size, t = t->_next)
	{
		glm_mat4_identity(model_mat);

		model_mat[3][0] += (t->center_position)[0];
		model_mat[3][1] += (t->center_position)[1];
		model_mat[3][2] += (t->center_position)[2];

		model_loc = glGetUniformLocation(coord_program, "model");
		glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *)model_mat);

		switch (t->p)
		{
		case PART_COORD_X:

			glUniform3f(glGetUniformLocation(coord_program, "activeColor"), 1.0f, 0.0f, 0.0f);
			glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));

			break;
		case PART_COORD_Y:

			glUniform3f(glGetUniformLocation(coord_program, "activeColor"), 0.0f, 1.0f, 0.0f);
			glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));

			break;
		case PART_COORD_Z:

			glUniform3f(glGetUniformLocation(coord_program, "activeColor"), 0.0f, 0.0f, 1.0f);
			glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));

			break;
		}
	}

	/* We finished using the buffers and coord_program */
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	/* DRAW FUNC END */

	/* Flush the contents of the pipeline */
	glFlush();

	return TRUE;
}

static gboolean
render(GtkGLArea *area,
	   GdkGLContext *context)
{
	if (gtk_gl_area_get_error(area) != NULL)
		return FALSE;

	gint width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
	gint height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

	glClearColor(0.8f, 0.8f, 0.8f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color and depth buffers

	/* DRAW FUNC BEGIN */

	/* Use our shaders */
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	mat4 model_mat, view_mat, projection_mat;
	int model_loc;
	glm_lookat(eye, center, lookup, view_mat);

	model_loc = glGetUniformLocation(coord_program, "view");
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *)view_mat);

	glm_perspective(45.0f * M_PI / 180.0f, (float)width / height, 0.1f, 100.0f, projection_mat);
	model_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *)projection_mat);

	model_loc = glGetUniformLocation(program, "lightPos");
	glUniform3fv(model_loc, 1, (float *)light_pos);

	struct queue_element *t;
	size_t offset;

	int count;
	for (count = 0, t = queue; t != NULL; count++, t = t->_next)
		;

	for (offset = 0, t = queue; t != NULL; offset += t->size, t = t->_next)
	{
		glm_mat4_identity(model_mat);

		model_mat[3][0] += (t->center_position)[0];
		model_mat[3][1] += (t->center_position)[1];
		model_mat[3][2] += (t->center_position)[2];
		model_loc = glGetUniformLocation(program, "model");
		glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *)model_mat);

		switch (t->p)
		{
		case PART_HEAD:
			if (head_style.is_solid)
			{
				glUniform1f(glGetUniformLocation(program, "isSolid"), 1.0f);
				glUniform3fv(glGetUniformLocation(program, "activeColor"), 1, (float *)head_style.color);
				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
			}
			else
			{
				unsigned char texture;
				glGenTextures(1, &texture);

				glBindTexture(GL_TEXTURE_2D, texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				int width, height, nrChannels;
				stbi_set_flip_vertically_on_load(true);
				unsigned char *rawdata = stbi_load(head_style.filepath, &width, &height, &nrChannels, 0);
				glUniform1f(glGetUniformLocation(program, "isSolid"), 0.0f);
				glUniform1i(glGetUniformLocation(program, "textureImg"), 0);
				if (rawdata)
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rawdata);
					glGenerateMipmap(GL_TEXTURE_2D);
				}
				else
				{
					fprintf(stderr, "ERROR: failed to load texture\n");
				}
				stbi_image_free(rawdata);

				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
				glBindTexture(GL_TEXTURE_2D, 0);
				gtk_widget_queue_draw(gl_area);
			}
			break;
		case PART_TORSO:
			if (torso_style.is_solid)
			{
				glUniform1f(glGetUniformLocation(program, "isSolid"), 1.0f);
				glUniform3fv(glGetUniformLocation(program, "activeColor"), 1, (float *)torso_style.color);
				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
			}
			else
			{
				unsigned char texture;
				glGenTextures(1, &texture);

				glBindTexture(GL_TEXTURE_2D, texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				int width, height, nrChannels;
				stbi_set_flip_vertically_on_load(true);
				unsigned char *rawdata = stbi_load(torso_style.filepath, &width, &height, &nrChannels, 0);
				glUniform1f(glGetUniformLocation(program, "isSolid"), 0.0f);
				glUniform1i(glGetUniformLocation(program, "textureImg"), 0);
				if (rawdata)
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rawdata);
					glGenerateMipmap(GL_TEXTURE_2D);
				}
				else
				{
					fprintf(stderr, "ERROR: failed to load texture\n");
				}
				stbi_image_free(rawdata);

				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
				glBindTexture(GL_TEXTURE_2D, 0);
				gtk_widget_queue_draw(gl_area);
			}
			break;
		case PART_WINGS:
			if (wings_style.is_solid)
			{
				glUniform1f(glGetUniformLocation(program, "isSolid"), 1.0f);
				glUniform3fv(glGetUniformLocation(program, "activeColor"), 1, (float *)wings_style.color);
				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
			}
			else
			{
				unsigned char texture;
				glGenTextures(1, &texture);

				glBindTexture(GL_TEXTURE_2D, texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				int width, height, nrChannels;
				stbi_set_flip_vertically_on_load(true);
				unsigned char *rawdata = stbi_load(wings_style.filepath, &width, &height, &nrChannels, 0);
				glUniform1f(glGetUniformLocation(program, "isSolid"), 0.0f);
				glUniform1i(glGetUniformLocation(program, "textureImg"), 0);
				if (rawdata)
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rawdata);
					glGenerateMipmap(GL_TEXTURE_2D);
				}
				else
				{
					fprintf(stderr, "ERROR: failed to load texture\n");
				}
				stbi_image_free(rawdata);

				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
				glBindTexture(GL_TEXTURE_2D, 0);
				gtk_widget_queue_draw(gl_area);
			}
			break;
		case PART_FEET:
			if (feet_style.is_solid)
			{
				glUniform1f(glGetUniformLocation(program, "isSolid"), 1.0f);
				glUniform3fv(glGetUniformLocation(program, "activeColor"), 1, (float *)feet_style.color);
				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
			}
			else
			{
				unsigned char texture;
				glGenTextures(1, &texture);

				glBindTexture(GL_TEXTURE_2D, texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				int width, height, nrChannels;
				stbi_set_flip_vertically_on_load(true);
				unsigned char *rawdata = stbi_load(feet_style.filepath, &width, &height, &nrChannels, 0);
				glUniform1f(glGetUniformLocation(program, "isSolid"), 0.0f);
				glUniform1i(glGetUniformLocation(program, "textureImg"), 0);
				if (rawdata)
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rawdata);
					glGenerateMipmap(GL_TEXTURE_2D);
				}
				else
				{
					fprintf(stderr, "ERROR: failed to load texture\n");
				}
				stbi_image_free(rawdata);

				glDrawArrays(t->gl_mode, offset / (6 * sizeof(float)), t->size / (6 * sizeof(float)));
				glBindTexture(GL_TEXTURE_2D, 0);
				gtk_widget_queue_draw(gl_area);
			}
			break;
		default:
			break;
		}
	}

	/* We finished using the buffers and program */
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	/* DRAW FUNC END */

	/* Flush the contents of the pipeline */
	glFlush();

	return TRUE;
}

static void
coord_on_realize(GtkGLArea *area)
{
	gtk_gl_area_make_current(area);

	if (gtk_gl_area_get_error(area) != NULL)
		return;

	gint width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
	gint height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

	coord_init_buffers(&coord_position_buffer, NULL);

	FILE *fragment, *vertex;
	char *fragment_source, *vertex_source;

	fragment = fopen("./fragment_coord.glsl", "r");
	assert(fragment);
	int len = 0;

	while (!feof(fragment))
	{
		fgetc(fragment);
		len++;
	}

	rewind(fragment);

	fragment_source = (char *)malloc((len + 1) * sizeof(char));
	assert(fragment_source);

	len = 0;
	while (!feof(fragment))
	{
		fragment_source[len++] = fgetc(fragment);
	}
	fragment_source[len] = '\0';
	fclose(fragment);

	vertex = fopen("./vertex_coord.glsl", "r");
	assert(vertex);

	len = 0;

	while (!feof(vertex))
	{
		fgetc(vertex);
		len++;
	}

	rewind(vertex);

	vertex_source = (char *)malloc((len + 1) * sizeof(char));
	assert(vertex_source);

	len = 0;
	while (!feof(vertex))
	{
		vertex_source[len++] = fgetc(vertex);
	}
	vertex_source[len] = '\0';
	fclose(vertex);

	coord_program = coord_load_shader(vertex_source, fragment_source);

	free(fragment_source);
	free(vertex_source);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	coord_update_buffer();
}

static void
coord_on_unrealize(GtkGLArea *area)
{
	gtk_gl_area_make_current(GTK_GL_AREA(area));

	if (gtk_gl_area_get_error(GTK_GL_AREA(area)) != NULL)
		return;
	glDeleteBuffers(1, &coord_position_buffer);
	glDeleteProgram(coord_program);
}

static gboolean
coord_on_reshape(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	glViewport(0, 0, gtk_widget_get_allocated_width(widget),
			   gtk_widget_get_allocated_height(widget));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	return TRUE;
}

static void
on_realize(GtkGLArea *area)
{
	gtk_gl_area_make_current(area);

	if (gtk_gl_area_get_error(area) != NULL)
		return;

	gint width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
	gint height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

	init_buffers(&position_buffer, NULL);

	FILE *fragment, *vertex;
	char *fragment_source, *vertex_source;

	fragment = fopen("./fragment.glsl", "r");
	assert(fragment);
	int len = 0;

	while (!feof(fragment))
	{
		fgetc(fragment);
		len++;
	}

	rewind(fragment);

	fragment_source = (char *)malloc((len + 1) * sizeof(char));
	assert(fragment_source);

	len = 0;
	while (!feof(fragment))
	{
		fragment_source[len++] = fgetc(fragment);
	}
	fragment_source[len] = '\0';
	fclose(fragment);

	vertex = fopen("./vertex.glsl", "r");
	assert(vertex);

	len = 0;

	while (!feof(vertex))
	{
		fgetc(vertex);
		len++;
	}

	rewind(vertex);

	vertex_source = (char *)malloc((len + 1) * sizeof(char));
	assert(vertex_source);

	len = 0;
	while (!feof(vertex))
	{
		vertex_source[len++] = fgetc(vertex);
	}
	vertex_source[len] = '\0';
	fclose(vertex);

	program = load_shader(vertex_source, fragment_source);

	free(fragment_source);
	free(vertex_source);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);

	update_buffer();
}

static void
on_unrealize(GtkGLArea *area)
{
	gtk_gl_area_make_current(GTK_GL_AREA(area));

	if (gtk_gl_area_get_error(GTK_GL_AREA(area)) != NULL)
		return;
	glDeleteBuffers(1, &position_buffer);
	glDeleteProgram(program);
}

void move_camera(float flg)
{
	vec3 t, prev;
	glm_vec3_sub(eye, center, t);
	glm_vec3_sub(eye, center, prev);
	glm_vec3_normalize(t);
	glm_vec3_mul(t, (vec3){flg, flg, flg}, t);
	glm_vec3_mul(t, (vec3){0.1f, 0.1f, 0.1f}, t);
	glm_vec3_add(prev, t, t);
	glm_vec3_add(center, t, eye);
}

static gboolean
on_reshape(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	float k_w = (float)width / prev_size[0];
	float k_h = (float)height / prev_size[1];
	float k_avg = (k_w + k_h) / 2.0f;

	int resize_factor = k_avg * 100 - 100;

	gboolean isGrowing = false;

	if (prev_size[0] < width || prev_size[1] < height)
		isGrowing = true;
	else
		resize_factor *= -1.0f;

	for (int i = 0; i < resize_factor; i++)
	{
		move_camera((isGrowing) ? 2.0f : -2.5f);
	}
	glViewport(0, 0,
			   400,
			   400);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	prev_size[0] = width;
	prev_size[1] = height;

	gtk_fixed_move(fixed, coord, 0, height - 140);

	gtk_widget_queue_draw(widget);
	return TRUE;
}

void rotate_vertically(float flg)
{
	vec3 f, l;
	mat4 m;
	glm_vec3_sub(coord_center, coord_eye, f);
	glm_vec3_normalize(f);
	glm_vec3_cross((vec3){0.0f, 1.0f, 0.0f}, f, l);

	glm_rotate_make(m, flg * M_PI / 180.0f, l);
	glm_vec3_rotate_m4(m, coord_eye, coord_eye);
	glm_mat4_mulv3(m, coord_lookup, 1.0f, coord_lookup);

	gtk_widget_queue_draw(coord);

	glm_vec3_sub(center, eye, f);
	glm_vec3_normalize(f);
	glm_vec3_cross((vec3){0.0f, 1.0f, 0.0f}, f, l);

	glm_rotate_make(m, flg * M_PI / 180.0f, l);
	glm_vec3_rotate_m4(m, eye, eye);
	glm_mat4_mulv3(m, lookup, 1.0f, lookup);

	gtk_widget_queue_draw(coord);
	gtk_widget_queue_draw(gl_area);
}

void rotate_horizontally(float flg)
{
	vec3 t;
	memcpy(coord_lookup, (vec3){0.0, 1.0f, 0.0f}, sizeof(vec3));
	glm_vec3_sub(coord_eye, coord_center, t);
	glm_vec3_rotate(t, flg * M_PI / 180.0f, (vec3){0.0f, 1.0f, 0.0f});
	glm_vec3_add(t, coord_center, coord_eye);

	memcpy(lookup, (vec3){0.0, 1.0f, 0.0f}, sizeof(vec3));
	glm_vec3_sub(eye, center, t);
	glm_vec3_rotate(t, flg * M_PI / 180.0f, (vec3){0.0f, 1.0f, 0.0f});
	glm_vec3_add(t, center, eye);
}

gboolean process_keyboard_input(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	switch (event->keyval)
	{
	case GDK_KEY_w:
		rotate_vertically(1.0f);
		break;
	case GDK_KEY_s:
		rotate_vertically(-1.0f);
		break;
	case GDK_KEY_a:
		rotate_horizontally(-1.0f);
		break;
	case GDK_KEY_d:
		rotate_horizontally(1.0f);
		break;
	default:
		return FALSE;
	}

	gtk_widget_queue_draw(coord);
	gtk_widget_queue_draw(gl_area);
	
	return TRUE;
}

gboolean process_mouse_input(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{

	switch (event->direction)
	{
	case GDK_SCROLL_UP:
		move_camera(-1.0f);
		break;
	case GDK_SCROLL_DOWN:
		move_camera(1.0f);
		break;
	default:
		return FALSE;
	}

	gtk_widget_queue_draw(coord);
	gtk_widget_queue_draw(gl_area);
	return TRUE;
}

void head_type_0_clicked(GtkButton *button)
{
	gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NONE);
	active_head_type = 0;
	gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void head_type_1_clicked(GtkButton *button)
{
	gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NONE);
	active_head_type = 1;
	gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void head_type_2_clicked(GtkButton *button)
{
	gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NONE);
	active_head_type = 2;
	gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void wings_type_0_clicked(GtkButton *button)
{
	gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NONE);
	active_wing_type = 0;
	gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void wings_type_1_clicked(GtkButton *button)
{
	gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NONE);
	active_wing_type = 1;
	gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void wings_type_2_clicked(GtkButton *button)
{
	gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NONE);
	active_wing_type = 2;
	gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void torso_type_0_clicked(GtkButton *button)
{
	gtk_button_set_relief(torso_type_buttons[active_torso_type], GTK_RELIEF_NONE);
	active_torso_type = 0;
	gtk_button_set_relief(torso_type_buttons[active_torso_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void torso_type_1_clicked(GtkButton *button)
{
	gtk_button_set_relief(torso_type_buttons[active_torso_type], GTK_RELIEF_NONE);
	active_torso_type = 1;
	gtk_button_set_relief(torso_type_buttons[active_torso_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void feet_type_0_clicked(GtkButton *button)
{
	gtk_button_set_relief(feet_type_buttons[active_feet_type], GTK_RELIEF_NONE);
	active_feet_type = 0;
	gtk_button_set_relief(feet_type_buttons[active_feet_type], GTK_RELIEF_NORMAL);
	update_buffer();
}
void feet_type_1_clicked(GtkButton *button)
{
	gtk_button_set_relief(feet_type_buttons[active_feet_type], GTK_RELIEF_NONE);
	active_feet_type = 1;
	gtk_button_set_relief(feet_type_buttons[active_feet_type], GTK_RELIEF_NORMAL);
	update_buffer();
}

void color_activated(
	GtkColorButton *self,
	gpointer user_data)
{
	GdkRGBA t;
	gtk_color_chooser_get_rgba(self, &t);

	if (!strcmp(active_combo_box_option, "Head"))
	{
		head_style.is_solid = 1;
		memcpy(&(head_style.color), (vec3){t.red, t.green, t.blue}, sizeof(vec3));
	}
	else if (!strcmp(active_combo_box_option, "Wings"))
	{
		wings_style.is_solid = 1;
		memcpy(&(wings_style.color), (vec3){t.red, t.green, t.blue}, sizeof(vec3));
	}
	else if (!strcmp(active_combo_box_option, "Torso"))
	{
		torso_style.is_solid = 1;
		memcpy(&(torso_style.color), (vec3){t.red, t.green, t.blue}, sizeof(vec3));
	}
	else if (!strcmp(active_combo_box_option, "Feet"))
	{
		feet_style.is_solid = 1;
		memcpy(&(feet_style.color), (vec3){t.red, t.green, t.blue}, sizeof(vec3));
	}

	gtk_widget_queue_draw(gl_area);
}

void activate_solid_color()
{
	gtk_widget_show(GTK_WIDGET(color_button));
	gtk_widget_hide(GTK_WIDGET(image));
	gtk_widget_hide(GTK_WIDGET(pick_file));
	gtk_image_clear(image);
}

void activate_texture()
{
	gtk_widget_hide(GTK_WIDGET(color_button));
	gtk_image_clear(image);
	gtk_widget_show(GTK_WIDGET(image));
	gtk_widget_show(GTK_WIDGET(pick_file));
}

void solid_color_clicked(GtkButton *self)
{
	activate_solid_color();
}

void png_texture_clicked(GtkButton *self)
{
	activate_texture();
}

void pick_file_clicked(GtkButton *self)
{
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new("Choose a texture",
										 window,
										 action,
										 "Cancel",
										 GTK_RESPONSE_CANCEL,
										 "Open",
										 GTK_RESPONSE_ACCEPT,
										 NULL);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		if (!strcmp(active_combo_box_option, "Head"))
		{
			head_style.is_solid = 0;
			head_style.filepath = filename;
		}
		else if (!strcmp(active_combo_box_option, "Wings"))
		{
			wings_style.is_solid = 0;
			wings_style.filepath = filename;
		}
		else if (!strcmp(active_combo_box_option, "Torso"))
		{
			torso_style.is_solid = 0;
			torso_style.filepath = filename;
		}
		else if (!strcmp(active_combo_box_option, "Feet"))
		{
			feet_style.is_solid = 0;
			feet_style.filepath = filename;
		}
		GdkPixbuf *pb = gdk_pixbuf_new_from_file(filename, NULL);
		pb = gdk_pixbuf_scale_simple(pb, 270, 400, GDK_INTERP_BILINEAR);
		gtk_image_set_from_pixbuf(image, pb);
	}
	gtk_widget_destroy(dialog);
}

void combo_box_changed(
	GtkComboBoxText *self,
	gpointer user_data)
{
	GdkRGBA t;

	active_combo_box_option = gtk_combo_box_text_get_active_text(self);

	if (!strcmp(active_combo_box_option, "Head"))
	{
		if (head_style.is_solid)
		{
			t.red = head_style.color[0];
			t.green = head_style.color[1];
			t.blue = head_style.color[2];
		}
		else
		{
			/* gdk_pixbuf_new_from_bytes()
			gtk_image_set_from_pixbuf(); 					CONTINUE*/
		}
	}
	else if (!strcmp(active_combo_box_option, "Wings"))
	{
		t.red = wings_style.color[0];
		t.green = wings_style.color[1];
		t.blue = wings_style.color[2];
	}
	else if (!strcmp(active_combo_box_option, "Torso"))
	{
		t.red = torso_style.color[0];
		t.green = torso_style.color[1];
		t.blue = torso_style.color[2];
	}
	else if (!strcmp(active_combo_box_option, "Feet"))
	{
		t.red = feet_style.color[0];
		t.green = feet_style.color[1];
		t.blue = feet_style.color[2];
	}

	t.alpha = 1.0;

	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_button), &t);
}

void check_resize(
	GtkContainer *self,
	gpointer user_data)
{

	gtk_widget_queue_draw(self);
	gtk_widget_queue_draw(gl_area);
	gtk_widget_queue_draw(coord);
}

void invoke_about_menu(void)
{
	gtk_dialog_run(GTK_DIALOG(about_dialog));
}

int read_callback(void *NotUsed, int argc, char **argv,
				  char **azColName)
{
	NotUsed = 0;
	enum part p;
	int index;
	int is_solid;
	vec3 color;
	char *filepath = (char *)malloc(40 * sizeof(char));

	assert(filepath);

	for (int i = 0; i < argc; i++)
	{
		if (!strcmp(azColName[i], "body_part"))
		{
			if (argv[i])
			{
				sscanf(argv[i], "%d", &p);
			}
		}
		else if (!strcmp(azColName[i], "is_solid"))
		{
			if (argv[i])
			{
				sscanf(argv[i], "%d", &is_solid);
			}
		}
		else if (!strcmp(azColName[i], "ind"))
		{
			if (argv[i])
			{
				sscanf(argv[i], "%d", &index);
			}
		}
		else if (!strcmp(azColName[i], "color"))
		{
			if (argv[i])
			{
				sscanf(argv[i], "%f,%f,%f", &color[0], &color[1], &color[2]);
			}
		}
		else if (!strcmp(azColName[i], "filepath"))
		{
			if (argv[i])
			{
				strcpy(filepath, argv[i]);
			}
		}
	}

	switch (p)
	{
	case PART_HEAD:
		head_style.is_solid = is_solid;
		memcpy(&(head_style.color), color, sizeof(vec3));
		if (!is_solid)
		{
			head_style.filepath = filepath;
		}
		gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NONE);
		active_head_type = index;
		gtk_button_set_relief(head_type_buttons[active_head_type], GTK_RELIEF_NORMAL);
		break;
	case PART_TORSO:
		torso_style.is_solid = is_solid;
		memcpy(&(torso_style.color), color, sizeof(vec3));
		if (!is_solid)
		{
			torso_style.filepath = filepath;
		}
		gtk_button_set_relief(torso_type_buttons[active_torso_type], GTK_RELIEF_NONE);
		active_torso_type = index;
		gtk_button_set_relief(torso_type_buttons[active_torso_type], GTK_RELIEF_NORMAL);
		break;
	case PART_WINGS:
		wings_style.is_solid = is_solid;
		memcpy(&(wings_style.color), color, sizeof(vec3));
		if (!is_solid)
		{
			wings_style.filepath = filepath;
		}
		gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NONE);
		active_wing_type = index;
		gtk_button_set_relief(wings_type_buttons[active_wing_type], GTK_RELIEF_NORMAL);
		break;
	case PART_FEET:
		feet_style.is_solid = is_solid;
		memcpy(&(feet_style.color), color, sizeof(vec3));
		if (!is_solid)
		{
			feet_style.filepath = filepath;
		}
		gtk_button_set_relief(feet_type_buttons[active_feet_type], GTK_RELIEF_NONE);
		active_feet_type = index;
		gtk_button_set_relief(feet_type_buttons[active_feet_type], GTK_RELIEF_NORMAL);
		break;
	}

	if (is_solid)
	{
		free(filepath);
	}
	

	return 0;
}

void load_data(char *filename)
{
	if (filename == NULL)
	{

		head_style.is_solid = 1;
		memcpy(&(head_style.color), (vec3){0.6, 0.6, 0.6}, sizeof(vec3));
		wings_style.is_solid = 1;
		memcpy(&(wings_style.color), (vec3){0.6, 0.6, 0.6}, sizeof(vec3));
		torso_style.is_solid = 1;
		memcpy(&(torso_style.color), (vec3){0.6, 0.6, 0.6}, sizeof(vec3));
		feet_style.is_solid = 1;
		memcpy(&(feet_style.color), (vec3){0.6, 0.6, 0.6}, sizeof(vec3));
	}
	else
	{
		sqlite3 *db;
		char *err_msg = 0;

		int rc = sqlite3_open(filename, &db);

		if (rc != SQLITE_OK)
		{

			fprintf(stderr, "Cannot open file: %s\n",
					sqlite3_errmsg(db));
			sqlite3_close(db);

			exit(EXIT_FAILURE);
		}

		char *sql = "SELECT * FROM mt;";

		rc = sqlite3_exec(db, sql, read_callback, 0, &err_msg);

		sqlite3_close(db);

		update_buffer();
	}
}

void save_procedure(char *filepath)
{
	sqlite3 *db;
	char *err_msg = 0;

	int rc = sqlite3_open(filepath, &db);

	if (rc != SQLITE_OK)
	{

		fprintf(stderr, "Cannot open file: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		exit(EXIT_FAILURE);
	}

	char *sql = (char *)malloc(10000 * sizeof(char));
	assert(sql);
	sql[0] = '\0';

	char *t = (char *)malloc(4000 * sizeof(char));
	assert(t);
	t[0] = '\0';

	strcat(sql, "DROP TABLE IF EXISTS mt;");

	strcat(sql, "CREATE TABLE mt(body_part INT, is_solid INT, ind INT, color TEXT, filepath TEXT);");

	snprintf(t, 100 * sizeof(char),
			 "INSERT INTO mt VALUES(%d,%d,%d,\"%.2f,%.2f,%.2f\",\"%s\");",
			 PART_HEAD, head_style.is_solid, active_head_type, head_style.color[0], head_style.color[1], head_style.color[2], (head_style.is_solid) ? "NULL" : head_style.filepath);

	strcat(sql, t);

	snprintf(t, 100 * sizeof(char),
			 "INSERT INTO mt VALUES(%d,%d,%d,\"%.2f,%.2f,%.2f\",\"%s\");",
			 PART_FEET, feet_style.is_solid, active_feet_type, feet_style.color[0], feet_style.color[1], feet_style.color[2], (feet_style.is_solid) ? "NULL" : feet_style.filepath);

	strcat(sql, t);
	snprintf(t, 100 * sizeof(char),
			 "INSERT INTO mt VALUES(%d,%d,%d,\"%.2f,%.2f,%.2f\",\"%s\");",
			 PART_WINGS, wings_style.is_solid, active_wing_type, wings_style.color[0], wings_style.color[1], wings_style.color[2], (wings_style.is_solid) ? "NULL" : wings_style.filepath);

	strcat(sql, t);
	snprintf(t, 100 * sizeof(char),
			 "INSERT INTO mt VALUES(%d,%d,%d,\"%.2f,%.2f,%.2f\",\"%s\");",
			 PART_TORSO, torso_style.is_solid, active_torso_type, torso_style.color[0], torso_style.color[1], torso_style.color[2], (torso_style.is_solid) ? "NULL" : torso_style.filepath);

	strcat(sql, t);

	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if (rc != SQLITE_OK)
	{

		fprintf(stderr, "SQL error: %s\n", err_msg);

		sqlite3_free(err_msg);
		sqlite3_close(db);

		return exit(EXIT_FAILURE);
	}

	sqlite3_close(db);
	free(sql);
	free(t);
}

void invoke_save_menu(void)
{
    GtkFileChooserNative *native;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    native = gtk_file_chooser_native_new(u8"Save",
                                         NULL,
                                         action,
                                         u8"Save",
                                         u8"Cancel");


    // default file name
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "my_model.myf");

    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        filename = gtk_file_chooser_get_filename(chooser);

        // save the file
        save_procedure(filename);

        g_free(filename);
    }

    g_object_unref(native);
}

void invoke_open_menu(void)
{
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new("Choose a texture",
										 window,
										 action,
										 "Cancel",
										 GTK_RESPONSE_CANCEL,
										 "Open",
										 GTK_RESPONSE_ACCEPT,
										 NULL);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		load_data(filename);
	}
	gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	builder = gtk_builder_new_from_file("./ui.glade");

	window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	gtk_widget_add_events(window, GDK_SCROLL_MASK);

	load_data(NULL);

	gl_area = GTK_WIDGET(gtk_builder_get_object(builder, "gl_drawing_area"));
	coord = GTK_WIDGET(gtk_builder_get_object(builder, "coord"));
	sizer = GTK_WIDGET(gtk_builder_get_object(builder, "sizer"));
	fixed = GTK_WIDGET(gtk_builder_get_object(builder, "fixed"));
	quit = GTK_WIDGET(gtk_builder_get_object(builder, "quit"));
	open = GTK_WIDGET(gtk_builder_get_object(builder, "open"));
	save = GTK_WIDGET(gtk_builder_get_object(builder, "save"));
	about = GTK_WIDGET(gtk_builder_get_object(builder, "about"));
	about_dialog = GTK_ABOUT_DIALOG(gtk_builder_get_object(builder, "about_dialog"));

	head_type_buttons[0] = GTK_BUTTON(gtk_builder_get_object(builder, "head_type_0"));
	gtk_button_set_relief(head_type_buttons[0], GTK_RELIEF_NORMAL);
	g_signal_connect(GTK_WIDGET(head_type_buttons[0]), "clicked", G_CALLBACK(head_type_0_clicked), NULL);

	head_type_buttons[1] = GTK_BUTTON(gtk_builder_get_object(builder, "head_type_1"));
	g_signal_connect(GTK_WIDGET(head_type_buttons[1]), "clicked", G_CALLBACK(head_type_1_clicked), NULL);

	head_type_buttons[2] = GTK_BUTTON(gtk_builder_get_object(builder, "head_type_2"));
	g_signal_connect(GTK_WIDGET(head_type_buttons[2]), "clicked", G_CALLBACK(head_type_2_clicked), NULL);

	wings_type_buttons[0] = GTK_BUTTON(gtk_builder_get_object(builder, "wings_type_0"));
	gtk_button_set_relief(wings_type_buttons[0], GTK_RELIEF_NORMAL);
	g_signal_connect(GTK_WIDGET(wings_type_buttons[0]), "clicked", G_CALLBACK(wings_type_0_clicked), NULL);

	wings_type_buttons[1] = GTK_BUTTON(gtk_builder_get_object(builder, "wings_type_1"));
	g_signal_connect(GTK_WIDGET(wings_type_buttons[1]), "clicked", G_CALLBACK(wings_type_1_clicked), NULL);

	wings_type_buttons[2] = GTK_BUTTON(gtk_builder_get_object(builder, "wings_type_2"));
	g_signal_connect(GTK_WIDGET(wings_type_buttons[2]), "clicked", G_CALLBACK(wings_type_2_clicked), NULL);

	torso_type_buttons[0] = GTK_BUTTON(gtk_builder_get_object(builder, "torso_type_0"));
	gtk_button_set_relief(torso_type_buttons[0], GTK_RELIEF_NORMAL);
	g_signal_connect(GTK_WIDGET(torso_type_buttons[0]), "clicked", G_CALLBACK(torso_type_0_clicked), NULL);

	torso_type_buttons[1] = GTK_BUTTON(gtk_builder_get_object(builder, "torso_type_1"));
	g_signal_connect(GTK_WIDGET(torso_type_buttons[1]), "clicked", G_CALLBACK(torso_type_1_clicked), NULL);

	feet_type_buttons[0] = GTK_BUTTON(gtk_builder_get_object(builder, "feet_type_0"));
	gtk_button_set_relief(feet_type_buttons[0], GTK_RELIEF_NORMAL);
	g_signal_connect(GTK_WIDGET(feet_type_buttons[0]), "clicked", G_CALLBACK(feet_type_0_clicked), NULL);

	feet_type_buttons[1] = GTK_BUTTON(gtk_builder_get_object(builder, "feet_type_1"));
	g_signal_connect(GTK_WIDGET(feet_type_buttons[1]), "clicked", G_CALLBACK(feet_type_1_clicked), NULL);

	body_parts = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "body_parts"));
	g_signal_connect(GTK_WIDGET(body_parts), "changed", G_CALLBACK(combo_box_changed), NULL);

	color_button = GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "color_button"));
	g_signal_connect(GTK_WIDGET(color_button), "color-set", G_CALLBACK(color_activated), NULL);

	image = GTK_IMAGE(gtk_builder_get_object(builder, "texture_preview"));
	gtk_widget_hide(image);

	solid_color = GTK_RADIO_BUTTON(gtk_builder_get_object(builder, "solid_color"));
	g_signal_connect(GTK_WIDGET(solid_color), "clicked", G_CALLBACK(solid_color_clicked), NULL);

	png_texture = GTK_RADIO_BUTTON(gtk_builder_get_object(builder, "png_texture"));
	g_signal_connect(GTK_WIDGET(png_texture), "clicked", G_CALLBACK(png_texture_clicked), NULL);

	pick_file = GTK_RADIO_BUTTON(gtk_builder_get_object(builder, "pick_file"));
	g_signal_connect(GTK_WIDGET(pick_file), "clicked", G_CALLBACK(pick_file_clicked), NULL);
	gtk_widget_hide(pick_file);

	g_signal_connect(gl_area, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(gl_area, "unrealize", G_CALLBACK(on_unrealize), NULL);
	g_signal_connect(gl_area, "resize", G_CALLBACK(on_reshape), NULL);
	g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

	g_signal_connect(coord, "realize", G_CALLBACK(coord_on_realize), NULL);
	g_signal_connect(coord, "unrealize", G_CALLBACK(coord_on_unrealize), NULL);
	g_signal_connect(coord, "render", G_CALLBACK(coord_render), NULL);
	g_signal_connect(coord, "resize", G_CALLBACK(coord_on_reshape), NULL);

	g_signal_connect(about, "activate", G_CALLBACK(invoke_about_menu), NULL);
	g_signal_connect(open, "activate", G_CALLBACK(invoke_open_menu), NULL);
	g_signal_connect(save, "activate", G_CALLBACK(invoke_save_menu), NULL);

	GtkOverlay *o = GTK_OVERLAY(gtk_builder_get_object(builder, "overlay"));

	g_signal_connect(G_OBJECT(o), "check-resize", G_CALLBACK(check_resize), NULL);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(quit, "activate", G_CALLBACK(gtk_main_quit), NULL);

	g_signal_connect(G_OBJECT(window), "key_press_event",
					 G_CALLBACK(process_keyboard_input), NULL);
	g_signal_connect(G_OBJECT(window), "scroll_event",
					 G_CALLBACK(process_mouse_input), NULL);

	GtkCssProvider *css_provider;
	css_provider = gtk_css_provider_new();

	gtk_css_provider_load_from_path(css_provider, "theme/gtk.css", NULL);

	gtk_builder_connect_signals(builder, NULL);

	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
											  GTK_STYLE_PROVIDER(css_provider),
											  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gtk_widget_show(window);

	gtk_main();

	return EXIT_SUCCESS;
}
