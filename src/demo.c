/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-execute' Advanced Motion Controller
	Copyright (C) 2016 Dephy, Inc. <http://dephy.com/>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************
	[Lead developper] Jean-Francois (JF) Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab 
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] demo: Demonstration Test Code
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2016-09-29 | jfduval | Released under GPL-3.0 release
	*
****************************************************************************/

//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"
#include "demo.h"

//****************************************************************************
// Variable(s)
//****************************************************************************


//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************	


//****************************************************************************
// Public Function(s)
//****************************************************************************

void csea_knee_up_down_test_demo(void)
{
	unsigned int state = 0;
	unsigned int traj_calculated = 0, steps = 0;
	int low = 0, up = 0;
	volatile int32 ff = 0;
	volatile double angle = 0.0, force = 0.0;
	volatile int32 force_int = 0, sign = 1;
	int32 wait_cycles = 0;
	int32 first = 1;
	
	ctrl.active_ctrl = CTRL_POSITION;	
	#if(MOTOR_COMMUT == COMMUT_BLOCK)
	Coast_Brake_Write(1);	//Brake
	#endif
	motor_open_speed_1(0);
	
	//Position PID gains:
	ctrl.position.gain.P_KP = 22;
	ctrl.position.gain.P_KI = 5;
	
	//Starts off:
	LED_R_Write(1);
	LED_G_Write(1);
	LED_B_Write(1);
	
	while(1)
	{		
		//Refresh encoder data:
		encoder.count_last = encoder.count;	
		encoder.count = refresh_enc_control();
		encoder.count_dif = encoder.count - encoder.count_last;
		
		ctrl.position.setp = trapez_get_pos(steps);	//New setpoint
		ctrl.position.pos = encoder.count;			//Current pos
		
		//PID:
		//motor_position_pid(ctrl.position.setp, ctrl.position.pos);
		
		//PID + Feed Forward:
		angle = (double)(((encoder.count - CSEA_DOWN)) / TICK_TO_ANGLE);
		force = 100*sin((angle*3.14)/180);
		force_int = (int32)force;
		ff = -sign*(force_int * FF_GAIN)/10;
		
		if(!first)
			motor_position_pid_ff_1(ctrl.position.setp, ctrl.position.pos, ff);

		//motor_position_pid_ff_1(ctrl.position.pos, ctrl.position.pos, ff);	//Test only
		
		//At power-up the knee is low:
		low = encoder.count;
		
		//There are 4 possible states:
		switch(state)
		{
			case 0:		//Knee is low, not moving
			
				if(!traj_calculated)
				{
					
					traj_calculated = 1;
					wait_cycles = 70;
					
					//CyDelay(250);
				}
				else
				{			
					wait_cycles--;
					if(wait_cycles <= 0)
					{
						state = 1;
						//Compute a trajectory that will bring the knee up:					
						steps = trapez_gen_motion_1(low, CSEA_UP, SPD_UP, ACC_UP);			
						ctrl.position.gain.P_KP = 22;
						ctrl.position.gain.P_KI = 5;
						first = 0;
					}
				}
			
				break;
			
			case 1: 	//Knee is rising
				
				sign = 1;	//FF
				
				if((encoder.count > (CSEA_UP - MARGIN)) && (encoder.count < (CSEA_UP + MARGIN)))
				{
					//We have reached our target, we can go to the next step
					traj_calculated = 0;
					up = encoder.count;
					state = 2;
				}
				
				LED_R_Write(1);
				LED_G_Write(0);
				
				break;
			
			case 2: 	//Knee is high, not moving
				
				if(!traj_calculated)
				{
					traj_calculated = 1;
					wait_cycles = 70;
					
					//CyDelay(250);
				}	
				else
				{			
					wait_cycles--;
					if(wait_cycles <= 0)
					{
						state = 3;
						//Compute a trajectory that will bring the knee up:
						steps = trapez_gen_motion_1(up, CSEA_DOWN, SPD_DOWN, ACC_DOWN);	
						ctrl.position.gain.P_KP = 5;
						ctrl.position.gain.P_KI = 7;
					}
				}
				
				break;
			
			case 3:		//Knee is lowering
				
				sign = 1;	//FF
				
				if((encoder.count > (CSEA_DOWN - MARGIN)) && (encoder.count < (CSEA_DOWN + MARGIN)))
				{
					//We have reached our target, we can go to the next step
					traj_calculated = 0;
					low = encoder.count;
					state = 0;
				}		
				
				LED_R_Write(0);
				LED_G_Write(1);
				
				break;
			
			default:
				break;
		}
		
		//Loop delay (otherwise we don't get a good difference)
		CyDelay(10);
	}
}

//****************************************************************************
// Private Function(s)
//****************************************************************************


//****************************************************************************
// Test Function(s) - Use with care!
//****************************************************************************
