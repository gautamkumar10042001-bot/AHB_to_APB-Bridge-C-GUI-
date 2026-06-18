# AHB-to-APB Bridge FSM

This repository is a small learning project that shows how an AHB master can communicate with APB peripherals through a bridge implemented as a finite state machine in C. It includes:

- the bridge implementation
- a console-based testbench
- a browser-based visual simulator
- a sample simulation log

If you are new to bus protocols, the main idea is simple: AHB is the faster side of the system, APB is the simpler peripheral side, and the bridge is the logic that translates between them.

## AHB and APB Basics

AHB and APB are both part of ARM's AMBA bus family, but they are designed for different jobs and different levels of complexity.

- AHB is the faster, more complex bus used for high-performance transfers.
- APB is the simpler, lower-power bus used to connect peripheral devices such as timers, UARTs, GPIO blocks, and control registers.

AHB supports pipelined transfers, multiple control signals, and higher throughput. APB keeps the interface minimal so peripherals are easier to implement and consume less power. Because of this difference, a direct connection between an AHB master and an APB peripheral is not practical.

The bridge sits between the two buses and translates an AHB transaction into the APB protocol. It also handles timing differences, slave selection, and the setup/enable phases required by APB.

## Why the Bridge Is Required in Pipelining

In a pipelined AHB system, the master can start a new transfer before the previous one has completely finished. That is efficient for performance, but APB does not work that way. APB expects a more serialized flow:

1. setup phase
2. enable phase
3. completion before the next transfer

The bridge is required because it buffers address and write-data information, keeps track of the current transfer state, and makes sure the APB side only sees a valid transaction when the timing is correct. In other words, the bridge converts the fast pipelined AHB side into the simpler step-by-step APB side without losing the meaning of the transaction.

## What the Bridge Does

The bridge accepts an AHB address phase, checks whether the transfer is valid, selects the target APB slave, and then drives the APB control signals through the proper setup and enable phases. The FSM in `bridge_fsm.c` is the heart of the project.

The model includes 8 FSM states:

- `ST_IDLE` - waiting for a valid transfer
- `ST_WWAIT` - write wait stage
- `ST_READ` - read setup stage
- `ST_WRITE` - write setup stage
- `ST_WRITEP` - write setup for pipelined transfers
- `ST_RENABLE` - read enable stage
- `ST_WENABLE` - write enable stage
- `ST_WENABLEP` - write enable for pipelined transfers

## Address Decoding

The bridge uses the AHB address to decide which APB slave to select:

- `0x80000000` to `0x83FFFFFF` -> slave 1, `Pselx = 1`
- `0x84000000` to `0x87FFFFFF` -> slave 2, `Pselx = 2`
- `0x88000000` to `0x8BFFFFFF` -> slave 3, `Pselx = 4`

A transfer is treated as valid when all of the following are true:

- reset is deasserted
- `Hreadyin` is high
- the address is inside the valid range
- `Htrans` is `2` (`NONSEQ`) or `3` (`SEQ`)

## Files in the Project

- `bridge_fsm.h` - shared types, FSM state definitions, and function declarations used by the whole project
- `bridge_fsm.c` - the bridge implementation, including reset handling, state transitions, and output generation
- `testbench.c` - an automated console testbench that drives a sample transaction and checks the results with assertions
- `gui.html` - an interactive browser visualization that helps you explore the FSM and signal behavior visually
- `Result.log` - a sample output log from a successful run of the testbench

## How to Build and Run the C Testbench

There is no Makefile in the repository, so compile the model directly with a C compiler.

```bash
gcc -std=c11 -Wall -Wextra bridge_fsm.c testbench.c -o testbench
./testbench
```

If everything is working, the program should print the reset cycle, a single read transaction, and finish with:

```text
All test cases PASSED successfully!
```

## What the Testbench Checks

The included testbench verifies a simple sequence:

1. reset behavior
2. a valid read transaction to `0x80001000`
3. APB setup and enable behavior
4. correct state transitions back to idle

It also checks that the FSM selects the expected slave and that the read data is returned through `Hrdata`.

## Browser Simulator

`gui.html` provides a richer visual view of the same bridge behavior. Open it in a browser to explore the FSM, signal values, and state transitions interactively.

Because it is a standalone HTML file, you can usually open it directly from your file manager or from VS Code.

## Expected Output

A successful run should resemble the sample in `Result.log`. The log shows the FSM moving from `ST_IDLE` to `ST_READ`, then to `ST_RENABLE`, and finally back to `ST_IDLE` after the transaction completes.

## Notes

- The C model is intentionally compact and educational rather than a full hardware verification environment.
- Output signals are driven in a way that matches the simple simulation flow used by the testbench.
- If you extend the bridge, update both the FSM logic and the testbench so the README stays accurate.

## Next Steps

If you want to keep improving the project, good follow-up additions would be:

- a Makefile for one-command builds
- more test cases for write transactions and pipelined transfers
- a short waveform or timing diagram in the README
- a small section documenting any intended RTL-vs-model differences
