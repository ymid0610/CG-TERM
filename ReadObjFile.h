#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

typedef struct {
	float x, y, z;
} Vertex;

typedef struct {
	unsigned int v1, v2, v3;
} Face;

typedef struct {
	Vertex* vertices;
	size_t vertex_count;
	Face* faces;
	size_t face_count;
} Model;

void read_newline(char* str) {
	char* pos;
	if ((pos = strchr(str, '\n')) != NULL) {
		*pos = '\0';
	}
}

void read_obj_file(const char* filename, Model* model) {
	FILE* file;
	fopen_s(&file, filename, "r");
	if (!file) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	char line[MAX_LINE_LENGTH];
	model->vertex_count = 0;
	model->face_count = 0;

	while (fgets(line, sizeof(line), file)) {
		read_newline(line);

		if (line[0] == 'v' && line[1] == ' ') {
			model->vertex_count++;
		}
		else if (line[0] == 'f' && line[1] == ' ') {
			model->face_count++;
		}
	}

	fseek(file, 0, SEEK_SET);

	model->vertices = (Vertex*)malloc(model->vertex_count * sizeof(Vertex));
	model->faces = (Face*)malloc(model->face_count * sizeof(Face));

	size_t vertax_index = 0;
	size_t face_index = 0;

	while (fgets(line, sizeof(line), file)) {
		read_newline(line);

		if (line[0] == 'v' && line[1] == ' ') {
			int result = sscanf_s(line + 2, "%f %f %f", &model->vertices[vertax_index].x,
				&model->vertices[vertax_index].y,
				&model->vertices[vertax_index].z);
			vertax_index++;
		}
		else if (line[0] == 'f' && line[1] == ' ') {
			unsigned int v1, v2, v3;
			int result = sscanf_s(line + 2, "%u %u %u", &v1, &v2, &v3);
			model->faces[face_index].v1 = v1 - 1; // OBJ indices start at 1
			model->faces[face_index].v2 = v2 - 1;
			model->faces[face_index].v3 = v3 - 1;
			face_index++;
		}
	}
	fclose(file);
}
