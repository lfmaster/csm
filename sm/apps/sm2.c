#include <time.h>
#include <string.h>

#include <options/options.h>
#include <csm/csm_all.h>

struct sm1_params {
	const char * file_in;
	const char * file_out;
	const char * file_jj;
	int format;
} p;

extern void sm_options(struct sm_params*p, struct option*ops);

extern int distance_counter;

void spit(LDP ld, FILE * stream) {
	switch(p.format) {
		case(0): {
			JO jo = ld_to_json(ld);
			fputs(json_object_to_json_string(jo), stream);
			fputs("\n", stream);
			jo_free(jo);
		}
		case(1): {
			
		}
	}
}

int main(int argc, const char*argv[]) {
	
	struct sm_params params;
	struct sm_result result;
	
	struct option* ops = options_allocate(30);
	options_string(ops, "in", &p.file_in, "stdin",
		"Input file ");
	options_string(ops, "out", &p.file_out, "stdout",
		"Output file ");
	options_string(ops, "file_jj", &p.file_jj, "",
		"File for journaling -- if left empty, journal not open.");
	p.format = 0;
/*	options_int(ops, "format", &p.format, 0,
		"Output format (0: json, 1: carmen, ... )"); */
	
	sm_options(&params, ops);
	if(!options_parse_args(ops, argc, argv)) {
		fprintf(stderr, "\n\nUsage:\n");
		options_print_help(ops, stderr);
		return -1;
	}

	FILE * file_in = open_file_for_reading(p.file_in);
	if(!file_in) return -1;
	FILE * file_out = open_file_for_writing(p.file_out);
	if(!file_out) return -1;
	
	if(strcmp(p.file_jj, "")) {
		FILE * jj = open_file_for_writing(p.file_jj);
		if(!jj) return -1;
		jj_set_stream(jj);
	}
	

	LDP laser_ref;
	if(!(laser_ref = ld_read_smart(file_in))) {
		sm_error("Could not read first scan.\n");
		return -1;
	}
	copy_d(laser_ref->odometry, 3, laser_ref->estimate);
	
	
	spit(laser_ref, file_out);
	while(1) {
		LDP laser_sens = ld_read_smart(file_in);
		if(!laser_sens) break;
		
		params.laser_ref  = laser_ref;
		params.laser_sens = laser_sens;
		pose_diff_d(laser_sens->odometry, laser_ref->odometry,
			/* = */ params.first_guess);

/*			sm_gpm(&params, &result); */
		sm_icp(&params, &result); 
		
		oplus_d(laser_ref->estimate, result.x, laser_sens->estimate);

		spit(laser_sens, file_out);

		ld_free(laser_ref); laser_ref = laser_sens;
	}
	ld_free(laser_ref);
	
	return 0;
}
