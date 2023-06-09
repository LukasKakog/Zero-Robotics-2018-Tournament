//Begin page main
//Begin page main
/****** PREPROCESSOR DECLARATIVES ******/
#define FLOATSIZE sizeof(float)
typedef float* floatarr;

/****** VARIABLE DECLARATION ******/

/* Own attributes */
floatarr position_own;          /* The own sphere's position */
floatarr velocity_own;          /* The own sphere's velocity vector */
floatarr attitude_own;          /* The own sphere's attitude vector */
floatarr rotation_own;          /* The own sphere's rotation speed */

floatarr position_other;          /* The other sphere's position */
floatarr velocity_other;          /* The other sphere's velocity vector */
floatarr attitude_other;          /* The other sphere's attitude vector */
floatarr rotation_other;          /* The other sphere's rotation speed */


float current_att_target[3];    /* Current euler target */


float course[1][3];           /* not in use */

float target[3];                /* Used to store a target point -> passed into setCourseTarget or addTarget */

float pi;                       /* guess again */
int hook_phase;
float prev_correction_vel[3];

/****** END OF VARIABLE DECLARATION ******/

/****** INIT FUNCTION ******/

void init(){
    /* Initialize arrays */
    /* allocate them all at once to decrease codesize */
    
    /* Using malloc and setting the other pointers manually is smaller
     * in codesize than declaring multiple arrays and copying the values
     * each loop iteration */
    position_own = (floatarr) malloc(24 * FLOATSIZE);
    
    velocity_own = position_own + 3;
    attitude_own = position_own + 6;
    rotation_own = position_own + 9;
    
    position_other = position_own + 12;
    
    velocity_other = position_other + 3;
    attitude_other = position_other + 6;
    rotation_other = position_other + 9;
    
    
    game.getOtherEulerState(position_other);
    game.getMyEulerState(position_own);
    /* End array initialization*/
    
    /* Constants */
    pi = 3.14159265359f;
    hook_phase = 0;
    /* End constants*/
    
    for (int i = 0; i<3; i++)
        prev_correction_vel[i] = 100;
    
    //game.enableHookGT();
}

/****** LOOP FUNCTION ******/

void loop(){
    // Get the ZR states 
    game.getOtherEulerState(position_other);
    game.getMyEulerState(position_own);
    
    if(game.getScore() < 1){
        hook();
    }
    else{
        tow();
    }
    DEBUG(("----------------------------------------"));
}


/* vectorTimesScalar:
 * Multiply each component of the vector with the scalar.
 * If the scalar is 1 this can be used as a copyVector function
 */
void vectorTimesScalar(float target[3], float vector[3], float scalar){
    for(int i = 0; i < 3; i++){
        target[i] = vector[i] * scalar;
    }
}

/* Set's the target which flyCourse will fly to */


/* getVectorAngleCos:
 * Returns the cosine of the angle between two vectors.
 */
float getVectorAngleCos(float vec1[3], float vec2[3]){
    return mathVecInner(vec1, vec2, 3) / (mathVecMagnitude(vec1, 3) * mathVecMagnitude(vec2, 3));
}

void attitudeToEuler(float target[3], float att_vector[3]){
    float att_copy[3];
    vectorTimesScalar(att_copy,att_vector,1);
    target[0] = 0;
    target[1] = -asinf(att_copy[2]);
    target[2] = asinf(att_copy[1]/cosf(target[1]));
}

void eulerToAttitude(float target[3], float att_vector[3]){
    float att_copy[3];
    vectorTimesScalar(att_copy,att_vector,1);
    target[0] = cosf(att_copy[2])*cosf(att_copy[1]);
    target[1] = sinf(att_copy[2])*cosf(att_copy[1]);
    target[2] = sinf(-att_copy[1]);
}

