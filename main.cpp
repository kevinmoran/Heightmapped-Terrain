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

int main(){
	if (!init_gl(window, gl_width, gl_height)){ return 1; }
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

	float* cube_vp = NULL;
	unsigned short* cube_indices = NULL;
	int cube_point_count = 0, cube_vert_count = 0;
	load_obj_indexed("cube.obj", &cube_vp, &cube_indices, &cube_point_count, &cube_vert_count);

	GLuint cube_vao;
	GLuint cube_points_vbo, cube_index_vbo;
	glGenBuffers(1, &cube_points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_points_vbo);
	glBufferData(GL_ARRAY_BUFFER, cube_vert_count*3*sizeof(float), cube_vp, GL_STATIC_DRAW);

	glGenBuffers(1, &cube_index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_index_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_point_count*sizeof(unsigned short), cube_indices, GL_STATIC_DRAW);
	free(cube_indices);
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, cube_points_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	free(cube_vp);

    init_terrain();

    //Load shaders
	Shader basic_shader = load_shader("MVP.vert", "uniform_colour.frag");
	
    g_camera.init(vec3(0,12,15), vec3(0,0,0));
	glUseProgram(heightmap_shader.id);
    glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, g_camera.P.m);
    glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
	GLuint colour_loc = glGetUniformLocation(basic_shader.id, "colour");

	//Player data
	vec3 player_pos = vec3(0,10,0);
	mat4 player_M = translate(identity_mat4(), player_pos);
	vec3 player_vel = vec3(0,0,0);

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

		static bool edit_mode = true;
		static bool G_was_pressed = false;
		if (glfwGetKey(window, GLFW_KEY_G)) {
			if(!G_was_pressed) {
				edit_mode = !edit_mode;
				write_height_pgm("terrain.pgm", height_data, width, height);
				G_was_pressed = true;
			}
		}
		else G_was_pressed = false;

		if(edit_mode){
       		editor_update(dt);
		}
		else {
			//Update player
			
			//WASD Movement (constrained to the x-z plane)
			if(g_input[MOVE_FORWARD]) {
				vec3 xz_proj = normalise(vec3(g_camera.fwd.v[0], 0, g_camera.fwd.v[2]));
				player_pos += xz_proj*g_camera.move_speed*dt;
			}
			if(g_input[MOVE_LEFT]) {
				vec3 xz_proj = normalise(vec3(g_camera.rgt.v[0], 0, g_camera.rgt.v[2]));
				player_pos -= xz_proj*g_camera.move_speed*dt;
			}
			if(g_input[MOVE_BACK]) {
				vec3 xz_proj = normalise(vec3(g_camera.fwd.v[0], 0, g_camera.fwd.v[2]));
				player_pos -= xz_proj*g_camera.move_speed*dt;			
			}
			if(g_input[MOVE_RIGHT]) {
				vec3 xz_proj = normalise(vec3(g_camera.rgt.v[0], 0, g_camera.rgt.v[2]));
				player_pos += xz_proj*g_camera.move_speed*dt;			
			}
			float player_mass = 10;
			float g = 9.81f;
			player_vel.v[1] -= player_mass*g*dt;
			player_pos.v[1] += player_vel.v[1]*dt;
			float ground_y = get_height_interp(player_pos.v[0], player_pos.v[2]);
			
			if(player_pos.v[1] - ground_y < 0.5f) {
				player_pos.v[1] = ground_y + 0.5f;
				player_vel.v[1] = 0.0f;
			}
			
			if(player_pos.v[0] < -heightmap_size) {
				player_pos.v[0] = -heightmap_size;
			}
			if(player_pos.v[0] > heightmap_size) {
				player_pos.v[0] = heightmap_size;
			}
			if(player_pos.v[2] < -heightmap_size) {
				player_pos.v[2] = -heightmap_size;
			}
			if(player_pos.v[2] > heightmap_size) {
				player_pos.v[2] = heightmap_size;
			}
			
			//Update matrices
			player_M = translate(identity_mat4(), player_pos);
		}

		//Rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(heightmap_shader.id);
		glUniformMatrix4fv(heightmap_shader.V_loc, 1, GL_FALSE, g_camera.V.m);

		//Draw terrain
		glBindVertexArray(terrain_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_index_vbo);
        glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_SHORT, 0);

		glUseProgram(basic_shader.id);
		//Draw terrain wireframe
		glUniform4fv(colour_loc, 1, vec4(0,0,0,1).v);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, translate(identity_mat4(), vec3(0,0.1f,0)).m);
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_SHORT, 0);
		//Draw player
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glBindVertexArray(cube_vao);
		glUniform4fv(colour_loc, 1, vec4(0.8f, 0.1f, 0.2f, 1).v);
		glUniformMatrix4fv(basic_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(basic_shader.P_loc, 1, GL_FALSE, g_camera.P.m);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, player_M.m);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_index_vbo);
        glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_SHORT, 0);

		glfwSwapBuffers(window);

	}//end main loop

    return 0;
}
