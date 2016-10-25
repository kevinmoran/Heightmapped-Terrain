#pragma once

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memset
#include <assert.h>

#include "Camera3D.h"
#include "maths_funcs.h"

int heightmap_n = 16; //number of verts on x, z axes
float heightmap_size = 20.0f; //world units
int width, height; //in case we want non-square height map
const int heightmap_scale = 10.0f; //y-axis range
unsigned char* height_data;

float* terrain_vp; //array of vertices
int* terrain_indices; //TODO change to GL_UNSIGNED_BYTE OR GL_SHORT (2 bytes)
int terrain_point_count, terrain_num_indices;
int terrain_edit_speed = 200;

GLuint terrain_vao;
GLuint terrain_points_vbo;
GLuint terrain_index_vbo;

void init_terrain();
int get_height_index(float x, float z);
void gen_height_field(float** verts, int &point_count, int n, float size);
void gen_height_field(float** verts, int &point_count, const unsigned char* image_data, int n, float size);
void reload_height_data();
void gen_heightmap_indices(int** indices, int &num_indices, int n);
void write_height_pgm(const char* filename, const unsigned char* image_data, int width, int height);
bool read_height_pgm(const char* filename, unsigned char** image_data, int &width, int &height);

void init_terrain(){
    if(!read_height_pgm("terrain.pgm", &height_data, width, height)){
        width = heightmap_n, height = heightmap_n;
        height_data = (unsigned char*)malloc(width*height*sizeof(unsigned char));
        memset(height_data, 0, width*height);
		write_height_pgm("terrain.pgm", height_data, width, height);
    }

    //TODO call this when done editing heightmap
    // free(height_data);

    gen_height_field(&terrain_vp, terrain_point_count, height_data, heightmap_n, heightmap_size);
    gen_heightmap_indices(&terrain_indices, terrain_num_indices, heightmap_n);

	glGenBuffers(1, &terrain_points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, terrain_points_vbo);
	glBufferData(GL_ARRAY_BUFFER, terrain_point_count*3*sizeof(float), terrain_vp, GL_STATIC_DRAW);

    glGenBuffers(1, &terrain_index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_index_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrain_num_indices*sizeof(int), terrain_indices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &terrain_vao);
	glBindVertexArray(terrain_vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, terrain_points_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//free(terrain_vp); //TODO should probably free this at some stage
    free(terrain_indices);
}

void update_terrain(double dt){
    //Ray picking to raise/lower ground
    static double mouse_repeat_timer = 0.0;
    mouse_repeat_timer += dt;
    
    if(mouse_repeat_timer>1/60.0){
        mouse_repeat_timer -= 1/60.0;

        double mouse_xpos, mouse_ypos;
        glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);

        float x_nds = (cam_mouse_controls) ? 0 :(2*mouse_xpos/gl_width) - 1;
        float y_nds = (cam_mouse_controls) ? 0 : 1- (2*mouse_ypos)/gl_height;
        //vec3 ray_nds = vec3(x_nds, y_nds, 1.0f);
        vec4 ray_clip = vec4(x_nds, y_nds, -1.0f, 1.0f);
        vec4 ray_eye = inverse(g_camera.P)*ray_clip;
        ray_eye = vec4(ray_eye.v[0], ray_eye.v[1], -1.0f, 0.0f);
        vec3 ray_world = vec3(inverse(g_camera.V)*ray_eye);
        ray_world = normalise(ray_world);
        
        for (int i=0; i<500; i++) {
            vec3 interval_pos = ray_world*(float)i;
            interval_pos = g_camera.pos+interval_pos;

            int height_index = get_height_index(interval_pos.v[0], interval_pos.v[2]);
            float ground_y = (height_index<0)? -INFINITY : heightmap_scale*height_data[height_index]/255.0f;
            if(interval_pos.v[1]<ground_y) {
                if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)){
                    height_data[height_index] = MIN(height_data[height_index]+terrain_edit_speed/60, 255);
                    height_data[height_index+1] = MIN(height_data[height_index+1]+terrain_edit_speed/60, 255);
                    height_data[height_index+heightmap_n] = MIN(height_data[height_index+heightmap_n]+terrain_edit_speed/60, 255);
                    height_data[height_index+heightmap_n+1] = MIN(height_data[height_index+heightmap_n+1]+terrain_edit_speed/60, 255);
                }
                if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)){
                    height_data[height_index] = MAX(height_data[height_index]-terrain_edit_speed/60, 0);
                    height_data[height_index+1] = MAX(height_data[height_index+1]-terrain_edit_speed/60, 0);
                    height_data[height_index+heightmap_n] = MAX(height_data[height_index+heightmap_n]-terrain_edit_speed/60, 0);
                    height_data[height_index+heightmap_n+1] = MAX(height_data[height_index+heightmap_n+1]-terrain_edit_speed/60, 0);
                }
                reload_height_data();
                break;
            }
        }//endfor

    }//end if mouse button 
    
}

