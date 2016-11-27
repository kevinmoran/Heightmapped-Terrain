#pragma once

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
float friction_factor = 0.3f; //higher is slippier
float player_jump_height = 4.0f; 
//TODO: I calculate the necessary impulse to jump this height 
// based on kinetic energy (0.5*m*v^2) I think
// Result is not totally accurate, better to make jumping code explicit

void player_update(double dt){

    //WASD Movement (constrained to the x-z plane)
    bool player_is_moving = false;
    {
        //Find player's forward and right movement directions
        vec3 fwd_xz_proj = normalise(vec3(g_camera.fwd.v[0], 0, g_camera.fwd.v[2]));
        vec3 rgt_xz_proj = normalise(vec3(g_camera.rgt.v[0], 0, g_camera.rgt.v[2]));

        if(g_input[MOVE_FORWARD]) {
            player_vel += fwd_xz_proj*player_acc*dt;
            player_is_moving = true;
        }
        else if(dot(fwd_xz_proj,player_vel)>0) player_vel -= fwd_xz_proj*player_acc*dt;

        if(g_input[MOVE_LEFT]) {
            player_vel += -rgt_xz_proj*player_acc*dt;
            player_is_moving = true;
        }
        else if(dot(-rgt_xz_proj,player_vel)>0) player_vel += rgt_xz_proj*player_acc*dt;

        if(g_input[MOVE_BACK]) {
            player_vel += -fwd_xz_proj*player_acc*dt;
            player_is_moving = true;			
        }
        else if(dot(-fwd_xz_proj,player_vel)>0) player_vel += fwd_xz_proj*player_acc*dt;

        if(g_input[MOVE_RIGHT]) {
            player_vel += rgt_xz_proj*player_acc*dt;
            player_is_moving = true;		
        }
        else if(dot(rgt_xz_proj,player_vel)>0) player_vel -= rgt_xz_proj*player_acc*dt;
        // NOTE about the else statements above: Checks if we aren't pressing a button 
        // but have velocity in that direction, if so slows us down faster w/ subtraction
        // This improves responsiveness and tightens up the feel of moving
        // Mult by friction_factor is good to kill speed when idle but feels drifty while moving
    }

    if(g_input[JUMP] && player_is_on_ground){
        //TODO this is a terrible way to do jumping
        //Instead set jump height and move up until you reach that height
        float jump_factor = sqrtf(player_jump_height/(0.5f*player_mass*g));
        player_vel.v[1] += jump_factor*player_mass*g;
        player_is_on_ground = false;
    }

    if(player_is_on_ground){
        //Clamp player speed
        if(length2(player_vel) > player_top_speed*player_top_speed) {
            player_vel = normalise(player_vel);
            player_vel *= player_top_speed;
        }

        //Deceleration
        if(!player_is_moving) 
        player_vel = player_vel*friction_factor;
    }
    
    //TODO: else if(player_is_jumping)
    else{ //(!player_is_on_ground)
        player_vel.v[1] -= player_mass*g*dt;
        //TODO: air steering?

        //Clamp player's xz speed
        vec3 xz_vel = vec3(player_vel.v[0], 0, player_vel.v[2]);
        if(length(xz_vel) > player_top_speed) {
            xz_vel = normalise(xz_vel);
            xz_vel *= player_top_speed;
            player_vel.v[0] = xz_vel.v[0];
            player_vel.v[2] = xz_vel.v[2];
        }
    }

    //Update player position
    player_pos += player_vel*dt;

    float ground_y = get_height_interp(player_pos.v[0], player_pos.v[2]);
    vec3 ground_norm = get_normal_interp(player_pos.v[0], player_pos.v[2]);
    float player_h_above_ground = player_pos.v[1] - ground_y;
    //height to differentiate between walking down a slope and walking off an edge:
    const float autosnap_height = 1.0f*player_scale;

    //TODO: move player's origin to base so code like this makes more sense
    // (player_h_above_ground = (0.5f*player_scale) currently means we're on the ground)

    //TODO Don't autosnap while the player is jumping

    //Collided into ground
    if(player_h_above_ground< 0.5f*player_scale) {
        //TODO: push player out of ground along normal?
        //and update velocity by angle of ground?
        player_pos.v[1] = ground_y + 0.5f*player_scale;
        player_vel.v[1] = 0.0f;
        player_is_on_ground = true;
    }
    else if(player_h_above_ground > autosnap_height){
        player_is_on_ground = false;
    }
    else {//We're not on ground but less than autosnap_height above it
        //snap player to ground
        //TODO: trying autosnapping along normal?
        player_pos.v[1] = ground_y + 0.5f*player_scale;
        player_is_on_ground = true;
    }

    //Slope checking
    {
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
    }
    
    //Constrain player_pos to map bounds
    {
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
    }

    //Update matrices
    player_M = translate(scale(identity_mat4(), player_scale), player_pos);
}
