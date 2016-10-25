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
#include "Camera3D.h"
#include "heightmap.h"

int main(){
	if (!init_gl(window, gl_width, gl_height)){ return 1; }
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

    init_terrain();

	float blah = get_height_interp(3,6);

    //Load shader
	Shader heightmap_shader("Heightmap.vert", "Heightmap.frag");
	glUseProgram(heightmap_shader.prog_id);

    g_camera.init(vec3(0,12,15), vec3(0,0,0));
    glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, g_camera.P.m);
    glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);

	//Player data
	//vec3 player_pos = g_camera.pos;
	// vec3 player_fwd = g_camera.fwd;
	// vec3 player_rgt = g_camera.rgt;
	// vec3 player_up  = g_camera.up;
	// mat4 player_M = identity_mat4();
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
       		update_terrain(dt);
			g_camera.update(dt);
		}
		else {
			//Update player
			float player_mass = 10;
			float g = 9.81f;
			player_vel.v[1] -= player_mass*g*dt;
			g_camera.pos.v[1] += player_vel.v[1]*dt;
			int height_index = get_height_index(g_camera.pos.v[0], g_camera.pos.v[2]);
			float ground_y = (height_index<0)? -INFINITY : heightmap_scale*height_data[height_index]/255.0f;
			
			if(g_camera.pos.v[1] - ground_y < 1) {
				g_camera.pos.v[1] = ground_y + 1;
				player_vel.v[1] = 0.0f;
			}
			
			if(g_camera.pos.v[0] < -heightmap_size) {
				g_camera.pos.v[0] = -heightmap_size;
			}
			if(g_camera.pos.v[0] > heightmap_size) {
				g_camera.pos.v[0] = heightmap_size;
			}
			if(g_camera.pos.v[2] < -heightmap_size) {
				g_camera.pos.v[2] = -heightmap_size;
			}
			if(g_camera.pos.v[2] > heightmap_size) {
				g_camera.pos.v[2] = heightmap_size;
			}
			//WASD Movement (constrained to the x-z plane)
			if(g_input[MOVE_FORWARD]) {
				vec3 xz_proj = normalise(vec3(g_camera.fwd.v[0], 0, g_camera.fwd.v[2]));
				g_camera.pos += xz_proj*g_camera.move_speed*dt;
			}
			if(g_input[MOVE_LEFT]) {
				vec3 xz_proj = normalise(vec3(g_camera.rgt.v[0], 0, g_camera.rgt.v[2]));
				g_camera.pos -= xz_proj*g_camera.move_speed*dt;
			}
			if(g_input[MOVE_BACK]) {
				vec3 xz_proj = normalise(vec3(g_camera.fwd.v[0], 0, g_camera.fwd.v[2]));
				g_camera.pos -= xz_proj*g_camera.move_speed*dt;			
			}
			if(g_input[MOVE_RIGHT]) {
				vec3 xz_proj = normalise(vec3(g_camera.rgt.v[0], 0, g_camera.rgt.v[2]));
				g_camera.pos += xz_proj*g_camera.move_speed*dt;			
			}
			//Increase/decrease elevation
			if(g_input[RAISE_CAM]) {
				g_camera.pos.v[1] += g_camera.move_speed*dt;			
			}
			if(g_input[LOWER_CAM]) {
				g_camera.pos.v[1] -= g_camera.move_speed*dt;			
			}
			//Rotation
			if(!cam_mouse_controls){
				if(g_input[TURN_CAM_LEFT]) {
					g_camera.yaw += g_camera.turn_speed*dt;			
				}
				if(g_input[TURN_CAM_RIGHT]) {
					g_camera.yaw -= g_camera.turn_speed*dt;			
				}
				if(g_input[TILT_CAM_UP]) {
					g_camera.pitch += g_camera.turn_speed*dt;			
				}
				if(g_input[TILT_CAM_DOWN]) {
					g_camera.pitch -= g_camera.turn_speed*dt;			
				}
			}
			else {
				static double prev_mouse_x, prev_mouse_y;
				static float mouse_sensitivity = 0.4f;
				double mouse_x, mouse_y;
				glfwGetCursorPos(window, &mouse_x, &mouse_y);
				g_camera.yaw   += (prev_mouse_x-mouse_x) * mouse_sensitivity * g_camera.turn_speed*dt;
				g_camera.pitch += (prev_mouse_y-mouse_y) * mouse_sensitivity * g_camera.turn_speed*dt;
				prev_mouse_x = mouse_x;
				prev_mouse_y = mouse_y;
			}
			//Update matrices
			g_camera.pitch = MIN(MAX(g_camera.pitch, -89), 80); //Clamp pitch 
			g_camera.V = translate(identity_mat4(), -g_camera.pos);
			print(g_camera.pos);
			mat4 R = rotate_x_deg(rotate_y_deg(identity_mat4(), -g_camera.yaw), -g_camera.pitch); //how does this work? 
			//Surely rotating by the yaw will misalign the x-axis for the pitch rotation and screw this up?
			//Seems to work for now! ¯\_(ツ)_/¯
			g_camera.V = R*g_camera.V;
			g_camera.rgt = inverse(R)*vec4(1,0,0,0); //I guess it makes sense that you have to invert
			g_camera.up = inverse(R)*vec4(0,1,0,0);  //R to calculate these??? I don't know anymore
			g_camera.fwd = inverse(R)*vec4(0,0,-1,0);
		}

		static bool M_was_pressed = false;
		if (glfwGetKey(window, GLFW_KEY_M)) {
			if(!M_was_pressed) {
				if(!cam_mouse_controls){
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
				}
				else{
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); 
				}
				cam_mouse_controls = !cam_mouse_controls;
				M_was_pressed = true;
			}
		}
		else M_was_pressed = false;

		if (glfwGetKey(window, GLFW_KEY_R)) {
			heightmap_shader.reload_shader_program("Heightmap.vert", "Heightmap.frag");
			glUseProgram(heightmap_shader.prog_id);
			glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, g_camera.P.m);
			glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
		}

		//Rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(heightmap_shader.prog_id);
		glUniformMatrix4fv(heightmap_shader.V_loc, 1, GL_FALSE, g_camera.V.m);

		//Draw terrain
		glBindVertexArray(terrain_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_index_vbo);
        glDrawElements(GL_TRIANGLES, terrain_num_indices, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);
	}//end main loop

    return 0;
}
