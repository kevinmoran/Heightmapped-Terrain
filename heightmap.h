#pragma once

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memset
#include <assert.h>

#include "Camera3D.h"
#include "maths_funcs.h"

int heightmap_n; //number of verts on x, z axes
float heightmap_size = 20.0f; //world units
int heightmap_size_x, heightmap_size_z; //in case we want non-square height map
const int MAX_HEIGHT = 20.0f; //y-axis range
unsigned char* height_data;

float* terrain_vp; //array of vertices
float* terrain_vn;
uint16_t* terrain_indices;
int terrain_point_count, terrain_num_indices;
int terrain_edit_speed = 200;

GLuint terrain_vao, terrain_points_vbo, terrain_normals_vbo, terrain_index_vbo;

Shader heightmap_shader;

void init_terrain();
void edit_terrain(double dt);
int get_height_index(float x, float z);
float get_height_interp(float x, float z);
void gen_height_field(float** verts, int* point_count, int n, float size);
void gen_height_field(float** verts, int* point_count, const unsigned char* image_data, int n, float size);
void reload_height_data();
void gen_heightmap_indices(uint16_t** indices, int* num_indices, int n);
void gen_heightmap_normals(const float* vp, int num_verts, float** normals);
void write_height_pgm(const char* filename, const unsigned char* image_data, int width, int height);
bool read_height_pgm(const char* filename, unsigned char** image_data, int width, int height);

void init_terrain(){
    heightmap_n = 2*heightmap_size + 1;
    heightmap_size_x = heightmap_n; heightmap_size_z=heightmap_n;

    if(!read_height_pgm("terrain.pgm", &height_data, heightmap_size_x, heightmap_size_z)){
        heightmap_size_x = heightmap_n, heightmap_size_z = heightmap_n;
        height_data = (unsigned char*)malloc(heightmap_size_x*heightmap_size_z*sizeof(unsigned char));
        memset(height_data, 0, heightmap_size_x*heightmap_size_z);
		write_height_pgm("terrain.pgm", height_data, heightmap_size_x, heightmap_size_x);
    }

    //TODO call this when done editing heightmap
    // free(height_data);

    gen_height_field(&terrain_vp, &terrain_point_count, height_data, heightmap_n, heightmap_size);
    gen_heightmap_normals(terrain_vp, terrain_point_count, &terrain_vn);
    gen_heightmap_indices(&terrain_indices, &terrain_num_indices, heightmap_n);

	glGenBuffers(1, &terrain_points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, terrain_points_vbo);
	glBufferData(GL_ARRAY_BUFFER, terrain_point_count*3*sizeof(float), terrain_vp, GL_STATIC_DRAW);

    glGenBuffers(1, &terrain_normals_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, terrain_normals_vbo);
	glBufferData(GL_ARRAY_BUFFER, terrain_point_count*3*sizeof(float), terrain_vn, GL_STATIC_DRAW);

    glGenBuffers(1, &terrain_index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_index_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrain_num_indices*sizeof(uint16_t), terrain_indices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &terrain_vao);
	glBindVertexArray(terrain_vao);
	glEnableVertexAttribArray(VP_ATTRIB_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, terrain_points_vbo);
	glVertexAttribPointer(VP_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(VN_ATTRIB_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, terrain_normals_vbo);
	glVertexAttribPointer(VN_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//free(terrain_vp); //TODO should probably free this at some stage
    free(terrain_indices);

    heightmap_shader = load_shader("Heightmap.vert", "Heightmap.frag");
}

void edit_terrain(double dt){
//TODO move mouse picking etc to editor.h
//Just provide functions for editing ground here

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
        
        for (int i=0; i<50; i++) {
            vec3 interval_pos = ray_world*(float)i;
            interval_pos = g_camera.pos+interval_pos;

            int height_index = get_height_index(interval_pos.v[0], interval_pos.v[2]);
            float ground_y = (height_index<0)? -INFINITY : MAX_HEIGHT*height_data[height_index]/255.0f;
            if(interval_pos.v[1]<ground_y) {
                int draw_radius = 1;
                for(int j= -draw_radius; j<=draw_radius; j++){
                    for(int k= -draw_radius; k<=draw_radius; k++){
                        int idx = height_index+k + j*heightmap_size_x;
                        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)){
                            height_data[idx] = MIN(height_data[idx]+terrain_edit_speed/60, 255);
                        }
                        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)){
                            height_data[idx] = MAX(height_data[idx]-terrain_edit_speed/60, 0);
                        }
                    }
                }
                reload_height_data();
                //TODO recalculate normals
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
    float y_tl = MAX_HEIGHT*height_data[i]/255.0f;
    float y_tr = MAX_HEIGHT*height_data[i+1]/255.0f;
    float y_bl = MAX_HEIGHT*height_data[i+heightmap_n]/255.0f;
    float y_br = MAX_HEIGHT*height_data[i+heightmap_n+1]/255.0f;

    //Get x,z position of top-left vert
    float x_tl = terrain_vp[3*i];
    float z_tl = terrain_vp[3*i + 2];
    float x_t = (x-x_tl)/cell_size; // % along cell point is on x-axis
    float z_t = (z-z_tl)/cell_size; // % along cell point is on z-axis

    //Barycentric Interpolation
    vec3 a = vec3(x_tl+cell_size, 0, z_tl); //tr
    vec3 b = vec3(x_tl, 0, z_tl+cell_size); //bl
    vec3 c; //will be tl or br depending on x,z
    vec3 p = vec3(x,0,z);
    if(x_t+z_t > cell_size) c = vec3(x_tl+cell_size, 0, z_tl+cell_size); //br
    else c = vec3(x_tl, 0, z_tl); //tl
    float tri_area = cell_size/2;

    float u = (length(cross(b-p,c-p))/2)/tri_area; //weight of a
    float v = (length(cross(a-p,c-p))/2)/tri_area; //weight of b
    float w = (length(cross(a-p,b-p))/2)/tri_area; //weight of c

    if(x_t+z_t > cell_size) 
        return u*y_tr + v*y_bl + w*y_br;
    
    return u*y_tr + v*y_bl + w*y_tl;
}

