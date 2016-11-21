#ifndef PROCSIM_HPP
#define PROCSIM_HPP

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ctime>

#define DEFAULT_K0 3
#define DEFAULT_K1 2
#define DEFAULT_K2 1
#define DEFAULT_R 2
#define DEFAULT_F 4
#define DEFAULT_E 250
#define DEFAULT_S 0

using namespace std;

typedef struct _proc_inst_t
{
    uint32_t instr_num;
	uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;
	bool exception;        
    
} proc_inst_t;

typedef struct _proc_stats_t
{
    float avg_inst_retired;
    float avg_inst_fired;
    float avg_disp_size;
    unsigned long max_disp_size;
    unsigned long retired_instruction;
    unsigned long cycle_count;
    
    unsigned long reg_file_hit_count;
    unsigned long rob_hit_count;
    unsigned long exception_count;
    unsigned long backup_count;
    unsigned long flushed_count;
} proc_stats_t;

struct sch_inst {
	uint32_t instr_num;
	int32_t dest_reg;
	int32_t op_code;
	uint32_t dest_tag;
	bool src_ready[2];
	uint32_t src_tag[2];
	uint32_t pending_since_cycle;	
	bool exception;
	uint32_t status; //status = 88 when issued/fired. Will finish computation in next cycle. status = 100 Awaiting broadcasting  status = 999 when broadcasted i.e completed. status = 1000  when retired
	
	//for 0 cycles, status incremented by 1 for each cycle it is in schQ after set to 'awaiting broadcasting'.
};

struct store_inst{
	
	proc_inst_t *p_inst1;
	bool already_fetched;
	
};

struct rob_entry {
	uint32_t tag;
	int32_t dest_reg;
	bool ready;
	bool exception;
	uint32_t instr_num;
	uint32_t status; //1000 = retired and too be removed.
};

struct reg {
	bool ready;
	uint32_t tag;
	//int32_t value;
};

struct CDB{
	uint32_t instr_num;
	bool busy;
	uint32_t tag;	
	int32_t dest_reg;
	bool exception;
};

/* struct FU {
	bool busy;
}; */
struct SB{
	uint32_t FU[3];
};

struct details{
	uint32_t seq;
	uint32_t tag;
	uint32_t since_cycle;	
};

struct exec_details{
	uint32_t seq;
	uint32_t tag;
	uint32_t since_cycle;
};

struct fu_details{
	uint32_t tag;
	uint32_t seq;
};

struct out_det{
	uint32_t inst;
	uint32_t fetch;
	uint32_t disp;
	uint32_t sched;
	uint32_t exec;
	uint32_t state;
};

// Global data structures
extern vector<proc_inst_t> dispQ;
extern vector<sch_inst> schQ;
extern uint32_t schQ_max_size;
extern SB scoreboard;
extern vector<CDB> result_bus;
extern uint32_t F;
extern reg reg_file[128];
extern bool debug;
extern float num_retired;
extern double dispQ_len;
extern float num_fired;
extern uint32_t max_dispQ_len;
extern vector<out_det> output_details;
extern vector<rob_entry> ROB;
extern vector<store_inst> store_fetched;
extern reg backup_1[128]; 
extern reg backup_2[128]; 

extern uint32_t reg_hit_cnt;
extern uint32_t rob_hit_cnt;
extern uint32_t exception_cnt;
//unsigned long backup_count;
extern uint32_t flushed_cnt;
extern uint32_t backup_copied;
extern uint32_t IB1;
extern uint32_t IB2;


extern uint32_t clk;
extern uint32_t PC;
extern uint32_t S;
extern uint32_t E;
extern uint32_t K[3];

//Functions
bool instr_fetch();
void dispatch();
void schedule();
void executed();
void state_update();
bool mark_retire();

//Supporting functions
bool comp_addr(proc_inst_t d1, proc_inst_t d2);
bool comp_cycle(exec_details d1, exec_details d2);
bool comp_tag_same_cycle(exec_details d1,exec_details d2);
bool comp_instr_num(proc_inst_t p1, proc_inst_t p2);
bool comp_fu_tag(fu_details f1, fu_details f2);
bool comp_sch_instr_num(sch_inst s1, sch_inst s2);
void display_schQ();
void display_dispQ();
void display_clk(uint32_t t);
void display_rob();
uint32_t flush_ROB();
void flush_schQ(uint32_t exp_inst_num);
void flush_dispQ();
void scoreboard_reset();
void clear_resultbus();
void display_resultbus();
void display_FU();
void display_reg();
void display_stored();
void print_time();


bool read_instruction(proc_inst_t* p_inst);

void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t e, uint64_t s);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

#endif /* PROCSIM_HPP */
