#pragma once

struct Terrain;
void  init_terrain        (Terrain* t);
void  edit_terrain        (Terrain* t, float paint_radius, int height_index, double dt);
int   get_height_index    (Terrain &t, float x, float z);
float get_height_interp   (Terrain &t, float x, float z);
vec3  get_normal_interp   (Terrain &t, float x, float z);
vec3  get_displacement    (Terrain &t, float x, float y, float z);
void  recalculate_normals (Terrain* t);
bool  load_terrain        (Terrain* t);
void  save_terrain        (Terrain &t);
void  clear_terrain       (Terrain *t);

#define TERRAIN_FILENAME_SIZE 32
struct Terrain {
    char filename[TERRAIN_FILENAME_SIZE];
    //vec3 pos;
    float width, length, height;
    float cell_size;
    int num_verts_x, num_verts_z;

    float *vp, *vn;
    int point_count, num_indices;

    GLuint vao, points_vbo, norm_vbo, index_vbo;
    int pgm_res; //resolution of height data for pgm storage
};
Terrain g_terrain;

Shader heightmap_shader;

#define DEFAULT_TERRAIN_HEIGHT 20
#define DEFAULT_TERRAIN_CELL_SIZE 2
#define DEFAULT_TERRAIN_NUM_X 8
#define DEFAULT_TERRAIN_NUM_Z 8
#define DEFAULT_TERRAIN_PGM_RES 255

