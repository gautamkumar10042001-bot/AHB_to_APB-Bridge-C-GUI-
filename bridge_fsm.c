// bridge_fsm.c

#include "bridge_fsm.h"

//  Initialization Function
// Sets all pipeline register to 0, reset FSM to ST_IDLE
// and deactivates all output signals
///...................................

void bridge_init(bridge_state_t *state, apb_outputs_t *outputs) {
  if (!state || !outputs)
    return;

  // Reset Registers
  state->Haddr1 = 0;
  state->Haddr2 = 0;
  state->Hwdata1 = 0;
  state->Hwdata2 = 0;
  state->Hwritereg = 0;

  state->valid = false;
  state->tempselx = 0;

  state->PRESENT_STATE = ST_IDLE;
  state->NEXT_STATE = ST_IDLE;

  // Clear temporary outputs
  state->Paddr_temp = 0;
  state->Pwdata_temp = 0;
  state->Pwrite_temp = 0;
  state->Penable_temp = 0;
  state->Hreadyout_temp = 0;
  state->Pselx_temp = 0;

  // Clear registered outputs
  outputs->Pwrite = 0;
  outputs->Penable = 0;
  outputs->Pselx = 0;
  outputs->Paddr = 0;
  outputs->Pwdata = 0;
  outputs->Hreadyout = 0;
  outputs->Hrdata = 0;
  outputs->Hresp = 0;
}

/*
FSM Function Setup
//Executes teh sequential updates on rising clock edge
// evaluates combintiaonal logic for next state and output
// latches
*/

