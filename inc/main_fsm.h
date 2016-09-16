//****************************************************************************
// MIT Media Lab - Biomechatronics
// Jean-Francois (Jeff) Duval
// jfduval@media.mit.edu
// 06/2016
//****************************************************************************
// main_fsm: Contains all the case() code for the main FSM
//****************************************************************************
	
#ifndef INC_MAIN_FSM_H
#define INC_MAIN_FSM_H

//****************************************************************************
// Include(s)
//****************************************************************************		
	
#include "main.h"
	
//****************************************************************************
// Shared variable(s)
//****************************************************************************

extern int steps;
extern uint16 angle;
	
//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void main_fsm_case_0(void);
void main_fsm_case_1(void);
void main_fsm_case_2(void);
void main_fsm_case_3(void);
void main_fsm_case_4(void);
void main_fsm_case_5(void);
void main_fsm_case_6(void);
void main_fsm_case_7(void);
void main_fsm_case_8(void);
void main_fsm_case_9(void);

void main_fsm_10kHz(void);
void main_fsm_asynchronous(void);

//****************************************************************************
// Definition(s):
extern int spi_read_flag;
//****************************************************************************	

//****************************************************************************
// Structure(s)
//****************************************************************************	

	
#endif	//INC_MAIN_FSM_H
	