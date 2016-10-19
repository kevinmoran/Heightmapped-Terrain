#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GLFWwindow* window = NULL;
int gl_width = 360;
int gl_height = 240;
float gl_aspect_ratio = (float)gl_width/gl_height;

#include "init_gl.h"
#include "maths_funcs.h"
#include "Shader.h"
#include "FlyCam.h"
#include "heightmap.h"

int main(){
	if (!init_gl(window, gl_width, gl_height)){ return 1; }

    int heightmap_n = 8;
	float heightmap_size = 10.0f;
    int width, height;
    unsigned char* image_data;

    if(!read_height_pgm("terrain.pgm", &image_data, width, height)){
        width = heightmap_n, height = heightmap_n;
        image_data = (unsigned char*)malloc(width*height*sizeof(unsigned char));
        memset(image_data, 0, width*height);
		write_height_pgm("terrain.pgm", image_data, width, height);
    }

    //TODO call this when done editing heightmap
    // free(image_data);

    float* vp;
    int* indices; //TODO change to GL_UNSIGNED_BYTE OR GL_SHORT (2 bytes)
    int point_count, num_indices;

    gen_height_field(&vp, point_count, image_data, heightmap_n, heightmap_size);
    gen_heightmap_indices(&indices, num_indices, heightmap_n);

	GLuint points_vbo;
	glGenBuffers(1, &points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glBufferData(GL_ARRAY_BUFFER, point_count*3*sizeof(float), vp, GL_STATIC_DRAW);

    GLuint index_vbo;
    glGenBuffers(1, &index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices*sizeof(int), indices, GL_STATIC_DRAW);

    GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//free(vp); //TODO should probably free this at some stage
    free(indices);

    //Load shader
	Shader pass_shader("MVP.vert", "uniform_colour.frag");
	GLuint colour_loc = glGetUniformLocation(pass_shader.prog_id, "colour");
	glUseProgram(pass_shader.prog_id);

    FlyCam fly_cam;
    fly_cam.init(vec3(0,12,15), vec3(0,0,0));
    glUniformMatrix4fv(pass_shader.P_loc, 1, GL_FALSE, fly_cam.P.m);
    glUniform4fv(colour_loc, 1, vec4(0.2f,0.4f,0.1f,1).v);
    glUniformMatrix4fv(pass_shader.M_loc, 1, GL_FALSE, identity_mat4().m);

    double curr_time = glfwGetTime(), prev_time, dt;
	//-------------------------------------------------------------------------------------//
	//-------------------------------------MAIN LOOP---------------------------------------//
	//-------------------------------------------------------------------------------------//
	while (!glfwWindowShouldClose(window)) {
		//Get dt
		prev_time = curr_time;
		curr_time = glfwGetTime();
		dt = curr_time - prev_time;
		if (dt > 0.1) dt = 0.1;

		//Get Input
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
			continue;
		}
		static bool fly_cam_enabled = true;
		static bool F_was_pressed = false;
		if (glfwGetKey(window, GLFW_KEY_F)) {
			if(!F_was_pressed) {
				fly_cam_enabled = !fly_cam_enabled;
				F_was_pressed = true;
			}
		}
		else F_was_pressed = false;

        if (glfwGetKey(window, GLFW_KEY_R)) {
            //Call this after altering image_data
            reload_height_data(&vp, image_data, heightmap_n, points_vbo);
        }

        if (glfwGetKey(window, GLFW_KEY_H)) {
            write_height_pgm("terrain.pgm", image_data, width, height);
        }

        //Ray picking
		if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)){
			double mouse_xpos, mouse_ypos;
			glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);

			float x_nds = (2*mouse_xpos/gl_width) - 1;
			float y_nds = 1- (2*mouse_ypos)/gl_height;
			//vec3 ray_nds = vec3(x_nds, y_nds, 1.0f);
			vec4 ray_clip = vec4(x_nds, y_nds, -1.0f, 1.0f);
			vec4 ray_eye = inverse(fly_cam.P)*ray_clip;
			ray_eye = vec4(ray_eye.v[0], ray_eye.v[1], -1.0f, 0.0f);
			vec3 ray_world = vec3(inverse(fly_cam.V)*ray_eye);
			ray_world = normalise(ray_world);
			
			for (int i=0; i<500; i++) {
				vec3 interval_pos = ray_world*(float)i;
				interval_pos = fly_cam.pos+interval_pos;

				int height_index = get_height_index(heightmap_n, heightmap_size, interval_pos.v[0], interval_pos.v[2]);
				float ground_y = (height_index<0)? -INFINITY : heightmap_scale*image_data[height_index]/255.0f;
				if(interval_pos.v[1]<ground_y) {
					image_data[height_index]++;
					image_data[height_index+1]++;
					image_data[height_index+heightmap_n]++;
					image_data[height_index+heightmap_n+1]++;
					reload_height_data(&vp, image_data, heightmap_n, points_vbo);
					break;
				}
			}//endfor

		}//end if mouse button 

		if(fly_cam_enabled) fly_cam.update(dt);
		else{
			// if(g_input[MOVE_FORWARD]) {
			// }
			// if(g_input[MOVE_LEFT]) {
			// }
			// if(g_input[MOVE_BACK]) {		
			// }
			// if(g_input[MOVE_RIGHT]) {		
			// }
		}

		//Rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(pass_shader.prog_id);
		glBindVertexArray(vao);
		glUniformMatrix4fv(pass_shader.V_loc, 1, GL_FALSE, fly_cam.V.m);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
        glDrawElements(GL_LINE_LOOP, num_indices, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);
	}//end main loop

    return 0;
}