void bridge_step(const ahb_inputs_t *inputs, bridge_state_t *state,
                 apb_outputs_t *outputs) {

  // BLOCK 1: SEQUENTIAL LOGIC(Posedge CLK)

  // Pipeline Register Updates (State Updates)
  if (!inputs->Hresetn) {

    // Reset Active
    state->Haddr1 = 0;
    state->Haddr2 = 0;
    state->Hwdata1 = 0;
    state->Hwdata2 = 0;
    state->Hwritereg = 0;

    state->PRESENT_STATE = ST_IDLE;

    outputs->Paddr = 0;
    outputs->Pwrite = false;
    outputs->Pselx = 0;
    outputs->Pwdata = 0;
    outputs->Penable = false;
    outputs->Hreadyout = false;
    outputs->Hrdata = 0;
    outputs->Hresp = 0;
  } else {
    // Registe rupdats on the risign edge of clock
    state->PRESENT_STATE = state->NEXT_STATE;

    state->Haddr2 = state->Haddr1;
    state->Haddr1 = inputs->Haddr;

    state->Hwdata2 = state->Hwdata1;
    state->Hwdata1 = inputs->Hwdata;

    state->Hwritereg = inputs->Hwrite;

    // Drive registerdd outputs
    outputs->Paddr = state->Paddr_temp;
    outputs->Pwrite = state->Pwrite_temp;
    outputs->Pselx = state->Pselx_temp;
    outputs->Pwdata = state->Pwdata_temp;
    outputs->Penable = state->Penable_temp;
    outputs->Hreadyout = state->Hreadyout_temp;
    outputs->Hrdata = inputs->Prdata;

    outputs->Hresp = 0; // Constant Response OKAY
  }

  // BLOCK 2 COMBINATIONAL LOGIC

  // 1. Valid Logic Generation
  state->valid = false;
  if (inputs->Hresetn && inputs->Hreadyin && (inputs->Haddr >= 0x80000000) &&
      (inputs->Haddr <= 0x8C000000) &&
      (inputs->Htrans == 0x02 || inputs->Htrans == 0x03)) {
    state->valid = true;
  }

  // 2.Tempselx Logic (Decoder)
  state->tempselx = 0;

  if (inputs->Hresetn) {
    if (inputs->Haddr >= 0x80000000 && inputs->Haddr < 0x84000000) {
      state->tempselx = 1; // Slave 1
    } else if (inputs->Haddr >= 0x84000000 && inputs->Haddr < 0x88000000) {
      state->tempselx = 2; // Slave 2
    } else if (inputs->Haddr >= 0x88000000 && inputs->Haddr < 0x8C000000) {
      state->tempselx = 4; // Slave 3
    }
  }

  // 3.State Machine Logic

  switch (state->PRESENT_STATE) {
  case ST_IDLE:
    if (!state->valid) {
      state->NEXT_STATE = ST_IDLE;
    } else if (inputs->Hwrite) {
      state->NEXT_STATE = ST_WWAIT; // Jump to write path
    } else {
      state->NEXT_STATE = ST_READ; // Jump to read path
    }
    break;

  case ST_WWAIT:
    if (!state->valid) {
      state->NEXT_STATE = ST_WRITE;
    } else {
      state->NEXT_STATE = ST_WRITEP;
    }
    break;

  case ST_READ:
    state->NEXT_STATE = ST_RENABLE;
    break;

  case ST_WRITE:
    if (!state->valid) {
      state->NEXT_STATE = ST_WENABLE;
    } else {
      state->NEXT_STATE = ST_WENABLEP; // Stay here
    }
    break;

  case ST_WRITEP:
    state->NEXT_STATE = ST_WENABLE;
    break;

  case ST_RENABLE:
    if (!state->valid) {
      state->NEXT_STATE = ST_IDLE;
    } else if (state->valid && inputs->Hwrite) {
      state->NEXT_STATE = ST_WWAIT;
    } else {
      state->NEXT_STATE = ST_READ;
    }

    break;

  case ST_WENABLE:
    if (inputs->Hreadyin) {
      state->NEXT_STATE = ST_IDLE;
    } else {
      state->NEXT_STATE = ST_WWAIT;
    }
    break;

  case ST_WENABLEP:
    if (!state->valid && state->Hwritereg) {
      state->NEXT_STATE = ST_WRITE;
    } else if (state->valid && !state->Hwritereg) {
      state->NEXT_STATE = ST_WRITEP;
    } else {
      state->NEXT_STATE = ST_READ;
    }

    break;

  default:
    state->NEXT_STATE = ST_IDLE;
    break;
  }

  // 4. Output logic : COmbinational (Temp values represent Latches)
  switch (state->PRESENT_STATE) {
  case ST_IDLE:
    if (state->valid && !inputs->Hwrite) {
      state->Paddr_temp = inputs->Haddr;
      state->Pwrite_temp = inputs->Hwrite;
      state->Pselx_temp = state->tempselx;
      state->Penable_temp = false;
      state->Hreadyout_temp = false;

    } else if (state->valid && inputs->Hwrite) {
      state->Paddr_temp = 0;
      state->Penable_temp = false;
      state->Hreadyout_temp = true;
    } else {
      state->Paddr_temp = 0;
      state->Penable_temp = false;
      state->Hreadyout_temp = true;
    }
    break;

  case ST_WWAIT:
    if (!state->valid) {
      state->Paddr_temp = state->Haddr1;
      state->Pwrite_temp = true;
      state->Pselx_temp = state->tempselx;
      state->Penable_temp = false;
      state->Pwdata_temp = inputs->Hwdata;
      state->Hreadyout_temp = false;
    } else {
      state->Paddr_temp = state->Haddr1;
      state->Pwrite_temp = true;
      state->Pselx_temp = state->tempselx;
      state->Pwdata_temp = state->Hwdata1;
      state->Penable_temp = false;
      state->Hreadyout_temp = false;
    }
    break;

  case ST_READ:
    state->Penable_temp = true;
    state->Hreadyout_temp = true;
    break;

  case ST_WRITEP:
    state->Penable_temp = true;
    state->Hreadyout_temp = true;
    break;

  case ST_RENABLE:
    if (state->valid && !inputs->Hwrite) {
      state->Paddr_temp = inputs->Haddr;
      state->Pwrite_temp = inputs->Hwrite;
      state->Pselx_temp = state->tempselx;
      state->Penable_temp = false;
      state->Hreadyout_temp = false;
    } else if (state->valid && inputs->Hwrite) {
      state->Pselx_temp = 0;
      state->Penable_temp = false;
      state->Hreadyout_temp = true;
    } else {
      state->Pselx_temp = 0;
      state->Penable_temp = false;
      state->Hreadyout_temp = true;
    }
    break;

  case ST_WENABLEP:
    if (!state->valid && state->Hwritereg) {
      state->Paddr_temp = state->Haddr2;
      state->Pwrite_temp = inputs->Hwrite;
      state->Pselx_temp = state->tempselx;
      state->Penable_temp = inputs->Hwdata;
      state->Hreadyout_temp = false;
    } else {
      state->Paddr_temp = state->Haddr2;
      state->Pwrite_temp = inputs->Hwrite;
      state->Pselx_temp = state->tempselx;
      state->Pwdata_temp = state->Hwdata2;
      state->Penable_temp = false;
      state->Hreadyout_temp = true;
    }
    break;

  case ST_WENABLE:
    if (!state->valid && state->Hwritereg) {
      state->Paddr_temp = 0;
      state->Penable_temp = false;
      state->Hreadyout_temp = false;
    } else {
      state->Pselx_temp = 0;
      state->Penable_temp = false;
      state->Hreadyout_temp = false;
    }
    break;

  default:
    break;
  }
}