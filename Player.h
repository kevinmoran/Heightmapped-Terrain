#pragma once

//Player data
vec3 player_pos = vec3(0,0,0);
float player_scale = 1.0f;
mat4 player_M = translate(scale(identity_mat4(), player_scale), player_pos);
vec3 player_vel = vec3(0,0,0);
vec4 player_colour;
bool player_is_on_ground = false;
bool player_is_jumping = false;
//Physics stuff
//Thanks to Kyle Pittman for his GDC talk:
// http://www.gdcvault.com/play/1023559/Math-for-Game-Programmers-Building
float player_top_speed = 15.0f;
float player_time_till_top_speed = 0.25f; //Human reaction time?
float player_acc = player_top_speed/player_time_till_top_speed;
float friction_factor = 0.3f; //higher is slippier
float player_jump_height = 3.0f;
float player_jump_dist_to_peak = 3.0f; //how far on xz p moves before reaching peak jump height
float g = -2*player_jump_height*player_top_speed*player_top_speed/(player_jump_dist_to_peak*player_jump_dist_to_peak);
float jump_vel = 2*player_jump_height*player_top_speed/player_jump_dist_to_peak;

void player_update(double dt){

    bool player_moved = false;
    //Find player's forward and right movement directions
    vec3 fwd_xz_proj = normalise(vec3(g_camera.fwd.x, 0, g_camera.fwd.z));
    vec3 rgt_xz_proj = normalise(vec3(g_camera.rgt.x, 0, g_camera.rgt.z));

    //WASD Movement (constrained to the x-z plane)
    if(g_input[MOVE_FORWARD]) {
        player_vel += fwd_xz_proj*player_acc*dt;
        player_moved = true;
    }
    else if(dot(fwd_xz_proj,player_vel)>0) player_vel -= fwd_xz_proj*player_acc*dt;

    if(g_input[MOVE_LEFT]) {
        player_vel += -rgt_xz_proj*player_acc*dt;
        player_moved = true;
    }
    else if(dot(-rgt_xz_proj,player_vel)>0) player_vel += rgt_xz_proj*player_acc*dt;

    if(g_input[MOVE_BACK]) {
        player_vel += -fwd_xz_proj*player_acc*dt;
        player_moved = true;			
    }
    else if(dot(-fwd_xz_proj,player_vel)>0) player_vel += fwd_xz_proj*player_acc*dt;

    if(g_input[MOVE_RIGHT]) {
        player_vel += rgt_xz_proj*player_acc*dt;
        player_moved = true;		
    }
    else if(dot(rgt_xz_proj,player_vel)>0) player_vel -= rgt_xz_proj*player_acc*dt;
    // NOTE about the else statements above: Checks if we aren't pressing a button 
    // but have velocity in that direction, if so slows us down faster w/ subtraction
    // This improves responsiveness and tightens up the feel of moving
    // Mult by friction_factor is good to kill speed when idle but feels drifty while moving
        
    static bool jump_button_was_pressed = false;
    if(player_is_on_ground){
        //Clamp player speed
        if(length2(player_vel) > player_top_speed*player_top_speed) {
            player_vel = normalise(player_vel);
            player_vel *= player_top_speed;
        }

        //Deceleration
        if(!player_moved) 
        player_vel = player_vel*friction_factor;

        if(g_input[JUMP]){
            if(!jump_button_was_pressed){
                player_vel.y += jump_vel;
                player_is_on_ground = false;
                player_is_jumping = true;
                jump_button_was_pressed = true;
            }
        }
        else jump_button_was_pressed = false;
    }

    else { //Player is not on ground

        //TODO: air steering?
        if(player_is_jumping){
            //If you don't hold jump you don't jump as high
            if(!g_input[JUMP] && player_vel.y>0) player_vel.y += 5*g*dt;
        }

        //Clamp player's xz speed
        vec3 xz_vel = vec3(player_vel.x, 0, player_vel.z);
        if(length(xz_vel) > player_top_speed) {
            xz_vel = normalise(xz_vel);
            xz_vel *= player_top_speed;
            player_vel.x = xz_vel.x;
            player_vel.z = xz_vel.z;
        }
        player_vel.y += g*dt;
    }

    //Update player position
    player_pos += player_vel*dt;

    float ground_y = get_height_interp(g_terrain, player_pos.x, player_pos.z);
    vec3 ground_norm = get_normal_interp(g_terrain, player_pos.x, player_pos.z);
    player_colour = vec4(ground_norm, 1.0f);
    //vec3 displacement = get_displacement(player_pos.x, player_pos.y, player_pos.z);

    //draw_vec(player_pos, ground_norm, vec4(ground_norm,1));
    //draw_vec(player_pos, displacement, vec4(0.8f,0,0,1));
    //print(displacement);

    float player_h_above_ground = player_pos.y - ground_y;
    //height to differentiate between walking down a slope and walking off an edge:
    const float autosnap_height = 0.5f*player_scale;

    //TODO: move player's origin to base so code like this makes more sense
    // (player_h_above_ground = (0.5f*player_scale) currently means we're on the ground)

    //Collided into ground
    if(player_h_above_ground< 0) {
        //TODO: push player out of ground along normal?
        //and update velocity by angle of ground?
         player_pos.y = ground_y;
        //if(glfwGetKey(window, GLFW_KEY_P)) 
        //player_pos -= displacement;
        player_vel.y = 0.0f;
        player_is_on_ground = true;
        player_is_jumping = false;
    }
    else if(player_h_above_ground > autosnap_height){
        player_is_on_ground = false;
    }
    else if(player_is_on_ground){//We're not on ground but less than autosnap_height above it
        //snap player to ground
        //TODO: trying autosnapping along normal?
        player_pos.y = ground_y;
    }

    //Slope checking
    {
        // float slope = ONE_RAD_IN_DEG*acos(dot(vec3(0,1,0), ground_norm));
        // if(slope>45) {
            //player_pos = prev_pos;
            // player_colour = vec4(0.8f, 0.8f, 0.2f, 1);
            // int i = get_height_index(player_pos.x, player_pos.y);
            // float cell_size = 2*heightmap_size/(heightmap_n-1);
            // float x_tl = terrain_vp[3*i];
            // float z_tl = terrain_vp[3*i + 2];
            // float x_t = (player_pos.x-x_tl)/cell_size; // % along cell point is on x-axis
            // float z_t = (player_pos.y-z_tl)/cell_size; // % along cell point is on z-axis

            // vec3 tri_v[3];
            // tri_v[0] = vec3(terrain_vp[3*(i+1)], terrain_vp[3*(i+1)+1], terrain_vp[3*(i+1)+2]);
            // tri_v[1] = vec3(terrain_vp[3*(i+heightmap_size_x)], terrain_vp[3*(i+heightmap_size_x)+1], terrain_vp[3*(i+heightmap_size_x)+2]);
            // if(x_t+z_t>1.0f) tri_v[2] = vec3(terrain_vp[3*(i+heightmap_size_x+1)], terrain_vp[3*(i+heightmap_size_x+1)+1], terrain_vp[3*(i+heightmap_size_x+1)+2]);
            // else tri_v[2] = vec3(terrain_vp[3*i], terrain_vp[3*i+1], terrain_vp[3*i+2]);

            // vec3 v_max = (tri_v[0].y>tri_v[1].y) ? tri_v[0] : tri_v[1];
            // v_max = (v_max.y>tri_v[2].y) ? v_max : tri_v[2];
            // vec3 v_min = (tri_v[0].y<tri_v[1].y) ? tri_v[0] : tri_v[1];
            // v_min = (v_min.y<tri_v[2].y) ? v_min : tri_v[2];

            // vec3 gradient = normalise(v_max - v_min);
            
            // player_vel -= gradient*10*9.81f*sinf(ONE_DEG_IN_RAD*slope)*dt;
        // }
        // else player_colour = vec4(0.8f, 0.1f, 0.2f, 1);
    }
    
    //Constrain player_pos to map bounds
    {
        if(player_pos.x < -g_terrain.width/2) {
            player_pos.x = -g_terrain.width/2;
        }
        if(player_pos.x > g_terrain.width/2) {
            player_pos.x = g_terrain.width/2;
        }
        if(player_pos.z < -g_terrain.length/2) {
            player_pos.z = -g_terrain.length/2;
        }
        if(player_pos.z > g_terrain.length/2) {
            player_pos.z = g_terrain.length/2;
        }
    }

    //Update matrices
    player_M = translate(scale(identity_mat4(), player_scale), player_pos);
}
