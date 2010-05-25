#include "stdafx.h"

void ExecuteInterpreterOps (DWORD Cycles)
{
	_Notify->BreakPoint(__FILE__,__LINE__);
}

void DoSomething ( void ) {
	if (g_CPU_Action->CloseCPU) { 
		return;
	}
	
	if (g_CPU_Action->SoftReset)
	{
		g_CPU_Action->SoftReset = false;

		_SystemTimer->SetTimer(CSystemTimer::SoftResetTimer,0x3000000,false);
		ShowCFB();
		_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP4;
		CheckInterrupts();
		_Plugins->Gfx()->SoftReset();
	}

	if (g_CPU_Action->GenerateInterrupt)
	{
		g_CPU_Action->GenerateInterrupt = FALSE;
		_Reg->MI_INTR_REG |= g_CPU_Action->InterruptFlag;
		g_CPU_Action->InterruptFlag = 0;
		CheckInterrupts();
	}
	if (g_CPU_Action->CheckInterrupts) {
		g_CPU_Action->CheckInterrupts = FALSE;
		CheckInterrupts();
	}
	if (g_CPU_Action->ProfileStartStop) {
		g_CPU_Action->ProfileStartStop = FALSE;
		ResetTimer();
	}
	if (g_CPU_Action->ProfileResetStats) {
		g_CPU_Action->ProfileResetStats = FALSE;
		ResetTimer();
	}
	if (g_CPU_Action->ProfileGenerateLogs) {
		g_CPU_Action->ProfileGenerateLogs = FALSE;
		GenerateProfileLog();
	}

	if (g_CPU_Action->DoInterrupt) {
		g_CPU_Action->DoInterrupt = FALSE;
		if (DoIntrException(FALSE) && !g_CPU_Action->InterruptExecuted)
		{
			g_CPU_Action->InterruptExecuted = TRUE;
			ClearRecompCodeInitialCode();
		}
	}

	if (g_CPU_Action->ChangeWindow) {
		g_CPU_Action->ChangeWindow = FALSE;
		ChangeFullScreenFunc();
	}

	if (g_CPU_Action->Pause) {
		PauseExecution();
		g_CPU_Action->Pause = FALSE;
	}
	if (g_CPU_Action->ChangePlugin) {
		ChangePluginFunc();
		g_CPU_Action->ChangePlugin = FALSE;
	}
	if (g_CPU_Action->GSButton) {
		ApplyGSButtonCheats();
		g_CPU_Action->GSButton = FALSE;
	}

	g_CPU_Action->DoSomething = FALSE;
	
	if (g_CPU_Action->SaveState) {
		//test if allowed
		g_CPU_Action->SaveState = FALSE;
		if (!Machine_SaveState()) {
			g_CPU_Action->SaveState = TRUE;
			g_CPU_Action->DoSomething = TRUE;
		}
	}
	if (g_CPU_Action->RestoreState) {
		g_CPU_Action->RestoreState = FALSE;
		Machine_LoadState();
	}
	if (g_CPU_Action->DoInterrupt == TRUE) { g_CPU_Action->DoSomething = TRUE; }
}

