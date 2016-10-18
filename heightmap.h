#pragma once
#include <assert.h>

//Save height data to file
void write_height_pgm(const char* filename, const unsigned char* image_data, int width, int height){
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "P2\n");
    fprintf(fp, "%d %d \n", width, height);
    fprintf(fp, "255\n"); //Colour resolution
    for(int i=0; i<height; i++){
        for(int j=0; j<width; j++){
            fprintf(fp, "%d ", image_data[width*i +j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

//Read height data from file
bool read_height_pgm(const char* filename, unsigned char** image_data, int &width, int &height){
    FILE* fp = fopen(filename, "r");
    if(!fp){
        printf("Error reading %s, file does not exist\n", filename);
        return false;
    }

    //Read header
    {
        char line[1024];
        line[0] = '\0';
        fgets(line, 1024, fp); 
        assert(line[0]=='P' && line[1]=='2');
        fgets(line, 1024, fp);
        if(sscanf(line, "%d %d", &width, &height) != 2) printf("Error reading width, height from pgm file\n");
        fgets(line, 1024, fp);
        assert(line[0]=='2' && line[1]=='5' && line[2]=='5');
    }
    *image_data = (unsigned char*)malloc(width*height*sizeof(unsigned char));

    int x = 0, i=0;
	while(!feof(fp)){
        fscanf(fp, "%d", &x);
		(*image_data)[i] = (unsigned char)x;
        i++;
    }
    fclose(fp);
    return true;
}

//Generates a square plane of n*n vertices spanning (2*size)*(2*size) world units (centered on origin)
void gen_height_field(float **verts, int &point_count, int n, float size){
    float cell_size = 2*size/(n-1);
    point_count = n*n;

    *verts = (float*)malloc(3*point_count*sizeof(float));

    int i=0;
    float z_pos =  -size;
    float x_pos;
    for(int r=0; r<n; r++){
        x_pos = -size;
        for(int c=0; c<n; c++, i++){
            (*verts)[3*i    ] = x_pos;
            (*verts)[3*i + 1] = 0;
            (*verts)[3*i + 2] = z_pos;
            x_pos+=cell_size;
        }
        z_pos+=cell_size;
    }
}

//Generates a square plane of n*n vertices spanning (2*size)*(2*size) world units (centered on origin)
//Incorporates height from image_data array
void gen_height_field(float **verts, int &point_count, const unsigned char* image_data, int n, float size){
    float cell_size = 2*size/(n-1);
    point_count = n*n;

    *verts = (float*)malloc(3*point_count*sizeof(float));

    int i=0;
    float z_pos =  -size;
    float x_pos;
    for(int r=0; r<n; r++){
        x_pos = -size;
        for(int c=0; c<n; c++, i++){
            (*verts)[3*i    ] = x_pos;
            (*verts)[3*i + 1] = image_data[i];
            (*verts)[3*i + 2] = z_pos;
            x_pos+=cell_size;
        }
        z_pos+=cell_size;
    }
}

//Writes y-components of image_data into verts and buffers data to points_vbo
void reload_height_data (float **verts, const unsigned char* image_data, int n, GLuint points_vbo){
    int i=0;
    for(int r=0; r<n; r++){
        for(int c=0; c<n; c++, i++){
            (*verts)[3*i + 1] = image_data[i];
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glBufferData(GL_ARRAY_BUFFER, n*n*3*sizeof(float), *verts, GL_STATIC_DRAW);
}

//Generates index buffer for n*n height field
void gen_heightmap_indices(int** indices, int &num_indices, int n){
    int num_quads = (n-1)*(n-1);
    num_indices = 3*2*num_quads; //3 verts per tri, 2 tris per quad
    *indices = (int*)malloc(num_indices*sizeof(int));

    int i=0;
    for(int v=n; v<n*n; v++){ //Add two triangles per quad
        if((v+1)%n != 0){
            (*indices)[i++] = v;
            (*indices)[i++] = v-n+1;
            (*indices)[i++] = v-n;

            (*indices)[i++] = v;
            (*indices)[i++] = v+1;
            (*indices)[i++] = v-n+1;
        }
    }
}
