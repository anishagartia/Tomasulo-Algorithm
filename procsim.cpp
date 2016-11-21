#include "procsim.hpp"

/*********************
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @r Number of result buses
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 ********************/
 
// Global data structures
vector<proc_inst_t> dispQ;
vector<sch_inst> schQ;
uint32_t schQ_max_size;
SB scoreboard;
vector<CDB> result_bus;
reg reg_file[128];
bool debug = true;
float num_retired = 0;
double dispQ_len = 0;
float num_fired = 0;
uint32_t max_dispQ_len = 0;
vector<out_det> output_details;
vector<rob_entry> ROB;
vector<store_inst> store_fetched;
reg backup_1[128]; 
reg backup_2[128]; 
uint32_t F;
uint32_t S;
uint32_t E;
uint32_t K[3];

uint32_t reg_hit_cnt = 0;
uint32_t rob_hit_cnt = 0;
uint32_t exception_cnt = 0;
//uint32_t backup_count;
uint32_t flushed_cnt = 0;
uint32_t backup_copied = 0;
uint32_t IB1;
uint32_t IB2;

//Global Clock that counts clock cycles
uint32_t clk = 0;
//Global Program coutner that countes the number of instructions fetched.
uint32_t PC = 0;

void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t e, uint64_t s)
{
//cout << "max " << store_fetched.max_size() << endl;
//cout << "max outp " << output_details.max_size() << endl;
//schQ is a vector schedule Q. 
//dispQ is dispatch Q
S = s;
E = e;
K[0] = k0;
K[1] = k1;
K[2] = k2;
schQ_max_size = 2*(k0 + k1 + k2);
//create scoreboard
scoreboard.FU[0] = k0;
scoreboard.FU[1] = k1;
scoreboard.FU[2] = k2;

CDB initial;
initial.busy = false;
initial.tag = 0;
initial.dest_reg = 0;
initial.exception = false;

//set all ready in Reg to true
for (int i = 0; i < 128; i++){
	reg_file[i].ready = true;
	backup_1[i].ready = true;
	backup_2[i].ready = true;
}

//create result bus
for (uint32_t i = 0; i < r; i++){
	result_bus.push_back(initial);
}

F = f;

if (ofstream("log.txt")){
	ofstream flog("log.txt", ios::trunc );
	flog.close();
}
else {
	ofstream flog("log.txt", ios::out | ios::app );
	cout << "log.txt created" << endl;
	flog.close();
}

if (ofstream("output.txt")){
	ofstream foutp("output.txt", ios::trunc);	
}

ofstream foutp("output.txt", ios::out | ios::app);

foutp << "Processor Settings" << '\n';
foutp << "R: " << r << "\n";
foutp << "k0: " << k0 << "\n" ;
foutp << "k1: " << k1 << "\n" ;
foutp << "k2: " << k2 << "\n";
foutp << "F: "  << f << "\n";
foutp << "E: "  << e << "\n";
foutp << "S: "  << s << "\n";
foutp << "\n";

foutp.close();

}

/******************
 Supporting function
 ******************/
bool comp_addr(proc_inst_t d1, proc_inst_t d2) { 
	return (d1.instruction_address < d2.instruction_address); 
}

void display_schQ(){
	cout << "***** SCH Q - cycle " << clk <<" *****" << endl;
	cout << "instr_num" << '\t' << "dest_reg"  << '\t' << "op_code"  << '\t' << "dest_tag" << '\t' << "src_ready[0]" << '\t' << "src_ready[1]"  << '\t' << "src_tag[0]" << '\t' << "src_tag[1]"<< '\t' << "status" << '\t' << "Exception" << '\n';
	for (uint32_t l = 0; l<schQ.size(); l++){
		cout << schQ[l].instr_num << '\t' << schQ[l].dest_reg  << '\t' << schQ[l].op_code  << '\t' << schQ[l].dest_tag << '\t' << schQ[l].src_ready[0] << '\t' << schQ[l].src_ready[1]  << '\t' << schQ[l].src_tag[0] << '\t' << schQ[l].src_tag[1]<< '\t' << schQ[l].status  << '\t' << schQ[l].exception << '\n';
	}
}

