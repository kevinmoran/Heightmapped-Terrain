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

	float* cube_vp = NULL;
	unsigned short* cube_indices = NULL;
	int cube_point_count = 0, cube_vert_count = 0;
	load_obj_indexed("cube.obj", &cube_vp, &cube_indices, &cube_point_count, &cube_vert_count);

	GLuint cube_vao;
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);

	GLuint cube_points_vbo, cube_index_vbo;
	glGenBuffers(1, &cube_points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_points_vbo);
	glBufferData(GL_ARRAY_BUFFER, cube_vert_count*3*sizeof(float), cube_vp, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	free(cube_vp);

	glGenBuffers(1, &cube_index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_index_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_point_count*sizeof(unsigned short), cube_indices, GL_STATIC_DRAW);
	free(cube_indices);

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
	float player_scale = 1.0f;
	mat4 player_M = translate(scale(identity_mat4(), player_scale), player_pos);
	vec3 player_vel = vec3(0,0,0);
	vec4 player_colour;
	bool player_is_on_ground = false;
	float player_mass = 20;
	float g = 9.81f;
	float player_top_speed = 20.0f;
	float player_time_till_top_speed = 0.25f; //Human reaction time?
	float player_acc = player_top_speed/player_time_till_top_speed;
	float friction_factor = 0.1f; //higher is slippier

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
		static bool tab_was_pressed = false;
		if (glfwGetKey(window, GLFW_KEY_TAB)) {
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
		else { //Update player
			
			//WASD Movement (constrained to the x-z plane)
			bool player_is_moving = false;
			//TODO: Need to improve responsiveness when changing direction
			if(g_input[MOVE_FORWARD]) {
				vec3 xz_proj = normalise(vec3(g_camera.fwd.v[0], 0, g_camera.fwd.v[2]));
				player_vel += xz_proj*player_acc*dt;
				player_is_moving = true;
			}
			if(g_input[MOVE_LEFT]) {
				vec3 xz_proj = normalise(vec3(g_camera.rgt.v[0], 0, g_camera.rgt.v[2]));
				player_vel -= xz_proj*player_acc*dt;
				player_is_moving = true;
			}
			if(g_input[MOVE_BACK]) {
				vec3 xz_proj = normalise(vec3(g_camera.fwd.v[0], 0, g_camera.fwd.v[2]));
				player_vel -= xz_proj*player_acc*dt;
				player_is_moving = true;			
			}
			if(g_input[MOVE_RIGHT]) {
				vec3 xz_proj = normalise(vec3(g_camera.rgt.v[0], 0, g_camera.rgt.v[2]));
				player_vel += xz_proj*player_acc*dt;	
				player_is_moving = true;		
			}

			if(!player_is_on_ground){
				player_vel.v[1] -= player_mass*g*dt;
				//TODO: air steering?
			}
			else {//Player is on ground
					//Clamp player speed
				if (length(player_vel) > player_top_speed) {
					player_vel = normalise(player_vel);
					player_vel *= player_top_speed;
				}

				//Deceleration
				if(!player_is_moving) 
				player_vel = player_vel*friction_factor;
			}
			
			//Update player position
			player_pos += player_vel*dt;

			float ground_y = get_height_interp(player_pos.v[0], player_pos.v[2]);
			vec3 ground_norm = get_normal_interp(player_pos.v[0], player_pos.v[2]);
			//Collided into ground
			if(player_pos.v[1] - ground_y < 0.5f*player_scale) {
				//TODO: push player out of ground along normal?
				//and update velocity by angle of ground?
				player_pos.v[1] = ground_y + 0.5f*player_scale;
				player_vel.v[1] = 0.0f;
				player_is_on_ground = true;
			}
			else if(player_pos.v[1] - ground_y > 0.5f*player_scale){
				//TODO if only slightly above ground just autosnap pos
				player_is_on_ground = false;
			}

			float slope = ONE_RAD_IN_DEG*acos(dot(vec3(0,1,0), ground_norm));
			
			if(slope>45) {
				//player_pos = prev_pos;
				player_colour = vec4(0.8f, 0.8f, 0.2f, 1);
				// int i = get_height_index(player_pos.v[0], player_pos.v[1]);
				// float cell_size = 2*heightmap_size/(heightmap_n-1);
				// float x_tl = terrain_vp[3*i];
				// float z_tl = terrain_vp[3*i + 2];
				// float x_t = (player_pos.v[0]-x_tl)/cell_size; // % along cell point is on x-axis
				// float z_t = (player_pos.v[1]-z_tl)/cell_size; // % along cell point is on z-axis

				// vec3 tri_v[3];
				// tri_v[0] = vec3(terrain_vp[3*(i+1)], terrain_vp[3*(i+1)+1], terrain_vp[3*(i+1)+2]);
				// tri_v[1] = vec3(terrain_vp[3*(i+heightmap_size_x)], terrain_vp[3*(i+heightmap_size_x)+1], terrain_vp[3*(i+heightmap_size_x)+2]);
				// if(x_t+z_t>1.0f) tri_v[2] = vec3(terrain_vp[3*(i+heightmap_size_x+1)], terrain_vp[3*(i+heightmap_size_x+1)+1], terrain_vp[3*(i+heightmap_size_x+1)+2]);
				// else tri_v[2] = vec3(terrain_vp[3*i], terrain_vp[3*i+1], terrain_vp[3*i+2]);

				// vec3 v_max = (tri_v[0].v[1]>tri_v[1].v[1]) ? tri_v[0] : tri_v[1];
				// v_max = (v_max.v[1]>tri_v[2].v[1]) ? v_max : tri_v[2];
				// vec3 v_min = (tri_v[0].v[1]<tri_v[1].v[1]) ? tri_v[0] : tri_v[1];
				// v_min = (v_min.v[1]<tri_v[2].v[1]) ? v_min : tri_v[2];

				// vec3 gradient = normalise(v_max - v_min);
				
				// player_vel -= gradient*10*9.81f*sinf(ONE_DEG_IN_RAD*slope)*dt;
			}
			else player_colour = vec4(0.8f, 0.1f, 0.2f, 1);
			
			//Constrain player_pos to map
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
			player_M = translate(scale(identity_mat4(), player_scale), player_pos);
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
		//if(edit_mode){
			glUniform4fv(colour_loc, 1, vec4(0,0,0,1).v);
			glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, translate(identity_mat4(), vec3(0,0.1f,0)).m);
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_SHORT, 0);
		//}
		//Draw player
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glBindVertexArray(cube_vao);
		glUniform4fv(colour_loc, 1, player_colour.v);
		glUniformMatrix4fv(basic_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(basic_shader.P_loc, 1, GL_FALSE, g_camera.P.m);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, player_M.m);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_index_vbo);
        glDrawElements(GL_TRIANGLES, cube_point_count, GL_UNSIGNED_SHORT, 0);

		glfwSwapBuffers(window);

	}//end main loop

    return 0;
}
