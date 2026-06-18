// Bridge_fsm.h

#ifndef BRIDGE_FSM_H
#define BRIDGE_FSM_H

#include <stdbool.h>
#include <stdint.h>

// 1. Define the 8 FSM States
typedef enum {
  ST_IDLE = 0,     // 3'b000
  ST_WWAIT = 1,    // 3'b001
  ST_READ = 2,     // 3'b010
  ST_WRITE = 3,    // 3'b011
  ST_WRITEP = 4,   // 3'b100
  ST_RENABLE = 5,  // 3'b101
  ST_WENABLE = 6,  // 3'b110
  ST_WENABLEP = 7, // 3'b111

} fsm_state_t;

// 2. Inputs coming from the AHB Master
typedef struct {
  bool Hresetn;    // Reset(Active Low)
  bool Hreadyin;   // Master ready signal
  uint32_t Haddr;  // AHB Address
  bool Hwrite;     // 0 = Read, 1 = Write
  uint32_t Htrans; // Transfer tupe(2 = NONSEQ, 3 = SEQ)
  uint32_t Hwdata; // Write Data
  uint32_t Prdata; // APB Read Data (from Slaves)

} ahb_inputs_t;

// 3. Registered Outputs going to APB Slaves
typedef struct {
  uint32_t Paddr;  // APB Address
  uint32_t Pwdata; // APB Write data
  bool Pwrite;     // APB Write Signal
  uint32_t Pselx;  // Slave Select lines (Bit 0 =slave1, 1 =slave2, etc)
  bool Penable;    // APB Enable
  bool Hreadyout;  // Ready signal back to APB
  uint32_t Hrdata; // Read data sent to AHB
  uint32_t Hresp;  // Response (Okay / Error)

} apb_outputs_t;

// 4. Internal registers of Bridge FSM
typedef struct {
  fsm_state_t PRESENT_STATE;
  fsm_state_t NEXT_STATE;

  // Pipeline registers (to match AHB 1-cycle data delay)
  uint32_t Haddr1, Haddr2;
  uint32_t Hwdata1, Hwdata2;
  bool Hwritereg;

  // Decoding & Handshake helpers
  bool valid;
  uint32_t tempselx;

  // Temp latch values (used during calcualtion)
  uint32_t Paddr_temp;
  uint32_t Pwdata_temp;
  bool Pwrite_temp;
  uint32_t Pselx_temp;
  bool Penable_temp;
  bool Hreadyout_temp;

} bridge_state_t;

// Functions
void bridge_init(bridge_state_t *state, apb_outputs_t *outputs);
void bridge_step(const ahb_inputs_t *inputs, bridge_state_t *state,
                 apb_outputs_t *outputs);

#endif