//Returns normal at world pos x,z interpolated from closest verts in height field
vec3 get_normal_interp(float x, float z){
    if(x<-heightmap_size || x>heightmap_size || z<-heightmap_size || z>heightmap_size) return vec3(-INFINITY, -INFINITY, -INFINITY);

    //Get index of top-left vert of quad containing x,z
    float cell_size = 2*heightmap_size/(heightmap_n-1);
    int row = (z+heightmap_size)/cell_size;
    int col = (x+heightmap_size)/cell_size;
    int i = heightmap_n*row+col;

    //Get x,z position of top-left vert
    float x_tl = terrain_vp[3*i];
    float z_tl = terrain_vp[3*i + 2];
    float x_t = (x-x_tl)/cell_size; // % along cell point is on x-axis
    float z_t = (z-z_tl)/cell_size; // % along cell point is on z-axis

    // Get surrounding normals
    vec3 norm_a = vec3(terrain_vn[3*(i+1)], terrain_vn[3*(i+1)+1], terrain_vn[3*(i+1)+2]); //tr
    vec3 norm_b = vec3(terrain_vn[3*(i+heightmap_size_x)], terrain_vn[3*(i+heightmap_size_x)+1], terrain_vn[3*(i+heightmap_size_x)+2]); //bl
    vec3 norm_c;
    if(x_t+z_t > cell_size) norm_c = vec3(terrain_vn[3*(i+heightmap_size_x+1)], terrain_vn[3*(i+heightmap_size_x+1)+1], terrain_vn[3*(i+heightmap_size_x+1)+2]); //br
    else norm_c = vec3(terrain_vn[3*i], terrain_vn[3*i+1], terrain_vn[3*i+2]); //tl

    //Barycentric Interpolation
    vec3 a = vec3(x_tl+cell_size, 0, z_tl); //tr
    vec3 b = vec3(x_tl, 0, z_tl+cell_size); //bl
    vec3 c; //will be tl or br depending on x,z
    vec3 p = vec3(x,0,z);
    if(x_t+z_t > cell_size) c = vec3(x_tl+cell_size, 0, z_tl+cell_size); //br
    else c = vec3(x_tl, 0, z_tl); //tl
    float tri_area = cell_size/2;

    float u = (length(cross(b-p,c-p))/2)/tri_area; //weight of a
    float v = (length(cross(a-p,c-p))/2)/tri_area; //weight of b
    float w = (length(cross(a-p,b-p))/2)/tri_area; //weight of c

    return normalise(norm_a*u + norm_b*v + norm_c*w);
}

