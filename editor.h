#pragma once
#include "heightmap.h"
//#include "slIMGUI.h"

void editor_update(double dt){
    //Data
    static int paint_radius;

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
    //Ray picking to raise/lower ground
    static double mouse_repeat_timer = 0.0;
    mouse_repeat_timer += dt;
    
    if(mouse_repeat_timer>1/60.0){
        mouse_repeat_timer -= 1/60.0;

        double mouse_xpos, mouse_ypos;
        glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);

        float x_nds = (cam_mouse_controls) ? 0 :(2*mouse_xpos/gl_width) - 1;
        float y_nds = (cam_mouse_controls) ? 0 : 1- (2*mouse_ypos)/gl_height;
        //vec3 ray_nds = vec3(x_nds, y_nds, 1.0f);
        vec4 ray_clip = vec4(x_nds, y_nds, -1.0f, 1.0f);
        vec4 ray_eye = inverse(g_camera.P)*ray_clip;
        ray_eye = vec4(ray_eye.v[0], ray_eye.v[1], -1.0f, 0.0f);
        vec3 ray_world = vec3(inverse(g_camera.V)*ray_eye);
        ray_world = normalise(ray_world);
        
        for(int i=0; i<50; i++) {
            vec3 interval_pos = ray_world*(float)i;
            interval_pos = g_camera.pos+interval_pos;

            int height_index = get_height_index(interval_pos.v[0], interval_pos.v[2]);
            float ground_y = (height_index<0)? -INFINITY : MAX_HEIGHT*height_data[height_index]/255.0f;
            if(interval_pos.v[1]<ground_y) {
                edit_terrain(paint_radius, height_index, dt);
                break;
            }
        }//endfor

    }//endif mouse_repeat_timer
    g_camera.update(dt);

}