void display_dispQ(){
	cout << "***** DISP Q - cycle " << clk << " *****" << endl;
	cout << "instr_num" << '\t' << "instr addr" << '\t' << "op_code" << '\t' << "src_reg[0]" << '\t' << "src_reg[1]" << '\t' << "dest_reg" << '\t' <<  "Exception" << '\n';
	for (uint32_t l = 0; l<dispQ.size(); l++){
		cout << dispQ[l].instr_num << '\t' << dispQ[l].instruction_address << '\t' << dispQ[l].op_code << '\t' << dispQ[l].src_reg[0] << '\t' << dispQ[l].src_reg[1] << '\t' << dispQ[l].dest_reg << '\t' << dispQ[l].exception << '\n';
	}
}

void display_stored(){
	cout << "***** Stored_fetched - cycle " << clk << " *****" << endl;
	cout << "instr_num" << '\t' << "instr addr" << '\t' << "op_code" << '\t' << "src_reg[0]" << '\t' << "src_reg[1]" << '\t' << "dest_reg" << '\t' <<  "Exception" << '\n';
	for (uint32_t l = 0; l< store_fetched.size(); l++){
		cout << store_fetched[l].p_inst1->instr_num << '\t' << store_fetched[l].p_inst1->instruction_address << '\t' << store_fetched[l].p_inst1->op_code << '\t' << store_fetched[l].p_inst1->src_reg[0] << '\t' << store_fetched[l].p_inst1->src_reg[1] << '\t' << store_fetched[l].p_inst1->dest_reg << '\t' << store_fetched[l].p_inst1->exception << '\n';
	}
}

void display_rob(){
	cout << "***** ROB - cycle " << clk << " *****" << endl;
	cout << "Instr Num" << '\t' << "TAG" << '\t' << "Dest Reg" << '\t' << "Ready" << '\t' << "Exception" << '\t' << "Status" << '\n' ;
	for (uint32_t i = 0; i < ROB.size(); i++){
		cout << ROB[i].instr_num << '\t' << ROB[i].tag << '\t' << ROB[i].dest_reg << '\t' << ROB[i].ready << '\t' << ROB[i].exception << '\t' << ROB[i].status << '\n';
	}
	
}

void display_clk(uint32_t t){
	if (clk % t == 0)
		cout << "Completed cycle " << clk << '\n';		
}

void display_resultbus(){
	cout << "***** Result Bus - cycle " << clk << " *****" << endl;
	cout << "Instr Num" << '\t' << "Busy" << '\t' << "TAG" << '\t' << "Dest Reg" << '\t' << "Exception" << '\n' ;
	for (uint32_t i = 0; i < result_bus.size(); i++){
		cout << result_bus[i].instr_num << '\t' << result_bus[i].busy << '\t' << result_bus[i].tag << '\t' << result_bus[i].dest_reg << '\t' << result_bus[i].exception << '\n';
	}
}

void display_FU(){
	cout << "** SB-cycle" << clk << " **" << endl;
	cout << scoreboard.FU[0] << '\t' << scoreboard.FU[1] << '\t' << scoreboard.FU[2] << endl;
}

void display_reg(){
	cout << "** Reg-cycle" << clk << " **" << endl;
	cout << "Reg Num" << '\t' << "Tag" << '\t' << "Ready" << '\n' ;
	for (uint32_t i = 0; i < 32; i++){
		cout << i << '\t' << reg_file[i].tag << '\t' << reg_file[i].ready << endl;
	}
}

void print_time(){
	time_t start_t = time(0);
	char* dt = ctime(&start_t);
	cout << "Time: " << dt << endl;
}

 
/****************************************
 * Subroutine that simulates the processor.
 *   The processor should fetch instructions as appropriate, until all instructions have executed
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 ****************************************/
 
