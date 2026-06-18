#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bridge_fsm.h"

// Helper to convert state enum to string
const char* state_to_string(fsm_state_t state) {
    switch(state) {
        case ST_IDLE:     return "ST_IDLE";
        case ST_WWAIT:    return "ST_WWAIT";
        case ST_READ:     return "ST_READ";
        case ST_WRITE:    return "ST_WRITE";
        case ST_WRITEP:   return "ST_WRITEP";
        case ST_RENABLE:  return "ST_RENABLE";
        case ST_WENABLE:  return "ST_WENABLE";
        case ST_WENABLEP: return "ST_WENABLEP";
        default:          return "UNKNOWN";
    }
}

// Function to print current simulation status
void print_cycle(int cycle, const ahb_inputs_t *inputs, const bridge_state_t *state, const apb_outputs_t *outputs) {
    printf("Cycle %02d | State: %-10s -> Next: %-10s | valid=%d tempselx=%d\n", 
           cycle, state_to_string(state->PRESENT_STATE), state_to_string(state->NEXT_STATE), 
           state->valid, state->tempselx);
    printf("         AHB Inputs : Haddr=0x%08X Hwrite=%d Htrans=%d Hwdata=0x%08X\n", 
           inputs->Haddr, inputs->Hwrite, inputs->Htrans, inputs->Hwdata);
    printf("         APB Outputs: Paddr=0x%08X Pwrite=%d Pselx=%d Penable=%d Pwdata=0x%08X Hreadyout=%d\n", 
           outputs->Paddr, outputs->Pwrite, outputs->Pselx, outputs->Penable, outputs->Pwdata, outputs->Hreadyout);
    printf("------------------------------------------------------------------------------------\n");
}

int main() {
    printf("====================================================================================\n");
    printf("                AHB-TO-APB BRIDGE FSM AUTOMATED TESTBENCH                           \n");
    printf("====================================================================================\n\n");

    ahb_inputs_t inputs;
    bridge_state_t state;
    apb_outputs_t outputs;

    int cycle = 0;

    // --- TEST 1: RESET TEST ---
    printf("[Test 1] Asserting Reset\n");
    bridge_init(&state, &outputs);
    inputs.Hresetn = false;
    inputs.Hwrite = false;
    inputs.Hreadyin = true;
    inputs.Htrans = 0;
    inputs.Haddr = 0;
    inputs.Hwdata = 0;
    inputs.Prdata = 0;

    bridge_step(&inputs, &state, &outputs);
    print_cycle(++cycle, &inputs, &state, &outputs);
    
    // Assert reset state outputs are all zero/defaults
    assert(state.PRESENT_STATE == ST_IDLE);
    assert(outputs.Paddr == 0);
    assert(outputs.Pwrite == false);
    assert(outputs.Pselx == 0);
    assert(outputs.Penable == false);

    // --- TEST 2: SINGLE READ TRANSACTION ---
    printf("\n[Test 2] Single Read Transaction (Address = 0x80001000)\n");
    inputs.Hresetn = true;
    inputs.Htrans = 2; // NONSEQ
    inputs.Hwrite = false; // Read
    inputs.Haddr = 0x80001000; // Selects Slave 1 (tempselx = 1)
    inputs.Hreadyin = true;
    inputs.Prdata = 0xABCDEFAF;

    // Cycle 1: Idle state detects valid read address. Transitions to READ next.
    bridge_step(&inputs, &state, &outputs);
    print_cycle(++cycle, &inputs, &state, &outputs);
    assert(state.PRESENT_STATE == ST_IDLE);
    assert(state.valid == true);
    assert(state.tempselx == 1);
    assert(state.NEXT_STATE == ST_READ);

    // Cycle 2: Read Setup. Pselx driven. Transitions to RENABLE next.
    inputs.Htrans = 0; // Deassert transfer on AHB side
    bridge_step(&inputs, &state, &outputs);
    print_cycle(++cycle, &inputs, &state, &outputs);
    assert(state.PRESENT_STATE == ST_READ);
    assert(outputs.Paddr == 0x80001000);
    assert(outputs.Pselx == 1);
    assert(outputs.Pwrite == false);
    assert(outputs.Penable == false); // Enable is registered, not high yet
    assert(outputs.Hreadyout == false);
    assert(state.NEXT_STATE == ST_RENABLE);

    // Cycle 3: Read Enable. Transitions to IDLE next.
    bridge_step(&inputs, &state, &outputs);
    print_cycle(++cycle, &inputs, &state, &outputs);
    assert(state.PRESENT_STATE == ST_RENABLE);
    assert(outputs.Penable == true);
    assert(outputs.Hreadyout == true);
    assert(outputs.Hrdata == 0xABCDEFAF);
    assert(state.NEXT_STATE == ST_IDLE);

    // Cycle 4: Complete transaction, back to IDLE.
    bridge_step(&inputs, &state, &outputs);
    print_cycle(++cycle, &inputs, &state, &outputs);
    assert(state.PRESENT_STATE == ST_IDLE);
    assert(outputs.Penable == false);
    assert(outputs.Pselx == 0);

    printf("\nAll test cases PASSED successfully!\n");
    return 0;
}