int DelaySlotEffectsCompare (DWORD PC, DWORD Reg1, DWORD Reg2) {
	OPCODE Command;

	if (!_MMU->LW_VAddr(PC + 4, Command.Hex)) {
		//DisplayError("Failed to load word 2");
		//ExitThread(0);
		return TRUE;
	}

	switch (Command.op) {
	case R4300i_SPECIAL:
		switch (Command.funct) {
		case R4300i_SPECIAL_SLL:
		case R4300i_SPECIAL_SRL:
		case R4300i_SPECIAL_SRA:
		case R4300i_SPECIAL_SLLV:
		case R4300i_SPECIAL_SRLV:
		case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_MFHI:
		case R4300i_SPECIAL_MTHI:
		case R4300i_SPECIAL_MFLO:
		case R4300i_SPECIAL_MTLO:
		case R4300i_SPECIAL_DSLLV:
		case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV:
		case R4300i_SPECIAL_ADD:
		case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:
		case R4300i_SPECIAL_SUBU:
		case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:
		case R4300i_SPECIAL_XOR:
		case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:
		case R4300i_SPECIAL_SLTU:
		case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:
		case R4300i_SPECIAL_DSUB:
		case R4300i_SPECIAL_DSUBU:
		case R4300i_SPECIAL_DSLL:
		case R4300i_SPECIAL_DSRL:
		case R4300i_SPECIAL_DSRA:
		case R4300i_SPECIAL_DSLL32:
		case R4300i_SPECIAL_DSRL32:
		case R4300i_SPECIAL_DSRA32:
			if (Command.rd == 0) { return FALSE; }
			if (Command.rd == Reg1) { return TRUE; }
			if (Command.rd == Reg2) { return TRUE; }
			break;
		case R4300i_SPECIAL_MULT:
		case R4300i_SPECIAL_MULTU:
		case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:
		case R4300i_SPECIAL_DMULT:
		case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:
		case R4300i_SPECIAL_DDIVU:
			break;
		default:
#ifndef EXTERNAL_RELEASE
			DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
			return TRUE;
		}
		break;
	case R4300i_CP0:
		switch (Command.rs) {
		case R4300i_COP0_MT: break;
		case R4300i_COP0_MF:
			if (Command.rt == 0) { return FALSE; }
			if (Command.rt == Reg1) { return TRUE; }
			if (Command.rt == Reg2) { return TRUE; }
			break;
		default:
			if ( (Command.rs & 0x10 ) != 0 ) {
				switch( Command.funct ) {
				case R4300i_COP0_CO_TLBR: break;
				case R4300i_COP0_CO_TLBWI: break;
				case R4300i_COP0_CO_TLBWR: break;
				case R4300i_COP0_CO_TLBP: break;
				default: 
#ifndef EXTERNAL_RELEASE
					DisplayError("Does %s effect Delay slot at %X?\n6",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
					return TRUE;
				}
			} else {
#ifndef EXTERNAL_RELEASE
				DisplayError("Does %s effect Delay slot at %X?\n7",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
				return TRUE;
			}
		}
		break;
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_MF:
			if (Command.rt == 0) { return FALSE; }
			if (Command.rt == Reg1) { return TRUE; }
			if (Command.rt == Reg2) { return TRUE; }
			break;
		case R4300i_COP1_CF: break;
		case R4300i_COP1_MT: break;
		case R4300i_COP1_CT: break;
		case R4300i_COP1_S: break;
		case R4300i_COP1_D: break;
		case R4300i_COP1_W: break;
		case R4300i_COP1_L: break;
#ifndef EXTERNAL_RELEASE
		default:
			DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
			return TRUE;
		}
		break;
	case R4300i_ANDI:
	case R4300i_ORI:
	case R4300i_XORI:
	case R4300i_LUI:
	case R4300i_ADDI:
	case R4300i_ADDIU:
	case R4300i_SLTI:
	case R4300i_SLTIU:
	case R4300i_DADDI:
	case R4300i_DADDIU:
	case R4300i_LB:
	case R4300i_LH:
	case R4300i_LW:
	case R4300i_LWL:
	case R4300i_LWR:
	case R4300i_LDL:
	case R4300i_LDR:
	case R4300i_LBU:
	case R4300i_LHU:
	case R4300i_LD:
	case R4300i_LWC1:
	case R4300i_LDC1:
		if (Command.rt == 0) { return FALSE; }
		if (Command.rt == Reg1) { return TRUE; }
		if (Command.rt == Reg2) { return TRUE; }
		break;
	case R4300i_CACHE: break;
	case R4300i_SB: break;
	case R4300i_SH: break;
	case R4300i_SW: break;
	case R4300i_SWR: break;
	case R4300i_SWL: break;
	case R4300i_SWC1: break;
	case R4300i_SDC1: break;
	case R4300i_SD: break;
	default:
#ifndef EXTERNAL_RELEASE
		DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
		return TRUE;
	}
	return FALSE;
}

void InPermLoop (void) {
	// *** Changed ***/
	if (g_CPU_Action->DoInterrupt) 
	{
		g_CPU_Action->DoSomething = TRUE;
		return; 
	}
	
	//if (CPU_Type == CPU_SyncCores) { SyncRegisters.CP0[9] +=5; }

	/* Interrupts enabled */
	if (( _Reg->STATUS_REGISTER & STATUS_IE  ) == 0 ) { goto InterruptsDisabled; }
	if (( _Reg->STATUS_REGISTER & STATUS_EXL ) != 0 ) { goto InterruptsDisabled; }
	if (( _Reg->STATUS_REGISTER & STATUS_ERL ) != 0 ) { goto InterruptsDisabled; }
	if (( _Reg->STATUS_REGISTER & 0xFF00) == 0) { goto InterruptsDisabled; }
	
	/* check sound playing */
	_N64System->SyncToAudio();
	
	/* check RSP running */
	/* check RDP running */

	if (*_NextTimer > 0) {
		//_Reg->COUNT_REGISTER += *_Timer + 1;
		//if (CPU_Type == CPU_SyncCores) { SyncRegisters.CP0[9] += Timers.Timer + 1; }
		*_NextTimer = -1;
	}
	return;

InterruptsDisabled:
	if (UpdateScreen != NULL) { UpdateScreen(); }
	//CurrentFrame = 0;
	//CurrentPercent = 0;
	//DisplayFPS();
	DisplayError(GS(MSG_PERM_LOOP));
	StopEmulation();

}

void TestInterpreterJump (DWORD PC, DWORD TargetPC, int Reg1, int Reg2) {
	if (PC != TargetPC) { return; }
	if (DelaySlotEffectsCompare(PC,Reg1,Reg2)) { return; }
	InPermLoop();
	R4300iOp::m_NextInstruction = DELAY_SLOT;
	R4300iOp::m_TestTimer = TRUE;
}

CInterpreterCPU::CInterpreterCPU () :
	m_R4300i_Opcode(NULL)
{

}

CInterpreterCPU::~CInterpreterCPU()
{
}

void CInterpreterCPU::StartInterpreterCPU (void )
{ 
	R4300iOp::m_TestTimer = FALSE;
	R4300iOp::m_NextInstruction = NORMAL;
	DWORD CountPerOp = _Settings->LoadDword(Game_CounterFactor);

	bool & Done = _N64System->m_EndEmulation;
	DWORD & PROGRAM_COUNTER = *_PROGRAM_COUNTER;
	OPCODE & Opcode = R4300iOp::m_Opcode;
	DWORD & JumpToLocation = R4300iOp::m_JumpToLocation;
	BOOL & TestTimer = R4300iOp::m_TestTimer;
	
	R4300iOp::Func * R4300i_Opcode = R4300iOp::BuildInterpreter();

	__try 
	{
		while(!Done)
		{
			if (_MMU->LW_VAddr(PROGRAM_COUNTER, Opcode.Hex)) 
			{
				/*if (PROGRAM_COUNTER > 0x80323000 && PROGRAM_COUNTER< 0x80380000)
				{
					WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER));
					//WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
					//WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*_NextTimer,_SystemTimer->CurrentType());
				}*/
				*_NextTimer -= CountPerOp;
				((void (_fastcall *)()) R4300i_Opcode[ Opcode.op ])();
				
				switch (R4300iOp::m_NextInstruction) {
				case NORMAL: 
					PROGRAM_COUNTER += 4; 
					break;
				case DELAY_SLOT:
					R4300iOp::m_NextInstruction = JUMP;
					PROGRAM_COUNTER += 4; 
					break;
				case JUMP:
					{
						BOOL CheckTimer = (JumpToLocation < PROGRAM_COUNTER || TestTimer); 
						PROGRAM_COUNTER  = JumpToLocation;
						R4300iOp::m_NextInstruction = NORMAL;
						if (CheckTimer)
						{
							TestTimer = FALSE;
							if (*_NextTimer < 0) 
							{ 
								_SystemTimer->TimerDone();
							}
							if (g_CPU_Action->DoSomething) { DoSomething(); }
						}
					}
				}
			} else { 
				DoTLBMiss(R4300iOp::m_NextInstruction == JUMP,PROGRAM_COUNTER);
				R4300iOp::m_NextInstruction = NORMAL;
			}
		}
	} __except( _MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
		ExitThread(0);
	}
}