void run_proc(proc_stats_t* p_stats)
{
bool exception_flag;

time_t start_t = time(0);
char* dt = ctime(&start_t);
cout << "Start time: " << dt << endl;
if (S == 2){
	IB1 = 20;
	IB2 = 1;
	}
					
//time_t  timev;
//cout << time(&timev) << endl;

do{	
			
	//uint32_t prev_clk = clk;
	
	clk = clk + 1;		
	exception_flag = false;		
	/* if (clk > 128){
		display_schQ();
	} */
	exception_flag = mark_retire();	
	if (exception_flag == true){
		continue;
	}	
	executed();		
	schedule();	
	dispatch();
	state_update();	
	bool fetch_valid = false;
	fetch_valid = instr_fetch();			
	
	if (debug == true)
		display_clk(1000);		
	
	/* if (clk == 135)
		break; */
	
}while ((schQ.size() > 0) || (dispQ.size() > 0) || (exception_flag == true));

time_t end_t = time(0);
char* dt_end = ctime(&end_t);
cout << "End time: " << dt_end << endl;

}


/******************
Subroutine: Instruction Fetch
******************/

bool instr_fetch(){
	proc_inst_t *p_inst2 = new proc_inst_t;
	store_inst store_temp;	
	out_det out_temp;
	bool confirm = false;
	bool refetched = false;		
	
	//Output log file
	ofstream flog("log.txt", ios::out | ios::app );	
	p_inst2->exception = false;
	for (uint32_t i=0; i<F; i++) {		
		if (store_fetched.size() > PC){
			PC = PC + 1 ;			
			p_inst2 = store_fetched[PC-1].p_inst1;			
			//p_inst2->exception = false;
			p_inst2->instr_num = PC;	
			refetched = true;
			
			dispQ.push_back(*p_inst2);

			out_temp.inst = PC;
			out_temp.fetch = clk;
			out_temp.disp = clk + 1;				
			output_details.push_back(out_temp);
			
		}
		else {			
			proc_inst_t *p_inst3 = new proc_inst_t;
			confirm = read_instruction(p_inst3);			
			if (confirm == true){
				PC = PC + 1;
				p_inst3->instr_num = PC;					
				if (PC % E == 0) {
					p_inst3->exception = true;
				}
				else {
					p_inst3->exception = false; 
				}										
				store_temp.p_inst1 = p_inst3;				
				store_temp.already_fetched = true;
				store_fetched.push_back(store_temp);
				refetched = false;	
				p_inst2 = p_inst3;				
				
				dispQ.push_back(*p_inst2);	
				out_temp.inst = PC;
				out_temp.fetch = clk;
				out_temp.disp = clk + 1;				
				output_details.push_back(out_temp);
			}
		}												
		
		if (debug == true){
			if (refetched == true)
				flog << clk << '\t' << "RE-FETCHED" << '\t' << PC << '\n';			
			else if (confirm == true)
				flog << clk << '\t' << "FETCHED" << '\t' << PC << '\n';			
		}
	}						
	flog.close();
		
	return confirm;
}