void flyRedo(float position_substitute[3],float vel_factor,float max_vel, bool use_the_other_movement_pls){
    int closest_course_i = 0;
    float vec_to_course_t[3];
    float correction_vel[3];
    float correction_force[3];
    float safe_vel[3];
    float projected_distance[3];
    mathVecSubtract(vec_to_course_t,course[closest_course_i],position_substitute,3);
    vectorTimesScalar(correction_vel,vec_to_course_t,vel_factor);
    //DEBUG(("correction_vel: <%f,%f,%f>", correction_vel[0],correction_vel[1],correction_vel[2]));
    if ((mathVecMagnitude(vec_to_course_t,3)>0.01 && mathVecMagnitude(velocity_own,3) > 0.01)){
        DEBUG(("faster ptp"));
        //vectorTimesScalar(safe_vel,correction_vel,1);
        //mathVecNormalize(correction_vel,3);
        //vectorTimesScalar(correction_vel,correction_vel,sqrtf(fabsf(2*(mathVecMagnitude(vec_to_course_t,3)-mathVecMagnitude(velocity_own,3)-0.005)*0.03/(3.8f+0.2*game.getFuelRemaining())))*0.7);
        for (int i = 0; i<3; i++){
            projected_distance[i] = fabsf((position_substitute[i] + velocity_own[i] + 0.005*velocity_own[i]/fabsf(velocity_own[i])) - (course[0][i] + velocity_other[i]));
            safe_vel[i] = sqrtf(fabsf(2*(projected_distance[i])*0.0055*(0.01*game.getThrusterHealth())))*1;
            //safe_vel[i] = sqrtf(fabsf(2*(fabsf(vec_to_course_t[i])-fabsf(velocity_own[i])-fabsf(safe_vel[i])*1.1-0.01)*0.03/(3.8f+0.2*game.getFuelRemaining())));
            correction_vel[i] = safe_vel[i]*correction_vel[i]/fabsf(correction_vel[i]);
            //DEBUG(("DIFF:%f",fabsf(correction_vel[i])-fabsf(velocity_own[i])));
            //DEBUG(("DIFF NUMBA TWOOO:%f",fabsf(vec_to_course_t[i])));
            //DEBUG(("dist_to_target_estimate: %f",fabsf(fabsf(vec_to_course_t[i])-velocity_own[i]*vec_to_course_t[i]/fabsf(vec_to_course_t[i])-0.005)));
            //DEBUG(("projected_distance: %f", projected_distance[i]));
            float factor = 1;
            if (fabsf(correction_vel[i])-fabsf(velocity_own[i])<1.0f/4.0f * fabsf(velocity_own[i]) || fabsf(vec_to_course_t[i]) < 0.01){
                factor = 0.00000001;
            }
            correction_vel[i] = factor*(correction_vel[i]);
            prev_correction_vel[i] = 1.0f/factor*correction_vel[i];
        }
          
                 
    }
    if (mathVecMagnitude(correction_vel,3) > max_vel){
        mathVecNormalize(correction_vel,3);
        vectorTimesScalar(correction_vel,correction_vel,max_vel);
    //DEBUG(("NIG"));
    }
    //mathVecSubtract(correction_vel,correction_vel,velocity_own,3);
    vectorTimesScalar(correction_force,correction_vel,(3.8f+0.2*game.getFuelRemaining()));
    //DEBUG(("%f",mathVecMagnitude(correction_vel,3)));
    api.setVelocityTarget(correction_vel); 
    DEBUG(("correction_vel: <%f,%f,%f>", correction_vel[0],correction_vel[1],correction_vel[2]));
    
    //api.setForces(correction_force);
}


