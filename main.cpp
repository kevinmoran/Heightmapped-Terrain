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
#include "load_obj.h"
#include "Shader.h"
#include "Camera3D.h"
#include "heightmap.h"
#include "editor.h"
#include "Player.h"

int main(){
	if(!init_gl(window, "Terrain", gl_width, gl_height)){ return 1; }

	float* cube_vp = NULL;
	unsigned short* cube_indices = NULL;
	int cube_num_indices = 0, cube_num_verts = 0;
	load_obj_indexed("cube.obj", &cube_vp, &cube_indices, &cube_num_indices, &cube_num_verts);

	GLuint cube_vao;
	{ //Setup cube geometry
		glGenVertexArrays(1, &cube_vao);
		glBindVertexArray(cube_vao);

		GLuint cube_points_vbo, cube_index_vbo;
		glGenBuffers(1, &cube_points_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, cube_points_vbo);
		glBufferData(GL_ARRAY_BUFFER, cube_num_verts*3*sizeof(float), cube_vp, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free(cube_vp);

		glGenBuffers(1, &cube_index_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_index_vbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_num_indices*sizeof(unsigned short), cube_indices, GL_STATIC_DRAW);
		free(cube_indices);
	}

    g_camera.init(vec3(0,2,5));

    init_terrain();

    //Load shaders
	Shader basic_shader = init_shader("MVP.vert", "uniform_colour.frag");
	GLuint colour_loc = glGetUniformLocation(basic_shader.id, "colour");

	//TODO: player_init();

	check_gl_error();

    double curr_time = glfwGetTime(), prev_time, dt;
	//-------------------------------------------------------------------------------------//
	//-------------------------------------MAIN LOOP---------------------------------------//
	//-------------------------------------------------------------------------------------//
	while(!glfwWindowShouldClose(window)) {
		//Get dt
		prev_time = curr_time;
		curr_time = glfwGetTime();
		dt = curr_time - prev_time;
		if(dt > 0.1) dt = 0.1;

		//Get Input
		glfwPollEvents();
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
			continue;
		}

		static bool edit_mode = false;
		static bool tab_was_pressed = false;
		if(glfwGetKey(window, GLFW_KEY_TAB)) {
			if(!tab_was_pressed) {
				edit_mode = !edit_mode;
				write_height_pgm("terrain.pgm", height_data, heightmap_size_x, heightmap_size_z);
				tab_was_pressed = true;
			}
		}
		else tab_was_pressed = false;

		if(edit_mode){
       		editor_update(dt);
		}
		else {
			player_update(dt);
		}

		//Rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(heightmap_shader.id);
		glUniformMatrix4fv(heightmap_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, g_camera.P.m);

		//Draw terrain
		glBindVertexArray(terrain_vao);
        glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_SHORT, 0);

		glUseProgram(basic_shader.id);
		glUniformMatrix4fv(basic_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(basic_shader.P_loc, 1, GL_FALSE, g_camera.P.m);

		//Draw terrain wireframe
		//if(edit_mode){
			//glBindVertexArray(terrain_vao); //Assumed to be still bound from above
			glUniform4fv(colour_loc, 1, vec4(0,0,0,1).v);
			glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, translate(identity_mat4(), vec3(0,0.1f,0)).m);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_SHORT, 0);
		//}
		//Draw player
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(cube_vao);
		glUniform4fv(colour_loc, 1, player_colour.v);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, player_M.m);
        glDrawElements(GL_TRIANGLES, cube_num_indices, GL_UNSIGNED_SHORT, 0);

		glUniform4fv(colour_loc, 1, vec4(0.8, 0.1, 0.9, 1).v);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, translate(scale(identity_mat4(),vec3(1,2,1)),vec3(2,0,0)).m);
        glDrawElements(GL_TRIANGLES, cube_num_indices, GL_UNSIGNED_SHORT, 0);

		glfwSwapBuffers(window);

		check_gl_error();
	}//end main loop

    return 0;
}
