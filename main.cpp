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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

    init_terrain();

    //Load shader
	Shader heightmap_shader("Heightmap.vert", "Heightmap.frag");
	glUseProgram(heightmap_shader.prog_id);

    fly_cam.init(vec3(0,12,15), vec3(0,0,0));
    glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, fly_cam.P.m);
    glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);

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
		// static bool fly_cam_enabled = true;
		// static bool F_was_pressed = false;
		// if (glfwGetKey(window, GLFW_KEY_F)) {
		// 	if(!F_was_pressed) {
		// 		fly_cam_enabled = !fly_cam_enabled;
		// 		F_was_pressed = true;
		// 	}
		// }
		// else F_was_pressed = false;

		static bool gravity_enabled = false;
		static bool G_was_pressed = false;
		if (glfwGetKey(window, GLFW_KEY_G)) {
			if(!G_was_pressed) {
				gravity_enabled = !gravity_enabled;
				G_was_pressed = true;
			}
		}
		else G_was_pressed = false;

       update_terrain();

		if(gravity_enabled){
			static float y_acceleration = 0.0f;
			y_acceleration -= 10*9.81*dt;
			fly_cam.pos.v[1] += y_acceleration*dt;
			int height_index = get_height_index(fly_cam.pos.v[0], fly_cam.pos.v[2]);
            float ground_y = (height_index<0)? -INFINITY : heightmap_scale*height_data[height_index]/255.0f;
            
			if(fly_cam.pos.v[1] - ground_y < 1) {
				fly_cam.pos.v[1] = 1;
				y_acceleration = 0.0f;
			}
			
			if(fly_cam.pos.v[0] < -heightmap_size) {
				fly_cam.pos.v[0] = -heightmap_size;
			}
			if(fly_cam.pos.v[0] > heightmap_size) {
				fly_cam.pos.v[0] = heightmap_size;
			}
			if(fly_cam.pos.v[2] < -heightmap_size) {
				fly_cam.pos.v[2] = -heightmap_size;
			}
			if(fly_cam.pos.v[2] > heightmap_size) {
				fly_cam.pos.v[2] = heightmap_size;
			}
		}
		fly_cam.update(dt);

		static bool M_was_pressed = false;
		if (glfwGetKey(window, GLFW_KEY_M)) {
			if(!M_was_pressed) {
				cam_mouse_controls = !cam_mouse_controls;
				if(glfwGetInputMode(window, GLFW_CURSOR)==GLFW_CURSOR_NORMAL){
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
				}
				else{
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); 
				}
				M_was_pressed = true;
			}
		}
		else M_was_pressed = false;

		if (glfwGetKey(window, GLFW_KEY_R)) {
			heightmap_shader.reload_shader_program("Heightmap.vert", "Heightmap.frag");
			glUseProgram(heightmap_shader.prog_id);
			glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, fly_cam.P.m);
			glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
		}

		//Rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(heightmap_shader.prog_id);
		glUniformMatrix4fv(heightmap_shader.V_loc, 1, GL_FALSE, fly_cam.V.m);

		//Draw terrain
		glBindVertexArray(terrain_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_index_vbo);
        glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);
	}//end main loop

    return 0;
}
