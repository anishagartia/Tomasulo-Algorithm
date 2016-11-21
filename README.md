# Tomasulo Algorithm Pipelined Processor

In this project, we construct a simulator for an out-of-order superscalar processor that uses the Tomasulo algorithm and fetches F instructions per cycle. We design the simulator to maintain consistent state in the presence of exceptions with two separate schemes: 1. ROB with bypass and 2. Checkpoint repair. We also use the simulator to find the appropriate number of function units, fetch rate and result buses for each benchmark.

## Pipeline Stages:

1. Instruction Fetch/Decode
2. Dispatch
3. Scheduling
4. Execution
5. State Update


The Dispatch Queue:

1. The fetch/decode rate is F instructions per cycle. Each cycle, the Fetch/Decode unit can always supply F instructions to the dispatch queue.
2. The dispatch queue length is unlimited, so there is always room for instructions in the dispatch queue.
3. The dispatch queue is scanned from head to tail (in program order).
4. When an instruction is inserted into the scheduling queue, it is deleted from the dispatch queue.
5. Tags are generated sequentially for every instruction, beginning with 0. The first instruction in a trace will use a tag value of 0, the second will use 1, etc. 

The Scheduling Queue:

1. The size of the scheduling queue is 2*(number of k0 function units + number of k1 function units + number of k2 function units)
2. If there are multiple independent instructions ready to fire during the same cycle in the scheduling queue, service them in tag order (i.e., lowest tag value to highest). (This will guarantee that your results can match ours.)
3. A fired instruction remains in the scheduling queue until it completes.
4. The fire/issue rate is only limited by the number of available FUs.

The Function Units: 

1. Funciton unit type: 0, number of units: k0
2. Funciton unit type: 1, number of units: k1
3. Funciton unit type: 2, number of units: k2

The Result Buses:

1. There are R result buses (also called common data buses, or CDBs). This means that up to R instructions that complete in the same cycle may update the schedule queues and register file in the same cycle.
2. If an instruction wants to retire but all result buses are used, it waits an additional cycle. The function unit is not freed in this case. Hence a subsequent instruction might stall because the function unit is busy. The function unit is freed only when the result is put onto a result bus.
3. The result buses do not have registers or latches. Thus broadcasts do not persist between clock cycles.
4. The order of result bus broadcasts is as follows:
  - Within any given cycle, earlier tags get priority over later tags.
  - Earlier cycles with pending broadcasts get priority over later cycles.
  
  
Variation of Parameters:

1. Number of FUs of each type (k0, k1, k2) of either 1 or 2 between 1 to 3 units each.
2. The number of result buses, R. This should never exceed the total number of FUs.
3. Fetch rate, F = 4 or 8
4. Exception rate, E = 333
5. Use both repair schemes

Based on the results, the least amount of hardware is selected (as measured by total number of FUs (k0+k1+k2) and result buses (R)) for each benchmark trace that provides nearly (>95%) of the highest value for retired IPC. 


## Breakdown of code:

1. Makefile: compiles the code
2. procsim_driver.cpp: contains the main() method to run the simulator. 
3. procsim.hpp: Used for defining structures and method declarations. 
4. procsim.cpp: Where all your methods should go. 
5. Traces folder: contains the traces to pass to the simulator.

Input Trace format:
The input traces will be given in the form:

(address) (function unit type) (dest reg #) (src1 reg #) (src2 reg#)   
(address) (function unit type) (dest reg #) (src1 reg #) (src2 reg#)    
...

Where   
(address) is the address of the instruction (in hex)   
(function unit type) is either "0", "1" or "2"   
(dest reg #), (src1 reg#) and (src2 reg #) are integers in the range [0..127]    
Note: If any reg # is -1, then there is no register for that part of the instruction (e.g., a branch instruction has -1 for its (dest reg #))    

The command line parameters are as follows:

1. R – Result buses
2. F – Fetch rate (instructions per cycle)
3. J – Number of k0 function units
4. K – Number of k1 function units
5. L – Number of k2 function units
6. trace_file – Path name to the trace file


## Output:
For each trace, the output contains 2 files:

1. An output file, which contains:
  - The processor settings
  - A record of when each instruction was in each stage
  - The processor statistics
2. A log file, which contains the cycle-by-cycle behavior of the machine.


The simulator outputs the following statistics after completion of the experimental run:

1. Total number of instructions in the trace
2. Average dispatch queue size
3. Maximum dispatch queue size
4. Average number of instructions fired per cycle
5. Average number of instructions retired per cycle (IPC)
6. Total run time (cycles)
7. Total number of register file hits (NEW)
8. Total number of ROB hits (NEW)
9. Total number of exceptions (NEW)
10. Total number of backup copies from B1 to B2 (NEW)
11. Total number of flushed instructions: any non-retired instruction that was dispatched some time before the exception occurred.