void hook(){
    float hook_offset[3];
    float hook_offset_two[3];
    float offset_len_one = 0.001f;
    float offset_len_two = -0.0005f;
    float offset_two = 0.996f;
    float vec_to_target[3];
    float vel_factor = 0.3f;
    float max_vel = 0.1;
    float reached_vel = 0.006;
    float reached_dist = 0.01;
    bool use_other = true;
    //DEBUG(("Euler Target: <%f,%f,%f>", current_att_target[0],current_att_target[1],current_att_target[2]));
    current_att_target[1] = -attitude_other[1];
    current_att_target[2] = attitude_other[2] - pi;
    current_att_target[0] = -(attitude_other[0]+rotation_other[0]);
    
    hook_offset_two[0] = attitude_other[0];
    hook_offset_two[1] = -attitude_other[0];
    hook_offset_two[2] = attitude_other[2] + pi/2;
    hook_offset[0] = attitude_other[0];
    hook_offset[1] = attitude_other[0]+pi/2;
    hook_offset[2] = attitude_other[2] - pi/2;
    eulerToAttitude(hook_offset,hook_offset);
    eulerToAttitude(hook_offset_two,hook_offset_two);
     
    if (hook_phase == 0){
        offset_len_one = 0.05f;
        offset_len_two = 0.02f;
        offset_two = 0.94f;
        vel_factor = 0.3f;
        max_vel = 0.16;
        reached_dist = 0.03;
    }
    else if (hook_phase == 1){
        offset_len_one = 0.002f;
        offset_len_two = 0.00f;
        offset_two = 0.96f;
        reached_dist = 0.01;
        max_vel = 0.005;
        use_other = true;
    }
    else if (hook_phase > 1){
        max_vel = 0.009;
    }
    
    vectorTimesScalar(hook_offset,hook_offset,offset_len_one);
    vectorTimesScalar(hook_offset_two,hook_offset_two,offset_len_two);
    mathVecAdd(hook_offset,hook_offset,hook_offset_two,3);
    
    float hook_blue[3];
    float hook_red[3];
    eulerToAttitude(hook_blue,attitude_own);
    eulerToAttitude(hook_red,attitude_other);
    vectorTimesScalar(hook_blue,hook_blue,0.17095);
    vectorTimesScalar(hook_red,hook_red,0.17095*offset_two);
    mathVecAdd(hook_blue,hook_blue,position_own,3);
    mathVecAdd(hook_red,hook_red,position_other,3);
    
    mathVecAdd(hook_red,hook_red,hook_offset,3);
    //mathVecAdd(hook_red,hook_red,velocity_other,3);
    vectorTimesScalar(course[0],hook_red,1);
    
    /*if (hook_phase == 0){
        current_att_target[1] = -attitude_other[1];
        current_att_target[2] = attitude_other[2] - pi;
    }
    else{
        mathVecSubtract(vec_to_target,hook_red,position_own,3);
        mathVecNormalize(vec_to_target,3);
        attitudeToEuler(current_att_target,vec_to_target);
    }*/
    //current_att_target[0] = -attitude_other[0];
    float red_att_vec[3];
    float red_to_hook_blue[3];
    float dist_to_att = 0;
    mathVecSubtract(red_to_hook_blue,hook_blue,position_other,3);
    eulerToAttitude(red_att_vec,attitude_other);
    dist_to_att = fabsf(sinf(acosf(getVectorAngleCos(red_att_vec,red_to_hook_blue))) * mathVecMagnitude(red_to_hook_blue,3));
    mathVecSubtract(vec_to_target,hook_red,hook_blue,3);
    //DEBUG(("velocity_own: %f",mathVecMagnitude(velocity_own,3)));
    //DEBUG(("dist_to_target: %f",fabsf(fabsf(hook_blue[1])-fabsf(course[0][1]))));
    if (mathVecMagnitude(velocity_own,3)<=reached_vel && ((hook_phase == 0)?(mathVecMagnitude(vec_to_target,3)) : dist_to_att)<reached_dist){
        hook_phase++;
        DEBUG(("Phase %d done", hook_phase));
    }
    /*if(hook_phase==0){
        flyRedo(hook_blue,vel_factor,max_vel,true);
    }
    else{
        eulerToAttitude(hook_blue,attitude_other);
        vectorTimesScalar(hook_blue,hook_blue,0.17095);
        mathVecAdd(hook_red,hook_red,hook_blue,3);
        vectorTimesScalar(course[0],hook_red,1);
        api.setPositionTarget(course[0]);
    }*/
    flyRedo(hook_blue,vel_factor,max_vel,use_other);
    
    float test_quat[4];
    //game.eulerToQuaternion(current_att_target,test_quat);
    //api.setQuatTarget(test_quat);
    game.setEulerTarget(current_att_target);
    
    
}

void tow(){
    //game.disableHookGT();
    /* Just flies into +y as fast as possible */
    if (position_own[1] > 0.13){
        /*current_att_target[0] = attitude_other[0];
        current_att_target[1] = attitude_other[1];
        current_att_target[2] = attitude_other[2];*/
        float torques_vec[3];
        torques_vec[0] = 0;
        torques_vec[1] = (rotation_own[1] < 0.2)?0.2:0;
        torques_vec[2] = 0;
        api.setTorques(torques_vec);
        course[0][1] = 100;
        course[0][0] = -velocity_other[0] * 100000 + 10;
        course[0][2] = -velocity_other[2] * 100000 + 10;
        
    }
    else{
        current_att_target[0] = -attitude_other[0];
        current_att_target[1] = -attitude_other[1];
        current_att_target[2] = attitude_other[2] -pi;
        course[0][1] = 100;
        game.setEulerTarget(current_att_target);
    }
    flyRedo(position_own,0.5,0.2,true);
    //if (game.getHooked() == false ){
    //    hook();
    //}
}
//End page phases

//End page main

//End page main
