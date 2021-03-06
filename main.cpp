#define GL_LITE_IMPLEMENTATION
#include "gl_lite.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GLFWwindow* window = NULL;
int gl_width = 1080;
int gl_height = 720;
float gl_aspect_ratio = (float)gl_width/gl_height;
bool gl_fullscreen = false;

#include "GameMaths.h"
#include "Input.h"
#include "Camera3D.h"
#include "init_gl.h"
#include "load_obj.h"
#include "Shader.h"
#include "DebugDrawing.h"
#include "Terrain.h"
#include "Editor.h"
#include "Player.h"

int main(){
	if(!init_gl(window, "Terrain", gl_width, gl_height)){ return 1; }
	
	//Load cube mesh
	GLuint cube_vao;
	unsigned int cube_num_indices = 0;
	{
		float* vp = NULL;
		float* vn = NULL;
		float* vt = NULL;
		uint16_t* indices = NULL;
		unsigned int num_verts = 0;
		load_obj_indexed("cube.obj", &vp, &vt, &vn, &indices, &num_verts, &cube_num_indices);

		glGenVertexArrays(1, &cube_vao);
		glBindVertexArray(cube_vao);
		
		GLuint points_vbo;
		glGenBuffers(1, &points_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_verts*3*sizeof(float), vp, GL_STATIC_DRAW);
		glEnableVertexAttribArray(VP_ATTRIB_LOC);
		glVertexAttribPointer(VP_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free(vp);

		GLuint tex_coords_vbo;
		glGenBuffers(1, &tex_coords_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tex_coords_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_verts*2*sizeof(float), vt, GL_STATIC_DRAW);
		glEnableVertexAttribArray(VT_ATTRIB_LOC);
		glVertexAttribPointer(VT_ATTRIB_LOC, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		free(vt);
		
		GLuint normals_vbo;
		glGenBuffers(1, &normals_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_verts*3*sizeof(float), vn, GL_STATIC_DRAW);
		glEnableVertexAttribArray(VN_ATTRIB_LOC);
		glVertexAttribPointer(VN_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free(vn);

		GLuint index_vbo;
		glGenBuffers(1, &index_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_num_indices*sizeof(unsigned short), indices, GL_STATIC_DRAW);
		free(indices);
	}

    g_camera.init(vec3(0,2,5));

	init_terrain(&g_terrain, "terrain.pgm");
	init_debug_draw();

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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Get dt
		prev_time = curr_time;
		curr_time = glfwGetTime();
		dt = curr_time - prev_time;
		if(dt > 0.1) dt = 0.1;

		//Get Input
		g_mouse.prev_xpos = g_mouse.xpos;
    	g_mouse.prev_ypos = g_mouse.ypos;
		
		glfwPollEvents();
		
		static bool edit_mode = false;
		//Check miscellaneous button presses 
		{
			//Toggle edit mode
			static bool tab_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_TAB)) {
				if(!tab_was_pressed) {
					edit_mode = !edit_mode;
				}
				tab_was_pressed = true;
			}
			else tab_was_pressed = false;
			
			if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, 1);
			}

			//Reload shaders
			static bool R_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_R)) {
				if(!R_was_pressed) {
					reload_shader_program("Heightmap.vert", "Heightmap.frag", &heightmap_shader);
					glUseProgram(heightmap_shader.id);
					glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);

					reload_shader_program("MVP.vert", "uniform_colour.frag", &basic_shader);
					glUseProgram(basic_shader.id);
					glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
				}
				R_was_pressed = true;
			}
			else R_was_pressed = false;

			//Save scene
			static bool s_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_S)) {
				if(!s_was_pressed){
					if(glfwGetKey(window, CTRL_KEY_LEFT) || (glfwGetKey(window, CTRL_KEY_RIGHT))) {
						save_terrain(g_terrain);
					}
				}
				s_was_pressed = true;
			}
			else s_was_pressed = false;

			//Ctrl/Command-F to toggle fullscreen
			//Note: window_resize_callback takes care of resizing viewport/recalculating P matrix
			static bool F_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_F)) {
				if(!F_was_pressed){
					if(glfwGetKey(window, CTRL_KEY_LEFT) || glfwGetKey(window, CTRL_KEY_RIGHT)){
						gl_fullscreen = !gl_fullscreen;
						static int old_win_x, old_win_y, old_win_w, old_win_h;
						if(gl_fullscreen){
							glfwGetWindowPos(window, &old_win_x, &old_win_y);
							glfwGetWindowSize(window, &old_win_w, &old_win_h);
							GLFWmonitor* mon = glfwGetPrimaryMonitor();
							const GLFWvidmode* vidMode = glfwGetVideoMode(mon);
							glfwSetWindowMonitor(window, mon, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
						}
						else glfwSetWindowMonitor(window, NULL, old_win_x, old_win_y, old_win_w, old_win_h, GLFW_DONT_CARE);
					}
				}
				F_was_pressed = true;
			}
			else F_was_pressed = false;
		}

		if(edit_mode){
       		editor_update(dt);
		}
		else {
			player_update(dt);
		}

		//Rendering
        glUseProgram(heightmap_shader.id);
		glUniformMatrix4fv(heightmap_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, g_camera.P.m);

		//Draw terrain

		//toggle wireframe
		static bool wireframe = false;
		static bool slash_was_pressed = false;
		if(glfwGetKey(window, GLFW_KEY_SLASH)) {
			if(!slash_was_pressed) {
				wireframe = !wireframe;
			}
			slash_was_pressed = true;
		}
		else slash_was_pressed = false;

		glPolygonMode(GL_FRONT_AND_BACK, wireframe? GL_LINE : GL_FILL);
		glBindVertexArray(g_terrain.vao);
        glDrawElements(GL_TRIANGLES, g_terrain.num_indices, GL_UNSIGNED_SHORT, 0);

		glUseProgram(basic_shader.id);
		glUniformMatrix4fv(basic_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(basic_shader.P_loc, 1, GL_FALSE, g_camera.P.m);

		//Draw terrain wireframe
		if(edit_mode && !wireframe){
			//glBindVertexArray(g_terrain.vao); //Assumed to be still bound from above
			glUniform4fv(colour_loc, 1, vec4(0,0,0,1).v);
			glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, translate(identity_mat4(), vec3(0,0.1f,0)).m);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        	glDrawElements(GL_TRIANGLES, g_terrain.num_indices, GL_UNSIGNED_SHORT, 0);
		}

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
