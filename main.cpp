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

    init_terrain();

    //Load shader
	Shader pass_shader("MVP.vert", "uniform_colour.frag");
	GLuint colour_loc = glGetUniformLocation(pass_shader.prog_id, "colour");
	glUseProgram(pass_shader.prog_id);

    fly_cam.init(vec3(0,12,15), vec3(0,0,0));
    glUniformMatrix4fv(pass_shader.P_loc, 1, GL_FALSE, fly_cam.P.m);
    glUniform4fv(colour_loc, 1, vec4(0.2f, 0.4f, 0.1f, 1).v);
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

       update_terrain();

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
		glUniformMatrix4fv(pass_shader.V_loc, 1, GL_FALSE, fly_cam.V.m);

		//Draw terrain
		glBindVertexArray(terrain_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_index_vbo);
        glDrawElements(GL_LINE_LOOP, terrain_num_indices, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);
	}//end main loop

    return 0;
}
