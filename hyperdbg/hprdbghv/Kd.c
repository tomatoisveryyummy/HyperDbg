/**
 * @file Kd.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Routines related to kernel debugging
 * @details 
 * @version 0.1
 * @date 2020-12-20
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief  apply step one instruction to the debuggee
 * @return VOID 
 */
VOID
KdStepInstruction(ULONG CoreId)
{
    //
    // It is because maximum instruction length in x64 is 16 bytes
    //
    BYTE InstructionBytesOnRip[16 * 2] = {0};

    DbgBreakPoint();

    //
    // Read the buffer at that place
    //
    MemoryMapperReadMemorySafe(g_GuestState[CoreId].LastVmexitRip, InstructionBytesOnRip, 16 * 2);

    //
    // Set vm-exit on hardware debug breakpoint exceptions
    //

    //
    // Set hardware breakpoint on next instruction
    //

    //
    // Send the handshake to show that it Stepped
    //
    SerialConnectionSend("Stepped", 7);
}

/**
 * @brief manage system halt on vmx-root mode 
 * @return VOID 
 */
VOID
KdManageSystemHaltOnVmxRoot()
{
    ULONG CurrentCore;
    BYTE  InstructionBytesOnRip[MAXIMUM_INSTR_SIZE * 2] = {0};

    CurrentCore = KeGetCurrentProcessorNumber();

    //
    // Send the handshake to show that it paused
    //
    SerialConnectionSend("Paused", 6);

    //
    // We check for receiving buffer (unhalting) only on the
    // first core and not on every cores
    //
    if (CurrentCore == 0)
    {
        //
        // *** First Core ***
        //

        //
        // Find the current instruction
        //
        MemoryMapperReadMemorySafe(g_GuestState[CurrentCore].LastVmexitRip, InstructionBytesOnRip, MAXIMUM_INSTR_SIZE * 2);

        //
        // Send it to the debugger
        //
        SerialConnectionSend(InstructionBytesOnRip, MAXIMUM_INSTR_SIZE * 2);

        //
        // Send it to the debuggee
        //
        SerialConnectionSend(InstructionBytesOnRip, MAXIMUM_INSTR_SIZE * 2);

        while (TRUE)
        {
            UCHAR Test = NULL;
            KdHyperDbgRecvByte(&Test);
            if (Test == 'G')
            {
                break;
            }
            if (Test == 'S')
            {
                KdStepInstruction(CurrentCore);
            }
        }
    }
    else
    {
        //
        // All cores except first core
        //
    }
}

/**
 * @brief routines for broadcast system halt 
 * @return VOID 
 */
VOID
KdBroadcastHaltOnAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcVmExitAndHaltSystemAllCores, NULL);
}

/**
 * @brief Halt the system
 * 
 * @return VOID 
 */
VOID
KdHaltSystem(PDEBUGGER_PAUSE_PACKET_RECEIVED PausePacket)
{
    //
    // Broadcast to halt everything
    //
    KdBroadcastHaltOnAllCores();

    //
    // Set the status
    //
    PausePacket->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
}
