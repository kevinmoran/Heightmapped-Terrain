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

    int heightmap_n = 64;
    int width=heightmap_n, height=heightmap_n;
    unsigned char* image_data;

    if(!read_height_pgm("terrain.pgm", &image_data, width, height)){
        width = 64, height = 64;
        image_data = (unsigned char*)malloc(width*height*sizeof(unsigned char));
        memset(image_data, 0, width*height);
    }

    //TODO call this when done editing heightmap
    // free(image_data);

    float* vp;
    int* indices; //TODO change to GL_UNSIGNED_BYTE OR GL_SHORT (2 bytes)
    int point_count, num_indices;

    gen_height_field(&vp, point_count, image_data, heightmap_n, 10.0f);
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

        //Save updated image_data to file
        if (glfwGetKey(window, GLFW_KEY_H)) {
            write_height_pgm("terrain.pgm", image_data, width, height);
        }

        //Ray picking
        double mouse_xpos, mouse_ypos;
        glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);
        if(mouse_xpos>0 && mouse_xpos<gl_width && mouse_ypos>0 && mouse_ypos<gl_height) {
            //Mouse is within viewport
            //Transform ray to world space
            //March ray forward and check if it is below heightfield at each step

        }

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
        glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);
	}//end main loop

    return 0;
}