/******************
Subroutine: Supporting functions for Dispatch
******************/
bool comp_instr_num(proc_inst_t p1, proc_inst_t p2){
	return( p1.instr_num < p2.instr_num);
}
/******************
Subroutine: Dispatch
******************/
void dispatch(){
	//Output log file
	ofstream flog("log.txt", ios::out | ios::app );
	sch_inst sch_temp;	
	dispQ_len = dispQ_len + dispQ.size();
	ofstream fdislog("dis_len.txt", ios::out | ios::app );
	fdislog << "DispQ len clk " << clk << '\t' << dispQ.size() << endl;
	fdislog.close();
	if (dispQ.size() > max_dispQ_len)
		max_dispQ_len = dispQ.size();	
	while((schQ.size() < schQ_max_size) && (dispQ.size() > 0)){			
		//sort(dispQ.begin(), dispQ.end(),comp_instr_num);
		sch_temp.instr_num = dispQ.front().instr_num;
		sch_temp.dest_reg = dispQ.front().dest_reg;
		sch_temp.op_code = dispQ.front().op_code;
		sch_temp.pending_since_cycle = 0;
		sch_temp.exception = dispQ.front().exception;		
		for (int i = 0; i<2; i++){			
			if (dispQ.front().src_reg[i] == -1){				
				sch_temp.src_ready[i] = true;	
				sch_temp.src_tag[i] = 0;				
			}			
			else {	
				bool found_flag = false;
				if (S == 1) {					
					for (uint32_t j = 0; j < ROB.size(); j++){											
						if (ROB[ROB.size() - 1 - j].dest_reg == dispQ.front().src_reg[i]){						
							if (ROB[ROB.size() - 1 - j].ready == false){				
								found_flag = false;
								rob_hit_cnt ++;
								break;						
							}
							else if (ROB[ROB.size() - 1 - j].ready == true){			
								sch_temp.src_ready[i] = true;	
								sch_temp.src_tag[i] = 0;
								found_flag = true;
								rob_hit_cnt ++;
								break;					
							}								
						}  				
					}				
					if (found_flag == true){					
						continue;
					}	
				}				
				if ((found_flag == false)&&(reg_file[dispQ.front().src_reg[i]].ready == true)){			
					sch_temp.src_ready[i] = true;	
					sch_temp.src_tag[i] = 0;
					reg_hit_cnt++;
				}				
				else if (reg_file[dispQ.front().src_reg[i]].ready == false){			
					sch_temp.src_ready[i] = false;
					sch_temp.src_tag[i] = reg_file[dispQ.front().src_reg[i]].tag;
					if (S == 2)
						{reg_hit_cnt++;}
				}				
			}			
		}	

		if (dispQ.front().dest_reg != -1){			
			//assign tag
			reg_file[dispQ.front().dest_reg].tag = dispQ.front().instr_num;
			sch_temp.dest_tag = reg_file[dispQ.front().dest_reg].tag;
			reg_file[dispQ.front().dest_reg].ready = false;				
		}	
		else if (dispQ.front().dest_reg == -1){		
			sch_temp.dest_tag = dispQ.front().instr_num;
		}		
		sch_temp.status = 0; // Instruction inserted to schQ in this cycle. cannot be fired in this cycle. 		
		schQ.push_back(sch_temp);			
	
		output_details[dispQ.front().instr_num - 1].sched = clk + 1;		
		
		if (S == 1) {
			rob_entry rob_temp;
			rob_temp.instr_num = dispQ.front().instr_num;
			rob_temp.tag = sch_temp.dest_tag;
			rob_temp.dest_reg = sch_temp.dest_reg;
			rob_temp.ready = false;
			rob_temp.exception = false;
			rob_temp.status = 0;	
			
			ROB.push_back(rob_temp);
		}		
		
		dispQ.erase(dispQ.begin());
				
		if (debug == true){
			flog << clk << '\t' << "DISPATCHED" << '\t' << schQ.back().instr_num << '\n';
		}		
	}	
	flog.close();
}


/********************
Subroutine: Supporting fucntions for Schedule
*********************/
bool comp_fu_tag(fu_details f1, fu_details f2){
	return(f1.tag < f2.tag);
}

/********************
Subroutine: Schedule
*********************/
void schedule(){	
	//Output log file
	ofstream flog("log.txt", ios::out | ios::app );

	fu_details fu_temp;
	vector<fu_details> fu_sort;
	for (uint32_t i = 0; i < schQ.size(); i++){
		if((schQ[i].src_ready[0] == true) && (schQ[i].src_ready[1] == true) && schQ[i].status == 0 ){
			if (((scoreboard.FU[1] != 0) && (schQ[i].op_code == -1)) || (scoreboard.FU[schQ[i].op_code] != 0)){
				fu_temp.tag = schQ[i].dest_tag;
				fu_temp.seq = i;		
				fu_sort.push_back(fu_temp);
			}
		}			
	}	
	sort(fu_sort.begin(), fu_sort.end(),comp_fu_tag);	
	
	for (uint32_t i = 0; i<fu_sort.size(); i++){
		if ((schQ[fu_sort[i].seq].op_code == -1) && (scoreboard.FU[1] == 0))
			continue;
		else if (scoreboard.FU[schQ[fu_sort[i].seq].op_code] == 0)
			continue;
		else {
			
			schQ[fu_sort[i].seq].status = 88; // status = 88 implies it is issued.
			num_fired++;
			if (schQ[fu_sort[i].seq].op_code == -1)
				scoreboard.FU[1] = scoreboard.FU[1] -1;
			else 
				scoreboard.FU[schQ[fu_sort[i].seq].op_code] = scoreboard.FU[schQ[fu_sort[i].seq].op_code] -1;
			
			output_details[schQ[fu_sort[i].seq].instr_num - 1].exec = clk + 1;
			if (debug == true){
				flog << clk << '\t' << "SCHEDULED" << '\t' << schQ[fu_sort[i].seq].instr_num << '\n';
			}
		}		
	}	
	//fully associative search through schedule Q to update it via Result Bus
	for (uint32_t r = 0; r<result_bus.size(); r++){
		if (result_bus[r].busy == true){
			for (uint32_t s = 0; s < schQ.size(); s++){
				if ((result_bus[r].tag == schQ[s].src_tag[0]) && (schQ[s].src_ready[0] == false)){
					schQ[s].src_ready[0] = true;
					schQ[s].src_tag[0] = 0;
				}
				if ((result_bus[r].tag == schQ[s].src_tag[1]) && (schQ[s].src_ready[1] == false)){
					schQ[s].src_ready[1] = true;
					schQ[s].src_tag[1] = 0;					
				}
			}
		}		
	}
	
	flog.close();		
}