void init_terrain(Terrain* t, const char* file)
{
    assert(strlen(file) < TERRAIN_FILENAME_SIZE);
    strncpy(t->filename, file, TERRAIN_FILENAME_SIZE);

    //Check if terrain is saved on disc
    if(!load_terrain(t))
    {
        //Init with defaults
        t->height = DEFAULT_TERRAIN_HEIGHT;
        t->cell_size = DEFAULT_TERRAIN_CELL_SIZE;
        t->num_verts_x = DEFAULT_TERRAIN_NUM_X;
        t->num_verts_z = DEFAULT_TERRAIN_NUM_Z;
        t->width = DEFAULT_TERRAIN_CELL_SIZE*(DEFAULT_TERRAIN_NUM_X-1); 
        t->length = DEFAULT_TERRAIN_CELL_SIZE*(DEFAULT_TERRAIN_NUM_Z-1);
        t->pgm_res = DEFAULT_TERRAIN_PGM_RES;
        t->point_count = t->num_verts_x * t->num_verts_z;
        t->vp = (float*)malloc(t->point_count*3*sizeof(float));

        int i=0;
        float z_pos = -t->length/2.0f;
        float x_pos;
        for(int r=0; r<t->num_verts_z; r++){
            x_pos = -t->width/2.0f;
            for(int c=0; c<t->num_verts_x; c++, i++){
                assert(i<t->point_count);
                t->vp[3*i    ] = x_pos;
                t->vp[3*i + 1] = 0;
                t->vp[3*i + 2] = z_pos;
                //print(vec3(x_pos,0,z_pos));
                x_pos+=t->cell_size;
            }
            z_pos+=t->cell_size;
        }
    }

    t->vn = (float*)malloc(t->point_count*3*sizeof(float));

    int num_quads = (t->num_verts_x-1)*(t->num_verts_z-1);
    t->num_indices = 3*2*num_quads; //3 verts per tri, 2 tris per quad

    uint16_t* indices = (uint16_t*)calloc(t->num_indices, sizeof(uint16_t));
    {
        int i=0;
        for(int v=t->num_verts_x; v<t->num_verts_x*t->num_verts_z; v++){ //Add two triangles per quad
            if((v+1) % t->num_verts_x != 0){
                indices[i++] = v;
                indices[i++] = v-t->num_verts_x+1;
                indices[i++] = v-t->num_verts_x;

                indices[i++] = v;
                indices[i++] = v+1;
                indices[i++] = v-t->num_verts_x+1;
            }
        }
    }

    glGenVertexArrays(1, &t->vao);
	glBindVertexArray(t->vao);

	glGenBuffers(1, &t->points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, t->points_vbo);
	glBufferData(GL_ARRAY_BUFFER, t->point_count*3*sizeof(float), t->vp, GL_STATIC_DRAW);

    glGenBuffers(1, &t->norm_vbo);
    recalculate_normals(t);

    glGenBuffers(1, &t->index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->index_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, t->num_indices*sizeof(uint16_t), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(VP_ATTRIB_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, t->points_vbo);
	glVertexAttribPointer(VP_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(VN_ATTRIB_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, t->norm_vbo);
	glVertexAttribPointer(VN_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    free(indices);

    heightmap_shader = init_shader("Heightmap.vert", "Heightmap.frag");
    glUseProgram(heightmap_shader.id);
    glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, g_camera.P.m);
    glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
}

//Returns index for closest terrain point to world pos x,z
int get_terrain_index(Terrain &t, float x, float z)
{
    if(x<-t.width/2 || x>t.width/2 || z<-t.length/2 || z>t.length/2) return -1;
    int row = (z+t.length/2)/t.cell_size;
    int col = (x+t.width/2)/t.cell_size;
    int i = t.num_verts_x*row+col;

    float x_tl = t.vp[3*i];
    float z_tl = t.vp[3*i + 2];
    float x_t = (x-x_tl)/t.cell_size; // % along cell point is on x-axis
    float z_t = (z-z_tl)/t.cell_size; // % along cell point is on z-axis

    if(x_t>0.5) i+=1;
    if(z_t>0.5) i+=t.num_verts_x;

    return i;
}

//Returns height at world pos x,z interpolated from closest verts in height field
float get_height_interp(Terrain &t, float x, float z)
{
    if(x<-t.width/2 || x>t.width/2 || z<-t.length/2 || z>t.length/2) return -INFINITY;

    //Get index of top-left vert of quad containing x,z
    int row = (z+t.length/2)/t.cell_size;
    int col = (x+t.width/2)/t.cell_size;
    int i = t.num_verts_x*row+col;

    //Get heights of this quad's verts
    float y_tl = t.vp[3*i + 1];
    float y_tr = t.vp[3*(i+1)+1];
    float y_bl = t.vp[3*(i+t.num_verts_x)+1];
    float y_br = t.vp[3*(i+t.num_verts_x+1)+1];

    //Get x,z position of top-left vert
    float x_tl = t.vp[3*i];
    float z_tl = t.vp[3*i + 2];
    float x_t = (x-x_tl)/t.cell_size; // % along cell point is on x-axis
    float z_t = (z-z_tl)/t.cell_size; // % along cell point is on z-axis

    // vec3 topleft  = vec3(x_tl, y_tl, z_tl);
    // vec3 topright = vec3(t.vp[3*(i+1)], y_tr, t.vp[3*(i+1)+2]);
    // vec3 botleft  = vec3(t.vp[3*(i+t.num_verts_x)], y_bl, t.vp[3*(i+t.num_verts_x)+2]);
    // vec3 botright = vec3(t.vp[3*(i+t.num_verts_x+1)], y_br, t.vp[3*(i+t.num_verts_x+1)+2]);

    // draw_point(topleft,  0.2, vec4(1,1,0,1));
    // draw_point(topright, 0.2, vec4(1,1,0,1));
    // draw_point(botleft,  0.2, vec4(1,1,0,1));
    // draw_point(botright, 0.2, vec4(1,1,0,1));

    //Barycentric Interpolation
    vec3 a = vec3(x_tl+t.cell_size, 0, z_tl); //tr
    vec3 b = vec3(x_tl, 0, z_tl+t.cell_size); //bl
    vec3 c; //will be tl or br depending on x,z
    vec3 p = vec3(x,0,z);
    if(x_t+z_t > 1) 
        c = vec3(x_tl+t.cell_size, 0, z_tl+t.cell_size); //br
    else 
        c = vec3(x_tl, 0, z_tl); //tl
    float tri_area = t.cell_size/2;

    float u = (length(cross(b-p,c-p))/(2*t.cell_size))/tri_area; //weight of a
    float v = (length(cross(a-p,c-p))/(2*t.cell_size))/tri_area; //weight of b
    float w = (length(cross(a-p,b-p))/(2*t.cell_size))/tri_area; //weight of c

    if(x_t+z_t > 1) 
        return u*y_tr + v*y_bl + w*y_br;
    
    return u*y_tr + v*y_bl + w*y_tl;
}

//Returns normal at world pos x,z interpolated from closest verts in height field
vec3 get_normal_interp(Terrain &t, float x, float z)
{
    if(x<-t.width/2 || x>t.width/2 || z<-t.length/2 || z>t.length/2) return vec3(-INFINITY, -INFINITY, -INFINITY);
    
    //Get index of top-left vert of quad containing x,z
    int row = (z+t.length/2)/t.cell_size;
    int col = (x+t.width/2)/t.cell_size;
    int i = t.num_verts_x*row+col;

    //Get x,z position of top-left vert
    float x_tl = t.vp[3*i];
    float z_tl = t.vp[3*i + 2];
    float x_t = (x-x_tl)/t.cell_size; // % along cell point is on x-axis
    float z_t = (z-z_tl)/t.cell_size; // % along cell point is on z-axis

    // Get surrounding normals
    vec3 norm_a = vec3(t.vn[3*(i+1)], t.vn[3*(i+1)+1], t.vn[3*(i+1)+2]); //tr
    vec3 norm_b = vec3(t.vn[3*(i+t.num_verts_x)], t.vn[3*(i+t.num_verts_x)+1], t.vn[3*(i+t.num_verts_x)+2]); //bl
    vec3 norm_c;
    if(x_t+z_t > t.cell_size) norm_c = vec3(t.vn[3*(i+t.num_verts_x+1)], t.vn[3*(i+t.num_verts_x+1)+1], t.vn[3*(i+t.num_verts_x+1)+2]); //br
    else norm_c = vec3(t.vn[3*i], t.vn[3*i+1], t.vn[3*i+2]); //tl

    //Barycentric Interpolation
    vec3 a = vec3(x_tl+t.cell_size, 0, z_tl); //tr
    vec3 b = vec3(x_tl, 0, z_tl+t.cell_size); //bl
    vec3 c; //will be tl or br depending on x,z
    vec3 p = vec3(x,0,z);
    if(x_t+z_t > t.cell_size) c = vec3(x_tl+t.cell_size, 0, z_tl+t.cell_size); //br
    else c = vec3(x_tl, 0, z_tl); //tl
    float tri_area = t.cell_size/2;

    float u = (length(cross(b-p,c-p))/2)/tri_area; //weight of a
    float v = (length(cross(a-p,c-p))/2)/tri_area; //weight of b
    float w = (length(cross(a-p,b-p))/2)/tri_area; //weight of c

    return normalise(norm_a*u + norm_b*v + norm_c*w);
}

//Returns vector to closest point on ground from point x,y,z along surface normal
vec3 get_displacement(Terrain &t, float x, float y, float z)
{
    if(x<-t.width/2 || x>t.width/2 || z<-t.length/2 || z>t.length/2) return vec3(-INFINITY, -INFINITY, -INFINITY);

    //Get index of top-left vert of quad containing x,z
    int row = (z+t.length)/t.cell_size;
    int col = (x+t.width)/t.cell_size;
    int i = t.num_verts_x*row+col;

    //Get x,z position of top-left vert
    float x_tl = t.vp[3*i];
    float z_tl = t.vp[3*i + 2];
    float x_t = (x-x_tl)/t.cell_size; // % along cell point is on x-axis
    float z_t = (z-z_tl)/t.cell_size; // % along cell point is on z-axis

    // Get surrounding normals
    vec3 norm_a = vec3(t.vn[3*(i+1)], t.vn[3*(i+1)+1], t.vn[3*(i+1)+2]); //tr
    vec3 norm_b = vec3(t.vn[3*(i+t.num_verts_x)], t.vn[3*(i+t.num_verts_x)+1], t.vn[3*(i+t.num_verts_x)+2]); //bl
    vec3 norm_c;
    if(x_t+z_t > t.cell_size) norm_c = vec3(t.vn[3*(i+t.num_verts_x+1)], t.vn[3*(i+t.num_verts_x+1)+1], t.vn[3*(i+t.num_verts_x+1)+2]); //br
    else norm_c = vec3(t.vn[3*i], t.vn[3*i+1], t.vn[3*i+2]); //tl

    //Barycentric Interpolation
    vec3 a = vec3(x_tl+t.cell_size, 0, z_tl); //tr
    vec3 b = vec3(x_tl, 0, z_tl+t.cell_size); //bl
    vec3 c; //will be tl or br depending on x,z
    vec3 p = vec3(x,0,z);
    if(x_t+z_t > t.cell_size) c = vec3(x_tl+t.cell_size, 0, z_tl+t.cell_size); //br
    else c = vec3(x_tl, 0, z_tl); //tl
    float tri_area = t.cell_size/2;

    float u = (length(cross(b-p,c-p))/2)/tri_area; //weight of a
    float v = (length(cross(a-p,c-p))/2)/tri_area; //weight of b
    float w = (length(cross(a-p,b-p))/2)/tri_area; //weight of c

    vec3 norm = normalise(norm_a*u + norm_b*v + norm_c*w);
    vec3 top_rgt = a;
    top_rgt.y = t.vp[3*(i+1)+1];
    float disp = dot(vec3(x,y,z)-top_rgt, norm);

    //DEBUG: Draw tri's normals
    // a.y = t.vp[3*(i+1) + 1];
    // b.y = t.vp[3*(i+t_num_verts_x) + 1];
    // if(x_t+z_t > t.cell_size) c.y = t.vp[3*(i+t.num_verts_x+1) + 1]; //br
    // else c.y = t.vp[3*i + 1];; //tl

    // draw_vec(a, norm_a, vec4(0.8f, 0.7f, 0.1f, 1));
    // draw_vec(b, norm_b, vec4(0.8f, 0.7f, 0.1f, 1));
    // draw_vec(c, norm_c, vec4(0.8f, 0.7f, 0.1f, 1));

    return norm*disp;
}

void recalculate_normals(Terrain* t)
{
    for(int i=0; i<(t->point_count); i++)
    {
        float l,r,a,b; // 4 surrounding height values
        if(i % (t->num_verts_x)) 
            l = t->vp[3*(i-1)+1];
        else l = t->vp[3*i+1];
        if((i+1)%t->num_verts_x) 
            r = t->vp[3*(i+1)+1];
        else r = t->vp[3*i+1];
        if(i>t->num_verts_x) 
            a = t->vp[3*(i-t->num_verts_x)+1];
        else a = t->vp[3*i+1];
        if(i+t->num_verts_x<t->point_count) 
            b = t->vp[3*(i+t->num_verts_x)+1];
        else b = t->vp[3*i+1];

        vec3 d_x = vec3(1, r-l, 0);
        vec3 d_y = vec3(0, b-a, 1);
        vec3 norm = normalise(cross(d_y,d_x));
        t->vn[3*i] = norm.x;
        t->vn[3*i + 1] = norm.y;
        t->vn[3*i + 2] = norm.z;
    }
    glBindBuffer(GL_ARRAY_BUFFER, t->norm_vbo);
	glBufferData(GL_ARRAY_BUFFER, t->point_count*3*sizeof(float), t->vn, GL_STATIC_DRAW);
}

//Utility function: reads past comments, spaces and newlines in a pgm file
void _pgm_skip_fluff(FILE* fp)
{
    char c;
    while((c = fgetc(fp)) && ((c==' ') || (c=='#') || (c=='\n')) )
        if(c=='#') while((c = fgetc(fp)) && (c!='\n'));

    fseek(fp, -1, SEEK_CUR); //rewind 1 char
}

//Read height data from file
bool load_terrain(Terrain* t){
    FILE* fp = fopen(t->filename, "r");
    if(!fp){
        printf("Error reading %s, file does not exist\n", t->filename);
        return false;
    }

    //Read header
    {
        //Preamble (comments, spaces etc)
        _pgm_skip_fluff(fp);

        //Magic number (should be P2)
        char type[2];
        type[0] = fgetc(fp); 
        type[1] = fgetc(fp);
        assert(type[0]=='P' && type[1]=='2');
        _pgm_skip_fluff(fp);

        //Dimensions
        int w;
        {
            char c[16]; int i=0;
            while((c[i]=fgetc(fp)) && (c[i]!=' ') && (c[i]!='\n')) ++i;
            sscanf(c, "%d", &w);
            //printf("width: %d\n", w);
            _pgm_skip_fluff(fp);
        }
        int l;
        {
            char c[16]; int i=0;
            while((c[i]=fgetc(fp)) && (c[i]!=' ') && (c[i]!='\n')) ++i;
            sscanf(c, "%d", &l);
            //printf("length: %d\n", l);
            _pgm_skip_fluff(fp);
        }
        
        t->num_verts_x = w;
        t->num_verts_z = l;
        t->point_count = t->num_verts_x * t->num_verts_z;

        //Colour resolution
        int res;
        {
            char c[16]; int i=0;
            while((c[i]=fgetc(fp)) && (c[i]!=' ') && (c[i]!='\n')) ++i;
            sscanf(c, "%d", &res);
            //printf("res: %d\n", res);
            _pgm_skip_fluff(fp);
        }
        t->pgm_res = res;
    }

    t->vp = (float*)malloc(t->point_count*3*sizeof(float));

    t->height = DEFAULT_TERRAIN_HEIGHT;
    t->cell_size = DEFAULT_TERRAIN_CELL_SIZE;
    t->width = (t->num_verts_x-1)*(t->cell_size); 
    t->length = (t->num_verts_z-1)*(t->cell_size);

    int i=0;
    float z_pos = -t->length/2.0f;
    float x_pos;
    for(int r=0; r<t->num_verts_z; r++){
        x_pos = -t->width/2.0f;
        for(int c=0; c<t->num_verts_x; c++, i++){
            assert(i<t->point_count);

            int height_value;
            fscanf(fp, "%d", &height_value);
            t->vp[3*i    ] = x_pos;
            t->vp[3*i + 1] = t->height*(float)height_value/t->pgm_res;
            t->vp[3*i + 2] = z_pos;
            x_pos+=t->cell_size;
        }
        z_pos+=t->cell_size;
    }

    fclose(fp);
    return true;
}

//Save height data to file
void save_terrain(Terrain &t){
    FILE* fp = fopen(t.filename, "w");
    fprintf(fp, "P2\n");
    fprintf(fp, "%d %d \n", t.num_verts_x, t.num_verts_z);
    fprintf(fp, "%d\n", t.pgm_res); //Colour resolution

    for(int i=0; i<t.num_verts_z; i++){
        for(int j=0; j<t.num_verts_x; j++){
            int index = t.num_verts_x*i+j;
            fprintf(fp, "%d ", int(t.pgm_res*t.vp[3*index+1]/t.height));
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void clear_terrain(Terrain *t)
{
    if(t->vp) free(t->vp);
    if(t->vn) free(t->vn);
    t->vao = -1;
}