//Generates a flat square plane of n*n vertices spanning (2*size)*(2*size) world units (centered on origin)
void gen_height_field(float** verts, int* point_count, int n, float size){
    float cell_size = 2*size/(n-1);
    *point_count = n*n;

    *verts = (float*)malloc(3*(*point_count)*sizeof(float));

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
void gen_height_field(float** verts, int* point_count, const unsigned char* image_data, int n, float size){
    float cell_size = 2*size/(n-1);
    *point_count = n*n;

    *verts = (float*)malloc(3*(*point_count)*sizeof(float));

    int i=0;
    float z_pos = -size;
    float x_pos;
    for(int r=0; r<n; r++){
        x_pos = -size;
        for(int c=0; c<n; c++, i++){
            (*verts)[3*i    ] = x_pos;
            (*verts)[3*i + 1] = MAX_HEIGHT*image_data[i]/255.0f;
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
            terrain_vp[3*i + 1] = MAX_HEIGHT*height_data[i]/255.0f;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, terrain_points_vbo);
	glBufferData(GL_ARRAY_BUFFER, heightmap_n*heightmap_n*3*sizeof(float), terrain_vp, GL_STATIC_DRAW);
}

//Generates index buffer for n*n height field
void gen_heightmap_indices(uint16_t** indices, int* num_indices, int n){
    int num_quads = (n-1)*(n-1);
    *num_indices = 3*2*num_quads; //3 verts per tri, 2 tris per quad
    *indices = (uint16_t*)malloc(*num_indices*sizeof(uint16_t));

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

//Generate normals from set of heightmap verts
void gen_heightmap_normals(const float* vp, int num_verts, float** normals){
    *normals = (float*)malloc(num_verts*3*sizeof(float));

    for(int i=0; i<num_verts; i++){
        float l,r,t,b; // 4 surrounding height values
        if(i%heightmap_size_x) 
            l = vp[3*(i-1)+1];
        else l = vp[3*i+1];
        if((i+1)%heightmap_size_x) 
            r = vp[3*(i+1)+1];
        else r = vp[3*i+1];
        if(i>heightmap_size_x) 
            t = vp[3*(i-heightmap_size_x)+1];
        else t = vp[3*i+1];
        if(i+heightmap_size_x<num_verts) 
            b = vp[3*(i+heightmap_size_x)+1];
        else b = vp[3*i+1];

        vec3 d_x = normalise(vec3(1, r-l, 0));
        vec3 d_y = normalise(vec3(0, b-t, 1));
        vec3 norm = cross(d_y,d_x);
        assert(length(norm)-1<0.00001f);
        (*normals)[3*i] = norm.v[0];
        (*normals)[3*i + 1] = norm.v[1];
        (*normals)[3*i + 2] = norm.v[2];
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
bool read_height_pgm(const char* filename, unsigned char** image_data, int width, int height){
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
        int w, h;
        if(sscanf(line, "%d %d", &w, &h) != 2) printf("Error reading width, height from pgm file\n");
        if(width!=w || height!=h) {
            printf("Error reading pgm file; unexpected dimensions\n");
            return false;
        }
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