/***************
Supporting Function for Executed
***************/
bool comp_cycle(exec_details d1, exec_details d2) {
	return (d1.since_cycle < d2.since_cycle);
}

bool comp_tag_same_cycle(exec_details d1,exec_details d2) {
	return ((d1.since_cycle == d2.since_cycle)&&(d1.tag < d2.tag));
}
/*********************
Subroutine: Executed
*********************/
void executed(){
	
	//Output log file
	ofstream flog("log.txt", ios::out | ios::app );
	vector<exec_details> to_broadcast;
	exec_details exdet_temp;
	
	for (uint32_t i = 0; i<schQ.size(); i++){		
		if (schQ[i].status == 88) {
			schQ[i].status = 100;
			schQ[i].pending_since_cycle = clk;
			if (debug == true){
				flog << clk << '\t' << "EXECUTED" << '\t' << schQ[i].instr_num << '\n';
			}
		}
		if ((schQ[i].status == 100)){ //&& (schQ[i].status < 999)) {			
			exdet_temp.seq = i;
			exdet_temp.tag = schQ[i].dest_tag;
			exdet_temp.since_cycle = schQ[i].pending_since_cycle; //schQ[i].status - 100;		
			to_broadcast.push_back(exdet_temp);
		}
	}
		
	// schQ entry set to pending retirement in this cycle has status as 100. status has been incremented by 1 for every cycle is has been in schQ after pending retirement. Thus Oldest instruction has highest cycle number (in details object)

	sort(to_broadcast.begin(), to_broadcast.end(),comp_cycle); // gives us array sorted as per cycle number. Within each cycle, messy tags
	
	sort(to_broadcast.begin(), to_broadcast.end(),comp_tag_same_cycle); // gives sorted array according to ascending order of tag within each cycle. This first element of this vector ahs larget cycle number, and lowest tag number. Next element has the next tag(if any) within same cycle.	
	
	for (uint32_t t = 0; t < to_broadcast.size(); t++){
		for (uint32_t j = 0; j<result_bus.size(); j++){
			if (result_bus[j].busy == false){
				result_bus[j].instr_num = schQ[to_broadcast[t].seq].instr_num;
				result_bus[j].dest_reg = schQ[to_broadcast[t].seq].dest_reg;
				result_bus[j].tag = schQ[to_broadcast[t].seq].dest_tag;
				result_bus[j].exception = schQ[to_broadcast[t].seq].exception;
				result_bus[j].busy = true;
				schQ[to_broadcast[t].seq].status = 999;
				if (schQ[to_broadcast[t].seq].op_code == -1)
					{scoreboard.FU[1] = scoreboard.FU[1] + 1;}
				else
					{
					scoreboard.FU[schQ[to_broadcast[t].seq].op_code] = scoreboard.FU[schQ[to_broadcast[t].seq].op_code] + 1;					
					}					
				//num_fired++;
				if (debug == true){
					flog << clk << '\t' << "BROADCASTED" << '\t' << result_bus[j].instr_num << '\n';
				}
				break;
				
			}
		}			
	}	
	
	//Rob is written via result bus. Marks exceptions
	if (S == 1){
		for (uint32_t rb = 0; rb < result_bus.size(); rb++){
			for (uint32_t r = 0; r < ROB.size(); r++){
				if ((result_bus[rb].instr_num == ROB[r].instr_num) &&(result_bus[rb].busy == true)) {
					ROB[r].tag = result_bus[rb].tag;
					ROB[r].dest_reg = result_bus[rb].dest_reg;
					ROB[r].exception = result_bus[rb].exception;
					ROB[r].ready = true;
					ROB[r].status = 0;
				}
			
			}
		}
	}
	else if (S == 2){
		//Ccheck point Replair. Copy to Ref_file and backup_1
		// register file written via result bus
		for (uint32_t r = 0; r < result_bus.size(); r++){
			if ((result_bus[r].busy == true) && (result_bus[r].dest_reg != -1)){
				if ((reg_file[result_bus[r].dest_reg].tag == result_bus[r].tag ) /*&& (reg_file[result_bus[r].dest_reg].ready == false)*/){
					reg_file[result_bus[r].dest_reg].ready = true;					
				}
			}
		}			
	}
			
	flog.close();
}
/*********************
Supporting functions for Exceptiion Handling - ROB w/bypass
*********************/
uint32_t flush_ROB(){
	uint32_t next_PC;
	next_PC = ROB.front().instr_num - 1;
	ROB.clear();
	return (next_PC);
}

