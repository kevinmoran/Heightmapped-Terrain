#pragma once
#include "heightmap.h"
//#include "slIMGUI.h"

void editor_update(double dt){
    //Data

    //Hotkeys
    static bool M_was_pressed = false;
    if(glfwGetKey(window, GLFW_KEY_M)) {
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

    if(glfwGetKey(window, GLFW_KEY_R)) {
        reload_shader_program("Heightmap.vert", "Heightmap.frag", &heightmap_shader);
        glUseProgram(heightmap_shader.id);
        glUniformMatrix4fv(heightmap_shader.P_loc, 1, GL_FALSE, g_camera.P.m);
        glUniformMatrix4fv(heightmap_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
    }

    //UI

    //Update stuff
    edit_terrain(dt);
    g_camera.update(dt);

}