//Returns index for height data array to get height at world pos x,z
int get_height_index(float x, float z){
    if(x<-heightmap_size || x>heightmap_size || z<-heightmap_size || z>heightmap_size) return -1;
    float cell_size = 2*heightmap_size/(heightmap_n-1);
    int row = (z+heightmap_size)/cell_size;
    int col = (x+heightmap_size)/cell_size;
    int i = heightmap_n*row+col;
    return i;
}

//Returns height at world pos x,z interpolated from closest verts in height field
float get_height_interp(float x, float z){
    if(x<-heightmap_size || x>heightmap_size || z<-heightmap_size || z>heightmap_size) return -INFINITY;

    //Get index of top-left vert of quad containing x,z
    float cell_size = 2*heightmap_size/(heightmap_n-1);
    int row = (z+heightmap_size)/cell_size;
    int col = (x+heightmap_size)/cell_size;
    int i = heightmap_n*row+col; 

    //Get heights of this quad's verts
    float y_tl = heightmap_scale*height_data[i]/255.0f;
    float y_tr = heightmap_scale*height_data[i+1]/255.0f;
    float y_bl = heightmap_scale*height_data[i+heightmap_n]/255.0f;
    float y_br = heightmap_scale*height_data[i+heightmap_n+1]/255.0f;

    //Get x,z position of top-left vert
    float x_tl = terrain_vp[3*i];
    float z_tl = terrain_vp[3*i + 2];
    float x_t = (x-x_tl)/cell_size;
    float z_t = (z-z_tl)/cell_size;

    //Construct quad abcd
    vec3 a, b, c, d;
    vec3 norm;
    vec3 p = vec3(x,0,z);
    a = vec3(x_tl, y_tl, z_tl);
    b = vec3(x_tl, y_bl, z_tl + cell_size);
    c = vec3(x_tl+ cell_size, y_br, z_tl + cell_size);
    d = vec3(x_tl + cell_size, y_tr, z_tl);

    //Check which triangle x,z is in
    if(x_t + z_t < 1){
        //In first triangle
        // a *       * d
        //      p
        //
        // b *
        norm = normalise(cross((d-b), (a-b)));
    }
    else {
        //In second triangle
        //           * d
        //
        //        p
        // b *       * c
        norm = normalise(cross((c-b), (d-b)));
    }

    //TODO this is wrong! rethink the problem

    float y_final = fabs(dot(p, norm));
    
    return y_final;
}

//Generates a flat square plane of n*n vertices spanning (2*size)*(2*size) world units (centered on origin)
void gen_height_field(float** verts, int &point_count, int n, float size){
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
void gen_height_field(float** verts, int &point_count, const unsigned char* image_data, int n, float size){
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
            (*verts)[3*i + 1] = heightmap_scale*image_data[i]/255.0f;
            (*verts)[3*i + 2] = z_pos;
            x_pos+=cell_size;
        }
        z_pos+=cell_size;
    }
}

//Writes y-components of image_data into verts and buffers data to points_vbo
void reload_height_data (){
    int i=0;
    for(int r=0; r<heightmap_n; r++){
        for(int c=0; c<heightmap_n; c++, i++){
            terrain_vp[3*i + 1] = heightmap_scale*height_data[i]/255.0f;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, terrain_points_vbo);
	glBufferData(GL_ARRAY_BUFFER, heightmap_n*heightmap_n*3*sizeof(float), terrain_vp, GL_STATIC_DRAW);
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