void flush_schQ(uint32_t exp_inst_num){
	ofstream fdislog("dis_len.txt", ios::out | ios::app );
	for (uint32_t i=0; i < schQ.size(); i++){		
		if ((S==1)&&(schQ[i].status != 1000)){
			flushed_cnt++;		
		}
		/* else if ((S==2)){//&&(schQ[i].status != 1000)){
			flushed_cnt++;
			if (clk > 128)
				fdislog << clk << '\t' <<  "flushed " << schQ[i].instr_num << endl;
		} */
	}
	if (S==2){
		vector<sch_inst> schq_temp;
		schq_temp = schQ;
		sort(schq_temp.begin(), schq_temp.end(), comp_sch_instr_num);
		flushed_cnt += (schq_temp.front().instr_num - IB2 + 1);
		//flushed_cnt += (schq_temp.back().instr_num - IB2);
		fdislog << clk << '\t'<< "largest " << schq_temp.front().instr_num << " IB2 " << IB2 << endl;
	}
	schQ.clear();
	fdislog.close();
}

void flush_dispQ(){
	dispQ.clear();
}

void scoreboard_reset(){
	scoreboard.FU[0] = K[0];
	scoreboard.FU[1] = K[1];
	scoreboard.FU[2] = K[2];
}

void clear_resultbus(){
	for (uint32_t r = 0; r < result_bus.size(); r++){
		if (result_bus[r].busy == true){			
			result_bus[r].instr_num = 0;
			result_bus[r].busy = false;
			result_bus[r].tag = 0;
			result_bus[r].dest_reg = 0;
			result_bus[r].exception = false;
		}
	}	
}

bool comp_sch_instr_num(sch_inst s1,sch_inst s2) {
	return (s1.instr_num > s2.instr_num);
}


