#pragma once
#include "Terrain.h"
//#include "slIMGUI.h"

void editor_update(double dt){
    //Data
    static int paint_radius = 2;
    static float edit_speed = 10.0f;

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
    //if(glfwGetKey(window, GLFW_KEY_G))
    {
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
        
        const int num_ray_marches = 64;
        const float ray_step_size = 0.5f;
        // static vec3 ray_positions[num_ray_marches-10];
        // static vec3 ground_positions[num_ray_marches-10];
        // static int num_vec_draws = 0;

        for(int i=0; i<num_ray_marches; ++i) 
        {
            vec3 ray_pos = g_camera.pos + ray_world * (float)i * ray_step_size;

            int height_index = get_terrain_index(g_terrain, ray_pos.v[0], ray_pos.v[2]);
            //float ground_y = (height_index<0)? -INFINITY : g_terrain.height*g_terrain.vp[3*height_index+1]/255.0f; 
            float ground_y = get_height_interp(g_terrain, ray_pos.v[0], ray_pos.v[2]);

            // ------------------------------------------------------------
            //DEBUG DRAWING STUFF
            // static bool k_was_pressed = false;
            // static bool freeze_ray_vis = false;
            // if(glfwGetKey(window, GLFW_KEY_K)) {
            // 	if(!k_was_pressed) {
            //         freeze_ray_vis = !freeze_ray_vis;
            // 		k_was_pressed = true;
            // 	}
            // }
            // else k_was_pressed = false;

            // if(i>=10 && !freeze_ray_vis) 
            // {
            //     ray_positions[i-10] = ray_pos;
            //     //ground_positions[i-10] = vec3(g_terrain.vp[3*height_index], g_terrain.vp[3*height_index+1], g_terrain.vp[3*height_index+2]);
            //     ground_positions[i-10] = vec3(ray_pos.v[0], ground_y, ray_pos.v[2]);
            //     num_vec_draws = i-10;
            // }
            // ------------------------------------------------------------

            //if we've gone "underground" without entering the terrain's xz perimeter, break
            //avoids the case where you arrive underground without ever hitting the surface:
            /* \  ___ground___
            //   \
            //     \  <- Like this */
            if(ray_pos.v[1]<0)
            {
                if(ray_pos.v[0] < -g_terrain.width/2)  break;
                if(ray_pos.v[0] >  g_terrain.width/2)  break;
                if(ray_pos.v[2] < -g_terrain.length/2) break;
                if(ray_pos.v[2] >  g_terrain.length/2) break;
            }
            
            //if(fabs(ray_pos.v[1]-ground_y)<0.1f) 
            if(ray_pos.v[1]<ground_y)
            {
                edit_terrain(&g_terrain, height_index, edit_speed, paint_radius, dt);
                //draw_vec(ray_pos, vec3(0,3,0), vec4(1,0,0,1));
                break;
            }
        }//endfor

        // ------------------------------------------------------------
        // for(int i=0; i<num_vec_draws; ++i)
        // {
        //     draw_point(ray_positions[i],    0.05, vec4(0.8, 0, 0.8, 1));
        //     draw_point(ground_positions[i], 0.05, vec4(0.8, 0.6, 0, 1));
        // }
        // ------------------------------------------------------------

    }//end glfw_key_G
    g_camera.update(dt);

}