/*********************
Subroutine: mark_retire
Description: Mark completed instructions as retired.
Increment status instruction that were to be retired in previous cycles.
*********************/
bool mark_retire(){
	
/* vector<uint32_t> to_retire_seq;
vector<uint32_t> to_retire_tag;
vector<uint32_t> to_retire_since_cycle;

 */	//Output log file
	ofstream flog("log.txt", ios::out | ios::app );
	ofstream fdislog("dis_len.txt", ios::out | ios::app );
	clear_resultbus();
	if (S == 1){
		for (uint32_t rob_itr = 0; rob_itr < ROB.size(); rob_itr ++){
			if (ROB.front().ready == false){
				return (false);
				break;
			}				
			if (ROB[rob_itr].ready == true){			
				if (ROB[rob_itr].exception == true){
					exception_cnt++;
					store_fetched[ROB[rob_itr].instr_num - 1].p_inst1->exception = false;
					if (debug == true){
						flog << clk << '\t' << "EXCEPTION" << '\t' << ROB[rob_itr].instr_num << '\n';
					}
					//Handle Exception
					PC = ROB[rob_itr].instr_num - 1;
					flush_ROB();				
					flush_dispQ();
					flush_schQ(ROB[rob_itr].instr_num);
					scoreboard_reset();
					clear_resultbus();
					for (uint32_t r = 0;r<128; r++){
						reg_file[r].ready = true;
						reg_file[r].tag = 0;
					}
					output_details.erase(output_details.begin()+PC, output_details.end());
					return (true);	
					break;
				}
				else{ // No exception. Proceed normally
					ROB[rob_itr].status = 1000; 
					//ROB[rob_itr].ready = false;
					// Write from ROB head to reg file
					if (reg_file[ROB[rob_itr].dest_reg].tag == ROB[rob_itr].tag){
						reg_file[ROB[rob_itr].dest_reg].ready = true;
						reg_file[ROB[rob_itr].dest_reg].tag = 0;				
					}
					// When we mark something on ROB to be retired, we mark the same one on SchQ to be retitired
					for (uint32_t i = 0; i<schQ.size(); i++){		
						if ((schQ[i].dest_tag == ROB[rob_itr].tag)&&(schQ[i].status == 999)){//check if instruction is complete
							schQ[i].status = 1000; // if instruction is completed, mark it as retired.
							num_retired++;
							if (debug == true){
								flog << clk << '\t' << "STATE UPDATED" << '\t' << schQ[i].instr_num << '\n';
							}
							output_details[schQ[i].instr_num - 1].state = clk;
							break;
						}	 					
					} 			
				}		
			}
			else{		
				break;		
				}
		}
	}	
	else if ((S == 2) && (schQ.size() > 0)){		
		//Handle CPR exceptions. 			
		for (uint32_t i = 0; i<schQ.size(); i++){			
			// Handle Exceptions
			if ((schQ.size() > 0)&&(schQ[i].status == 999)&&(schQ[i].exception == true)){
				exception_cnt++;
				store_fetched[schQ[i].instr_num - 1].p_inst1->exception = false;
				if (debug == true){
					flog << clk << '\t' << "EXCEPTION" << '\t' << schQ[i].instr_num << '\n';
				}				
				if (PC < 20)
					{PC = IB2 - 1;}
				else 
					{PC = IB2-1;}
				for (uint32_t r = 0; r < 128; r++){
					reg_file[r].ready = true;
					reg_file[r].tag = 0;						
				}					
				flush_dispQ();
				flush_schQ(schQ[i].instr_num);
				clear_resultbus();
				scoreboard_reset();			
				output_details.erase(output_details.begin()+PC, output_details.end());				
				return (true);	
				break;
			}		
			// Else no exception, proceed normally
			else if (schQ[i].status == 999){ //check if instruction is complete
				schQ[i].status = 1000; // if instruction is completed, mark it as retired.
				
				//num_retired++;
				if (debug == true){
					flog << clk << '\t' << "STATE UPDATED" << '\t' << schQ[i].instr_num << '\n';
				}
				output_details[schQ[i].instr_num - 1].state = clk;	
				//num_retired++;
			}
			//check if all instructions less than IB1 have completed
			bool IBnotdone_flag = false;			
			for (uint32_t k = 0; k<schQ.size(); k++){				
				if ((schQ[k].instr_num <= IB1)&& (schQ[k].status != 1000))
					{IBnotdone_flag = true;}
			}							
			if ((schQ.size() > 0)&&(IBnotdone_flag == false)){
				backup_copied++;
				//sort schQ to make most recent PC to IB1.
				vector<sch_inst> schq_temp;
				schq_temp = schQ;
				sort(schq_temp.begin(), schq_temp.end(), comp_sch_instr_num);
				
				flog << clk << '\t' << "Backup2" << '\t' << IB2 << " to " << IB1 << '\n';
				fdislog << clk << '\t' << "Backup " << IB2 <<" to " << IB1 << ", New IB1: " << schq_temp.front().instr_num << ", New IB2: " << IB1+1 << endl;
											
				IB2 = IB1+1;			
				IB1 = schq_temp.front().instr_num;						
				num_retired = IB2 - 1;
			}							
		}
	}
	clear_resultbus();	
	flog.close();	
	fdislog.close();
	return (false);	
	
}

/***************
Supporting Function for State Update
***************/
/* bool comp_cycle(details d1, details d2) {
	return (d1.since_cycle < d2.since_cycle);
}

bool comp_tag_same_cycle(details d1,details d2) {
	return ((d1.since_cycle == d2.since_cycle)&&(d1.tag < d2.tag));
} */

/*********************
Subroutine: State Update
Description: Retire (i.e. delete from SchQ) functions marked to retire
*********************/
void state_update(){

vector<details> to_retire;
details det_temp;
for (uint32_t i = 0; i<schQ.size(); i++){	
	if (schQ[i].status == 1000) {//check if any instruction is pending retirement		
		det_temp.seq = i;
		det_temp.tag = schQ[i].dest_tag;
		det_temp.since_cycle = schQ[i].status;		
		to_retire.push_back(det_temp);				
	}
}

if (S == 1){
	while ((ROB.size() > 0)&&(ROB.front().status == 1000)){
		ROB.erase(ROB.begin());
	}
}

//Output log file
ofstream flog("log.txt", ios::out | ios::app );

//Delete the tags by order of sorted list.
for(uint32_t i = 0; i< to_retire.size();  i++){	
	schQ.erase(schQ.begin() + to_retire[i].seq - (i));	
}

}

/*********************
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 ********************/
void complete_proc(proc_stats_t *p_stats) 
{
	
//Output log file
ofstream foutp("output.txt", ios::out | ios::app );

p_stats-> avg_inst_retired = (num_retired/clk);
p_stats-> avg_inst_fired = (num_fired/clk);
p_stats-> avg_disp_size = (dispQ_len/clk);
p_stats-> max_disp_size = (max_dispQ_len);
p_stats-> retired_instruction = (num_retired);
p_stats-> cycle_count = clk;

p_stats-> reg_file_hit_count = reg_hit_cnt;
p_stats-> rob_hit_count = rob_hit_cnt;
p_stats-> exception_count = exception_cnt;
p_stats-> flushed_count = flushed_cnt;
p_stats->backup_count = backup_copied;



	
foutp << "INST" << '\t' << "FETCH" << '\t' << "DISP" << '\t' << "SCHED" << '\t' << "EXEC" << '\t' << "STATE" << '\n';

cout << '\n' << "INST" << '\t' << "FETCH" << '\t' << "DISP" << '\t' << "SCHED" << '\t' << "EXEC" << '\t' << "STATE" << '\n';


for (uint32_t i = 0; i < output_details.size(); i++){
	foutp << output_details[i].inst << '\t' << output_details[i].fetch << '\t' <<output_details[i].disp << '\t' <<output_details[i].sched << '\t' <<output_details[i].exec << '\t' << output_details[i].state << '\n';	
	
	//cout << output_details[i].inst << '\t' << output_details[i].fetch << '\t' <<output_details[i].disp << '\t' <<output_details[i].sched << '\t' <<output_details[i].exec << '\t' << output_details[i].state << '\n';
}


foutp << "\nProcessor stats:" << "\n";
foutp <<  "Total instructions: " <<  p_stats->retired_instruction << '\n';
foutp << "Avg Dispatch queue size: " << setprecision(6) << fixed << p_stats->avg_disp_size << '\n';
foutp <<  "Maximum Dispatch queue size: " << p_stats->max_disp_size << '\n';
foutp <<  "Avg inst fired per cycle: " << p_stats->avg_inst_fired << '\n';
foutp <<  "Avg inst retired per cycle: " << p_stats->avg_inst_retired << '\n';
foutp <<  "Total run time (cycles): " << p_stats->cycle_count << '\n';

foutp <<  "Total register file hits: " << p_stats->reg_file_hit_count << '\n';
foutp <<  "Total ROB hits: " << p_stats->rob_hit_count << '\n';
foutp <<  "Total exceptions: " << p_stats->exception_count << '\n';
foutp <<  "Total B1 to B2 backups: " << p_stats->backup_count << '\n';
foutp <<  "Total flushed instructions: " << p_stats->flushed_count << '\n';



foutp.close();

}
