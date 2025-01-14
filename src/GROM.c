/*
 *
 * Copyright (c) 2017, Sean Smith <ss1917@camden.rutgers.edu> and Andrey Grigoriev <andrey.grigoriev@rutgers.edu>
 *
 * This program is distributed under those terms in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License v2 for more details.
 *
 * This program is available as free software solely for research and educational purposes: The code of this program
 * can be redistributed it and/or modified only for those purposes and under the terms of the GPL2 as published by
 * the Free Software Foundation. All rights are reserved with respect to commercial applications of this program.
 * Please contact Andrey Grigoriev <andrey.grigoriev@rutgers.edu> for additional details.
 *
 * You should have received a copy of the GNU General Public License v2
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>

#include "bam.h"
#include "sam.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <omp.h>

#include <dirent.h>
#include <sys/stat.h>

#define max_chr_names 30000

#define CONFIG_FILENAME "config-v1.0.3.conf"
#define MAXBUF 1024
#define DELIM "="

#define CONFIG_HEADER_COMMENT1 "# Command-line arguments override values in this file."
#define CONFIG_HEADER_COMMENT2 "# Values after the equal sign and before comments starting with # can be changed, but descriptions before the equal sign should not be changed and lines should not be moved, added or deleted."
#define CONFIG_HEADER_COMMENT3 "# To restore defaults, delete this file, and run GROM with no arguments. This file will be rewritten to gromconfig directory in user home."
#define CONFIG_HEADER_COMMENT4 "# See README for more detailed description of parameters. A config file with default values can be found in test directory of GROM release."
#define CONFIG_HEADER_LENGTH 4

#define VERBOSE_DESCRIPTION "Verbose"
#define VERBOSE_COMMENT "# print_gromconfig_to_standard_output (0-OFF,1-ON)"
#define VERBOSE_LINE 0

#define INPUT_FILE_DESCRIPTION "Input_file"
#define INPUT_FILE_COMMENT "# path to BAM file can be given here (if blank, path is required on the command line) Option -i"
#define INPUT_FILE_LINE 1

#define REF_FILE_DESCRIPTION "Reference_file"
#define REF_FILE_COMMENT "# path to reference file in fasta format can be given here (if blank, path is required on the command line) Option -r"
#define REF_FILE_LINE 2

#define RESULTS_FILE_DESCRIPTION "Output_file"
#define RESULTS_FILE_COMMENT "# path to output file can be given here (if blank, path is required on the command line) Option -o"
#define RESULTS_FILE_LINE 3

#define DUP_FILTERING_DESCRIPTION "Turn_on_duplicate_read_filtering"
#define DUP_FILTERING_COMMENT "# (0-OFF,1-ON) Option -M"
#define DUP_FILTERING_LINE 4

#define GENDER_DESCRIPTION "Gender"
#define GENDER_COMMENT "# (0-female,1-male)  Option -g"
#define GENDER_LINE 5

#define PLOIDY_DESCRIPTION "Ploidy"
#define PLOIDY_COMMENT "# Option -p"
#define PLOIDY_LINE 6

#define PROCESSES_DESCRIPTION "Number_of_processes"
#define PROCESSES_COMMENT "# 2_threads_per_process Option -P"
#define PROCESSES_LINE 7

#define BASE_PHRED_QUAL_DESCRIPTION "Base_phred_quality_score_threshold"
#define BASE_PHRED_QUAL_COMMENT "# Option -b"
#define BASE_PHRED_QUAL_LINE 8

#define MAP_QUAL_THRESHOLD_DESCRIPTION "Low_mapping_quality_threshold"
#define MAP_QUAL_THRESHOLD_COMMENT "# Option -q"
#define MAP_QUAL_THRESHOLD_LINE 9

#define PROB_THRESHOLD_DESCRIPTION "Probability_score_threshold"
#define PROB_THRESHOLD_COMMENT "# Option -v"
#define PROB_THRESHOLD_LINE 10

#define PROB_THRESHOLD_INS_DESCRIPTION "Probability_score_threshold_for_insertions"
#define PROB_THRESHOLD_INS_COMMENT "# Option -e"
#define PROB_THRESHOLD_INS_LINE 11

#define PROB_THRESHOLD_CNV_DESCRIPTION "Probability_score_threshold_for_CNVs"
#define PROB_THRESHOLD_CNV_COMMENT "# Option -V"
#define PROB_THRESHOLD_CNV_LINE 12

#define WIN_SAMP_FACTOR_DESCRIPTION "Sampling_rate"
#define WIN_SAMP_FACTOR_COMMENT "# Option -A"
#define WIN_SAMP_FACTOR_LINE 13

#define MAX_CHR_LEN_DESCRIPTION "Chromosome_maximum_length"
#define MAX_CHR_LEN_COMMENT "# Option -B"
#define MAX_CHR_LEN_LINE 14

#define MIN_CHR_LEN_DESCRIPTION "Chromosome_minimum_length"
#define MIN_CHR_LEN_COMMENT "# Option -C"
#define MIN_CHR_LEN_LINE 15

#define MIN_REPEAT_DESCRIPTION "Dinucleotide_repeat_minimum_length"
#define MIN_REPEAT_COMMENT "# Option -D"
#define MIN_REPEAT_LINE 16

#define MIN_REPEAT_STDEV_DESCRIPTION "Dinucleotide_repeat_minimum_standard_deviation"
#define MIN_REPEAT_STDEV_COMMENT "# Option -E"
#define MIN_REPEAT_STDEV_LINE 17

#define RANK_DESCRIPTION "Ranking"
#define RANK_COMMENT "# Option -K"
#define RANK_LINE 18

#define DUP_COV_THRESHOLD_DESCRIPTION "Duplication_coverage_threshold"
#define DUP_COV_THRESHOLD_COMMENT "# Option -L"
#define DUP_COV_THRESHOLD_LINE 19

#define TURN_OFF_SPLIT_READ_DESCRIPTION "Turn_off_split-read_analysis"
#define TURN_OFF_SPLIT_READ_COMMENT "# Option -S"
#define TURN_OFF_SPLIT_READ_LINE 20

#define EXCESS_COV_THRESHOLD_DESCRIPTION "Excessive_coverage_threshold"
#define EXCESS_COV_THRESHOLD_COMMENT "# Option -U"
#define EXCESS_COV_THRESHOLD_LINE 21

#define WINDOW_MIN_SIZE_DESCRIPTION "Window_minimum_size"
#define WINDOW_MIN_SIZE_COMMENT "# Option -W"
#define WINDOW_MIN_SIZE_LINE 22

#define WINDOW_MAX_SIZE_DESCRIPTION "Window_maximum_size"
#define WINDOW_MAX_SIZE_COMMENT "# Option -X"
#define WINDOW_MAX_SIZE_LINE 23

#define MIN_NUM_BLOCKS_DESCRIPTION "Minimum_number_of_blocks"
#define MIN_NUM_BLOCKS_COMMENT "# Option -Y"
#define MIN_NUM_BLOCKS_LINE 24

#define BLOCK_SIZE_DESCRIPTION "Block_size"
#define BLOCK_SIZE_COMMENT "# Option -Z"
#define BLOCK_SIZE_LINE 25

#define MIN_SNV_RATIO_DESCRIPTION "Minimum_SNV_ratio"
#define MIN_SNV_RATIO_COMMENT "# Option -a"
#define MIN_SNV_RATIO_LINE 26

#define MIN_READS_SUPPORT_BREAKPOINT_DESCRIPTION "Minimum_reads_supporting_breakpoint"
#define MIN_READS_SUPPORT_BREAKPOINT_COMMENT "# Option -d"
#define MIN_READS_SUPPORT_BREAKPOINT_LINE 27

#define MIN_SV_RATIO_DESCRIPTION "Minimum_SV_ratio"
#define MIN_SV_RATIO_COMMENT "# Option -j"
#define MIN_SV_RATIO_LINE 28

#define MAX_HOMOPOLYMER_THRESHOLD_DESCRIPTION "Maximum_homopolymer_threshold"
#define MAX_HOMOPOLYMER_THRESHOLD_COMMENT "# Option -k"
#define MAX_HOMOPOLYMER_THRESHOLD_LINE 29

#define MIN_INDEL_RATIO_DESCRIPTION "Minimum_indel_ratio"
#define MIN_INDEL_RATIO_COMMENT "# Option -m"
#define MIN_INDEL_RATIO_LINE 30

#define MIN_SNV_READS_SUPP_VAR_DESCRIPTION "Minimum_SNV_reads_supporting_variant"
#define MIN_SNV_READS_SUPP_VAR_COMMENT "# Option -n"
#define MIN_SNV_READS_SUPP_VAR_LINE 31

#define MIN_SD_DISCORDANCE_DESCRIPTION "Minimum_standard_deviations_for_discordance"
#define MIN_SD_DISCORDANCE_COMMENT "# Option -s"
#define MIN_SD_DISCORDANCE_LINE 32

#define MAX_EVIDENCE_RATIO_DESCRIPTION "Maximum_evidence_ratio"
#define MAX_EVIDENCE_RATIO_COMMENT "# Option -u"
#define MAX_EVIDENCE_RATIO_LINE 33

#define MAX_INS_BREAKPOINT_THRESHOLD_DESCRIPTION "Maximum_insertion_breakpoint_threshold"
#define MAX_INS_BREAKPOINT_THRESHOLD_COMMENT "# Option -w"
#define MAX_INS_BREAKPOINT_THRESHOLD_LINE 34

#define MIN_AVG_BASE_QUAL_THRESHOLD_DESCRIPTION "Minimum_average_base_quality_threshold"
#define MIN_AVG_BASE_QUAL_THRESHOLD_COMMENT "# Option -x"
#define MIN_AVG_BASE_QUAL_THRESHOLD_LINE 35

#define MAX_UNMAPPED_GAP_SR_DESCRIPTION "Maximum_unmapped_gap_or_overlap_for_split-reads"
#define MAX_UNMAPPED_GAP_SR_COMMENT "# Option -y"
#define MAX_UNMAPPED_GAP_SR_LINE 36

#define MIN_SR_MAPPED_LEN_DESCRIPTION "Minimum_split-read_mapped_length"
#define MIN_SR_MAPPED_LEN_COMMENT "# Option -z"
#define MIN_SR_MAPPED_LEN_LINE 37

#define IGNORE_CONTIG_CASE_DIFF_DESCRIPTION "Ignore_case_difference_in_contig_names"
#define IGNORE_CONTIG_CASE_DIFF_COMMENT "# (0-OFF,1-ON) Option -I"
#define IGNORE_CONTIG_CASE_DIFF_LINE 38

#define SAMPLE_ID_VCF_DESCRIPTION "Sample_ID_to_be_used_in_VCF_header"
#define SAMPLE_ID_VCF_COMMENT "# Option -O"
#define SAMPLE_ID_VCF_LINE 39

#define TAB_TXT_OUTPUT_DESCRIPTION "Tabular_text_output"
#define TAB_TXT_OUTPUT_COMMENT "# (0-OFF,1-ON) Option -f"
#define TAB_TXT_OUTPUT_LINE 40

#define CONFIG_NOT_FOUND_MESSAGE "Config file does not exist. Writing config file. Please rerun GROM."


int verbose = 1;

struct config
{
   char verbose[MAXBUF];
   char input_filename[MAXBUF]; 
   char ref_filename[MAXBUF];  
   char results_filename[MAXBUF];
   char duplicate_filtering_on[MAXBUF];
   char gender[MAXBUF];
   char ploidy[MAXBUF];
   char processes[MAXBUF];
   char min_base_phred_qual[MAXBUF];
   char maq_qual_threshold[MAXBUF];
   char prob_threshold[MAXBUF];
   char prob_threshold_ins[MAXBUF];
   char prob_threshold_cnv[MAXBUF];   
   char win_samp_factor[MAXBUF];
   char max_chr_len[MAXBUF];
   char min_chr_len[MAXBUF];
   char min_rep[MAXBUF];
   char min_rep_stdev[MAXBUF];
   char rank[MAXBUF];
   char dup_cov_threshold[MAXBUF];
   char turn_off_split_read[MAXBUF];   
   char exc_cov_threshold[MAXBUF];
   char win_min_size[MAXBUF];
   char win_max_size[MAXBUF];
   char min_num_blocks[MAXBUF];
   char block_size[MAXBUF];  
   char min_snv_ratio[MAXBUF];
   char min_reads_supp_breakpoint[MAXBUF];   
   char min_sv_ratio[MAXBUF];
   char max_hp_threshold[MAXBUF];
   char min_indel_ratio[MAXBUF];
   char min_snv_reads_supp_var[MAXBUF];
   char min_sd_discordance[MAXBUF];
   char max_evidence_ratio[MAXBUF];
   char max_ins_breakpoint_thr[MAXBUF];
   char min_avg_base_qual_thr[MAXBUF];
   char max_unmapped_gap_sr[MAXBUF];
   char min_sr_mapped_len[MAXBUF];
   char ignore_contig_case_diff[MAXBUF];
   char sample_id_vcf[MAXBUF];
   char tab_txt_output[MAXBUF];
};

char *configdir = NULL;
char configname[16];
char config_file_path[32];
char config_contents[MAXBUF*5];

static int ccount = 0;

static int ix = 0;

#ifdef DO_TIMING
static double timers[11+2*max_chr_names];
#endif
static long basetime = 0;

double gettime()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (basetime == 0) basetime = tv.tv_sec;
	return tv.tv_usec * 1e-6 + (tv.tv_sec - basetime);
}

#define MAX_REGION 300000000
#define IO_REGION  1000000
#define NUM_REGIONS (MAX_REGION/IO_REGION+1)
#define IO_BUFFER  (10*1000000)
#define MAX_READS  50000
#define CHROMOSOMES_IN_PARALLEL 256


int g_sub_region_size = MAX_REGION;
int g_sub_region_overlap = 10000;
int g_sub_region_start = 0;
int g_sub_region_end = MAX_REGION;


typedef struct single_io {
    bamFile bam_file;
    bam_header_t *bam_hdr;
    bam_index_t *bam_idx;
    int range_id;
    int range_bottom;
    int range_top;
    unsigned int read_in;
    unsigned int read_out;
    unsigned int read_max;
    unsigned int reads[MAX_READS];
    unsigned int len[MAX_READS];
    unsigned int buffer_in;
    unsigned int buffer_out;
    unsigned int buffer_max;
    char buffer[IO_BUFFER];
    volatile int completed;
    volatile int ignore_all;
    volatile int done_reading;
    volatile int end_of_file;
} single_io_t;


int init_reads(single_io_t *s)
{
    s->read_in = s->read_out = 0;
    s->read_max = MAX_READS;
    s->buffer_in = s->buffer_out = 0;
    s->buffer_max = IO_BUFFER;
    s->completed = 0;
    s->done_reading = 0;
    return 0;
}

int single_free_reads(single_io_t *s)
{
    if (s->read_in < s->read_out)
	return s->read_out - s->read_in;
    return MAX_READS - (s->read_in - s->read_out);
}


int insert_read(single_io_t *s, const bam1_t *b)
{
    unsigned int size = sizeof(bam1_t);
    if (b != NULL)
	size += b->data_len;
    size = ((size + 7) / 8) * 8;

    if (single_free_reads(s) <= 2)
	return -1;

    if (s->buffer_in < s->buffer_out) {
	if (s->buffer_out - s->buffer_in <= size)
	    return -1;
    }
    else {
	if (IO_BUFFER - s->buffer_in <= size) {
	    if (s->buffer_out <= size)
		return -1;
	    s->buffer_in = 0;
	}
    }
    memcpy(s->buffer + s->buffer_in, b, sizeof(bam1_t));
    if (b->data && b->data_len)
	memcpy(s->buffer + s->buffer_in + sizeof(bam1_t), b->data, b->data_len);
    s->reads[s->read_in] = s->buffer_in;
    s->len[s->read_in] = size;
    s->buffer_in += size;
    if ( s->read_in+1 >= MAX_READS)
	s->read_in = 0;
    else
	s->read_in++;
    return 0;
}


int get_read(single_io_t *s, bam1_t *b)
{
    if (b == NULL)
	return -1;

    unsigned int len = b->m_data;
    uint8_t *data = b->data;

    if (s->read_out == s->read_in)
	return -1;

    memcpy(b, s->buffer + s->reads[s->read_out], sizeof(bam1_t));
    b->m_data = len;
    b->data = data;

    if (b->data_len > 0) {
	if (data == NULL) {
	    len = ((b->data_len + 31) / 32) * 32;
	    b->data = malloc(len);
	    b->m_data = len;
	}
	else if (b->data_len > b->m_data) {
	    len = ((b->data_len + 31) / 32) * 32;
	    b->data = realloc(b->data, len);
	    b->m_data = len;
	}
	memcpy(b->data, s->buffer + s->reads[s->read_out] + sizeof(bam1_t), b->data_len);
    }
    s->buffer_out = s->reads[s->read_out] + s->len[s->read_out];
    if ( s->read_out+1 >= MAX_READS)
	s->read_out = 0;
    else
	s->read_out++;
    return 0;
}


int my_back_reader(const bam1_t *b, void *data)
{
    single_io_t *s = (single_io_t *)data;

    while (!s->ignore_all) {
	if (insert_read(s, b) >= 0)
	    break;

    }
    return 0;
}


int my_bam_open(single_io_t *s, char *bam_file_name)
{
    s->bam_file = bam_open(bam_file_name, "rb");
    if (s->bam_file == NULL)
	return -1;
    s->bam_hdr = bam_header_read(s->bam_file);
    s->bam_idx = bam_index_load(bam_file_name);

    s->ignore_all = 0;
    s->end_of_file = 0;
    init_reads(s);
    return 0;
}


int my_bam_close(single_io_t *s)
{
    if (s->bam_idx) {
	bam_index_destroy(s->bam_idx);
	s->bam_idx = NULL;
    }
    if (s->bam_hdr) {
	bam_header_destroy(s->bam_hdr);
	s->bam_hdr = NULL;
    }
    if (s->bam_file) {
	bam_close(s->bam_file);
	s->bam_file = NULL;
    }
    init_reads(s);
    return 0;
}


int my_engage_read(single_io_t *s, int cid, int bottom, int top)
{
    if (cid < 0 || cid >= s->bam_hdr->n_targets)
	return -1;

    init_reads(s);
    s->range_id     = cid;
    s->range_bottom = bottom;
    s->range_top    = top;
    bam_fetch(s->bam_file, s->bam_idx, cid, bottom, top, s, my_back_reader);
    s->completed = 1;
    return 0;
}


int my_bam_read(single_io_t *s, bam1_t *b)
{
    int rc;

    if (s == NULL || (s->end_of_file && s->read_out == s->read_in) )
	return -1;

    while (s->done_reading == 1 || (rc = get_read(s,b)) < 0) {
	if (s->completed == 1 && s->read_out == s->read_in && s->done_reading == 0) {
	    s->completed = 2;
	    s->done_reading = 1;
	}
	if (s->end_of_file)
	    return -1;

    }
    return 0;
}


int my_before_reading(single_io_t *s)
{
    init_reads(s);
    s->ignore_all = 0;
    s->end_of_file = 0;
    return 0;
}


int my_stop_reading(single_io_t *s)
{
    s->ignore_all = 1;
    return 0;
}


int single_chromosome_read(single_io_t *s, int cid)
{
    s->end_of_file = 0;
#ifdef READ_IN_CHUNKS
    int i = g_sub_region_start;
    while (i < g_sub_region_end) {
	int size = (i + IO_REGION >= g_sub_region_end) ? g_sub_region_end - i : IO_REGION;
	my_engage_read(s, cid, i, i + size - 1);
	i += size;
	while (s->done_reading == 0) {

	}
	if (s->ignore_all)
	    break;
    }
#else
    my_engage_read(s, cid, g_sub_region_start, g_sub_region_end-1);
#endif
    s->end_of_file = 1;
    return 0;
}


single_io_t g_single;
int g_parallel_mode = 0;
int g_one_chromosome = -1;
int g_sub_child_number = 0;
int g_save_argc = 0;
char **g_save_argv = NULL;

#define CHILD_UNFINISHED (-1)
#define MAX_SUB_PROCESSES 10000

int g_child_max = 0;
int *g_slots_used = NULL;
int *g_child_list = NULL;
int *g_child_done = NULL;
int *g_child_count = NULL;

int *g_subchild_slot = NULL;
int *g_subchild_list = NULL;
int *g_subchild_status = NULL;

int g_pid_list_index = 0;

int g_cpulist[CHROMOSOMES_IN_PARALLEL*2];

void cpus_print_error(char *reason, char *envp, char *errp)
{
  printf("Error in AFFINITY: %s in GROM_AFFINITY\n",reason);
  printf("         AFFINITY: %s\n",envp);
  printf("         AFFINITY:%*c^\n", (int)(errp - envp)+1, ' ');
  exit(1);
}

int get_number_of_cpus(void)
{
    int c;

#ifdef _SC_NPROCESSORS_ONLN
    c = sysconf(_SC_NPROCESSORS_ONLN);
    if (c < 0)
	c = sizeof(g_cpulist)/sizeof(int);
#else
    c = sizeof(g_cpulist)/sizeof(int);
#endif
    return c;
}


int get_affinity_map(void)
{
  int num_cpus = get_number_of_cpus();
  int cpu_count = 0;
  int first_cpu;
  int last_cpu;
  int stride;
  int count;
  int max = sizeof(g_cpulist)/sizeof(int);
  char *envp, *strp, *endp;

  for (count = 0; count < max; count++)
      g_cpulist[count] = count % num_cpus;

  if (NULL == (envp = strp = getenv("GROM_AFFINITY")))
    return 0;

  do {
    stride = 1;
    first_cpu = last_cpu = strtol(strp, &endp, 0);
    if (strp != endp && first_cpu >= 0 && first_cpu < num_cpus) {
      strp = endp;
      if ('-' == *strp) {
        last_cpu = strtol(++strp, &endp, 0);
        if (strp != endp && last_cpu >= 0 && last_cpu < num_cpus) {
          strp = endp;
          if (last_cpu < first_cpu)
            stride = -1;
        }
        else {
          cpus_print_error("invalid CPU number", envp, strp);
          return 0;
        }
      }
      count = (last_cpu - first_cpu) / stride + 1;
      while (cpu_count < max && count--) {
        g_cpulist[cpu_count++] = first_cpu;
        first_cpu += stride;
      }
      while (' ' == *strp || '\t' == *strp || ',' == *strp)
        strp++;
    }
    else {
      cpus_print_error("invalid CPU number", envp, strp);
      return 0;
    }
  } while ('\0' != *strp);

  for (count = cpu_count; count < max; count++)
      g_cpulist[count] = g_cpulist[count % cpu_count];

  return 1;
}


void allcate_children(int max)
{
    int i;

    g_child_max = max;
    g_slots_used = calloc(g_parallel_mode, sizeof(int));
    g_child_list = calloc(g_child_max, sizeof(int));
    g_child_done = calloc(g_child_max, sizeof(int));
    g_child_count = calloc(g_child_max, sizeof(int));
    g_subchild_slot = calloc(MAX_SUB_PROCESSES, sizeof(int));
    g_subchild_list = calloc(MAX_SUB_PROCESSES, sizeof(int));
    g_subchild_status = calloc(MAX_SUB_PROCESSES, sizeof(int));

    if (g_slots_used == NULL || g_child_list == NULL || g_child_done == NULL || g_child_count == NULL || g_subchild_slot == NULL || g_subchild_list == NULL ||g_subchild_status == NULL) {
	printf("Error: Unable to allocate memory for children list.\n");
	exit(1);
    }
    for (i = 0; i < MAX_SUB_PROCESSES; i++)
	g_subchild_status[i] = CHILD_UNFINISHED;

    get_affinity_map();
}


void check_parallel_slots(void)
{
    int i, j;
    int pid;
    int status;
    int count = g_parallel_mode;

    while (count >= g_parallel_mode) {
	count = 0;
	for (i = 0; i < g_child_max; i++) {
	    for (j = 0; j < g_child_count[i]; j++) {
		if (g_subchild_list[g_child_list[i]+j] != 0 && g_subchild_status[g_child_list[i]+j] == CHILD_UNFINISHED)
		    count++;
	    }
	}
	if (count >= g_parallel_mode) {
	    pid = wait(&status);
	    if (pid > 0) {
		for (i = 0; i < g_child_max; i++) {
		    for (j = 0; j < g_child_count[i]; j++) {
			if (pid == g_subchild_list[g_child_list[i]+j]) {
			    g_subchild_status[g_child_list[i]+j] = status;
			    g_slots_used[g_subchild_slot[g_child_list[i]+j]] = 0;
			    printf("Found completed child %d at %d/%d. clearing slot %d\n", pid, i, j, g_subchild_slot[g_child_list[i]+j]);
			    pid = 0;
			    break;
			}
		    }
		    if (pid == 0)
			break;
		}
	    }
	}
    }
}


int get_free_slot(void)
{
    int i;

    while ( 1 ) {
	for (i = 0; i < g_parallel_mode; i++) {
	    if (g_slots_used[i] == 0) {
		return i;
	    }
	}
	check_parallel_slots();
    }
}


void save_new_child(int index, int pid, int slot)
{
    int i;
    if (g_child_count[index] == 0)
	g_child_list[index] = g_pid_list_index;
    i = g_child_list[index]+g_child_count[index];
    g_subchild_list[i] = pid;
    g_subchild_slot[i] = slot;
    g_slots_used[slot] = pid;
    g_child_count[index]++;
    g_pid_list_index++;
}


int wait_for_child(int index)
{
    int i;
    int status = 0;

    for (i = 0; i < g_child_count[index]; i++) {
	if (g_subchild_status[g_child_list[index]+i] == CHILD_UNFINISHED) {
	    waitpid(g_subchild_list[g_child_list[index]+i], &g_subchild_status[g_child_list[index]+i], 0);
	    g_slots_used[g_subchild_slot[g_child_list[index]+i]] = 0;
	}
    }
    for (i = 0; i < g_child_count[index]; i++) {
	status = g_subchild_status[g_child_list[index]+i];
	if (WEXITSTATUS(status) > 0)
	    return status;
    }
    return status;
}


int launch_one_chromosome(int chr, char *chr_name, long size)
{
    int i, rc, pid, slot;
    int subchild = 0;
    int region_start = 0;
    int region_end;
    char my_chr[100];

    while (size > 0) {
	region_start = subchild * g_sub_region_size;
	if (size > g_sub_region_size / 4 * 5) {
	    region_end = (subchild + 1) * g_sub_region_size + g_sub_region_overlap;
	    size -= g_sub_region_size;
	} else {
	    region_end = region_start + size;
	    size = 0;
	}
	sprintf(my_chr, "%d,%d,%d,%d", chr, subchild, region_start, region_end);
	slot = get_free_slot();
	pid = fork();
	if (pid == 0) {

	    char placement[100];
	    char **new_argv = calloc(g_save_argc + 1, sizeof(char *));
	    sprintf(placement,"GOMP_CPU_AFFINITY=%d,%d", g_cpulist[slot*2], g_cpulist[slot*2+1]);
	    putenv(placement);
	    putenv("OMP_NUM_THREADS=2");
	    for (i = 0; i < g_save_argc; i++) {
		new_argv[i] = g_save_argv[i];
		if (strcmp(g_save_argv[i], "-P") == 0) {
		    new_argv[i] = "-c";
		    new_argv[++i] = my_chr;
		}
	    }
	    printf("Launching background GROM for single chromosome '%s' (%s) on CPUs %d,%d\n", chr_name, my_chr, g_cpulist[slot*2], g_cpulist[slot*2+1]);
	    rc = execv(g_save_argv[0], new_argv);
	    if (rc == -1) {
		printf("Unable to execute GROM for single chromosome '%s' (%s)\n", chr_name, my_chr);
		exit(1);
	    }
	}
	else if (pid == -1) {
	    printf("Error lanuching GROM for single chromosome '%s' (%s)\n", chr_name, my_chr);
	    exit(1);
	}
	save_new_child(chr, pid, slot);
	subchild++;
	printf("*** %d Lanuched '%s' (%s) on slot %d\n", pid, chr_name, my_chr, slot);
    }
    return pid;
}


void add_one_chromosome_result(int chr, char *chr_name, char *results_name, long size)
{
    int i, rc, status;
    char command[4096];
    char ctx_string[4] = "ctx";

    if (g_child_count[chr] == 0)
	return;

    status = wait_for_child(chr);
    rc = WEXITSTATUS(status);
    if (rc > 0) {
	printf("Error: Background process for processing chromosome '%s' failed to complete : %08x\n", chr_name, status);
	exit(1);
    }
    for (i=0; i < g_child_count[chr]; i++) {
	sprintf(command, "cat %s.%s-%d >>%s ; rm -f %s.%s-%d", results_name, chr_name, i, results_name, results_name, chr_name, i);
	system(command);
	sprintf(command, "cat %s.%s-%d.%s >>%s.%s ; rm -f %s.%s-%d.%s", results_name, chr_name, i, ctx_string, results_name, ctx_string, results_name, chr_name, i, ctx_string);
	system(command);
    }
}


#define file_name_len 1024
#define max_trials 1000

#define max_block_list_len 10000
#define max_lowvar_block_list_len 10001
#define max_chr_name_len 50

#define max_tumor_sv_list_len 100000

#define aux_str_len 100

#define DEL 0
#define DUP 1
#define INS 2
#define INV 3
#define INDEL_INS 4
#define INDEL_DEL 5
#define CTX_F 6
#define CTX_R 7
#define INV_F 8
#define INV_R 9
#define SNV 10
#define CTX_FF 11
#define CTX_FR 12
#define CTX_RF 13
#define CTX_RR 14

#define BAM_CINS_AUX 'I'
#define BAM_CDEL_AUX 'D'
#define BAM_CSOFT_CLIP_AUX 'S'

#define OTHER_EMPTY 0
#define OTHER_DEL_F 1
#define OTHER_DEL_R 2
#define OTHER_DUP_F 3
#define OTHER_DUP_R 4
#define OTHER_INV_F1 5
#define OTHER_INV_R1 6
#define OTHER_INV_F2 7
#define OTHER_INV_R2 8
#define OTHER_CTX_F 9
#define OTHER_CTX_R 10
#define OTHER_INDEL_I 11
#define OTHER_INDEL_D_F 12
#define OTHER_INDEL_D_R 13

#define DNA_A 0
#define DNA_C 1
#define DNA_G 2
#define DNA_T 3

int g_other_types_len = 14;
char g_other_types[14][20] = {"EMPTY", "DEL FOR", "DEL REV", "DUP FOR", "DUP REV", "INV FOR START", "INV REV START", "INV FOR END", "INV REV END", "CTX FOR", "CTX REV", "INS INDEL", "DEL INDEL FOR", "DEL INDEL REV"};
char *g_version_name = "GROM, Version 1.0.4\n";

int cmpfunc_2dm (const void * a, const void * b);
int cmpfunc_2d (const void * a, const void * b);
int cmpfunc (const void * a, const void * b);

int compare_chr_pos(const void *a, const void *b);
void print_help();
int find_insert_mean(samfile_t *fim_bam_file, int *fim_gc_lseq, int *fim_insert_min_size, int *fim_insert_max_size);

long grom_rand(long gr_max);
long bisect_left(int *bl_list, int bl_rd, long bl_start, long bl_end);
long bisect_right(int *br_list, int br_rd, long br_start, long br_end);
long bisect_left_double(double *bld_list, double bld_prob, long bld_start, long bld_end);
long bisect_right_double(double *brd_list, double brd_prob, long brd_start, long brd_end);

int bisect_list(int *bl_list, int bl_pos, int bl_start, int bl_end, int bl_type);
void detect_del_dup(int ddd_chr, int ddd_begin, long ddd_chr_fasta_len, int *ddd_one_base_rd_gc_weighted, int *ddd_one_base_rd_acgt_weighted, int *ddd_rd_mq_list, int *ddd_rd_rd_list, int *ddd_rd_low_mq_rd_list, int **ddd_sample_high_mq_rd_list, int **ddd_sample_low_mq_rd_list, int **ddd_sample_repeat_rd_list, long *ddd_low_mq_index, long *ddd_high_mq_index, long *ddd_low_mq_index_all, long *ddd_high_mq_index_all, double *ddd_pval2sd_pval_list, double *ddd_pval2sd_sd_list, int ddd_pval2sd_list_len, long *ddd_del_list_index, int *ddd_del_list_ref, long *ddd_del_list_start, long *ddd_del_list_end, double *ddd_del_list_stdev, double *ddd_del_list_cn, double *ddd_del_list_cn_stdev, long *ddd_dup_list_index, int *ddd_dup_list_ref, long *ddd_dup_list_start, long *ddd_dup_list_end, double *ddd_dup_list_stdev, double *ddd_dup_list_cn, double *ddd_dup_list_cn_stdev, int ddd_ploidy, int *ddd_repeat_type_list, long *ddd_repeat_start_list, long *ddd_repeat_end_list, long ddd_repeat_index, char *ddd_results_file_name, char *ddd_chr_name);

int delete_files();
void write_config_file(char *filename);
struct config get_config(char *filename);
void check_config_descriptor(char *entry, char *default_descr);
void check_if_boolean(char *entry, char *default_descr);
int valid_integer_entry(char *entry);

int g_tumor_indel_len_divisor = 10;
int g_vcf_spr_index[6] = {0, 0, 0, 0, 10, 10};
int g_vcf_epr_index[6] = {0, 0, 0, 0, 10, 11};
int g_vcf_sev_index[6] = {0, 0, 0, 0, 11, 12};
int g_vcf_eev_index[6] = {0, 0, 0, 0, 11, 13};
int g_vcf_srd_index[6] = {0, 0, 0, 0, 12, 14};
int g_vcf_erd_index[6] = {0, 0, 0, 0, 12, 15};
int g_vcf_sco_index[6] = {0, 0, 0, 0, 13, 16};
int g_vcf_eco_index[6] = {0, 0, 0, 0, 14, 17};
int g_vcf_sot_index[6] = {0, 0, 0, 0, 15, 18};
int g_vcf_eot_index[6] = {0, 0, 0, 0, 16, 19};
int g_vcf_ssc_index[6] = {0, 0, 0, 0, 17, 20};
int g_vcf_esc_index[6] = {0, 0, 0, 0, 17, 21};
int g_vcf_hp_index[6] = {0, 0, 0, 0, 18, 22};

int g_max_tumor_str_list_len = 1000;
int g_max_tumor_str_len = 10000;
char g_dna_upper_str[4][2] = {"A", "C", "G", "T"};
char g_dna_lower_str[4][2] = {"a", "c", "g", "t"};
int g_vcf_chr_index = 0;
int g_vcf_pos_index = 1;
int g_vcf_ref_index = 3;
int g_vcf_alt_index = 4;
int g_vcf_qual_index = 5;
int g_vcf_filter_index = 6;
int g_vcf_info_index = 7;
int g_vcf_format_index = 8;
int g_vcf_sample_index = 9;
int g_vcf_sample_pr_index = 11;
int g_vcf_sample_af_index = 12;
int g_vcf_sample_snv_index = 13;
int g_vcf_sample_snv_lowmq_index = 17;
int g_vcf_sample_bq_index = 21;
int g_vcf_sample_mq_index = 22;
int g_vcf_sample_pir_index = 23;
int g_vcf_sample_fs_index = 24;

int g_vcf_sample_ol_index = 25;

int g_vcf_sample_in_index = 26;

int g_vcf_sample_sc_index = 27;

int g_do_rd = 0;

int g_normal = 0;


char *g_chrx = "chrx";
char *g_x = "x";
int g_chrx_len = 4;
int g_x_len = 1;

int g_rd_max_mapq = 60;
double g_mapq_factor = 0.5;

double g_rd_pval_threshold = 0.000000001;

long g_sample_lists_len = 100000;

long g_genome_reduction_factor = 1;
long g_windows_sampling_factor = 2;

int g_most_biased_repeat = -1;
long g_most_biased_repeat_count = 0;
long g_dup_threshold_factor = 2;

int g_repeat_segments = 10;

long g_min_repeat = 20;

double g_min_repeat_stdev = 1.5;


int g_chr_rd_threshold_factor = 2;
long g_block_factor = 4;
long g_min_blocks = 4;
long g_block_unit_size = 10000;

long g_min_n_size = 100;




long g_min_dist_from_contig_border = 10000;


char g_blocks_line[100];
char g_block_separator[2] = "\t";
char *g_block_chr, *g_block_start_str, *g_block_end_str;
long g_block_index = 0;
long g_lowvar_block_index = 0;
long g_lowvar_block_sample_index = 0;
long g_block_start, g_block_end;
long g_block_min = 10000;
long g_max_block_list_len = max_block_list_len;
long g_block_chr_list[max_block_list_len], g_block_start_list[max_block_list_len], g_block_end_list[max_block_list_len];
long g_lowvar_block_chr_list[max_lowvar_block_list_len], g_lowvar_block_start_list[max_lowvar_block_list_len], g_lowvar_block_end_list[max_lowvar_block_list_len];
long g_lowvar_block_sample_start_list[max_lowvar_block_list_len], g_lowvar_block_sample_end_list[max_lowvar_block_list_len];


char *g_hez_prob_binom_cdf_file_base = "GROM_hez_binom_table_";
char *g_prob_binom_cdf_file_base = "GROM_binom_table_";
char *g_prob2_binom_cdf_file_base = "GROM_binom_table2_";
char *g_mq_prob_binom_cdf_file_base = "GROM_mq_binom_table_";
char *g_mq_prob_half_binom_cdf_file_base = "GROM_mq_half_binom_table_";
char g_hez_prob_binom_cdf_file[file_name_len];
char g_prob_binom_cdf_file[file_name_len];
char g_prob2_binom_cdf_file[file_name_len];
char g_mq_prob_binom_cdf_file[file_name_len];
char g_mq_prob_half_binom_cdf_file[file_name_len];

char *g_binom_file_end = ".txt";
char *g_binom_separator = "\t";


double g_hez_prob_binom_cdf_table[max_trials+1][max_trials+1];
double g_mq_prob_binom_cdf_table[max_trials+1][max_trials+1];


int g_min_mapq = 20;

double g_insert_num_st_devs = 3;
int g_sc_min = 1;
int g_min_mapq_sr = 20;
int g_min_disc = 3;
int g_sc_range = 35;
int g_sc_effect_range = 50;

int g_vcf = 1;

int contig_lower_case = 0;

char *sample_id = NULL;
char *ctx_sample_id = NULL;
int sample_id_cmd_arg_present = 0;

char *results_file_name = NULL;
char *results_file_name1 = NULL;
char *results_file_name_ctx = NULL;
char fds_results_file_name_bnd[1024];

char infinity_qual_entry[10];
char unknown_gt_entry[10];
char heterozygous_gt_entry[10];
char homozygous_gt_entry[10];

double indel_del_homozygous_cutoff = 0.9;
double indel_del_unknown_cutoff = 0.2;
double indel_ins_homozygous_cutoff = 0.9;
double indel_ins_unknown_cutoff = 0.2;

double ins_homozygous_p_value_cutoff = 1.0E-10;
double ins_unknown_p_value_cutoff = 1.0E-6;

double del_homozygous_p_value_cutoff = 1.0E-12;

double g_min_overlap_ratio = 0.5;
int g_internal = 0;
int g_troubleshooting = 0;

double sqrt_2;

#ifdef DO_TIMING
long long timers_ss[300];
#endif

int g_lseq;
int g_insert_min_size, g_insert_max_size;
int g_insert_max_mult = 5;
double g_range_mult = 0.75;
int g_sv_list_len = 1000000;
int g_sv_list2_len = 100000;
int g_sv_list_len_over = 0;
int g_sv_list2_len_over = 0;
int g_overlap_mult = 1;


int g_other_len = 50;

int g_max_trials = max_trials;


int g_max_combinations = 1000;
int g_other_len_sampling = 10000;

int g_splitread = 1;
int g_rmdup = 0;
int g_rmdup_list_len = 10000;

double g_p, g_a1, g_a2, g_a3, g_a4, g_a5, g_xc, g_t, g_erf, g_prob2, g_prob;
double g_mq_prob, g_mq_prob_half;

int g_one_base_rd_len, g_half_one_base_rd_len, g_14_one_base_rd_len, g_34_one_base_rd_len;

int g_tumor_sv = 0;
int g_tumor_sv_type_max_name_len = 10;
char *g_tumor_separator = "\t";
char *g_aux_separator = ",";

int g_sv_types_len = 13;
char g_sv_types[13][10] = {"DEL", "DUP", "INS", "INV", "INDEL_INS", "INDEL_DEL", "CTX_F", "CTX_R", "INV_F", "INV_R", "SNV", "DEL RD", "DUP RD"};  

int g_normal_sc_range = 3;


int g_max_chr_names = max_chr_names;
int g_max_chr_name_len = max_chr_name_len;
char g_chr_names[max_chr_names][max_chr_name_len];
int g_chr_names_len[max_chr_names];
int g_chr_analyzed[max_chr_names];
int g_parallel_fasta_chr_index[max_chr_names];
long g_min_chr_fasta_len = 1000000;
long g_fasta_file_position[max_chr_names];
long g_chr_len[max_chr_names];
int g_chr_names_index = 0;
int g_chr_analyzed_index = 0;

int g_snv_indel_range = 20;
int g_read_name_len = 50;
int g_max_seq_len = 1000;
int g_nucleotides = 4;
char g_dna[4] = "ACGT";
int g_min_snv = 3;
int g_min_base_qual = 20;
double g_min_snv_ratio = 0.2;

int g_max_homopolymer = 10;
int g_max_ins_range = 10;
double g_max_evidence_ratio = 0.25;
double g_min_sv_ratio = 0.05;
double g_min_indel_ratio = 0.125;
double g_min_ave_bq = 15;
int g_indel_i_seq_len = 50;
double g_snv_rd_min_factor = 1.75;
double g_high_cov_min_snv_ratio = 0.4;
double g_max_inv_rd_diff = 1.75;


char *g_chry = "chry";
char *g_y = "y";
int g_chry_len = 4;
int g_y_len = 1;

int insert_sample_size = 10000000;

int g_gender = 0;
int g_ploidy = 2;
int gc_lseq;
int g_max_rd_over_ave = 5;
int g_rd_min_mapq = 20;
int g_ranks_stdev = 1;
int g_insert_min_acgt = 99;
int g_rd_min_windows = 20;
int g_rd_no_combine_min_windows = 100;
long g_min_rd_window_len = 100;
long g_max_rd_window_len = 10000;
long g_one_base_read_depth_min_rd_low_stdev = 3;
long g_max_distance_since_last_del_good;
double g_max_rd_low_acgt_or_windows = 2;
double g_ploidy_threshold_numerator = 0.6;
double g_stdev_step = 0.01;

double g_pval_threshold1 = 0.01;
double g_pval_threshold = 0.001;
double g_pval_insertion1 = 0.01;
double g_pval_insertion = 0.0000000001;
long g_max_chr_fasta_len = 300000000;
long g_num_gc_bins = 101;
long g_mapped_reads = 0;

long g_mappable_genome_length = 0;


char *temp_chr_fasta = NULL;

int g_chr_name_start = 4;

int g_insert_mean;
long g_one_base_window_size, g_one_base_window_size_total;

int g_indel_d_dist_range = 5.0;

int g_max_split_loss = 20;
int g_min_sr_len = 30;

int min_dup_inv_pair_distance = 10;


typedef struct vcf_chr_pos_data {
	char *chr_name;
	int pos;
	char vcf_data[300];
} vcf_chr_pos_data_t;

int my_samread(samfile_t *fp, bam1_t *b)
{
    int rc;
    if (g_parallel_mode > 0) {
	rc = my_bam_read(&g_single, b);
	if (rc == 0) rc = 1;
    }
    else {
	rc = samread(fp, b);
    }
    return rc;
}

int save_insert_mean(char *bam_file_name, int insert_mean, int gc_lseq, int insert_min_size, int insert_max_size, long int mapped_reads)
{
    char name[1024];
    FILE *fp;


	strcpy(name, results_file_name);
    strcat(name, ".mean");
    fp = fopen(name,"w");
    if (fp != NULL) {
	printf("Saving insert_mean et al to %s\n",name);
	fprintf(fp,"%d %d %d %d %ld\n",insert_mean, gc_lseq, insert_min_size, insert_max_size, mapped_reads);
	fclose(fp);
	return insert_mean;
    }
    return -1;
}

int load_insert_mean(char *bam_file_name, int *insert_mean, int *gc_lseq, int *insert_min_size, int *insert_max_size, long int *mapped_reads)
{
    char name[1024];
    FILE *fp;
    int rc = 0;


	strcpy(name, results_file_name);
    strcat(name, ".mean");
    fp = fopen(name,"r");
    if (fp != NULL) {
	printf("Loading insert_mean et al from %s\n", name);
	rc = fscanf(fp,"%d %d %d %d %ld", insert_mean, gc_lseq, insert_min_size, insert_max_size, mapped_reads);
	fclose(fp);
    }
    return (rc == 5 ? *insert_mean : -1);
}


int cmpfunc_2dm (const void * a, const void * b)
{
  const int *a_2d = *(const int**)a;
  const int *b_2d = *(const int**)b;
  return ( a_2d[0] - b_2d[0] );
}


int cmpfunc_2d (const void * a, const void * b)
{
  const int *a_2d = a;
  const int *b_2d = b;
  return ( a_2d[0] - b_2d[0] );
}



int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}


int compare_chr_pos(const void *a, const void *b)
{
    const struct vcf_chr_pos_data *pa = a;
    const struct vcf_chr_pos_data *pb = b;

    int diff = strcmp(pa->chr_name, pb->chr_name);

    if (diff == 0) {
        if (pa->pos < pb->pos)
            diff = -1;
        else if (pa->pos > pb->pos)
            diff = +1;
        else
            diff = 0;
    }

    return diff;
}

inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ __volatile__ (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      : "=a" (lo), "=d" (hi)
      :
      : "%ebx", "%ecx");
    return (uint64_t)hi << 32 | lo;
}



void print_help()
{
	printf("\n%s", g_version_name);
	printf("\nUsage: GROM -i <BAM input file> -r <REFERENCE input file> -o <output file> [optional parameters]\n");
	printf("\nRequired Parameters:\n");
	printf("\t-i \tBAM input file\n");
	printf("\t-r \tREFERENCE input file (fasta)\n");
	printf("\t-o \tSTRUCTURAL VARIANT output file\n");

	printf("\nOptional Parameters:\n");
	printf("\t-P \tthreads [1]\n");
	printf("\t-M \tturn on GROM's duplicate read filtering\n");
	printf("\t-q \tmapping quality threshold [20]\n");
	printf("\t-s \tminimum standard deviations for discordance [3]\n");
	printf("\t-v \tprobability threshold [0.001]\n");
	printf("\t-g \tgender (0=female,1=male) [0]\n");
	printf("\t-d \tminimum discordant pairs [3]\n");
	printf("\t-b \tbase phred quality score threshold [35]\n");
	printf("\t-n \tminimum SNV bases [3]\n");
	printf("\t-a \tminimum SNV ratio [0.2]\n");
	printf("\t-C \tchromosome minimum length [1000000]\n");
	printf("\t-B \tchromosome maximum length [300000000]\n");
	printf("\t-y \tmaximum unmapped gap or overlap for split reads [20]\n");
	printf("\t-z \tminimum split read mapped length (each split) [30]\n");
	printf("\t-S \tturn off split-read detection\n");
	printf("\t-p \tploidy [2]\n");
	printf("\t-e \tinsertion probability threshold [0.0000000001]\n");
	printf("\t-j \tminimum SV ratio [0.05]\n");
	printf("\t-k \tmaximum homopolymer (indels) [0.05]\n");
	printf("\t-m \tminimum indel ratio [0.125]\n");
	printf("\t-u \tmaximum evidence ratio (SV except insertion) [0.25]\n");
	printf("\t-A \tsampling rate [2]\n");
	printf("\t-V \tread depth p-value threshold [0.000001]\n");
	printf("\t-W \twindow minimum size [100]\n");
	printf("\t-X \twindow maximum size [10000]\n");
	printf("\t-Y \tminimum number of blocks [4]\n");
	printf("\t-Z \tblock minimum size [10000]\n");
	printf("\t-U \texcessive coverage threshold [2]\n");
	printf("\t-D \tdinucleotide repeat minimum length [20]\n");
	printf("\t-E \tdinucleotide repeat minimum standard deviation [1.5]\n");
	printf("\t-K \tranks (0=no ranking,1=use ranks) [1]\n");
	printf("\t-L \tduplication coverage threshold [2]\n");
	printf("\t-I \tignore case difference in contigs in reference and BAM file\n");

	printf("\t-O \tsample ID. ID will be obtained from BAM file name if not provided\n");
	printf("\t-f \ttabular text output\n");

	printf("\nSee README file for additional help.\n");

	printf("\n");
}

long grom_rand(long gr_max)
{
	long gr_rand = 0;
	long gr_counter = 1;
	long gr_temp = 0;
	while( gr_counter < gr_max )
	{
		gr_temp = (rand() % 10) * gr_counter;
		while( (gr_temp + gr_rand) >= gr_max )
		{
			gr_temp = (rand() % 10) * gr_counter;
		}
		gr_rand += gr_temp;
		gr_counter = gr_counter * 10;
	}
	return gr_rand;
}


int find_insert_mean(samfile_t *fim_bam_file, int *fim_gc_lseq, int *fim_insert_min_size, int *fim_insert_max_size)
{
	bam1_t *fim_b = NULL;
	int32_t fim_pos = 0;
	uint16_t fim_flag = 0;
	int fim_insert_count = 0;
	int fim_insert_mean;
	int *fim_insert_sizes, *fim_lseq_sizes;
	fim_insert_sizes  = (int *) malloc(insert_sample_size * sizeof(int));
	fim_lseq_sizes  = (int *) malloc(insert_sample_size * sizeof(int));

	int fim_a;
	int fim_start, fim_end, fim_max_insert;
	int fim_insert_min_index, fim_insert_max_index;


	fim_b = bam_init1();

	g_mapped_reads = 0;


	while(samread(fim_bam_file, fim_b) > 0 && fim_insert_count < insert_sample_size )
	{

		fim_pos = fim_b->core.pos;
		fim_flag = fim_b->core.flag;


		if( fim_insert_count < insert_sample_size )
		{

			if( (fim_flag & BAM_FUNMAP) == 0 && (fim_flag & BAM_FDUP) == 0 && (fim_flag & BAM_FQCFAIL) == 0 )
			{
				if( (fim_flag & BAM_FPAIRED) == 0 )
				{

					fim_insert_sizes[fim_insert_count] = fim_b->core.l_qseq;
					fim_lseq_sizes[fim_insert_count] = fim_b->core.l_qseq;
					fim_insert_count += 1;
				}

				else if( (fim_flag & BAM_FMUNMAP) == 0 && fim_b->core.tid == fim_b->core.mtid )
				{

					if( fim_pos < fim_b->core.mpos && (fim_flag & BAM_FPROPER_PAIR) != 0 && fim_b->core.isize > 0 )
					{
						fim_insert_sizes[fim_insert_count] = fim_b->core.isize;
						fim_lseq_sizes[fim_insert_count] = fim_b->core.l_qseq;
						fim_insert_count += 1;
					}
				}

				if( fim_b->core.qual >= g_rd_min_mapq )
				{
					g_mapped_reads += fim_b->core.l_qseq;
				}
			}
		}
	}
	bam_destroy1(fim_b);

	qsort(fim_insert_sizes, fim_insert_count, sizeof(int), cmpfunc);
	fim_insert_mean = fim_insert_sizes[fim_insert_count/2];
	fim_max_insert = fim_insert_mean * g_insert_max_mult;

	if( g_internal == 1 )
	{
	  printf("fim_insert_mean, fim_max_insert is %d %d\n", fim_insert_mean, fim_max_insert);
	}

	fim_start = 0;
	fim_end = 0;

	for(fim_a=fim_insert_count-1;fim_a>=0;fim_a--)
	{
	  if( fim_insert_sizes[fim_a] <= fim_max_insert )
	  {
	    fim_end = fim_a;
	    break;
	  }
	}
	fim_end += 1;
	fim_insert_mean = fim_insert_sizes[fim_start + (fim_end-fim_start)/2];
	fim_insert_min_index = (int)(g_prob2 * (fim_end-fim_start)/2) + fim_start;
	fim_insert_max_index = fim_end - fim_insert_min_index;
	*fim_insert_min_size = fim_insert_sizes[fim_insert_min_index];
	*fim_insert_max_size = fim_insert_sizes[fim_insert_max_index];

	if( g_internal == 1 )
	{
	  printf("fim_max_insert is %d\n", fim_max_insert);

	  printf("fim_start, fim_end, fim_insert_min_index, fim_insert_max_index %d %d %d %d\n", fim_start, fim_end, fim_insert_min_index, fim_insert_max_index);
	}
	printf("insert_min_size, insert_max_size %d %d\n", *fim_insert_min_size, *fim_insert_max_size);
	qsort(fim_lseq_sizes, fim_insert_count, sizeof(int), cmpfunc);
	*fim_gc_lseq = fim_lseq_sizes[fim_insert_count/2];
	if( g_internal == 1 )
	{
	  printf("fim_insert_count is %d\n", fim_insert_count);
	}


	free(fim_insert_sizes);
	free(fim_lseq_sizes);

	return fim_insert_mean;
}


void find_genome_length(FILE *fgl_fasta_handle)
{

	char fgl_fasta_line[1000];
	long fgl_a_loop;
	long fgl_fasta_file_position = 0;
	long fgl_chr_len = 0;

	int fgl_fasta_name_len, fgl_fasta_while, fgl_fasta_name_alpha_len;


	while(fgets(fgl_fasta_line, sizeof(fgl_fasta_line), fgl_fasta_handle) )
	{
		if( fgl_fasta_line[0] != '>' )
		{
			for( fgl_a_loop=0;fgl_a_loop<strlen(fgl_fasta_line);fgl_a_loop++)
			{
				if( isalpha(fgl_fasta_line[fgl_a_loop]) != 0 )
				{
					if( fgl_fasta_line[fgl_a_loop] != 'N' && fgl_fasta_line[fgl_a_loop] != 'n' )
					{
						g_mappable_genome_length += 1;
					}
					fgl_chr_len += 1;
				}
			}
		}
		else
		{
			fgl_fasta_file_position = ftell(fgl_fasta_handle);

			fgl_fasta_name_len = strlen(fgl_fasta_line);
			fgl_fasta_while = fgl_fasta_name_len - 1;
			fgl_fasta_name_alpha_len = fgl_fasta_name_len;
			while( fgl_fasta_while > 0 )
			{
			  if( isgraph(fgl_fasta_line[fgl_fasta_while]) == 0 )
			  {
				fgl_fasta_name_alpha_len = fgl_fasta_while;
			  }
			  fgl_fasta_while -= 1;
			}

			if( g_internal == 1 )
			{
			  printf("%d %d\n", fgl_fasta_name_len, fgl_fasta_name_alpha_len);
			}

			if( g_chr_names_index < g_max_chr_names )
			{
				if( fgl_fasta_name_alpha_len >= g_max_chr_name_len )
				{
					fgl_fasta_name_alpha_len = g_max_chr_name_len;
				}
				for(fgl_a_loop=1;fgl_a_loop<fgl_fasta_name_alpha_len;fgl_a_loop++)
				{

					if (contig_lower_case == 1) {
						g_chr_names[g_chr_names_index][fgl_a_loop-1] = tolower(fgl_fasta_line[fgl_a_loop]);
					} else {
						g_chr_names[g_chr_names_index][fgl_a_loop-1] = fgl_fasta_line[fgl_a_loop];
					}
				}
				g_fasta_file_position[g_chr_names_index] = fgl_fasta_file_position;
				if( g_chr_names_index > 0 )
				{
					g_chr_len[g_chr_names_index-1] = fgl_chr_len;
				}
				g_chr_names_len[g_chr_names_index] = fgl_fasta_name_alpha_len - 1;
				if( g_chr_names_len[g_chr_names_index] > 0 )
				{
					g_chr_analyzed[g_chr_analyzed_index] = g_chr_names_index;
					if( g_internal == 1 )
					{
					  printf("analyzed %s\n", g_chr_names[g_chr_names_index]);
					}
					g_chr_analyzed_index += 1;
				}
			}
			else
			{
				printf("Warning: Maximum number of chromosomes (%d) exceeded.\n", g_max_chr_names);
				exit(1);
			}
			g_chr_names_index += 1;
			fgl_chr_len = 0;

		}
	}
	if( g_chr_names_index > 0 && g_chr_names_index < g_max_chr_names )
	{
		g_chr_len[g_chr_names_index-1] = fgl_chr_len;
	}


	fseek(fgl_fasta_handle, 0, SEEK_SET);


	printf("Chromosomes in reference: %d\n", g_chr_names_index);
	for(fgl_a_loop=0;fgl_a_loop<g_chr_names_index;fgl_a_loop++)
	{
		printf("%ld %ld %s\n", fgl_a_loop, g_fasta_file_position[fgl_a_loop], g_chr_names[fgl_a_loop]);
	}

}



void count_discordant_pairs(samfile_t *cdp_bam_file, char *cdp_bam_file_name, char *cdp_chr_fasta, long cdp_chr_fasta_len, char *cdp_chr_name, int cdp_chr_name_len, FILE *cdp_results_file, char *cdp_tumor_sv_file_name, int cdp_tumor_len, FILE *cdp_results_file_ctx, int **caf_sample_high_mq_rd_list, int **caf_sample_low_mq_rd_list, int **caf_sample_repeat_rd_list, long *caf_low_mq_index, long *caf_high_mq_index, long *caf_low_mq_index_all, long *caf_high_mq_index_all, double *caf_pval2sd_pval_list, double *caf_pval2sd_sd_list, int caf_pval2sd_list_len, char *caf_results_file_name, FILE *cdp_results_file_snv, FILE *cdp_results_file_ind, FILE *cdp_results_file_sv)
{
  if( cdp_chr_fasta_len > 0 && cdp_chr_fasta_len <= g_max_chr_fasta_len )
  {
    if( g_internal == 1 )
    {
      printf("cdp_chr_fasta_len is %ld\n", cdp_chr_fasta_len);
    }

#ifdef DO_TIMING
    unsigned long long start_t;
    unsigned long long end_t;
#endif

    FILE *cdp_tumor_handle;


    bam1_t *cdp_b = NULL;
    int32_t cdp_pos = 0;
    int32_t cdp_mpos = 0;
    uint16_t cdp_flag = 0;
    uint16_t cdp_mq = 0;
    int32_t cdp_chr = -1;
    int32_t cdp_mchr;
    int32_t cdp_tlen;
    int32_t cdp_lseq;


    int32_t cdp_temp_pos;

    int cdp_base_index;

    int cdp_72_loop_start, cdp_72_loop_end;

    char cdp_ctx_string[4] = "ctx";

    int cdp_snv_cn = 0;
    int cdp_snv_gt_string_len = 100;
    char cdp_snv_gt_string[cdp_snv_gt_string_len];


	int cdp_del_string_len = 50;
    char cdp_del_string[cdp_del_string_len];
	int cdp_ins_string_len = g_indel_i_seq_len;
    char cdp_ins_string[cdp_ins_string_len];


    int cdp_bl_range, cdp_bl_low_index, cdp_bl_high_index, cdp_bl_index;
    int cdp_bl_index_found = 0;
    int *cdp_bl_list;
    int cdp_bl_pos, cdp_bl_start, cdp_bl_end, cdp_bl_type;


    long cdp_snv_read_count_total = 0;
    long cdp_snv_base_total = 0;
    double cdp_snv_ave_rd = 0;
    int caf_last_snv_group_pos = 0;

    double cdp_inv_rd1_ave = 0;
    double cdp_inv_rd2_ave = 0;

    int cdp_l_aux;
    char cdp_aux_str[aux_str_len];
    char *cdp_aux_str_temp;
    char *cdp_aux_chr;
    char *cdp_aux_cigar = NULL;
    int cdp_aux_pos = -1;
    int cdp_aux_strand = 0;
    int cdp_aux_mq;

    int32_t cdp_a_int32_t;
    char cdp_seq[g_max_seq_len];

    int cdp_snv_a_loop, cdp_snv_b_loop;
    int cdp_snv_total;
    double cdp_snv_ratio[g_nucleotides];

    int cdp_read_name_found;
    char cdp_temp_snv_string[g_read_name_len];

    int cdp_a_loop, cdp_b_loop, cdp_c_loop;

    int cdp_begin = 0;
    char *cdp_target_name;
    char *cdp_test_chr = "chr";
    int cdp_chr_match = -1;
    char cdp_bam_name[g_max_chr_names];
    int cdp_bam_name_len;
    int cdp_int_a;

    int cdp_tumor_loop, cdp_tumor_loop2;
    int cdp_tumor_start2_index = 0;
    int cdp_tumor_end2_index = 0;

    int cdp_tumor_read_count, cdp_normal_read_count;
    int cdp_tumor_snv_count, cdp_total_read_count;

    int cdp_add;
    int cdp_add_factor = 6;
    int cdp_add_factor_lowmq = 0;
    int cdp_add_factor_double = 6.0;

    int cdp_max_sc_left, cdp_max_sc_right;

    int cdp_max_sc;
    int cdp_temp_other_len;

    double cdp_overlap_ratio_1, cdp_overlap_ratio_2;

#ifdef DO_OTHERLEN_STDEV
    int cdp_other_len_count;
    int cdp_other_len_total_list_len = cdp_chr_fasta_len/g_other_len_sampling + 1;
    int cdp_other_len_total_list[cdp_other_len_total_list_len];
    int cdp_other_len_stdev[10];
    for(cdp_a_loop=0;cdp_a_loop<cdp_other_len_total_list_len;cdp_a_loop++)
    {
      cdp_other_len_total_list[cdp_a_loop] = 0;
    }
    cdp_other_len_count = 0;
#endif



    FILE *caf_output_file;

    char *caf_del_text = "DEL RD";
    char *caf_dup_text = "DUP RD";

	int sort_indel_count = 0;
	int sort_sv_count = 0;

    long caf_a_loop, caf_b_loop, caf_c_loop;
    long caf_block_loop, caf_block_loop2;
    int caf_ploidy;

    time_t caf_time;

    srand((unsigned) time(&caf_time));

    long caf_chr_fasta_len = cdp_chr_fasta_len;

    int32_t caf_pos = 0;

    char caf_bam_name[g_max_chr_names];
    int caf_bam_name_len;


    char *caf_gc_chars = "CGcg";
    char *caf_at_chars = "ATat";
    char *caf_n_chars = "Nn";
    long caf_n_index = 0;
    long caf_ref_pos;
    long caf_gc_count = 0;
    long caf_acgt_count = 0;
    long caf_gc_increasing = 0;
    long caf_acgt_increasing = 0;
    long caf_gc_decreasing = 0;
    long caf_acgt_decreasing = 0;
    long caf_gc_decreasing_adj = 0;
    long caf_acgt_decreasing_adj = 0;
    int *caf_gc_count_list = (int *) malloc(caf_chr_fasta_len * sizeof(int));
    if( caf_gc_count_list == NULL )
    {
      printf("1 NULL\n");
      exit(0);
    }
    int *caf_acgt_count_list = (int *) malloc(caf_chr_fasta_len * sizeof(int));
    if( caf_acgt_count_list == NULL )
    {
      printf("2 NULL\n");
      exit(0);
    }
    int *caf_one_base_rd_gc_weighted = (int *) malloc(caf_chr_fasta_len * sizeof(int));
    if( caf_one_base_rd_gc_weighted == NULL )
    {
      printf("3 NULL\n");
      exit(0);
    }
    int *caf_one_base_rd_acgt_weighted = (int *) malloc(caf_chr_fasta_len * sizeof(int));
    if( caf_one_base_rd_acgt_weighted == NULL )
    {
      printf("4 NULL\n");
      exit(0);
    }
    long *caf_n_blocks_start = (long *) malloc((1+caf_chr_fasta_len/g_min_n_size) * sizeof(long));
    if( caf_n_blocks_start == NULL )
    {
      printf("5 NULL\n");
      exit(0);
    }
    long *caf_n_blocks_end = (long *) malloc((1+caf_chr_fasta_len/g_min_n_size) * sizeof(long));
    if( caf_n_blocks_end == NULL )
    {
      printf("6 NULL\n");
      exit(0);
    }

    char caf_repeat_chars[10][2] = {{'A','A'}, {'A','C'}, {'A','G'}, {'A','T'}, {'C','C'}, {'C','G'}, {'C','T'}, {'G','G'}, {'G','T'}, {'T','T'}};
    char caf_repeat_lower_chars[10][2] = {{'a','a'}, {'a','c'}, {'a','g'}, {'a','t'}, {'c','c'}, {'c','g'}, {'c','t'}, {'g','g'}, {'g','t'}, {'t','t'}};

	double caf_repeat_rd_average[10];

    double caf_repeat_rd_stdev[10];

    long caf_repeat_rd_type_count[10];
    int caf_repeat_types = 10;
    int caf_old_repeat_type = 10;
    int caf_new_repeat_type = 10;
    long caf_repeat_start = 0;
    long caf_repeat_end = 0;
    long caf_repeat_index = 0;
    int *caf_repeat_type_list = (int *) malloc((1+caf_chr_fasta_len/g_min_repeat) * sizeof(int));
    if( caf_repeat_type_list == NULL )
    {
      printf("7 NULL\n");
      exit(0);
    }
    long *caf_repeat_start_list = (long *) malloc((1+caf_chr_fasta_len/g_min_repeat) * sizeof(long));
    if( caf_repeat_start_list == NULL )
    {
      printf("8 NULL\n");
      exit(0);
    }
    long *caf_repeat_end_list = (long *) malloc((1+caf_chr_fasta_len/g_min_repeat) * sizeof(long));
    if( caf_repeat_end_list == NULL )
    {
      printf("9 NULL\n");
      exit(0);
    }

    long caf_last_n = 1;
    long caf_first_n_pos = 0;

    caf_n_blocks_start[caf_n_index] = 0;

    for(caf_ref_pos=g_insert_mean-1;caf_ref_pos<caf_chr_fasta_len-g_one_base_window_size;caf_ref_pos++)
    {

      if( caf_last_n > 0 )
      {
	if( strchr(caf_n_chars, cdp_chr_fasta[caf_ref_pos]) == NULL )
	{
	  if( caf_last_n >= g_min_n_size )
	  {
	    if( caf_n_index == 0 && caf_n_blocks_start[caf_n_index] == 0 )
	    {
	      caf_n_blocks_start[caf_n_index] = caf_ref_pos;
	    }
	    else
	    {
	      caf_n_blocks_end[caf_n_index] = caf_first_n_pos;
	      caf_n_index += 1;
	      caf_n_blocks_start[caf_n_index] = caf_ref_pos;
	    }
	    if( caf_n_blocks_start[caf_n_index] >= caf_chr_fasta_len )
	    {
	      caf_n_blocks_start[caf_n_index] = caf_chr_fasta_len - 1;
	    }
	  }
	  caf_last_n = 0;
	}
	else
	{
	  caf_last_n += 1;
	}
      }
      else if( caf_last_n == 0 )
      {
	if( strchr(caf_n_chars, cdp_chr_fasta[caf_ref_pos]) != NULL )
	{
	  caf_last_n = 1;
	  caf_first_n_pos = caf_ref_pos;
	}
      }



      caf_new_repeat_type = 10;
      for(caf_a_loop=0;caf_a_loop<10;caf_a_loop++)
      {
	if( (caf_repeat_chars[caf_a_loop][0] == cdp_chr_fasta[caf_ref_pos] && caf_repeat_chars[caf_a_loop][1] == cdp_chr_fasta[caf_ref_pos+1]) || (caf_repeat_chars[caf_a_loop][1] == cdp_chr_fasta[caf_ref_pos] && caf_repeat_chars[caf_a_loop][0] == cdp_chr_fasta[caf_ref_pos+1]) || (caf_repeat_lower_chars[caf_a_loop][0] == cdp_chr_fasta[caf_ref_pos] && caf_repeat_lower_chars[caf_a_loop][1] == cdp_chr_fasta[caf_ref_pos+1]) || (caf_repeat_lower_chars[caf_a_loop][1] == cdp_chr_fasta[caf_ref_pos] && caf_repeat_lower_chars[caf_a_loop][0] == cdp_chr_fasta[caf_ref_pos+1]) )
	{
	  caf_new_repeat_type = caf_a_loop;
	  break;
	}
      }

      if( caf_new_repeat_type != caf_old_repeat_type || caf_new_repeat_type == 10 )
      {
	if( caf_repeat_end > 0 )
	{
	  if( caf_repeat_end - caf_repeat_start >= (g_min_repeat-1) )
	  {
	    caf_repeat_start_list[caf_repeat_index] = caf_repeat_start;
	    caf_repeat_end_list[caf_repeat_index] = caf_repeat_end + 1;
	    caf_repeat_type_list[caf_repeat_index] = caf_old_repeat_type;
	    caf_repeat_index += 1;
	  }
	}
	if( caf_new_repeat_type == 10 )
	{
	  caf_repeat_start = 0;
	  caf_repeat_end = 0;
	}
	else
	{
	  caf_repeat_start = caf_ref_pos;
	  caf_repeat_end = caf_ref_pos;
	}
      }
      else
      {
	caf_repeat_end = caf_ref_pos;
      }
      caf_old_repeat_type = caf_new_repeat_type;

      if( caf_ref_pos == (g_insert_mean - 1) )
      {
	for( caf_a_loop=0;caf_a_loop<g_insert_mean;caf_a_loop++)
	{
	  if( strchr(caf_gc_chars, cdp_chr_fasta[caf_a_loop]) != NULL )
	  {
	    caf_gc_count_list[caf_a_loop] = 1;
	    caf_acgt_count_list[caf_a_loop] = 1;
	    caf_gc_count += caf_a_loop + 1;
	    caf_acgt_count += caf_a_loop + 1;
	    caf_gc_decreasing += 1;
	    caf_acgt_decreasing += 1;
	  }
	  else
	  {

		caf_gc_count_list[caf_a_loop] = 0;
	    if( strchr(caf_at_chars, cdp_chr_fasta[caf_a_loop]) != NULL )
	    {
	      caf_acgt_count_list[caf_a_loop] = 1;
	      caf_acgt_count += caf_a_loop + 1;
	      caf_acgt_decreasing += 1;
	    }
	    else
	    {
	      caf_acgt_count_list[caf_a_loop] = 0;
	    }
	  }
	}

	for( caf_a_loop=g_insert_mean;caf_a_loop<g_one_base_window_size;caf_a_loop++)
	{
	  if( strchr(caf_gc_chars, cdp_chr_fasta[caf_a_loop]) != NULL )
	  {
	    caf_gc_count_list[caf_a_loop] = 1;
	    caf_acgt_count_list[caf_a_loop] = 1;
	    caf_gc_count += g_one_base_window_size - caf_a_loop;
	    caf_acgt_count += g_one_base_window_size - caf_a_loop;
	    caf_gc_increasing += 1;
	    caf_acgt_increasing += 1;
	  }
	  else
	  {

	    caf_gc_count_list[caf_a_loop] = 0;
	    if( strchr(caf_at_chars, cdp_chr_fasta[caf_a_loop]) != NULL )
	    {
	      caf_acgt_count_list[caf_a_loop] = 1;
	      caf_acgt_count += g_one_base_window_size - caf_a_loop;
	      caf_acgt_increasing += 1;
	    }
	  }
	}
      }

      else
      {

		caf_gc_decreasing_adj = (long) caf_gc_count_list[caf_ref_pos-(long)g_insert_mean];
		caf_acgt_decreasing_adj = (long) caf_acgt_count_list[caf_ref_pos-(long)g_insert_mean];

	if( strchr(caf_gc_chars, cdp_chr_fasta[caf_ref_pos+g_insert_mean-1]) != NULL )
	{
	  caf_gc_count_list[caf_ref_pos+g_insert_mean-1] = 1;
	  caf_acgt_count_list[caf_ref_pos+g_insert_mean-1] = 1;
	  caf_gc_increasing += 1;
	  caf_acgt_increasing += 1;
	}
	else if( strchr(caf_at_chars, cdp_chr_fasta[caf_ref_pos+g_insert_mean-1]) != NULL )
	{
	  caf_gc_count_list[caf_ref_pos+g_insert_mean-1] = 0;
	  caf_acgt_count_list[caf_ref_pos+g_insert_mean-1] = 1;
	  caf_acgt_increasing += 1;
	}
	else
	{
	  caf_gc_count_list[caf_ref_pos+g_insert_mean-1] = 0;
	  caf_acgt_count_list[caf_ref_pos+g_insert_mean-1] = 0;
	}

	caf_gc_count += caf_gc_increasing - caf_gc_decreasing;
	caf_acgt_count += caf_acgt_increasing - caf_acgt_decreasing;
	caf_gc_decreasing -= caf_gc_decreasing_adj;
	caf_acgt_decreasing -= caf_acgt_decreasing_adj;
	if( caf_gc_count_list[caf_ref_pos] == 1 )
	{
	  caf_gc_decreasing += 1;
	  caf_gc_increasing -= 1;
	  caf_acgt_decreasing += 1;
	  caf_acgt_increasing -= 1;
	}
	else if( caf_acgt_count_list[caf_ref_pos] == 1 )
	{
	  caf_acgt_decreasing += 1;
	  caf_acgt_increasing -= 1;
	}
      }
      caf_one_base_rd_gc_weighted[caf_ref_pos] = 100*caf_gc_count/g_one_base_window_size_total;
      caf_one_base_rd_acgt_weighted[caf_ref_pos] = 100*caf_acgt_count/g_one_base_window_size_total;
    }

    free(caf_gc_count_list);
    free(caf_acgt_count_list);



    caf_n_blocks_end[caf_n_index] = caf_chr_fasta_len - 1;
    caf_n_index += 1;


    if( g_internal == 1 )
    {
      printf("caf_n_index %ld\n", caf_n_index);
      for(caf_a_loop=0;caf_a_loop<caf_n_index;caf_a_loop++)
      {
	printf("%ld %ld %ld\n", caf_a_loop, caf_n_blocks_start[caf_a_loop], caf_n_blocks_end[caf_a_loop]);
      }
    }

    uint32_t caf_c, caf_c_type, caf_c_len;
    int *caf_rd_mq_list = malloc(caf_chr_fasta_len * sizeof(int));
    if( caf_rd_mq_list == NULL )
    {
      printf("10 NULL\n");
      exit(0);
    }
    int *caf_rd_rd_list = (int *) malloc(caf_chr_fasta_len * sizeof(int));
    if( caf_rd_rd_list == NULL )
    {
      printf("11 NULL\n");
      exit(0);
    }
    int *caf_rd_low_mq_rd_list = (int *) malloc(caf_chr_fasta_len * sizeof(int));
    if( caf_rd_low_mq_rd_list == NULL )
    {
      printf("12 NULL\n");
      exit(0);
    }

    for(caf_a_loop=0;caf_a_loop<caf_chr_fasta_len;caf_a_loop++)
    {
	    caf_rd_mq_list[caf_a_loop] = 0;
	    caf_rd_rd_list[caf_a_loop] = 0;
	    caf_rd_low_mq_rd_list[caf_a_loop] = 0;
    }

    cdp_b = bam_init1();

    for(cdp_a_loop=0;cdp_a_loop<cdp_bam_file->header->n_targets;cdp_a_loop++)
    {
      cdp_target_name = cdp_bam_file->header->target_name[cdp_a_loop];
      cdp_bam_name_len = strlen(cdp_target_name);
      for(cdp_int_a=0;cdp_int_a<cdp_bam_name_len;cdp_int_a++)
      {

		if (contig_lower_case == 1) {
			cdp_bam_name[cdp_int_a] = tolower(cdp_target_name[cdp_int_a]);
		} else {
			cdp_bam_name[cdp_int_a] = cdp_target_name[cdp_int_a];
		}
      }
      cdp_int_a = cdp_bam_name_len - 1;
      while( cdp_int_a > 0 )
      {
	if( isgraph(cdp_bam_name[cdp_int_a]) == 0 )
	{
	  cdp_bam_name_len = cdp_int_a;
	}
	cdp_int_a -= 1;
      }
      if( g_internal == 1 )
      {
		printf("cdp_bam_name, cdp_bam_name_len, cdp_chr_name, cdp_chr_name_len %s %d %s %d\n", cdp_bam_name, cdp_bam_name_len, cdp_chr_name, cdp_chr_name_len);
      }
      if( cdp_bam_name_len == cdp_chr_name_len && strncmp(cdp_bam_name, cdp_chr_name, cdp_chr_name_len) == 0 )
      {
		cdp_chr_match = cdp_a_loop;
		break;
      }
      else if( (cdp_bam_name_len-g_chr_name_start+1) == cdp_chr_name_len && strncmp(cdp_bam_name, cdp_test_chr, (g_chr_name_start-1)) == 0 )
      {

		char *cdp_temp_string = malloc(strlen(cdp_test_chr)+strlen(cdp_chr_name)+1);
		strcpy(cdp_temp_string, cdp_test_chr);
		strcat(cdp_temp_string, cdp_chr_name);
		if( strncmp(cdp_bam_name, cdp_temp_string, cdp_bam_name_len) == 0 )
		{
		  cdp_chr_match = cdp_a_loop;
		  break;
		}
		free(cdp_temp_string);
      }
      else if( (cdp_bam_name_len+g_chr_name_start-1) == cdp_chr_name_len && strncmp(cdp_chr_name, cdp_test_chr, (g_chr_name_start-1)) == 0 )
      {

		char *cdp_temp_string = malloc(strlen(cdp_test_chr)+strlen(cdp_bam_name)+1);
		strcpy(cdp_temp_string, cdp_test_chr);
		strcat(cdp_temp_string, cdp_bam_name);
		if( strncmp(cdp_chr_name, cdp_temp_string, cdp_chr_name_len) == 0 )
		{
		  cdp_chr_match = cdp_a_loop;
		  break;
		}
		free(cdp_temp_string);
      }
    }

    if( g_internal == 1 )
    {
      printf("cdp_chr_match %d\n", cdp_chr_match);
    }


    if( g_internal == 1 )
    {
      printf("cdp_chr_match %d\n", cdp_chr_match);
    }


    double cdp_comb_div_list[g_max_combinations];
    double cdp_comb_mult_list[g_max_combinations];
    int cdp_comb_div_list_len, cdp_comb_mult_list_len;
    int cdp_n1, cdp_k1, cdp_n2, cdp_k2;
    int cdp_div_index, cdp_mult_index;

	
    int cdp_tumor_chr_len = 0;
    char cdp_tumor_line[100000];

    int cdp_tumor_chr_indel_len = 0;
    int cdp_tumor_indel_len = cdp_tumor_len;
    if( cdp_tumor_indel_len > 10000 )
    {
      cdp_tumor_indel_len = cdp_tumor_indel_len / g_tumor_indel_len_divisor;
    }

    char *cdp_tumor_str;

    if( g_internal == 1 )
    {
      printf("cdp_tumor_len %d\n", cdp_tumor_len);
    }


    int *cdp_tumor_type = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_type == NULL )
    {
      printf("13 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start == NULL )
    {
      printf("14 NULL\n");
      exit(0);
    }
   int *cdp_tumor_end = malloc(cdp_tumor_len * sizeof(int));
     if( cdp_tumor_end == NULL )
    {
      printf("15 NULL\n");
      exit(0);
    }


    int **cdp_tumor_start2;
    cdp_tumor_start2 = malloc(cdp_tumor_len*sizeof(int*));
    if( cdp_tumor_start2 == NULL )
    {
      printf("16 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<cdp_tumor_len;cdp_a_loop++)
    {
      cdp_tumor_start2[cdp_a_loop] = malloc(2*sizeof(int));
      if( cdp_tumor_start2[cdp_a_loop] == NULL )
      {
	printf("17 NULL\n");
	exit(0);
      }
    }
    int **cdp_tumor_end2;
    cdp_tumor_end2 = malloc(cdp_tumor_len*sizeof(int*));
    if( cdp_tumor_end2 == NULL )
    {
      printf("18 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<cdp_tumor_len;cdp_a_loop++)
    {
      cdp_tumor_end2[cdp_a_loop] = malloc(2*sizeof(int));
      if( cdp_tumor_end2 == NULL )
      {
	printf("19 NULL\n");
	exit(0);
      }
    }
    for(cdp_a_loop=0;cdp_a_loop<cdp_tumor_len;cdp_a_loop++)
    {
      cdp_tumor_start2[cdp_a_loop][1] = cdp_a_loop;
      cdp_tumor_end2[cdp_a_loop][1] = cdp_a_loop;
    }

    double *cdp_tumor_dist = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_dist == NULL )
    {
      printf("20 NULL\n");
      exit(0);
    }
    double *cdp_tumor_start_binom_cdf = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_start_binom_cdf == NULL )
    {
      printf("21 NULL\n");
      exit(0);
    }
    double *cdp_tumor_end_binom_cdf = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_end_binom_cdf == NULL )
    {
      printf("22 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start_sv_evidence = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_sv_evidence == NULL )
    {
      printf("23 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_sv_evidence = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_sv_evidence == NULL )
    {
      printf("24 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start_conc = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_conc == NULL )
    {
      printf("25 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_conc = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_conc == NULL )
    {
      printf("26 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start_other_len = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_other_len == NULL )
    {
      printf("27 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_other_len = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_other_len == NULL )
    {
      printf("28 NULL\n");
      exit(0);
    }
    int *cdp_tumor_mchr = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_mchr == NULL )
    {
      printf("29 NULL\n");
      exit(0);
    }
    int *cdp_tumor_mpos = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_mpos == NULL )
    {
      printf("30 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start_read_start = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_read_start == NULL )
    {
      printf("31 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start_read_end = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_read_end == NULL )
    {
      printf("32 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_read_start = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_read_start == NULL )
    {
      printf("33 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_read_end = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_read_end == NULL )
    {
      printf("34 NULL\n");
      exit(0);
    }
    double *cdp_tumor_start_hez_binom_cdf = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_start_hez_binom_cdf == NULL )
    {
      printf("34-2 NULL\n");
      exit(0);
    }
    double *cdp_tumor_end_hez_binom_cdf = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_end_hez_binom_cdf == NULL )
    {
      printf("34-3 NULL\n");
      exit(0);
    }


    char *cdp_tumor_snv_base = malloc(cdp_tumor_len * sizeof(char));
    if( cdp_tumor_snv_base == NULL )
    {
      printf("35 NULL\n");
      exit(0);
    }
    double *cdp_tumor_snv_ratio = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_snv_ratio == NULL )
    {
      printf("36 NULL\n");
      exit(0);
    }
    int **cdp_tumor_snv;
    cdp_tumor_snv = malloc(g_nucleotides*sizeof(int*));
    if( cdp_tumor_snv == NULL )
    {
      printf("37 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_tumor_snv[cdp_a_loop] = malloc(cdp_tumor_len * sizeof(int));
      if( cdp_tumor_snv[cdp_a_loop] == NULL )
      {
	printf("38 NULL\n");
	exit(0);
      }
    }
    int **cdp_tumor_snv_lowmq;
    cdp_tumor_snv_lowmq = malloc(g_nucleotides*sizeof(int*));
    if( cdp_tumor_snv_lowmq == NULL )
    {
      printf("39 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_tumor_snv_lowmq[cdp_a_loop] = malloc(cdp_tumor_len * sizeof(int));
      if( cdp_tumor_snv_lowmq[cdp_a_loop] == NULL )
      {
	printf("40 NULL\n");
	exit(0);
      }
    }


    int *cdp_tumor_start_indel = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_indel == NULL )
    {
      printf("41 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_indel = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_indel == NULL )
    {
      printf("42 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start_rd = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_rd == NULL )
    {
      printf("43 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_rd = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_rd == NULL )
    {
      printf("44 NULL\n");
      exit(0);
    }
    int *cdp_tumor_start_sc = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_start_sc == NULL )
    {
      printf("45 NULL\n");
      exit(0);
    }
    int *cdp_tumor_end_sc = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_end_sc == NULL )
    {
      printf("46 NULL\n");
      exit(0);
    }
    int *cdp_tumor_snv_bq = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_snv_bq == NULL )
    {
      printf("47 NULL\n");
      exit(0);
    }
    int *cdp_tumor_snv_bq_all = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_snv_bq_all == NULL )
    {
      printf("48 NULL\n");
      exit(0);
    }
    int *cdp_tumor_snv_mq = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_snv_mq == NULL )
    {
      printf("49 NULL\n");
      exit(0);
    }
    int *cdp_tumor_snv_mq_all = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_snv_mq_all == NULL )
    {
      printf("50 NULL\n");
      exit(0);
    }
    int *cdp_tumor_snv_bq_read_count = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_snv_bq_read_count == NULL )
    {
      printf("51 NULL\n");
      exit(0);
    }
    int *cdp_tumor_snv_mq_read_count = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_snv_mq_read_count == NULL )
    {
      printf("52 NULL\n");
      exit(0);
    }
    int *cdp_tumor_snv_read_count_all = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_snv_read_count_all == NULL )
    {
      printf("53 NULL\n");
      exit(0);
    }
    double *cdp_tumor_snv_pos_in_read = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_snv_pos_in_read == NULL )
    {
      printf("54 NULL\n");
      exit(0);
    }
    double *cdp_tumor_snv_fstrand = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_snv_fstrand == NULL )
    {
      printf("55 NULL\n");
      exit(0);
    }
    double *cdp_tumor_snv_start_binom_cdf = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_snv_start_binom_cdf == NULL )
    {
      printf("56 NULL\n");
      exit(0);
    }
    double *cdp_tumor_snv_start_hez_binom_cdf = malloc(cdp_tumor_len * sizeof(double));
    if( cdp_tumor_snv_start_hez_binom_cdf == NULL )
    {
      printf("57 NULL\n");
      exit(0);
    }
    int *cdp_tumor_hp = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_hp == NULL )
    {
      printf("homopolymer NULL\n");
      exit(0);
    }

    int *cdp_tumor_indel_seq_index = malloc(cdp_tumor_len * sizeof(int));
    if( cdp_tumor_indel_seq_index == NULL )
    {
      printf("indel_seq_index NULL\n");
      exit(0);
    }

    char **cdp_tumor_indel_seq;
    cdp_tumor_indel_seq = malloc(cdp_snv_gt_string_len*sizeof(char*));
    if( cdp_tumor_indel_seq == NULL )
    {
      printf("indel_seq 1 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<cdp_snv_gt_string_len;cdp_a_loop++)
    {
      cdp_tumor_indel_seq[cdp_a_loop] = malloc(cdp_tumor_indel_len * sizeof(char));
      if( cdp_tumor_indel_seq[cdp_a_loop] == NULL )
      {
	printf("indel_seq 2 NULL\n");
	exit(0);
      }
    }


    if( cdp_tumor_sv_file_name != NULL )
    {
      char *cdp_tumor_sv_file_name_ctx = malloc(strlen(cdp_tumor_sv_file_name) + 4 + 1);
      strcpy(cdp_tumor_sv_file_name_ctx, cdp_tumor_sv_file_name);

      if( strlen(cdp_tumor_sv_file_name) > 4 && cdp_tumor_sv_file_name[strlen(cdp_tumor_sv_file_name)-4] == '.' && cdp_tumor_sv_file_name[strlen(cdp_tumor_sv_file_name)-3] == 'v' && cdp_tumor_sv_file_name[strlen(cdp_tumor_sv_file_name)-2] == 'c' && cdp_tumor_sv_file_name[strlen(cdp_tumor_sv_file_name)-1] == 'f' )
      {
		sprintf(cdp_tumor_sv_file_name_ctx, "%s.%s", cdp_tumor_sv_file_name, cdp_ctx_string);
		cdp_tumor_sv_file_name_ctx[strlen(cdp_tumor_sv_file_name)-3] = 'c';
		cdp_tumor_sv_file_name_ctx[strlen(cdp_tumor_sv_file_name)-2] = 't';
		cdp_tumor_sv_file_name_ctx[strlen(cdp_tumor_sv_file_name)-1] = 'x';
		cdp_tumor_sv_file_name_ctx[strlen(cdp_tumor_sv_file_name)+1] = 'v';
		cdp_tumor_sv_file_name_ctx[strlen(cdp_tumor_sv_file_name)+2] = 'c';
		cdp_tumor_sv_file_name_ctx[strlen(cdp_tumor_sv_file_name)+3] = 'f';
      }
      else
      {
		sprintf(cdp_tumor_sv_file_name_ctx, "%s.%s", cdp_tumor_sv_file_name, cdp_ctx_string);
      }

#ifdef DO_PRINT
      printf("ctx file %s\n", cdp_tumor_sv_file_name_ctx);
#endif
      int cdp_tumor_file_loop;
      for(cdp_tumor_file_loop=0;cdp_tumor_file_loop<2;cdp_tumor_file_loop++)
      {
	if( cdp_tumor_file_loop == 0 )
	{
	  cdp_tumor_handle = fopen(cdp_tumor_sv_file_name, "r");
	}
	else
	{
	  cdp_tumor_handle = fopen(cdp_tumor_sv_file_name_ctx, "r");
	  if( cdp_tumor_handle == NULL )
	  {
	    printf("WARNING: Unable to open %s\n", cdp_tumor_sv_file_name_ctx);
	    break;
	  }
	}

	for(cdp_a_loop=0;cdp_a_loop<cdp_tumor_len;cdp_a_loop++)
	{
	  cdp_tumor_dist[cdp_a_loop] = (double)0.0;
	}

	if( g_vcf == 1 )
	{
	  int cdp_is_snv = 0;
	  int cdp_is_indel = 0;
	  int cdp_indel_found = 0;
	  int cdp_tumor_str_index = 0;
	  int cdp_tumor_str_list_index = 0;
	  int cdp_tumor_line_end = 0;
	  char **cdp_tumor_str_list;
	  cdp_tumor_str_list = malloc(g_max_tumor_str_list_len*sizeof(char*));
	  if( cdp_tumor_str_list == NULL )
	  {
	    printf("Tumor str NULL\n");
	    exit(0);
	  }
	  for(cdp_a_loop=0;cdp_a_loop<g_max_tumor_str_list_len;cdp_a_loop++)
	  {
	    cdp_tumor_str_list[cdp_a_loop] = malloc(g_max_tumor_str_len * sizeof(char));
	    memset(cdp_tumor_str_list[cdp_a_loop], 0, g_max_tumor_str_len);
	  }

	  while(fgets(cdp_tumor_line, sizeof(cdp_tumor_line), cdp_tumor_handle) )
	  {
	    if( cdp_tumor_line[0] != '#' )
	    {

	      cdp_tumor_str_list_index = 0;
	      cdp_tumor_str_index = 0;
	      cdp_tumor_line_end = 0;
	      cdp_b_loop = 0;
	      while( cdp_tumor_line_end == 0 )
	      {
		if( cdp_tumor_line[cdp_b_loop] == '\t' )
		{
		  cdp_tumor_str_list[cdp_tumor_str_list_index][cdp_tumor_str_index] = '\0';
		  cdp_tumor_str_list_index += 1;
		  cdp_tumor_str_index = 0;
		}
		else if( cdp_tumor_line[cdp_b_loop] == '\n' )
		{
		  cdp_tumor_str_list[cdp_tumor_str_list_index][cdp_tumor_str_index] = '\0';
		  cdp_tumor_str_list_index += 1;
		  cdp_tumor_line_end = 1;
		}
		else
		{
		  cdp_tumor_str_list[cdp_tumor_str_list_index][cdp_tumor_str_index] = cdp_tumor_line[cdp_b_loop];
		  cdp_tumor_str_index += 1;
		}
		cdp_b_loop += 1;
	      }



	      if( strcmp(cdp_tumor_str_list[0], cdp_chr_name) == 0 )
	      {
		cdp_is_snv = 0;
		cdp_is_indel = 0;
		cdp_indel_found = 0;
		if( strlen(cdp_tumor_str_list[g_vcf_ref_index]) > 1 || strlen(cdp_tumor_str_list[g_vcf_alt_index]) > 1 )
		{

		  for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
		  {
		    if( cdp_tumor_str_list[g_vcf_ref_index][0] == g_dna_upper_str[cdp_b_loop][0] || cdp_tumor_str_list[g_vcf_ref_index][0] == g_dna_lower_str[cdp_b_loop][0] || cdp_tumor_str_list[g_vcf_alt_index][0] == g_dna_upper_str[cdp_b_loop][0] || cdp_tumor_str_list[g_vcf_alt_index][0] == g_dna_lower_str[cdp_b_loop][0] )
		    {
		      cdp_indel_found = 1;
		      break;
		    }
		  }
		  if( cdp_indel_found == 1 )
		  {

		    if( strlen(cdp_tumor_str_list[g_vcf_ref_index]) < strlen(cdp_tumor_str_list[g_vcf_alt_index]) )
		    {
		      cdp_is_indel = INDEL_INS;
		      if( cdp_tumor_chr_indel_len < cdp_tumor_indel_len )
		      {
			for(cdp_b_loop=0;cdp_b_loop<strlen(cdp_tumor_str_list[g_vcf_alt_index]);cdp_b_loop++)
			{
			  cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_chr_indel_len] = cdp_tumor_str_list[g_vcf_alt_index][cdp_b_loop];
			}
			cdp_tumor_indel_seq[strlen(cdp_tumor_str_list[g_vcf_alt_index])][cdp_tumor_chr_indel_len] = '\0';
		      }
		    }
		    else
		    {
		      cdp_is_indel = INDEL_DEL;
		      if( cdp_tumor_chr_indel_len < cdp_tumor_indel_len )
		      {
			for(cdp_b_loop=0;cdp_b_loop<strlen(cdp_tumor_str_list[g_vcf_ref_index]);cdp_b_loop++)
			{
			  cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_chr_indel_len] = cdp_tumor_str_list[g_vcf_ref_index][cdp_b_loop];
			}
			cdp_tumor_indel_seq[strlen(cdp_tumor_str_list[g_vcf_ref_index])][cdp_tumor_chr_indel_len] = '\0';
		      }
		    }
		    if( cdp_tumor_chr_indel_len < cdp_tumor_indel_len )
		    {
		      cdp_tumor_indel_seq_index[cdp_tumor_chr_len] = cdp_tumor_chr_indel_len;
		      cdp_tumor_chr_indel_len += 1;
		    }
		    else
		    {
		      cdp_tumor_indel_seq_index[cdp_tumor_chr_len] = 0;
		    }
		  }
		}
		for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
		{
		  if( strcmp(cdp_tumor_str_list[g_vcf_ref_index], g_dna_upper_str[cdp_b_loop]) == 0 )
		  {
		    cdp_is_snv += 1;
		  }
		  if( strcmp(cdp_tumor_str_list[g_vcf_alt_index], g_dna_upper_str[cdp_b_loop]) == 0 )
		  {
		    cdp_is_snv += 1;
		  }
		  if( strcmp(cdp_tumor_str_list[g_vcf_ref_index], g_dna_lower_str[cdp_b_loop]) == 0 )
		  {
		    cdp_is_snv += 1;
		  }
		  if( strcmp(cdp_tumor_str_list[g_vcf_alt_index], g_dna_lower_str[cdp_b_loop]) == 0 )
		  {
		    cdp_is_snv += 1;
		  }
		}


		if( cdp_is_indel == INDEL_INS || cdp_is_indel == INDEL_DEL || cdp_is_snv == 2 )
		{

		  cdp_tumor_str_index = 0;
		  cdp_tumor_line_end = 0;
		  cdp_b_loop = 0;
		  while( cdp_tumor_line_end == 0 )
		  {
		    if( cdp_tumor_str_list[g_vcf_sample_index][cdp_b_loop] == ':' )
		    {
		      cdp_tumor_str_list[cdp_tumor_str_list_index][cdp_tumor_str_index] = '\0';
		      cdp_tumor_str_list_index += 1;
		      cdp_tumor_str_index = 0;
		    }
		    else if( cdp_tumor_str_list[g_vcf_sample_index][cdp_b_loop] == '\n' || cdp_tumor_str_list[g_vcf_sample_index][cdp_b_loop] == '\0' )
		    {
		      cdp_tumor_str_list[cdp_tumor_str_list_index][cdp_tumor_str_index] = '\0';
		      cdp_tumor_str_list_index += 1;
		      cdp_tumor_line_end = 1;
		    }
		    else
		    {
		      cdp_tumor_str_list[cdp_tumor_str_list_index][cdp_tumor_str_index] = cdp_tumor_str_list[g_vcf_sample_index][cdp_b_loop];
		      cdp_tumor_str_index += 1;
		    }
		    cdp_b_loop += 1;
		  }

		}

		if( cdp_is_indel > 0 )
		{
		  cdp_tumor_type[cdp_tumor_chr_len] = cdp_is_indel;
		  cdp_tumor_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_pos_index]) - 1;
		  cdp_tumor_start2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		  if( strlen(cdp_tumor_str_list[g_vcf_info_index]) > 4 )
		  {
		    memcpy(cdp_tumor_str_list[g_vcf_info_index], &cdp_tumor_str_list[g_vcf_info_index][4], strlen(cdp_tumor_str_list[g_vcf_info_index]) - 4 + 1 );
		  }
		  cdp_tumor_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_info_index]) - 1;
		  cdp_tumor_end2[cdp_tumor_chr_len][0] = atoi(cdp_tumor_str_list[g_vcf_info_index]) - 1;
		  cdp_tumor_start_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_spr_index[cdp_is_indel]]);
		  cdp_tumor_end_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_epr_index[cdp_is_indel]]);
		  cdp_tumor_start_indel[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sev_index[cdp_is_indel]]) * cdp_add_factor;
		  cdp_tumor_end_indel[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_eev_index[cdp_is_indel]]) * cdp_add_factor;
		  cdp_tumor_start_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_srd_index[cdp_is_indel]]);
		  cdp_tumor_end_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_erd_index[cdp_is_indel]]);
		  cdp_tumor_start_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sco_index[cdp_is_indel]]);
		  cdp_tumor_end_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_eco_index[cdp_is_indel]]);
		  cdp_tumor_start_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sot_index[cdp_is_indel]]);
		  cdp_tumor_end_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_eot_index[cdp_is_indel]]);
		  cdp_tumor_start_sc[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_ssc_index[cdp_is_indel]]);
		  cdp_tumor_end_sc[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_esc_index[cdp_is_indel]]);
		  cdp_tumor_hp[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_hp_index[cdp_is_indel]]);

		  cdp_tumor_snv_base[cdp_tumor_chr_len] = '-';

		  cdp_tumor_snv_ratio[cdp_tumor_chr_len] = 0;
		  for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
		  {

			cdp_tumor_snv[cdp_a_loop][cdp_tumor_chr_len] = 0;
		    cdp_tumor_snv_lowmq[cdp_a_loop][cdp_tumor_chr_len] = 0;
		  }

		  cdp_tumor_snv_bq[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_bq_all[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_mq[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_mq_all[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_bq_read_count[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_mq_read_count[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_read_count_all[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_pos_in_read[cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_fstrand[cdp_tumor_chr_len] = 0;

		  cdp_tumor_chr_len += 1;
		}

		else if( cdp_is_snv == 2 )
		{
		  cdp_tumor_type[cdp_tumor_chr_len] = SNV;
		  cdp_tumor_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_pos_index]) - 1;
		  cdp_tumor_start2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		  cdp_tumor_end[cdp_tumor_chr_len] = cdp_tumor_start[cdp_tumor_chr_len];
		  cdp_tumor_end2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		  cdp_tumor_snv_base[cdp_tumor_chr_len] = cdp_tumor_str_list[g_vcf_alt_index][0];
		  cdp_tumor_snv_ratio[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_af_index]);
		  cdp_c_loop = 0;
		  for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
		  {

			cdp_tumor_snv[cdp_b_loop][cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sample_snv_index+cdp_b_loop]);
		    cdp_c_loop += cdp_tumor_snv[cdp_b_loop][cdp_tumor_chr_len];
		  }

		  cdp_tumor_start_conc[cdp_tumor_chr_len] = cdp_c_loop;
		  cdp_c_loop = 0;
		  for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
		  {

			cdp_tumor_snv_lowmq[cdp_b_loop][cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sample_snv_lowmq_index+cdp_b_loop]);
		    cdp_c_loop += cdp_tumor_snv_lowmq[cdp_b_loop][cdp_tumor_chr_len];
		  }

		  cdp_tumor_end_conc[cdp_tumor_chr_len] = cdp_c_loop;
		  cdp_tumor_snv_bq[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_bq_index]) * cdp_tumor_start_conc[cdp_tumor_chr_len];
		  cdp_tumor_snv_bq_all[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_bq_index]) * (cdp_tumor_start_conc[cdp_tumor_chr_len] + cdp_tumor_end_conc[cdp_tumor_chr_len]);
		  cdp_tumor_snv_mq[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_mq_index]) * cdp_tumor_start_conc[cdp_tumor_chr_len];
		  cdp_tumor_snv_mq_all[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_mq_index]) * (cdp_tumor_start_conc[cdp_tumor_chr_len] + cdp_tumor_end_conc[cdp_tumor_chr_len]);
		  cdp_tumor_snv_bq_read_count[cdp_tumor_chr_len] = cdp_tumor_start_conc[cdp_tumor_chr_len];
		  cdp_tumor_snv_mq_read_count[cdp_tumor_chr_len] = cdp_tumor_start_conc[cdp_tumor_chr_len];
		  cdp_tumor_snv_read_count_all[cdp_tumor_chr_len] = cdp_tumor_end_conc[cdp_tumor_chr_len];
		  cdp_tumor_snv_pos_in_read[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_pir_index]);
		  cdp_tumor_snv_fstrand[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_fs_index]);

		  cdp_tumor_start_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str_list[g_vcf_sample_pr_index]);
		  cdp_tumor_end_binom_cdf[cdp_tumor_chr_len] = cdp_tumor_start_binom_cdf[cdp_tumor_chr_len];
		  if( cdp_tumor_str_list_index > g_vcf_sample_sc_index )
		  {

			cdp_tumor_start_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sample_ol_index]);

		    cdp_tumor_end_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sample_in_index]);

		    cdp_tumor_end_read_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str_list[g_vcf_sample_sc_index]);
		  }
		  else
		  {
		    cdp_tumor_start_other_len[cdp_tumor_chr_len] = 0;
		    cdp_tumor_end_other_len[cdp_tumor_chr_len] = 0;
		    cdp_tumor_end_read_start[cdp_tumor_chr_len] = 0;
		  }

		  cdp_tumor_mchr[cdp_tumor_chr_len] = 0;
		  cdp_tumor_mpos[cdp_tumor_chr_len] = 0;
		  cdp_tumor_start_read_start[cdp_tumor_chr_len] = 0;
		  cdp_tumor_start_read_end[cdp_tumor_chr_len] = 0;
		  cdp_tumor_end_read_end[cdp_tumor_chr_len] = 0;

		  cdp_tumor_chr_len += 1;
		}
	      }

	    }
	  }


	  for(cdp_a_loop=0;cdp_a_loop<g_max_tumor_str_list_len;cdp_a_loop++)
	  {
	    free(cdp_tumor_str_list[cdp_a_loop]);
	  }
	  free(cdp_tumor_str_list);


	}
	else
	{

	  fgets(cdp_tumor_line, sizeof(cdp_tumor_line), cdp_tumor_handle);

	  fgets(cdp_tumor_line, sizeof(cdp_tumor_line), cdp_tumor_handle);
	  while(fgets(cdp_tumor_line, sizeof(cdp_tumor_line), cdp_tumor_handle) )
	  {
	    cdp_tumor_str = strtok(cdp_tumor_line, g_tumor_separator);
	    for(cdp_a_loop=0;cdp_a_loop<g_sv_types_len;cdp_a_loop++)
	    {
	      if( strcmp(cdp_tumor_str, g_sv_types[cdp_a_loop]) == 0 )
	      {
		cdp_tumor_type[cdp_tumor_chr_len] = cdp_a_loop;
		break;
	      }
	    }
	    cdp_tumor_str = strtok(NULL, g_tumor_separator);
	    if( strcmp(cdp_tumor_str, cdp_chr_name) == 0 )
	    {

		  if( cdp_tumor_type[cdp_tumor_chr_len] == 6 || cdp_tumor_type[cdp_tumor_chr_len] == 7 )
	      {
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str)-1;
		cdp_tumor_start2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		cdp_tumor_end[cdp_tumor_chr_len] = cdp_tumor_start[cdp_tumor_chr_len];
		cdp_tumor_end2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_end_binom_cdf[cdp_tumor_chr_len] = cdp_tumor_start_binom_cdf[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_sv_evidence[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_end_sv_evidence[cdp_tumor_chr_len] = cdp_tumor_start_sv_evidence[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_end_rd[cdp_tumor_chr_len] = cdp_tumor_start_rd[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_end_conc[cdp_tumor_chr_len] = cdp_tumor_start_conc[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_end_other_len[cdp_tumor_chr_len] = cdp_tumor_start_other_len[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_mchr[cdp_tumor_chr_len] = atoi(cdp_tumor_str);

		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_mpos[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_read_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_end_read_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_read_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_end_read_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_hez_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_end_hez_binom_cdf[cdp_tumor_chr_len] = cdp_tumor_start_hez_binom_cdf[cdp_tumor_chr_len];


		cdp_tumor_snv_base[cdp_tumor_chr_len] = '-';
		cdp_tumor_snv_ratio[cdp_tumor_chr_len] = 0;
		for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
		{

		  cdp_tumor_snv[cdp_a_loop][cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_lowmq[cdp_a_loop][cdp_tumor_chr_len] = 0;
		}
		cdp_tumor_snv_bq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_read_count_all[cdp_tumor_chr_len] = 0;

		cdp_tumor_snv_pos_in_read[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_fstrand[cdp_tumor_chr_len] = 0;

	      }

	      else if( cdp_tumor_type[cdp_tumor_chr_len] == SNV )
	      {
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
			cdp_tumor_start2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
			cdp_tumor_end[cdp_tumor_chr_len] = cdp_tumor_start[cdp_tumor_chr_len];
			cdp_tumor_end2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_base[cdp_tumor_chr_len] = cdp_tumor_str[0];
			cdp_tumor_str = strtok(NULL, g_tumor_separator);

			cdp_tumor_snv_ratio[cdp_tumor_chr_len] = atof(cdp_tumor_str);

			cdp_tumor_str = strtok(NULL, g_tumor_separator);

			cdp_tumor_start_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);

			cdp_tumor_end_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
			{
			  cdp_tumor_str = strtok(NULL, g_tumor_separator);

			  cdp_tumor_snv[cdp_a_loop][cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			}
			for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
			{
			  cdp_tumor_str = strtok(NULL, g_tumor_separator);

			  cdp_tumor_snv_lowmq[cdp_a_loop][cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			}

			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_bq[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_bq_all[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_mq[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_mq_all[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_bq_read_count[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_mq_read_count[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_read_count_all[cdp_tumor_chr_len] = atoi(cdp_tumor_str);

			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_pos_in_read[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
			cdp_tumor_str = strtok(NULL, g_tumor_separator);
			cdp_tumor_snv_fstrand[cdp_tumor_chr_len] = atoi(cdp_tumor_str);

			cdp_tumor_start_binom_cdf[cdp_tumor_chr_len] = 0;
			cdp_tumor_end_binom_cdf[cdp_tumor_chr_len] = 0;
			cdp_tumor_start_other_len[cdp_tumor_chr_len] = 0;
			cdp_tumor_end_other_len[cdp_tumor_chr_len] = 0;
			cdp_tumor_mchr[cdp_tumor_chr_len] = 0;
			cdp_tumor_mpos[cdp_tumor_chr_len] = 0;
			cdp_tumor_start_read_start[cdp_tumor_chr_len] = 0;
			cdp_tumor_end_read_start[cdp_tumor_chr_len] = 0;
			cdp_tumor_start_read_end[cdp_tumor_chr_len] = 0;
			cdp_tumor_end_read_end[cdp_tumor_chr_len] = 0;

	      }

	      else if( cdp_tumor_type[cdp_tumor_chr_len] == INDEL_DEL )
	      {
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_start2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_end2[cdp_tumor_chr_len][0] = cdp_tumor_end[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_dist[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str);

		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_indel[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_indel[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_sc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_sc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);


		cdp_tumor_str = strtok(NULL, g_tumor_separator);

		cdp_tumor_str = strtok(NULL, g_tumor_separator);

		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		if( cdp_tumor_chr_indel_len < cdp_tumor_indel_len )
		{
		  for(cdp_b_loop=0;cdp_b_loop<cdp_snv_gt_string_len;cdp_b_loop++)
		  {
		    if( cdp_tumor_str[cdp_b_loop] != '\n' )
		    {
		      cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_chr_indel_len] = cdp_tumor_str[cdp_b_loop];
		    }
		    else
		    {
		      cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_chr_indel_len] = '\0';
		    }
		  }
		}

		if( cdp_tumor_chr_indel_len < cdp_tumor_indel_len )
		{
		  cdp_tumor_indel_seq_index[cdp_tumor_chr_len] = cdp_tumor_chr_indel_len;
		  cdp_tumor_chr_indel_len += 1;
		}
		else
		{
		  cdp_tumor_indel_seq_index[cdp_tumor_chr_len] = 0;
		}

		cdp_tumor_snv_base[cdp_tumor_chr_len] = '-';

		cdp_tumor_snv_ratio[cdp_tumor_chr_len] = 0;
		for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
		{

		  cdp_tumor_snv[cdp_a_loop][cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_lowmq[cdp_a_loop][cdp_tumor_chr_len] = 0;
		}

		cdp_tumor_snv_bq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_read_count_all[cdp_tumor_chr_len] = 0;

		cdp_tumor_snv_pos_in_read[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_fstrand[cdp_tumor_chr_len] = 0;
	      }
	      else if( cdp_tumor_type[cdp_tumor_chr_len] == INDEL_INS )
	      {
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_start2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_end2[cdp_tumor_chr_len][0] = cdp_tumor_end[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_dist[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str);

		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_indel[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_end_indel[cdp_tumor_chr_len] = cdp_tumor_start_indel[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_end_rd[cdp_tumor_chr_len] = cdp_tumor_start_rd[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_sc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_end_sc[cdp_tumor_chr_len] = cdp_tumor_start_sc[cdp_tumor_chr_len];

		cdp_tumor_snv_base[cdp_tumor_chr_len] = '-';

		cdp_tumor_snv_ratio[cdp_tumor_chr_len] = 0;
		for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
		{

		  cdp_tumor_snv[cdp_a_loop][cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_lowmq[cdp_a_loop][cdp_tumor_chr_len] = 0;
		}

		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		if( cdp_tumor_chr_indel_len < cdp_tumor_indel_len )
		{
		  for(cdp_b_loop=0;cdp_b_loop<cdp_snv_gt_string_len;cdp_b_loop++)
		  {
		    if( cdp_tumor_str[cdp_b_loop] != '\n' )
		    {
		      cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_chr_indel_len] = cdp_tumor_str[cdp_b_loop];
		    }
		    else
		    {
		      cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_chr_indel_len] = '\0';
		    }
		  }
		}

		if( cdp_tumor_chr_indel_len < cdp_tumor_indel_len )
		{
		  cdp_tumor_indel_seq_index[cdp_tumor_chr_len] = cdp_tumor_chr_indel_len;
		  cdp_tumor_chr_indel_len += 1;
		}
		else
		{
		  cdp_tumor_indel_seq_index[cdp_tumor_chr_len] = 0;
		}

		cdp_tumor_snv_bq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_read_count_all[cdp_tumor_chr_len] = 0;

		cdp_tumor_snv_pos_in_read[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_fstrand[cdp_tumor_chr_len] = 0;
	      }
	      else if( cdp_tumor_type[cdp_tumor_chr_len] < 11 )
	      {
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_start2[cdp_tumor_chr_len][0] = cdp_tumor_start[cdp_tumor_chr_len];
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		cdp_tumor_end2[cdp_tumor_chr_len][0] = cdp_tumor_end[cdp_tumor_chr_len];
		if( cdp_tumor_type[cdp_tumor_chr_len] != 2 )
		{
		  cdp_tumor_str = strtok(NULL, g_tumor_separator);
		  cdp_tumor_dist[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		}
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_binom_cdf[cdp_tumor_chr_len] = atof(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_sv_evidence[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_sv_evidence[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_rd[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_conc[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_start_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		cdp_tumor_str = strtok(NULL, g_tumor_separator);
		cdp_tumor_end_other_len[cdp_tumor_chr_len] = atoi(cdp_tumor_str);
		if( cdp_tumor_type[cdp_tumor_chr_len] == 0 || cdp_tumor_type[cdp_tumor_chr_len] == 1 || cdp_tumor_type[cdp_tumor_chr_len] == 8 || cdp_tumor_type[cdp_tumor_chr_len] == 9 )
		{
		  cdp_tumor_str = strtok(NULL, g_tumor_separator);
		  cdp_tumor_start_read_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		  cdp_tumor_str = strtok(NULL, g_tumor_separator);
		  cdp_tumor_start_read_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		  cdp_tumor_str = strtok(NULL, g_tumor_separator);
		  cdp_tumor_end_read_start[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		  cdp_tumor_str = strtok(NULL, g_tumor_separator);
		  cdp_tumor_end_read_end[cdp_tumor_chr_len] = atoi(cdp_tumor_str) - 1;
		}

		cdp_tumor_snv_base[cdp_tumor_chr_len] = '-';

		cdp_tumor_snv_ratio[cdp_tumor_chr_len] = 0;
		for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
		{

		  cdp_tumor_snv[cdp_a_loop][cdp_tumor_chr_len] = 0;
		  cdp_tumor_snv_lowmq[cdp_a_loop][cdp_tumor_chr_len] = 0;
		}

		cdp_tumor_snv_bq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_all[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_bq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_mq_read_count[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_read_count_all[cdp_tumor_chr_len] = 0;

		cdp_tumor_snv_pos_in_read[cdp_tumor_chr_len] = 0;
		cdp_tumor_snv_fstrand[cdp_tumor_chr_len] = 0;
	      }

	      cdp_tumor_chr_len += 1;
	    }
	  }
	}
	fclose(cdp_tumor_handle);
      }
      free(cdp_tumor_sv_file_name_ctx);

      if( cdp_tumor_chr_len > 0 )
      {
	qsort((int*) cdp_tumor_start2, cdp_tumor_chr_len, sizeof(*cdp_tumor_start2), cmpfunc_2dm);
	qsort((int*) cdp_tumor_end2, cdp_tumor_chr_len, sizeof(*cdp_tumor_start2), cmpfunc_2dm);
      }
    }



    uint32_t cdp_c;
    uint32_t cdp_c_type[1000], cdp_c_len[1000];
    int cdp_c_type_len = 1000;

    char *cdp_end_ptr;
    char cdp_aux_c_type_str[1000];
    uint32_t cdp_aux_c_type[1000], cdp_aux_c_len[1000];
    int cdp_aux_c_type_len = 1000;
    int cdp_aux_cigar_len;
    int cdp_aux_lp_len;
    int cdp_aux_c_type_str_len;
    int cdp_aux_start_adj, cdp_aux_end_adj, cdp_aux_end_adj_indel;
    uint8_t *cdp_aux;

    int cdp_one_base_index_start = g_one_base_rd_len/4 + 1;
    int cdp_one_base_index = cdp_one_base_index_start;
    int cdp_pos_in_contig_start = cdp_one_base_index_start;
    int cdp_start_adj, cdp_end_adj, cdp_end_adj_indel;
    int cdp_cigar_len;
    int cdp_lp_start, cdp_lp_end;
    int cdp_start_pos, cdp_end_pos;
    int cdp_other_loop, cdp_temp_other, cdp_temp_other_mchr;
    int cdp_temp_other_read_start, cdp_temp_other_read_end;
    int cdp_found_other;
    double cdp_temp_other_dist;

	
    int *cdp_one_base_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_rd == NULL )
    {
      printf("58 NULL\n");
      exit(0);
    }

    int *cdp_one_base_conc = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_conc == NULL )
    {
      printf("60 NULL\n");
      exit(0);
    }
    int *cdp_one_base_sc_left = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_sc_left == NULL )
    {
      printf("61 NULL\n");
      exit(0);
    }
    int *cdp_one_base_sc_right = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_sc_right == NULL )
    {
      printf("62 NULL\n");
      exit(0);
    }
    int *cdp_one_base_sc_left_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_sc_left_rd == NULL )
    {
      printf("63 NULL\n");
      exit(0);
    }
    int *cdp_one_base_sc_right_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_sc_right_rd == NULL )
    {
      printf("64 NULL\n");
      exit(0);
    }
    int *cdp_one_base_sc_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_sc_rd == NULL )
    {
      printf("65 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ins = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ins == NULL )
    {
      printf("66 NULL\n");
      exit(0);
    }
    int cdp_one_base_del_set[2];
    cdp_one_base_del_set[0] = 0;
    cdp_one_base_del_set[1] = 0;
    int *cdp_one_base_del_f = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_del_f == NULL )
    {
      printf("67 NULL\n");
      exit(0);
    }
    int *cdp_one_base_del_r = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_del_r == NULL )
    {
      printf("68 NULL\n");
      exit(0);
    }
    double *cdp_one_base_del_fdist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_del_fdist == NULL )
    {
      printf("69 NULL\n");
      exit(0);
    }
    double *cdp_one_base_del_rdist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_del_rdist == NULL )
    {
      printf("70 NULL\n");
      exit(0);
    }
    int *cdp_one_base_del_f_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_del_f_read_start == NULL )
    {
      printf("71 NULL\n");
      exit(0);
    }
    int *cdp_one_base_del_r_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_del_r_read_start == NULL )
    {
      printf("72 NULL\n");
      exit(0);
    }
    int *cdp_one_base_del_f_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_del_f_read_end == NULL )
    {
      printf("73 NULL\n");
      exit(0);
    }
    int *cdp_one_base_del_r_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_del_r_read_end == NULL )
    {
      printf("74 NULL\n");
      exit(0);
    }
    int cdp_one_base_dup_set[2];
    cdp_one_base_dup_set[0] = 0;
    cdp_one_base_dup_set[1] = 0;
    int *cdp_one_base_dup_f = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_dup_f == NULL )
    {
      printf("75 NULL\n");
      exit(0);
    }
    int *cdp_one_base_dup_r = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_dup_r == NULL )
    {
      printf("76 NULL\n");
      exit(0);
    }
    double *cdp_one_base_dup_fdist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_dup_fdist == NULL )
    {
      printf("77 NULL\n");
      exit(0);
    }
    double *cdp_one_base_dup_rdist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_dup_rdist == NULL )
    {
      printf("78 NULL\n");
      exit(0);
    }
    int *cdp_one_base_dup_f_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_dup_f_read_start == NULL )
    {
      printf("79 NULL\n");
      exit(0);
    }
    int *cdp_one_base_dup_r_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_dup_r_read_start == NULL )
    {
      printf("80 NULL\n");
      exit(0);
    }
    int *cdp_one_base_dup_f_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_dup_f_read_end == NULL )
    {
      printf("81 NULL\n");
      exit(0);
    }
    int *cdp_one_base_dup_r_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_dup_r_read_end == NULL )
    {
      printf("82 NULL\n");
      exit(0);
    }
    int cdp_one_base_inv_f_set[2];
    cdp_one_base_inv_f_set[0] = 0;
    cdp_one_base_inv_f_set[1] = 0;
    int *cdp_one_base_inv_f1 = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_f1 == NULL )
    {
      printf("83 NULL\n");
      exit(0);
    }
    int cdp_one_base_inv_r_set[2];
    cdp_one_base_inv_r_set[0] = 0;
    cdp_one_base_inv_r_set[1] = 0;
    int *cdp_one_base_inv_r1 = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_r1 == NULL )
    {
      printf("84 NULL\n");
      exit(0);
    }
    double *cdp_one_base_inv_f1dist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_inv_f1dist == NULL )
    {
      printf("85 NULL\n");
      exit(0);
    }
    double *cdp_one_base_inv_r1dist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_inv_r1dist == NULL )
    {
      printf("86 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_f1_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_f1_read_start == NULL )
    {
      printf("87 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_r1_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_r1_read_start == NULL )
    {
      printf("88 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_f1_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_f1_read_end == NULL )
    {
      printf("89 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_r1_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_r1_read_end == NULL )
    {
      printf("90 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_f2 = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_f2 == NULL )
    {
      printf("91 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_r2 = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_r2 == NULL )
    {
      printf("92 NULL\n");
      exit(0);
    }
    double *cdp_one_base_inv_f2dist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_inv_f2dist == NULL )
    {
      printf("93 NULL\n");
      exit(0);
    }
    double *cdp_one_base_inv_r2dist = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_inv_r2dist == NULL )
    {
      printf("94 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_f2_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_f2_read_start == NULL )
    {
      printf("95 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_r2_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_r2_read_start == NULL )
    {
      printf("96 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_f2_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_f2_read_end == NULL )
    {
      printf("97 NULL\n");
      exit(0);
    }
    int *cdp_one_base_inv_r2_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_inv_r2_read_end == NULL )
    {
      printf("98 NULL\n");
      exit(0);
    }
    int cdp_one_base_ctx_f_set[2];
    cdp_one_base_ctx_f_set[0] = 0;
    cdp_one_base_ctx_f_set[1] = 0;
    int *cdp_one_base_ctx_f = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_f == NULL )
    {
      printf("99 NULL\n");
      exit(0);
    }
    int cdp_one_base_ctx_r_set[2];
    cdp_one_base_ctx_r_set[0] = 0;
    cdp_one_base_ctx_r_set[1] = 0;
    int *cdp_one_base_ctx_r = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_r == NULL )
    {
      printf("100 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_f_mchr = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_f_mchr == NULL )
    {
      printf("101 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_r_mchr = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_r_mchr == NULL )
    {
      printf("102 NULL\n");
      exit(0);
    }
    double *cdp_one_base_ctx_f_mpos = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_ctx_f_mpos == NULL )
    {
      printf("103 NULL\n");
      exit(0);
    }
    double *cdp_one_base_ctx_r_mpos = malloc(g_one_base_rd_len * sizeof(double));
    if( cdp_one_base_ctx_r_mpos == NULL )
    {
      printf("104 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_f_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_f_read_start == NULL )
    {
      printf("105 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_r_read_start = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_r_read_start == NULL )
    {
      printf("106 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_f_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_f_read_end == NULL )
    {
      printf("107 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_r_read_end = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_r_read_end == NULL )
    {
      printf("108 NULL\n");
      exit(0);
    }
    int cdp_one_base_munmapped_f_set[2];
    cdp_one_base_munmapped_f_set[0] = 0;
    cdp_one_base_munmapped_f_set[1] = 0;
    int *cdp_one_base_munmapped_f = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_munmapped_f == NULL )
    {
      printf("109 NULL\n");
      exit(0);
    }
    int cdp_one_base_munmapped_r_set[2];
    cdp_one_base_munmapped_r_set[0] = 0;
    cdp_one_base_munmapped_r_set[1] = 0;
    int *cdp_one_base_munmapped_r = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_munmapped_r == NULL )
    {
      printf("110 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_i = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_i == NULL )
    {
      printf("111 NULL\n");
      exit(0);
    }
    int cdp_one_base_indel_i_seq_set[g_indel_i_seq_len][2];
    for(cdp_a_loop=0;cdp_a_loop<g_indel_i_seq_len;cdp_a_loop++)
    {
      cdp_one_base_indel_i_seq_set[cdp_a_loop][0] = 0;
      cdp_one_base_indel_i_seq_set[cdp_a_loop][1] = 0;
    }
    char **cdp_one_base_indel_i_seq;
    cdp_one_base_indel_i_seq = malloc(g_indel_i_seq_len*sizeof(char*));
    for(cdp_a_loop=0;cdp_a_loop<g_indel_i_seq_len;cdp_a_loop++)
    {
      cdp_one_base_indel_i_seq[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(char));
    }
    int *cdp_one_base_indel_idist = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_idist == NULL )
    {
      printf("112 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_d_f = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_d_f == NULL )
    {
      printf("113 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_d_fdist = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_d_fdist == NULL )
    {
      printf("114 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_d_f_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_d_f_rd == NULL )
    {
      printf("115 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_d_r = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_d_r == NULL )
    {
      printf("116 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_d_rdist = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_d_rdist == NULL )
    {
      printf("117 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_d_r_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_d_r_rd == NULL )
    {
      printf("118 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_sc_left = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_sc_left == NULL )
    {
      printf("119 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_sc_right = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_sc_right == NULL )
    {
      printf("120 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_sc_left_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_sc_left_rd == NULL )
    {
      printf("121 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_sc_right_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_sc_right_rd == NULL )
    {
      printf("122 NULL\n");
      exit(0);
    }
    int *cdp_one_base_ctx_sc_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_ctx_sc_rd == NULL )
    {
      printf("123 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_sc_left = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_sc_left == NULL )
    {
      printf("124 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_sc_right = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_sc_right == NULL )
    {
      printf("125 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_sc_left_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_sc_left_rd == NULL )
    {
      printf("126 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_sc_right_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_sc_right_rd == NULL )
    {
      printf("127 NULL\n");
      exit(0);
    }
    int *cdp_one_base_indel_sc_rd = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_indel_sc_rd == NULL )
    {
      printf("128 NULL\n");
      exit(0);
    }
    int *cdp_one_base_bq = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_bq == NULL )
    {
      printf("129 NULL\n");
      exit(0);
    }
    int *cdp_one_base_bq_all = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_bq_all == NULL )
    {
      printf("130 NULL\n");
      exit(0);
    }
    int *cdp_one_base_mq = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_mq == NULL )
    {
      printf("131 NULL\n");
      exit(0);
    }
    int *cdp_one_base_mq_all = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_mq_all == NULL )
    {
      printf("132 NULL\n");
      exit(0);
    }
    int *cdp_one_base_bq_read_count = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_bq_read_count == NULL )
    {
      printf("133 NULL\n");
      exit(0);
    }
    int *cdp_one_base_mq_read_count = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_mq_read_count == NULL )
    {
      printf("134 NULL\n");
      exit(0);
    }
    int *cdp_one_base_read_count_all = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_read_count_all == NULL )
    {
      printf("135 NULL\n");
      exit(0);
    }
    int **cdp_one_base_pos_in_read;
    cdp_one_base_pos_in_read = malloc(g_nucleotides*sizeof(int*));
    if( cdp_one_base_pos_in_read == NULL )
    {
      printf("136 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_one_base_pos_in_read[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_pos_in_read[cdp_a_loop] == NULL )
      {
	printf("137 NULL\n");
	exit(0);
      }
    }
    int **cdp_one_base_fstrand;
    cdp_one_base_fstrand = malloc(g_nucleotides*sizeof(int*));
    if( cdp_one_base_fstrand == NULL )
    {
      printf("138 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_one_base_fstrand[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
    if( cdp_one_base_fstrand[cdp_a_loop] == NULL )
    {
      printf("139 NULL\n");
      exit(0);
    }
    }

    int cdp_one_base_other_set[g_other_len][2];
    for(cdp_a_loop=0;cdp_a_loop<g_other_len;cdp_a_loop++)
    {
      cdp_one_base_other_set[cdp_a_loop][0] = 0;
      cdp_one_base_other_set[cdp_a_loop][1] = 0;
    }
    int **cdp_one_base_other;
    cdp_one_base_other = malloc(g_other_len*sizeof(int*));
    if( cdp_one_base_other == NULL )
    {
      printf("140 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_other_len;cdp_a_loop++)
    {
      cdp_one_base_other[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_other[cdp_a_loop] == NULL )
      {
	printf("141 NULL\n");
	exit(0);
      }
    }
    int **cdp_one_base_other_type;
    cdp_one_base_other_type = malloc(g_other_len*sizeof(int*));
    if( cdp_one_base_other_type == NULL )
    {
      printf("142 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_other_len;cdp_a_loop++)
    {
      cdp_one_base_other_type[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_other_type[cdp_a_loop] == NULL )
      {
	printf("143 NULL\n");
	exit(0);
      }
    }
    int **cdp_one_base_other_mchr;
    cdp_one_base_other_mchr = malloc(g_other_len*sizeof(int*));
    if( cdp_one_base_other_mchr == NULL )
    {
      printf("144 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_other_len;cdp_a_loop++)
    {
      cdp_one_base_other_mchr[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_other_mchr[cdp_a_loop] == NULL )
      {
	printf("145 NULL\n");
	exit(0);
      }
    }
    double **cdp_one_base_other_dist;
    cdp_one_base_other_dist = malloc(g_other_len*sizeof(double*));
    if( cdp_one_base_other_dist == NULL )
    {
      printf("146 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_other_len;cdp_a_loop++)
    {
      cdp_one_base_other_dist[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(double));
      if( cdp_one_base_other_dist[cdp_a_loop] == NULL )
      {
	printf("147 NULL\n");
	exit(0);
      }
    }
    int **cdp_one_base_other_read_start;
    cdp_one_base_other_read_start = malloc(g_other_len*sizeof(int*));
    if( cdp_one_base_other_read_start == NULL )
    {
      printf("148 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_other_len;cdp_a_loop++)
    {
      cdp_one_base_other_read_start[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_other_read_start[cdp_a_loop] == NULL )
      {
	printf("149 NULL\n");
	exit(0);
      }
    }
    int **cdp_one_base_other_read_end;
    cdp_one_base_other_read_end = malloc(g_other_len*sizeof(int*));
    if( cdp_one_base_other_read_end == NULL )
    {
      printf("150 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_other_len;cdp_a_loop++)
    {
      cdp_one_base_other_read_end[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_other_read_end[cdp_a_loop] == NULL )
      {
	printf("151 NULL\n");
	exit(0);
      }
    }

    int **cdp_one_base_snv;
    cdp_one_base_snv = malloc(g_nucleotides*sizeof(int*));
    if( cdp_one_base_snv == NULL )
    {
      printf("152 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_one_base_snv[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_snv[cdp_a_loop] == NULL )
      {
	printf("153 NULL\n");
	exit(0);
      }
    }
    int **cdp_one_base_snv_lowmq;
    cdp_one_base_snv_lowmq = malloc(g_nucleotides*sizeof(int*));
    if( cdp_one_base_snv_lowmq == NULL )
    {
      printf("154 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_one_base_snv_lowmq[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_snv_lowmq[cdp_a_loop] == NULL )
      {
	printf("155 NULL\n");
	exit(0);
      }
    }
    int **cdp_one_base_snv_sc;
    cdp_one_base_snv_sc = malloc(g_nucleotides*sizeof(int*));
    if( cdp_one_base_snv_sc == NULL )
    {
      printf("156 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_one_base_snv_sc[cdp_a_loop] = malloc(g_one_base_rd_len * sizeof(int));
      if( cdp_one_base_snv_sc[cdp_a_loop] == NULL )
      {
	printf("157 NULL\n");
	exit(0);
      }
    }

    char ***cdp_one_base_snv_read_names;
    if( (cdp_one_base_snv_read_names = malloc(g_min_snv * sizeof(char**))) == NULL )
    {
      printf("NULL for ***read_names\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_min_snv;cdp_a_loop++)
    {
      if( (cdp_one_base_snv_read_names[cdp_a_loop] = malloc(g_one_base_rd_len*sizeof(char*))) == NULL )
      {
	printf("null at %d\n", cdp_a_loop);
	exit(0);
      }
      for(cdp_b_loop=0;cdp_b_loop<g_one_base_rd_len;cdp_b_loop++)
      {
	if( (cdp_one_base_snv_read_names[cdp_a_loop][cdp_b_loop] = malloc(g_read_name_len*sizeof(char))) == NULL )
	{
	  printf("null at %d %d\n", cdp_a_loop, cdp_b_loop);
	  exit(0);
	}
      }
    }



    int cdp_lp_temp;
    int cdp_sc_left_rd_temp, cdp_sc_right_rd_temp;
    int cdp_indel_i_temp, cdp_indel_d_f_temp, cdp_indel_d_r_temp, cdp_indel_rd_temp;
    int cdp_temp_snv_base_index;
    double cdp_temp_snv_pir, cdp_temp_snv_fs;
    double cdp_temp_snv_ratio;
    double cdp_binom_cdf;
    double cdp_hez_binom_cdf;
    int cdp_snv_list_index = 0;
    int cdp_ins_list_index = -1;
    int cdp_indel_i_list_index = 0;
    int cdp_indel_d_list_index = -1;
    int cdp_dup_list_index = 0;
    int cdp_del_list_index = 0;
    int cdp_inv_f_list_index = 0;
    int cdp_inv_r_list_index = 0;
    int cdp_ctx_f_list_index = 0;
    int cdp_ctx_r_list_index = 0;
    int cdp_dup_min, cdp_dup_max;
    int cdp_del_min, cdp_del_max;
    int cdp_inv_min, cdp_inv_max;
    double cdp_inv_temp_dist;


    if( g_internal == 1 )
    {
      printf("before allocate SV lists\n");
    }

    int *cdp_snv_pos_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_pos_list == NULL )
    {
      printf("158 NULL\n");
      exit(0);
    }
    int *cdp_snv_base_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_base_list == NULL )
    {
      printf("159 NULL\n");
      exit(0);
    }
    double *cdp_snv_ratio_list = malloc(g_sv_list_len * sizeof(double));
    if( cdp_snv_ratio_list == NULL )
    {
      printf("160 NULL\n");
      exit(0);
    }
    int **cdp_snv_list;
    cdp_snv_list = malloc(g_nucleotides*sizeof(int*));
    if( cdp_snv_list == NULL )
    {
      printf("161 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_snv_list[cdp_a_loop] = malloc(g_sv_list_len * sizeof(int));
      if( cdp_snv_list[cdp_a_loop] == NULL )
      {
	printf("162 NULL\n");
	exit(0);
      }
    }
    int **cdp_snv_lowmq_list;
    cdp_snv_lowmq_list = malloc(g_sv_list_len*sizeof(int*));
    if( cdp_snv_lowmq_list == NULL )
    {
      printf("163 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_snv_lowmq_list[cdp_a_loop] = malloc(g_sv_list_len * sizeof(int));
      if( cdp_snv_lowmq_list[cdp_a_loop] == NULL )
      {
	printf("164 NULL\n");
	exit(0);
      }
    }
    int *cdp_snv_rd_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_rd_list == NULL )
    {
      printf("165 NULL\n");
      exit(0);
    }
    int *cdp_snv_low_mq_rd_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_low_mq_rd_list == NULL )
    {
      printf("166 NULL\n");
      exit(0);
    }
    int *cdp_snv_bq_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_bq_list == NULL )
    {
      printf("167 NULL\n");
      exit(0);
    }
    int *cdp_snv_bq_all_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_bq_all_list == NULL )
    {
      printf("168 NULL\n");
      exit(0);
    }
    int *cdp_snv_mq_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_mq_list == NULL )
    {
      printf("169 NULL\n");
      exit(0);
    }
    int *cdp_snv_mq_all_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_mq_all_list == NULL )
    {
      printf("170 NULL\n");
      exit(0);
    }
    int *cdp_snv_bq_read_count_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_bq_read_count_list == NULL )
    {
      printf("171 NULL\n");
      exit(0);
    }
    int *cdp_snv_mq_read_count_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_mq_read_count_list == NULL )
    {
      printf("172 NULL\n");
      exit(0);
    }
    int *cdp_snv_read_count_all_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_snv_read_count_all_list == NULL )
    {
      printf("173 NULL\n");
      exit(0);
    }

    int **cdp_snv_pos_in_read_list;
    cdp_snv_pos_in_read_list = malloc(g_nucleotides*sizeof(int*));
    if( cdp_snv_pos_in_read_list == NULL )
    {
      printf("174 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_snv_pos_in_read_list[cdp_a_loop] = malloc(g_sv_list_len * sizeof(int));
      if( cdp_snv_pos_in_read_list[cdp_a_loop] == NULL )
      {
	printf("175 NULL\n");
	exit(0);
      }
    }
    int **cdp_snv_fstrand_list;
    cdp_snv_fstrand_list = malloc(g_nucleotides*sizeof(int*));
    if( cdp_snv_fstrand_list == NULL )
    {
      printf("176 NULL\n");
      exit(0);
    }
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      cdp_snv_fstrand_list[cdp_a_loop] = malloc(g_sv_list_len * sizeof(int));
      if( cdp_snv_fstrand_list[cdp_a_loop] == NULL )
      {
	printf("177 NULL\n");
	exit(0);
      }
    }

    double *cdp_snv_start_binom_cdf_list = malloc(g_sv_list_len * sizeof(double));
    if( cdp_snv_start_binom_cdf_list == NULL )
    {
      printf("178 NULL\n");
      exit(0);
    }
    double *cdp_snv_start_hez_binom_cdf_list = malloc(g_sv_list_len * sizeof(double));
    if( cdp_snv_start_hez_binom_cdf_list == NULL )
    {
      printf("179 NULL\n");
      exit(0);
    }

    int *cdp_ins_list_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_start == NULL )
    {
      printf("180 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_start_ins = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_start_ins == NULL )
    {
      printf("181 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_start_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_start_rd == NULL )
    {
      printf("182 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_start_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_start_conc == NULL )
    {
      printf("183 NULL\n");
      exit(0);
    }
    double *cdp_ins_list_start_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_ins_list_start_binom_cdf == NULL )
    {
      printf("184 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_end == NULL )
    {
      printf("185 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_end_ins = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_end_ins == NULL )
    {
      printf("186 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_end_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_end_rd == NULL )
    {
      printf("187 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_end_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_end_conc == NULL )
    {
      printf("188 NULL\n");
      exit(0);
    }
    double *cdp_ins_list_end_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_ins_list_end_binom_cdf == NULL )
    {
      printf("189 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_start_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_start_other_len == NULL )
    {
      printf("190 NULL\n");
      exit(0);
    }
    int *cdp_ins_list_end_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ins_list_end_other_len == NULL )
    {
      printf("191 NULL\n");
      exit(0);
    }

    int *cdp_dup_list_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_start == NULL )
    {
      printf("192 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_start_dup_r = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_start_dup_r == NULL )
    {
      printf("193 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_start_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_start_rd == NULL )
    {
      printf("194 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_start_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_start_conc == NULL )
    {
      printf("195 NULL\n");
      exit(0);
    }
    double *cdp_dup_list_start_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_dup_list_start_binom_cdf == NULL )
    {
      printf("196 NULL\n");
      exit(0);
    }
    double *cdp_dup_list_start_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_dup_list_start_hez_binom_cdf == NULL )
    {
      printf("197 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_end == NULL )
    {
      printf("198 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_end_dup_f = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_end_dup_f == NULL )
    {
      printf("199 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_end_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_end_rd == NULL )
    {
      printf("200 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_end_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_end_conc == NULL )
    {
      printf("201 NULL\n");
      exit(0);
    }
    double *cdp_dup_list_end_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_dup_list_end_binom_cdf == NULL )
    {
      printf("202 NULL\n");
      exit(0);
    }
    double *cdp_dup_list_end_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_dup_list_end_hez_binom_cdf == NULL )
    {
      printf("203 NULL\n");
      exit(0);
    }
    double *cdp_dup_list_dist = malloc(g_sv_list_len * sizeof(double));
    if( cdp_dup_list_dist == NULL )
    {
      printf("204 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_start_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_start_other_len == NULL )
    {
      printf("205 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_end_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_end_other_len == NULL )
    {
      printf("206 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_start_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_start_read_start == NULL )
    {
      printf("207 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_start_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_start_read_end == NULL )
    {
      printf("208 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_end_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_end_read_start == NULL )
    {
      printf("209 NULL\n");
      exit(0);
    }
    int *cdp_dup_list_end_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_dup_list_end_read_end == NULL )
    {
      printf("210 NULL\n");
      exit(0);
    }

    int *cdp_del_list_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_start == NULL )
    {
      printf("211 NULL\n");
      exit(0);
    }
    int *cdp_del_list_start_del_f = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_start_del_f == NULL )
    {
      printf("212 NULL\n");
      exit(0);
    }
    int *cdp_del_list_start_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_start_rd == NULL )
    {
      printf("213 NULL\n");
      exit(0);
    }
    int *cdp_del_list_start_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_start_conc == NULL )
    {
      printf("214 NULL\n");
      exit(0);
    }
    double *cdp_del_list_start_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_del_list_start_binom_cdf == NULL )
    {
      printf("215 NULL\n");
      exit(0);
    }
    double *cdp_del_list_start_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_del_list_start_hez_binom_cdf == NULL )
    {
      printf("216 NULL\n");
      exit(0);
    }
    int *cdp_del_list_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_end == NULL )
    {
      printf("217 NULL\n");
      exit(0);
    }
    int *cdp_del_list_end_del_r = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_end_del_r == NULL )
    {
      printf("218 NULL\n");
      exit(0);
    }
    int *cdp_del_list_end_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_end_rd == NULL )
    {
      printf("219 NULL\n");
      exit(0);
    }
    int *cdp_del_list_end_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_end_conc == NULL )
    {
      printf("220 NULL\n");
      exit(0);
    }
    double *cdp_del_list_end_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_del_list_end_binom_cdf == NULL )
    {
      printf("221 NULL\n");
      exit(0);
    }
    double *cdp_del_list_end_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_del_list_end_hez_binom_cdf == NULL )
    {
      printf("222 NULL\n");
      exit(0);
    }
    double *cdp_del_list_dist = malloc(g_sv_list_len * sizeof(double));
    if( cdp_del_list_dist == NULL )
    {
      printf("223 NULL\n");
      exit(0);
    }
    int *cdp_del_list_start_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_start_other_len == NULL )
    {
      printf("224 NULL\n");
      exit(0);
    }
    int *cdp_del_list_end_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_end_other_len == NULL )
    {
      printf("225 NULL\n");
      exit(0);
    }
    int *cdp_del_list_start_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_start_read_start == NULL )
    {
      printf("226 NULL\n");
      exit(0);
    }
    int *cdp_del_list_start_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_start_read_end == NULL )
    {
      printf("227 NULL\n");
      exit(0);
    }
    int *cdp_del_list_end_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_end_read_start == NULL )
    {
      printf("228 NULL\n");
      exit(0);
    }
    int *cdp_del_list_end_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_del_list_end_read_end == NULL )
    {
      printf("229 NULL\n");
      exit(0);
    }

    int *cdp_inv_f_list_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_start == NULL )
    {
      printf("230 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_start_inv = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_start_inv == NULL )
    {
      printf("231 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_start_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_start_rd == NULL )
    {
      printf("232 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_start_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_start_conc == NULL )
    {
      printf("233 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list_start_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_f_list_start_binom_cdf == NULL )
    {
      printf("234 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list_start_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_f_list_start_hez_binom_cdf == NULL )
    {
      printf("235 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_end == NULL )
    {
      printf("236 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_end_inv = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_end_inv == NULL )
    {
      printf("237 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_end_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_end_rd == NULL )
    {
      printf("238 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_end_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_end_conc == NULL )
    {
      printf("239 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list_end_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_f_list_end_binom_cdf == NULL )
    {
      printf("240 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list_end_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_f_list_end_hez_binom_cdf == NULL )
    {
      printf("241 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list_dist = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_f_list_dist == NULL )
    {
      printf("242 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_start_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_start_other_len == NULL )
    {
      printf("243 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_end_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_end_other_len == NULL )
    {
      printf("244 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_start_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_start_read_start == NULL )
    {
      printf("245 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_start_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_start_read_end == NULL )
    {
      printf("246 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_end_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_end_read_start == NULL )
    {
      printf("247 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list_end_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_f_list_end_read_end == NULL )
    {
      printf("248 NULL\n");
      exit(0);
    }

    int *cdp_inv_r_list_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_start == NULL )
    {
      printf("249 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_start_inv = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_start_inv == NULL )
    {
      printf("250 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_start_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_start_rd == NULL )
    {
      printf("251 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_start_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_start_conc == NULL )
    {
      printf("252 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list_start_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_r_list_start_binom_cdf == NULL )
    {
      printf("253 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list_start_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_r_list_start_hez_binom_cdf == NULL )
    {
      printf("254 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_end == NULL )
    {
      printf("255 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_end_inv = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_end_inv == NULL )
    {
      printf("256 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_end_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_end_rd == NULL )
    {
      printf("257 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_end_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_end_conc == NULL )
    {
      printf("258 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list_end_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_r_list_end_binom_cdf == NULL )
    {
      printf("259 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list_end_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_r_list_end_hez_binom_cdf == NULL )
    {
      printf("260 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list_dist = malloc(g_sv_list_len * sizeof(double));
    if( cdp_inv_r_list_dist == NULL )
    {
      printf("261 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_start_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_start_other_len == NULL )
    {
      printf("262 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_end_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_end_other_len == NULL )
    {
      printf("263 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_start_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_start_read_start == NULL )
    {
      printf("264 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_start_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_start_read_end == NULL )
    {
      printf("265 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_end_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_end_read_start == NULL )
    {
      printf("266 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list_end_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_inv_r_list_end_read_end == NULL )
    {
      printf("267 NULL\n");
      exit(0);
    }

    char **cdp_indel_i_list_seq;
    cdp_indel_i_list_seq = malloc(g_indel_i_seq_len*sizeof(char*));
    for(cdp_a_loop=0;cdp_a_loop<g_indel_i_seq_len;cdp_a_loop++)
    {
      cdp_indel_i_list_seq[cdp_a_loop] = malloc(g_sv_list_len * sizeof(char));
    }

    int *cdp_indel_i_list_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_start == NULL )
    {
      printf("268 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_start_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_start_conc == NULL )
    {
      printf("269 NULL\n");
      exit(0);
    }
    double *cdp_indel_i_list_start_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_indel_i_list_start_binom_cdf == NULL )
    {
      printf("270 NULL\n");
      exit(0);
    }
    double *cdp_indel_i_list_start_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_indel_i_list_start_hez_binom_cdf == NULL )
    {
      printf("271 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_end == NULL )
    {
      printf("272 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_end_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_end_conc == NULL )
    {
      printf("273 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_dist = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_dist == NULL )
    {
      printf("274 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_start_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_start_other_len == NULL )
    {
      printf("275 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_end_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_end_other_len == NULL )
    {
      printf("276 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_start_i = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_start_i == NULL )
    {
      printf("277 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_start_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_start_rd == NULL )
    {
      printf("278 NULL\n");
      exit(0);
    }
    int *cdp_indel_i_list_start_sc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_i_list_start_sc == NULL )
    {
      printf("279 NULL\n");
      exit(0);
    }

    int *cdp_indel_d_list_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_start == NULL )
    {
      printf("280 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_start_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_start_conc == NULL )
    {
      printf("281 NULL\n");
      exit(0);
    }
    double *cdp_indel_d_list_start_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_indel_d_list_start_binom_cdf == NULL )
    {
      printf("282 NULL\n");
      exit(0);
    }
    double *cdp_indel_d_list_start_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_indel_d_list_start_hez_binom_cdf == NULL )
    {
      printf("283 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_end == NULL )
    {
      printf("284 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_end_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_end_conc == NULL )
    {
      printf("285 NULL\n");
      exit(0);
    }
    double *cdp_indel_d_list_end_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_indel_d_list_end_binom_cdf == NULL )
    {
      printf("286 NULL\n");
      exit(0);
    }
    double *cdp_indel_d_list_end_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_indel_d_list_end_hez_binom_cdf == NULL )
    {
      printf("287 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_start_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_start_other_len == NULL )
    {
      printf("288 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_end_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_end_other_len == NULL )
    {
      printf("289 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_start_f = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_start_f == NULL )
    {
      printf("290 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_end_r = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_end_r == NULL )
    {
      printf("291 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_start_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_start_rd == NULL )
    {
      printf("292 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_end_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_end_rd == NULL )
    {
      printf("293 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_start_sc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_start_sc == NULL )
    {
      printf("294 NULL\n");
      exit(0);
    }
    int *cdp_indel_d_list_end_sc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_indel_d_list_end_sc == NULL )
    {
      printf("295 NULL\n");
      exit(0);
    }

    int *cdp_ctx_f_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list == NULL )
    {
      printf("296 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_ctx_f = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_ctx_f == NULL )
    {
      printf("297 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_rd == NULL )
    {
      printf("298 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_conc == NULL )
    {
      printf("299 NULL\n");
      exit(0);
    }
    double *cdp_ctx_f_list_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_ctx_f_list_binom_cdf == NULL )
    {
      printf("300 NULL\n");
      exit(0);
    }
    double *cdp_ctx_f_list_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_ctx_f_list_hez_binom_cdf == NULL )
    {
      printf("301 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_mchr = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_mchr == NULL )
    {
      printf("302 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_mpos = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_mpos == NULL )
    {
      printf("303 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_other_len == NULL )
    {
      printf("304 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_read_start == NULL )
    {
      printf("305 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_f_list_read_end == NULL )
    {
      printf("306 NULL\n");
      exit(0);
    }

    int *cdp_ctx_r_list = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list == NULL )
    {
      printf("307 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_ctx_r = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_ctx_r == NULL )
    {
      printf("308 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_rd = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_rd == NULL )
    {
      printf("309 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_conc = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_conc == NULL )
    {
      printf("310 NULL\n");
      exit(0);
    }
    double *cdp_ctx_r_list_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_ctx_r_list_binom_cdf == NULL )
    {
      printf("311 NULL\n");
      exit(0);
    }
    double *cdp_ctx_r_list_hez_binom_cdf = malloc(g_sv_list_len * sizeof(double));
    if( cdp_ctx_r_list_hez_binom_cdf == NULL )
    {
      printf("312 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_mchr = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_mchr == NULL )
    {
      printf("313 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_mpos = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_mpos == NULL )
    {
      printf("314 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_other_len = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_other_len == NULL )
    {
      printf("315 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_read_start = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_read_start == NULL )
    {
      printf("316 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list_read_end = malloc(g_sv_list_len * sizeof(int));
    if( cdp_ctx_r_list_read_end == NULL )
    {
      printf("317 NULL\n");
      exit(0);
    }

    int *cdp_ins_list2_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_start == NULL )
    {
      printf("318 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_start_ins = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_start_ins == NULL )
    {
      printf("319 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_start_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_start_rd == NULL )
    {
      printf("320 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_start_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_start_conc == NULL )
    {
      printf("321 NULL\n");
      exit(0);
    }
    double *cdp_ins_list2_start_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_ins_list2_start_binom_cdf == NULL )
    {
      printf("322 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_end == NULL )
    {
      printf("323 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_end_ins = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_end_ins == NULL )
    {
      printf("324 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_end_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_end_rd == NULL )
    {
      printf("325 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_end_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_end_conc == NULL )
    {
      printf("326 NULL\n");
      exit(0);
    }
    double *cdp_ins_list2_end_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_ins_list2_end_binom_cdf == NULL )
    {
      printf("327 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_start_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_start_other_len == NULL )
    {
      printf("328 NULL\n");
      exit(0);
    }
    int *cdp_ins_list2_end_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ins_list2_end_other_len == NULL )
    {
      printf("329 NULL\n");
      exit(0);
    }

    int *cdp_dup_list2_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_start == NULL )
    {
      printf("330 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_start_dup_r = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_start_dup_r == NULL )
    {
      printf("331 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_start_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_start_rd == NULL )
    {
      printf("332 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_start_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_start_conc == NULL )
    {
      printf("333 NULL\n");
      exit(0);
    }
    double *cdp_dup_list2_start_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_dup_list2_start_binom_cdf == NULL )
    {
      printf("334 NULL\n");
      exit(0);
    }
    double *cdp_dup_list2_start_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_dup_list2_start_hez_binom_cdf == NULL )
    {
      printf("335 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_end == NULL )
    {
      printf("336 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_end_dup_f = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_end_dup_f == NULL )
    {
      printf("337 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_end_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_end_rd == NULL )
    {
      printf("338 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_end_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_end_conc == NULL )
    {
      printf("339 NULL\n");
      exit(0);
    }
    double *cdp_dup_list2_end_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_dup_list2_end_binom_cdf == NULL )
    {
      printf("340 NULL\n");
      exit(0);
    }
    double *cdp_dup_list2_end_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_dup_list2_end_hez_binom_cdf == NULL )
    {
      printf("341 NULL\n");
      exit(0);
    }
    double *cdp_dup_list2_dist = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_dup_list2_dist == NULL )
    {
      printf("342 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_start_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_start_other_len == NULL )
    {
      printf("343 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_end_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_end_other_len == NULL )
    {
      printf("344 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_start_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_start_read_start == NULL )
    {
      printf("345 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_start_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_start_read_end == NULL )
    {
      printf("346 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_end_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_end_read_start == NULL )
    {
      printf("347 NULL\n");
      exit(0);
    }
    int *cdp_dup_list2_end_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_dup_list2_end_read_end == NULL )
    {
      printf("348 NULL\n");
      exit(0);
    }

    int *cdp_del_list2_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_start == NULL )
    {
      printf("349 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_start_del_f = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_start_del_f == NULL )
    {
      printf("350 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_start_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_start_rd == NULL )
    {
      printf("351 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_start_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_start_conc == NULL )
    {
      printf("352 NULL\n");
      exit(0);
    }
    double *cdp_del_list2_start_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_del_list2_start_binom_cdf == NULL )
    {
      printf("353 NULL\n");
      exit(0);
    }
    double *cdp_del_list2_start_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_del_list2_start_hez_binom_cdf == NULL )
    {
      printf("354 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_end == NULL )
    {
      printf("349 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_end_del_r = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_end_del_r == NULL )
    {
      printf("350 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_end_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_end_rd == NULL )
    {
      printf("351 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_end_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_end_conc == NULL )
    {
      printf("352 NULL\n");
      exit(0);
    }
    double *cdp_del_list2_end_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_del_list2_end_binom_cdf == NULL )
    {
      printf("353 NULL\n");
      exit(0);
    }
    double *cdp_del_list2_end_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_del_list2_end_hez_binom_cdf == NULL )
    {
      printf("354 NULL\n");
      exit(0);
    }
    double *cdp_del_list2_dist = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_del_list2_dist == NULL )
    {
      printf("355 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_start_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_start_other_len == NULL )
    {
      printf("356 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_end_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_end_other_len == NULL )
    {
      printf("357 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_start_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_start_read_start == NULL )
    {
      printf("358 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_start_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_start_read_end == NULL )
    {
      printf("359 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_end_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_end_read_start == NULL )
    {
      printf("360 NULL\n");
      exit(0);
    }
    int *cdp_del_list2_end_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_del_list2_end_read_end == NULL )
    {
      printf("361 NULL\n");
      exit(0);
    }

    int *cdp_inv_f_list2_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_start == NULL )
    {
      printf("362 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_start_inv = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_start_inv == NULL )
    {
      printf("363 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_start_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_start_rd == NULL )
    {
      printf("364 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_start_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_start_conc == NULL )
    {
      printf("365 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list2_start_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_f_list2_start_binom_cdf == NULL )
    {
      printf("366 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list2_start_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_f_list2_start_hez_binom_cdf == NULL )
    {
      printf("367 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_end == NULL )
    {
      printf("368 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_end_inv = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_end_inv == NULL )
    {
      printf("369 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_end_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_end_rd == NULL )
    {
      printf("370 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_end_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_end_conc == NULL )
    {
      printf("371 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list2_end_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_f_list2_end_binom_cdf == NULL )
    {
      printf("372 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list2_end_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_f_list2_end_hez_binom_cdf == NULL )
    {
      printf("373 NULL\n");
      exit(0);
    }
    double *cdp_inv_f_list2_dist = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_f_list2_dist == NULL )
    {
      printf("374 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_start_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_start_other_len == NULL )
    {
      printf("375 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_end_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_end_other_len == NULL )
    {
      printf("376 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_start_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_start_read_start == NULL )
    {
      printf("377 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_start_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_start_read_end == NULL )
    {
      printf("378 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_end_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_end_read_start == NULL )
    {
      printf("379 NULL\n");
      exit(0);
    }
    int *cdp_inv_f_list2_end_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_f_list2_end_read_end == NULL )
    {
      printf("380 NULL\n");
      exit(0);
    }

    int *cdp_inv_r_list2_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_start == NULL )
    {
      printf("381 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_start_inv = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_start_inv == NULL )
    {
      printf("382 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_start_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_start_rd == NULL )
    {
      printf("383 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_start_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_start_conc == NULL )
    {
      printf("384 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list2_start_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_r_list2_start_binom_cdf == NULL )
    {
      printf("385 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list2_start_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_r_list2_start_hez_binom_cdf == NULL )
    {
      printf("386 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_end == NULL )
    {
      printf("387 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_end_inv = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_end_inv == NULL )
    {
      printf("388 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_end_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_end_rd == NULL )
    {
      printf("389 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_end_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_end_conc == NULL )
    {
      printf("390 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list2_end_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_r_list2_end_binom_cdf == NULL )
    {
      printf("391 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list2_end_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_r_list2_end_hez_binom_cdf == NULL )
    {
      printf("392 NULL\n");
      exit(0);
    }
    double *cdp_inv_r_list2_dist = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_inv_r_list2_dist == NULL )
    {
      printf("393 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_start_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_start_other_len == NULL )
    {
      printf("394 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_end_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_end_other_len == NULL )
    {
      printf("395 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_start_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_start_read_start == NULL )
    {
      printf("396 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_start_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_start_read_end == NULL )
    {
      printf("397 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_end_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_end_read_start == NULL )
    {
      printf("398 NULL\n");
      exit(0);
    }
    int *cdp_inv_r_list2_end_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_inv_r_list2_end_read_end == NULL )
    {
      printf("399 NULL\n");
      exit(0);
    }

    int *cdp_ctx_f_list2 = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2 == NULL )
    {
      printf("400 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_ctx_f = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_ctx_f == NULL )
    {
      printf("401 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_rd == NULL )
    {
      printf("402 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_conc == NULL )
    {
      printf("403 NULL\n");
      exit(0);
    }
    double *cdp_ctx_f_list2_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_ctx_f_list2_binom_cdf == NULL )
    {
      printf("404 NULL\n");
      exit(0);
    }
    double *cdp_ctx_f_list2_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_ctx_f_list2_hez_binom_cdf == NULL )
    {
      printf("405 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_mchr = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_mchr == NULL )
    {
      printf("406 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_mpos = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_mpos == NULL )
    {
      printf("407 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_other_len == NULL )
    {
      printf("408 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_read_start == NULL )
    {
      printf("409 NULL\n");
      exit(0);
    }
    int *cdp_ctx_f_list2_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_f_list2_read_end == NULL )
    {
      printf("410 NULL\n");
      exit(0);
    }

    int *cdp_ctx_r_list2 = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2 == NULL )
    {
      printf("411 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_ctx_r = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_ctx_r == NULL )
    {
      printf("412 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_rd = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_rd == NULL )
    {
      printf("412 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_conc = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_conc == NULL )
    {
      printf("413 NULL\n");
      exit(0);
    }
    double *cdp_ctx_r_list2_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_ctx_r_list2_binom_cdf == NULL )
    {
      printf("414 NULL\n");
      exit(0);
    }
    double *cdp_ctx_r_list2_hez_binom_cdf = malloc(g_sv_list2_len * sizeof(double));
    if( cdp_ctx_r_list2_hez_binom_cdf == NULL )
    {
      printf("415 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_mchr = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_mchr == NULL )
    {
      printf("416 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_mpos = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_mpos == NULL )
    {
      printf("417 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_other_len = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_other_len == NULL )
    {
      printf("418 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_read_start = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_read_start == NULL )
    {
      printf("419 NULL\n");
      exit(0);
    }
    int *cdp_ctx_r_list2_read_end = malloc(g_sv_list2_len * sizeof(int));
    if( cdp_ctx_r_list2_read_end == NULL )
    {
      printf("420 NULL\n");
      exit(0);
    }


    if( g_internal == 1 )
    {
      printf("after allocate SV lists\n");
    }


    int *cdp_rmdup_mchr_list = malloc(g_rmdup_list_len * sizeof(int));
    if( cdp_rmdup_mchr_list == NULL )
    {
      printf("421 NULL\n");
      exit(0);
    }
    int *cdp_rmdup_mpos_list = malloc(g_rmdup_list_len * sizeof(int));
    if( cdp_rmdup_mpos_list == NULL )
    {
      printf("422 NULL\n");
      exit(0);
    }
    int *cdp_rmdup_lseq_list = malloc(g_rmdup_list_len * sizeof(int));
    if( cdp_rmdup_lseq_list == NULL )
    {
      printf("423 NULL\n");
      exit(0);
    }
    int *cdp_rmdup_tlen_list = malloc(g_rmdup_list_len * sizeof(int));
    if( cdp_rmdup_tlen_list == NULL )
    {
      printf("424 NULL\n");
      exit(0);
    }
    int *cdp_rmdup_svtype_list = malloc(g_rmdup_list_len * sizeof(int));
    if( cdp_rmdup_svtype_list == NULL )
    {
      printf("425 NULL\n");
      exit(0);
    }



	vcf_chr_pos_data_t* vcf_indel_data_list = malloc(sizeof(vcf_chr_pos_data_t) * g_sv_list_len);

	if( vcf_indel_data_list == NULL )
    {
      printf("426 NULL\n");
      exit(0);
    }

	vcf_chr_pos_data_t* vcf_sv_data_list = malloc(sizeof(vcf_chr_pos_data_t) * g_sv_list_len);
	if( vcf_sv_data_list == NULL )
    {
      printf("427 NULL\n");
      exit(0);
    }




    int cdp_rmdup_a;
    int cdp_rmdup_index = 0;
    int cdp_add_to_list;
    int cdp_svtype;
    int cdp_old_pos = -1;

    int caf_add_to_list;



    int cdp_ins_list2_index = 0;
    int cdp_ins_begin;
    int cdp_dup_list2_index = 0;
    int cdp_dup_begin;
    int cdp_del_list2_index = 0;
    int cdp_del_begin;
    int cdp_inv_begin;
    int cdp_inv_f_list2_index = 0;
    int cdp_inv_r_list2_index = 0;
    int cdp_ctx_f_list2_index;
    int cdp_ctx_f_begin;
    int cdp_ctx_r_list2_index;
    int cdp_ctx_r_begin;
    int cdp_first_start;
    int cdp_last_start;
    int cdp_first_end;
    int cdp_last_end;
    double cdp_first_dist;
    double cdp_last_dist;
    double cdp_max_binom_cdf, cdp_max_binom_cdf2;




    for(cdp_a_loop=0;cdp_a_loop<g_sv_list_len;cdp_a_loop++)
    {
      cdp_ins_list_start[cdp_a_loop] = -1;
      cdp_ins_list_end[cdp_a_loop] = -1;
      cdp_dup_list_start[cdp_a_loop] = -1;
      cdp_dup_list_end[cdp_a_loop] = -1;
      cdp_del_list_start[cdp_a_loop] = -1;
      cdp_del_list_end[cdp_a_loop] = -1;
      cdp_inv_f_list_start[cdp_a_loop] = -1;
      cdp_inv_f_list_end[cdp_a_loop] = -1;
      cdp_inv_r_list_start[cdp_a_loop] = -1;
      cdp_inv_r_list_end[cdp_a_loop] = -1;
      cdp_ctx_f_list[cdp_a_loop] = -1;
      cdp_ctx_r_list[cdp_a_loop] = -1;
      cdp_indel_i_list_start[cdp_a_loop] = -1;
      cdp_indel_i_list_end[cdp_a_loop] = -1;
      cdp_indel_d_list_start[cdp_a_loop] = -1;
      cdp_indel_d_list_end[cdp_a_loop] = -1;
    }


    for(cdp_a_loop=0;cdp_a_loop<g_sv_list2_len;cdp_a_loop++)
    {
      cdp_ins_list2_start[cdp_a_loop] = -1;
      cdp_ins_list2_end[cdp_a_loop] = -1;
      cdp_dup_list2_start[cdp_a_loop] = -1;
      cdp_dup_list2_end[cdp_a_loop] = -1;
      cdp_del_list2_start[cdp_a_loop] = -1;
      cdp_del_list2_end[cdp_a_loop] = -1;
      cdp_inv_f_list2_start[cdp_a_loop] = -1;
      cdp_inv_f_list2_end[cdp_a_loop] = -1;
      cdp_inv_r_list2_start[cdp_a_loop] = -1;
      cdp_inv_r_list2_end[cdp_a_loop] = -1;
      cdp_ctx_f_list2[cdp_a_loop] = -1;
      cdp_ctx_r_list2[cdp_a_loop] = -1;
    }


    if( g_internal == 1 )
    {
      printf("after SV lists2 -1\n");
    }


    for(cdp_a_loop=0;cdp_a_loop<g_one_base_rd_len;cdp_a_loop++)
    {
      cdp_one_base_rd[cdp_a_loop] = 0;
      cdp_one_base_conc[cdp_a_loop] = 0;
      cdp_one_base_sc_left[cdp_a_loop] = 0;
      cdp_one_base_sc_right[cdp_a_loop] = 0;
      cdp_one_base_sc_left_rd[cdp_a_loop] = 0;
      cdp_one_base_sc_right_rd[cdp_a_loop] = 0;
      cdp_one_base_sc_rd[cdp_a_loop] = 0;
      cdp_one_base_ins[cdp_a_loop] = 0;
      cdp_one_base_del_f[cdp_a_loop] = 0;
      cdp_one_base_del_r[cdp_a_loop] = 0;
      cdp_one_base_del_fdist[cdp_a_loop] = 0;
      cdp_one_base_del_rdist[cdp_a_loop] = 0;
      cdp_one_base_del_f_read_start[cdp_a_loop] = 0;
      cdp_one_base_del_r_read_start[cdp_a_loop] = 0;
      cdp_one_base_del_f_read_end[cdp_a_loop] = 0;
      cdp_one_base_del_r_read_end[cdp_a_loop] = 0;
      cdp_one_base_dup_f[cdp_a_loop] = 0;
      cdp_one_base_dup_r[cdp_a_loop] = 0;
      cdp_one_base_dup_fdist[cdp_a_loop] = 0;
      cdp_one_base_dup_rdist[cdp_a_loop] = 0;
      cdp_one_base_dup_f_read_start[cdp_a_loop] = 0;
      cdp_one_base_dup_r_read_start[cdp_a_loop] = 0;
      cdp_one_base_dup_f_read_end[cdp_a_loop] = 0;
      cdp_one_base_dup_r_read_end[cdp_a_loop] = 0;
      cdp_one_base_inv_f1[cdp_a_loop] = 0;
      cdp_one_base_inv_r1[cdp_a_loop] = 0;
      cdp_one_base_inv_f1dist[cdp_a_loop] = 0;
      cdp_one_base_inv_r1dist[cdp_a_loop] = 0;
      cdp_one_base_inv_f1_read_start[cdp_a_loop] = 0;
      cdp_one_base_inv_r1_read_start[cdp_a_loop] = 0;
      cdp_one_base_inv_f1_read_end[cdp_a_loop] = 0;
      cdp_one_base_inv_r1_read_end[cdp_a_loop] = 0;
      cdp_one_base_inv_f2[cdp_a_loop] = 0;
      cdp_one_base_inv_r2[cdp_a_loop] = 0;
      cdp_one_base_inv_f2dist[cdp_a_loop] = 0;
      cdp_one_base_inv_r2dist[cdp_a_loop] = 0;
      cdp_one_base_inv_f2_read_start[cdp_a_loop] = 0;
      cdp_one_base_inv_r2_read_start[cdp_a_loop] = 0;
      cdp_one_base_inv_f2_read_end[cdp_a_loop] = 0;
      cdp_one_base_inv_r2_read_end[cdp_a_loop] = 0;
      cdp_one_base_ctx_f[cdp_a_loop] = 0;
      cdp_one_base_ctx_r[cdp_a_loop] = 0;
      cdp_one_base_ctx_f_mchr[cdp_a_loop] = 0;
      cdp_one_base_ctx_r_mchr[cdp_a_loop] = 0;
      cdp_one_base_ctx_f_mpos[cdp_a_loop] = 0;
      cdp_one_base_ctx_r_mpos[cdp_a_loop] = 0;
      cdp_one_base_ctx_f_read_start[cdp_a_loop] = 0;
      cdp_one_base_ctx_r_read_start[cdp_a_loop] = 0;
      cdp_one_base_ctx_f_read_end[cdp_a_loop] = 0;
      cdp_one_base_ctx_r_read_end[cdp_a_loop] = 0;
      cdp_one_base_munmapped_f[cdp_a_loop] = 0;
      cdp_one_base_munmapped_r[cdp_a_loop] = 0;
      cdp_one_base_indel_i[cdp_a_loop] = 0;

      for(cdp_other_loop=0;cdp_other_loop<g_indel_i_seq_len;cdp_other_loop++)
      {
	cdp_one_base_indel_i_seq[cdp_other_loop][cdp_a_loop] = 0;
      }

      cdp_one_base_indel_idist[cdp_a_loop] = 0;
      cdp_one_base_indel_d_f[cdp_a_loop] = 0;
      cdp_one_base_indel_d_fdist[cdp_a_loop] = 0;
      cdp_one_base_indel_d_f_rd[cdp_a_loop] = 0;
      cdp_one_base_indel_d_r[cdp_a_loop] = 0;
      cdp_one_base_indel_d_rdist[cdp_a_loop] = 0;
      cdp_one_base_indel_d_r_rd[cdp_a_loop] = 0;
      cdp_one_base_ctx_sc_left[cdp_a_loop] = 0;
      cdp_one_base_ctx_sc_right[cdp_a_loop] = 0;
      cdp_one_base_ctx_sc_left_rd[cdp_a_loop] = 0;
      cdp_one_base_ctx_sc_right_rd[cdp_a_loop] = 0;
      cdp_one_base_ctx_sc_rd[cdp_a_loop] = 0;
      cdp_one_base_indel_sc_left[cdp_a_loop] = 0;
      cdp_one_base_indel_sc_right[cdp_a_loop] = 0;
      cdp_one_base_indel_sc_left_rd[cdp_a_loop] = 0;
      cdp_one_base_indel_sc_right_rd[cdp_a_loop] = 0;
      cdp_one_base_indel_sc_rd[cdp_a_loop] = 0;
      cdp_one_base_bq[cdp_a_loop] = 0;
      cdp_one_base_bq_all[cdp_a_loop] = 0;
      cdp_one_base_mq[cdp_a_loop] = 0;
      cdp_one_base_mq_all[cdp_a_loop] = 0;
      cdp_one_base_bq_read_count[cdp_a_loop] = 0;
      cdp_one_base_mq_read_count[cdp_a_loop] = 0;
      cdp_one_base_read_count_all[cdp_a_loop] = 0;

      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
      {
	cdp_one_base_pos_in_read[cdp_other_loop][cdp_a_loop] = 0;
	cdp_one_base_fstrand[cdp_other_loop][cdp_a_loop] = 0;
      }

      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
      {
	cdp_one_base_other[cdp_other_loop][cdp_a_loop] = 0;
	cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = 0;
	cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = 0;
	cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = 0;
	cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = 0;
	cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = 0;
      }

      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
      {
	cdp_one_base_snv[cdp_other_loop][cdp_a_loop] = 0;
      }
      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
      {
	cdp_one_base_snv_lowmq[cdp_other_loop][cdp_a_loop] = 0;
      }

      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
      {
	cdp_one_base_snv_sc[cdp_other_loop][cdp_a_loop] = 0;
      }

      for(cdp_other_loop=0;cdp_other_loop<g_min_snv;cdp_other_loop++)
      {

	for(cdp_b_loop=0;cdp_b_loop<g_read_name_len;cdp_b_loop++)
	{
	  cdp_one_base_snv_read_names[cdp_other_loop][cdp_a_loop][cdp_b_loop] = NULL;
	}
      }
    }


    if( g_internal == 1 )
    {
      printf("after set counters to 0\n");
    }


    if (cdp_pos_in_contig_start < g_sub_region_start - g_sub_region_overlap)
	cdp_pos_in_contig_start = g_sub_region_start - g_sub_region_overlap;


    uint32_t *cdp_cigar_ptr;
    if( cdp_chr_match >= 0 )
    {
      while(my_samread(cdp_bam_file, cdp_b) > 0 && cdp_begin < 2 )
      {
	cdp_pos = cdp_b->core.pos;
	cdp_flag = cdp_b->core.flag;
	cdp_mq = cdp_b->core.qual;
	cdp_chr = cdp_b->core.tid;
	cdp_mchr = cdp_b->core.mtid;
	cdp_mpos = cdp_b->core.mpos;
	cdp_tlen = cdp_b->core.isize;
	cdp_lseq = cdp_b->core.l_qseq;

	cdp_aux_pos = -1;
	cdp_aux_mq = -1;
	cdp_l_aux = bam_get_l_aux(cdp_b);


	if( (cdp_flag & BAM_FUNMAP) == 0 && (cdp_flag & BAM_FDUP) == 0 && (cdp_flag & BAM_FQCFAIL) == 0 )
	{
	  if( cdp_l_aux > 0 && cdp_l_aux < aux_str_len )
	  {
	    cdp_aux = bam_aux_get(cdp_b,"XP");
	    if( cdp_aux )
	    {
	      if( cdp_aux[0] == 'Z' )
	      {
		memmove(cdp_aux_str, cdp_aux+1, cdp_l_aux);
	      }
	      else
	      {
		memmove(cdp_aux_str, cdp_aux, cdp_l_aux+1);
	      }
	      cdp_aux_chr = strtok((char *) cdp_aux_str, g_aux_separator);
	      cdp_aux_str_temp = strtok(NULL, g_aux_separator);
	      if( cdp_aux_str_temp[0] == '+' )
	      {
		cdp_aux_strand = 0;
	      }
	      else
	      {
		cdp_aux_strand = 1;
	      }
	      memmove(cdp_aux_str_temp, cdp_aux_str_temp+1, strlen(cdp_aux_str_temp));
	      cdp_aux_pos = atoi(cdp_aux_str_temp);
	      cdp_aux_cigar = strtok(NULL, g_aux_separator);
	      cdp_aux_str_temp = strtok(NULL, g_aux_separator);
	      cdp_aux_mq = atoi(cdp_aux_str_temp);
	    }

	    else
	    {
	      cdp_aux = bam_aux_get(cdp_b,"SA");
	      if( cdp_aux )
	      {
		if( cdp_aux[0] == 'Z' )
		{
		  memmove(cdp_aux_str, cdp_aux+1, cdp_l_aux);
		}
		else
		{
		  memmove(cdp_aux_str, cdp_aux, cdp_l_aux+1);
		}
		cdp_aux_chr = strtok((char *) cdp_aux_str, g_aux_separator);
		cdp_aux_str_temp = strtok(NULL, g_aux_separator);
		cdp_aux_pos = atoi(cdp_aux_str_temp);
		cdp_aux_str_temp = strtok(NULL, g_aux_separator);
		if( cdp_aux_str_temp[0] == '+' )
		{
		  cdp_aux_strand = 0;
		}
		else
		{
		  cdp_aux_strand = 1;
		}
		cdp_aux_cigar = strtok(NULL, g_aux_separator);
		cdp_aux_str_temp = strtok(NULL, g_aux_separator);
		cdp_aux_mq = atoi(cdp_aux_str_temp);
	      }
	    }

	  }
	}


	if( cdp_mq >= g_min_mapq )
	{
	  cdp_add = cdp_add_factor;
	}
	else
	{
	  cdp_add = cdp_add_factor_lowmq;
	}
	cdp_add_factor_double = (double) cdp_add;

	if( cdp_chr == cdp_chr_match )
	{
	  cdp_begin = 1;
	  while( cdp_begin < 2 )
	  {

	    cdp_one_base_index += 1;
	    if( cdp_one_base_index == g_34_one_base_rd_len )
	    {
#ifdef DO_TIMING
	      start_t = rdtsc();
#endif

	      memcpy(cdp_one_base_rd, &cdp_one_base_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_conc, &cdp_one_base_conc[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_sc_left, &cdp_one_base_sc_left[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_sc_right, &cdp_one_base_sc_right[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_sc_left_rd, &cdp_one_base_sc_left_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_sc_right_rd, &cdp_one_base_sc_right_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_sc_rd, &cdp_one_base_sc_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_ins, &cdp_one_base_ins[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      if( cdp_one_base_del_set[0] != 0 || cdp_one_base_del_set[1] != 0 )
	      {
		if( cdp_one_base_del_set[1] == 0 )
		{
		  memset(cdp_one_base_del_f, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_del_r, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_del_fdist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_del_rdist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_del_f_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_del_f_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_del_r_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_del_r_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_del_f, &cdp_one_base_del_f[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_del_r, &cdp_one_base_del_r[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_del_fdist, &cdp_one_base_del_fdist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_del_rdist, &cdp_one_base_del_rdist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_del_f_read_start, &cdp_one_base_del_f_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_del_r_read_start, &cdp_one_base_del_r_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_del_f_read_end, &cdp_one_base_del_f_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_del_r_read_end, &cdp_one_base_del_r_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }

	      if( cdp_one_base_dup_set[0] != 0 || cdp_one_base_dup_set[1] != 0 )
	      {
		if( cdp_one_base_dup_set[1] == 0 )
		{
		  memset(cdp_one_base_dup_f, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_dup_r, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_dup_fdist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_dup_rdist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_dup_f_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_dup_f_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_dup_r_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_dup_r_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_dup_f, &cdp_one_base_dup_f[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_dup_r, &cdp_one_base_dup_r[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_dup_fdist, &cdp_one_base_dup_fdist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_dup_rdist, &cdp_one_base_dup_rdist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_dup_f_read_start, &cdp_one_base_dup_f_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_dup_r_read_start, &cdp_one_base_dup_r_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_dup_f_read_end, &cdp_one_base_dup_f_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_dup_r_read_end, &cdp_one_base_dup_r_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }

	      if( cdp_one_base_inv_f_set[0] != 0 || cdp_one_base_inv_f_set[1] != 0 )
	      {
		if( cdp_one_base_inv_f_set[1] == 0 )
		{
		  memset(cdp_one_base_inv_f1, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_f2, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_f1dist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_inv_f2dist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_inv_f1_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_f1_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_f2_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_f2_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_inv_f1, &cdp_one_base_inv_f1[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_f1dist, &cdp_one_base_inv_f1dist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_inv_f1_read_start, &cdp_one_base_inv_f1_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_f1_read_end, &cdp_one_base_inv_f1_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_f2, &cdp_one_base_inv_f2[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_f2dist, &cdp_one_base_inv_f2dist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_inv_f2_read_start, &cdp_one_base_inv_f2_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_f2_read_end, &cdp_one_base_inv_f2_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }
	      if( cdp_one_base_inv_r_set[0] != 0 || cdp_one_base_inv_r_set[1] != 0 )
	      {
		if( cdp_one_base_inv_r_set[1] == 0 )
		{
		  memset(cdp_one_base_inv_r1, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_r2, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_r1dist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_inv_r2dist, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_inv_r1_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_r1_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_r2_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_inv_r2_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_inv_r1, &cdp_one_base_inv_r1[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_r1dist, &cdp_one_base_inv_r1dist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_inv_r1_read_start, &cdp_one_base_inv_r1_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_r1_read_end, &cdp_one_base_inv_r1_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_r2, &cdp_one_base_inv_r2[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_r2dist, &cdp_one_base_inv_r2dist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_inv_r2_read_start, &cdp_one_base_inv_r2_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_inv_r2_read_end, &cdp_one_base_inv_r2_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }

	      if( cdp_one_base_ctx_f_set[0] != 0 || cdp_one_base_ctx_f_set[1] != 0 )
	      {
		if( cdp_one_base_ctx_f_set[1] == 0 )
		{
		  memset(cdp_one_base_ctx_f, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_ctx_f_mchr, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_ctx_f_mpos, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_ctx_f_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_ctx_f_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_ctx_f, &cdp_one_base_ctx_f[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_ctx_f_mchr, &cdp_one_base_ctx_f_mchr[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_ctx_f_mpos, &cdp_one_base_ctx_f_mpos[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_ctx_f_read_start, &cdp_one_base_ctx_f_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_ctx_f_read_end, &cdp_one_base_ctx_f_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }
	      if( cdp_one_base_ctx_r_set[0] != 0 || cdp_one_base_ctx_r_set[1] != 0 )
	      {
		if( cdp_one_base_ctx_r_set[1] == 0 )
		{
		  memset(cdp_one_base_ctx_r, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_ctx_r_mchr, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_ctx_r_mpos, 0, g_half_one_base_rd_len*sizeof(double));
		  memset(cdp_one_base_ctx_r_read_start, 0, g_half_one_base_rd_len*sizeof(int));
		  memset(cdp_one_base_ctx_r_read_end, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_ctx_r, &cdp_one_base_ctx_r[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_ctx_r_mchr, &cdp_one_base_ctx_r_mchr[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_ctx_r_mpos, &cdp_one_base_ctx_r_mpos[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		  memcpy(cdp_one_base_ctx_r_read_start, &cdp_one_base_ctx_r_read_start[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  memcpy(cdp_one_base_ctx_r_read_end, &cdp_one_base_ctx_r_read_end[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }

	      if( cdp_one_base_munmapped_f_set[0] != 0 || cdp_one_base_munmapped_f_set[1] != 0 )
	      {
		if( cdp_one_base_munmapped_f_set[1] == 0 )
		{
		  memset(cdp_one_base_munmapped_f, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_munmapped_f, &cdp_one_base_munmapped_f[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }
	      if( cdp_one_base_munmapped_r_set[0] != 0 || cdp_one_base_munmapped_r_set[1] != 0 )
	      {
		if( cdp_one_base_munmapped_r_set[1] == 0 )
		{
		  memset(cdp_one_base_munmapped_r, 0, g_half_one_base_rd_len*sizeof(int));
		}
		else
		{
		  memcpy(cdp_one_base_munmapped_r, &cdp_one_base_munmapped_r[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		}
	      }

	      memcpy(cdp_one_base_indel_i, &cdp_one_base_indel_i[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));

	      for(cdp_other_loop=0;cdp_other_loop<g_indel_i_seq_len;cdp_other_loop++)
	      {
		if( cdp_one_base_indel_i_seq_set[cdp_other_loop][0] != 0 || cdp_one_base_indel_i_seq_set[cdp_other_loop][1] != 0 )
		{
		  if( cdp_one_base_indel_i_seq_set[cdp_other_loop][1] == 0 )
		  {
		    memset(cdp_one_base_indel_i_seq[cdp_other_loop], 0, g_half_one_base_rd_len*sizeof(char));
		  }
		  else
		  {
		    memcpy(cdp_one_base_indel_i_seq[cdp_other_loop], &cdp_one_base_indel_i_seq[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(char));
		  }
		}

	      }

	      memcpy(cdp_one_base_indel_idist, &cdp_one_base_indel_idist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_d_f, &cdp_one_base_indel_d_f[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_d_fdist, &cdp_one_base_indel_d_fdist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_d_f_rd, &cdp_one_base_indel_d_f_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_d_r, &cdp_one_base_indel_d_r[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_d_rdist, &cdp_one_base_indel_d_rdist[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_d_r_rd, &cdp_one_base_indel_d_r_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_ctx_sc_left, &cdp_one_base_ctx_sc_left[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_ctx_sc_right, &cdp_one_base_ctx_sc_right[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_ctx_sc_left_rd, &cdp_one_base_ctx_sc_left_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_ctx_sc_right_rd, &cdp_one_base_ctx_sc_right_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_ctx_sc_rd, &cdp_one_base_ctx_sc_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_sc_left, &cdp_one_base_indel_sc_left[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_sc_right, &cdp_one_base_indel_sc_right[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_sc_left_rd, &cdp_one_base_indel_sc_left_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_sc_right_rd, &cdp_one_base_indel_sc_right_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_indel_sc_rd, &cdp_one_base_indel_sc_rd[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));


	      memcpy(cdp_one_base_bq, &cdp_one_base_bq[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_bq_all, &cdp_one_base_bq_all[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_mq, &cdp_one_base_mq[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_mq_all, &cdp_one_base_mq_all[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_bq_read_count, &cdp_one_base_bq_read_count[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_mq_read_count, &cdp_one_base_mq_read_count[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      memcpy(cdp_one_base_read_count_all, &cdp_one_base_read_count_all[g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));


	      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
	      {
			memcpy(cdp_one_base_pos_in_read[cdp_other_loop], &cdp_one_base_pos_in_read[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
			memcpy(cdp_one_base_fstrand[cdp_other_loop], &cdp_one_base_fstrand[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      }

	      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
	      {
		if( cdp_one_base_other_set[cdp_other_loop][0] != 0 || cdp_one_base_other_set[cdp_other_loop][1] != 0 )
		{
		  if( cdp_one_base_other_set[cdp_other_loop][1] == 0 )
		  {
		    memset(cdp_one_base_other[cdp_other_loop], 0, g_half_one_base_rd_len*sizeof(int));
		    memset(cdp_one_base_other_type[cdp_other_loop], 0, g_half_one_base_rd_len*sizeof(int));
		    memset(cdp_one_base_other_mchr[cdp_other_loop], 0, g_half_one_base_rd_len*sizeof(int));
		    memset(cdp_one_base_other_dist[cdp_other_loop], 0, g_half_one_base_rd_len*sizeof(double));
		    memset(cdp_one_base_other_read_start[cdp_other_loop], 0, g_half_one_base_rd_len*sizeof(int));
		    memset(cdp_one_base_other_read_end[cdp_other_loop], 0, g_half_one_base_rd_len*sizeof(int));
		  }
		  else
		  {
		    memcpy(cdp_one_base_other[cdp_other_loop], &cdp_one_base_other[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		    memcpy(cdp_one_base_other_type[cdp_other_loop], &cdp_one_base_other_type[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		    memcpy(cdp_one_base_other_mchr[cdp_other_loop], &cdp_one_base_other_mchr[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		    memcpy(cdp_one_base_other_dist[cdp_other_loop], &cdp_one_base_other_dist[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(double));
		    memcpy(cdp_one_base_other_read_start[cdp_other_loop], &cdp_one_base_other_read_start[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		    memcpy(cdp_one_base_other_read_end[cdp_other_loop], &cdp_one_base_other_read_end[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		  }
		}
	      }

	      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
	      {
		memcpy(cdp_one_base_snv[cdp_other_loop], &cdp_one_base_snv[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		memcpy(cdp_one_base_snv_lowmq[cdp_other_loop], &cdp_one_base_snv_lowmq[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
		memcpy(cdp_one_base_snv_sc[cdp_other_loop], &cdp_one_base_snv_sc[cdp_other_loop][g_half_one_base_rd_len], g_half_one_base_rd_len*sizeof(int));
	      }

	      for(cdp_other_loop=0;cdp_other_loop<g_min_snv;cdp_other_loop++)
	      {

		for(cdp_b_loop=0;cdp_b_loop<g_half_one_base_rd_len;cdp_b_loop++)
		{
		  strcpy(cdp_temp_snv_string, cdp_one_base_snv_read_names[cdp_other_loop][cdp_b_loop+g_half_one_base_rd_len]);
		  strcpy(cdp_one_base_snv_read_names[cdp_other_loop][cdp_b_loop], cdp_temp_snv_string);
		}
	      }

	      memset(&cdp_one_base_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_conc[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_sc_left[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_sc_right[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_sc_left_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_sc_right_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_sc_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_ins[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      if( cdp_one_base_del_set[1] != 0 )
	      {
		memset(&cdp_one_base_del_f[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_del_r[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_del_fdist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_del_rdist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_del_f_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_del_r_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_del_f_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_del_r_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      if( cdp_one_base_dup_set[1] != 0 )
	      {
		memset(&cdp_one_base_dup_f[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_dup_r[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_dup_fdist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_dup_rdist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_dup_f_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_dup_r_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_dup_f_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_dup_r_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      if( cdp_one_base_inv_f_set[1] != 0 )
	      {
		memset(&cdp_one_base_inv_f1[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_f1dist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_inv_f1_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_f1_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_f2[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_f2dist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_inv_f2_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_f2_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      if( cdp_one_base_inv_r_set[1] != 0 )
	      {
		memset(&cdp_one_base_inv_r1[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_r1dist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_inv_r1_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_r1_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_r2[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_r2dist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_inv_r2_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_inv_r2_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      if( cdp_one_base_ctx_f_set[1] != 0 )
	      {
		memset(&cdp_one_base_ctx_f[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_ctx_f_mchr[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_ctx_f_mpos[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_ctx_f_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_ctx_f_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      if( cdp_one_base_ctx_r_set[1] != 0 )
	      {
		memset(&cdp_one_base_ctx_r[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_ctx_r_mchr[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_ctx_r_mpos[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		memset(&cdp_one_base_ctx_r_read_start[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_ctx_r_read_end[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      if( cdp_one_base_munmapped_f_set[1] != 0 )
	      {
		memset(&cdp_one_base_munmapped_f[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      if( cdp_one_base_munmapped_r_set[1] != 0 )
	      {
		memset(&cdp_one_base_munmapped_r[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      }
	      memset(&cdp_one_base_indel_i[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));

	      for(cdp_other_loop=0;cdp_other_loop<g_indel_i_seq_len;cdp_other_loop++)
	      {
		if( cdp_one_base_indel_i_seq_set[cdp_other_loop][1] != 0 )
		{
		  memset(&cdp_one_base_indel_i_seq[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(char));
		}

	      }

	      memset(&cdp_one_base_indel_idist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_d_f[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_d_fdist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_d_f_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_d_r[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_d_rdist[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_d_r_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_ctx_sc_left[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_ctx_sc_right[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_ctx_sc_left_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_ctx_sc_right_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_ctx_sc_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_sc_left[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_sc_right[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_sc_left_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_sc_right_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_indel_sc_rd[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_bq[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_bq_all[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_mq[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_mq_all[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_bq_read_count[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_mq_read_count[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
	      memset(&cdp_one_base_read_count_all[g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));

	      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
	      {
		memset(&cdp_one_base_pos_in_read[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_fstrand[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));

	      }

	      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
	      {
		if( cdp_one_base_other_set[cdp_other_loop][1] != 0 )
		{
		  memset(&cdp_one_base_other[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		  memset(&cdp_one_base_other_type[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		  memset(&cdp_one_base_other_mchr[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		  memset(&cdp_one_base_other_dist[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(double));
		  memset(&cdp_one_base_other_read_start[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		  memset(&cdp_one_base_other_read_end[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		}

	      }

	      for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
	      {
		memset(&cdp_one_base_snv[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_snv_lowmq[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));
		memset(&cdp_one_base_snv_sc[cdp_other_loop][g_half_one_base_rd_len], 0, g_half_one_base_rd_len*sizeof(int));

	      }

	      for(cdp_other_loop=0;cdp_other_loop<g_min_snv;cdp_other_loop++)
	      {
		for(cdp_b_loop=0;cdp_b_loop<g_half_one_base_rd_len;cdp_b_loop++)
		{
		  strcpy(cdp_one_base_snv_read_names[cdp_other_loop][cdp_b_loop+g_half_one_base_rd_len], "");
		}
	      }

	      cdp_one_base_index = g_14_one_base_rd_len;

	      cdp_one_base_del_set[0] = cdp_one_base_del_set[1];
	      cdp_one_base_del_set[1] = 0;
	      cdp_one_base_dup_set[0] = cdp_one_base_dup_set[1];
	      cdp_one_base_dup_set[1] = 0;
	      cdp_one_base_inv_f_set[0] = cdp_one_base_inv_f_set[1];
	      cdp_one_base_inv_f_set[1] = 0;
	      cdp_one_base_inv_r_set[0] = cdp_one_base_inv_r_set[1];
	      cdp_one_base_inv_r_set[1] = 0;
	      cdp_one_base_ctx_f_set[0] = cdp_one_base_ctx_f_set[1];
	      cdp_one_base_ctx_f_set[1] = 0;
	      cdp_one_base_ctx_r_set[0] = cdp_one_base_ctx_r_set[1];
	      cdp_one_base_ctx_r_set[1] = 0;
	      for(cdp_other_loop=0;cdp_other_loop<g_indel_i_seq_len;cdp_other_loop++)
	      {
		cdp_one_base_indel_i_seq_set[cdp_other_loop][0] = cdp_one_base_indel_i_seq_set[cdp_other_loop][1];
		cdp_one_base_indel_i_seq_set[cdp_other_loop][1] = 0;
	      }
	      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
	      {
		cdp_one_base_other_set[cdp_other_loop][0] = cdp_one_base_other_set[cdp_other_loop][1];
		cdp_one_base_other_set[cdp_other_loop][1] = 0;
	      }
	      cdp_one_base_munmapped_f_set[0] = cdp_one_base_munmapped_f_set[1];
	      cdp_one_base_munmapped_f_set[1] = 0;
	      cdp_one_base_munmapped_r_set[0] = cdp_one_base_munmapped_r_set[1];
	      cdp_one_base_munmapped_r_set[1] = 0;

#ifdef DO_TIMING
	      end_t = rdtsc();
	      timers_ss[0] += end_t - start_t;
#endif

	    }

	    if( cdp_begin < 2 && cdp_pos >= cdp_one_base_index_start )
	    {
	      if( cdp_pos - g_overlap_mult*g_insert_max_size <= cdp_pos_in_contig_start )
	      {
		while( cdp_pos - g_overlap_mult*g_insert_max_size <= cdp_pos_in_contig_start && cdp_begin < 2 )
		{
		  cdp_cigar_ptr = bam1_cigar(cdp_b);

		  if( (cdp_flag & BAM_FUNMAP) == 0 && (cdp_flag & BAM_FDUP) == 0 && (cdp_flag & BAM_FQCFAIL) == 0 )
		  {

#ifdef DO_TIMING
		    start_t = rdtsc();
#endif
		    cdp_add_to_list = 1;
		    cdp_svtype = -1;
		    caf_add_to_list = 1;

		    if( g_rmdup > 0 && (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 )
		    {

		      if( cdp_chr == cdp_mchr )
		      {

			if( cdp_mpos > cdp_pos )
			{

			  if( (cdp_flag & BAM_FREVERSE) == 0 && (cdp_flag & BAM_FMREVERSE) != 0 )
			  {
			    cdp_svtype = DEL;
			  }

			  else if( (cdp_flag & BAM_FREVERSE) == 0 && (cdp_flag & BAM_FMREVERSE) == 0 )
			  {
			    cdp_svtype = INV_F;
			  }



			  else if( (cdp_flag & BAM_FREVERSE) != 0 )
			  {

			    if( (cdp_flag & BAM_FMREVERSE) != 0 )
			    {
			      cdp_svtype = INV_R;
			    }

			    else
			    {
			      cdp_svtype = DUP;
			    }

			  }



			}



			else
			{

			  if( (cdp_flag & BAM_FREVERSE) != 0 && (cdp_flag & BAM_FMREVERSE) == 0 )
			  {
			    cdp_svtype = DEL;
			  }

			  else if( (cdp_flag & BAM_FREVERSE) == 0 && (cdp_flag & BAM_FMREVERSE) == 0 )
			  {
			    cdp_svtype = INV_F;
			  }



			  else if( (cdp_flag & BAM_FMREVERSE) != 0 )
			  {

			    if( (cdp_flag & BAM_FREVERSE) == 0 )
			    {
			      cdp_svtype = DUP;
			    }



			    else if( (cdp_flag & BAM_FREVERSE) != 0 )
			    {
			      cdp_svtype = INV_R;
			    }

			  }


			}
		      }



		      else
		      {

			if( (cdp_flag & BAM_FREVERSE) == 0 )
			{
			  if( (cdp_flag & BAM_FMREVERSE) == 0 )
			  {
			    cdp_svtype = CTX_FF;
			  }
			  else
			  {

				cdp_svtype = CTX_FR;
			  }
			}



			else
			{
			  if( (cdp_flag & BAM_FMREVERSE) == 0 )
			  {
			    cdp_svtype = CTX_RF;
			  }
			  else
			  {

				cdp_svtype = CTX_RR;
			  }
			}

		      }


		      if( cdp_svtype >= 0 )
		      {
			if( cdp_pos != cdp_old_pos )
			{
			  cdp_rmdup_index = 0;
			  cdp_old_pos = cdp_pos;
			}
			else
			{
			  for(cdp_rmdup_a=0;cdp_rmdup_a<cdp_rmdup_index;cdp_rmdup_a++)
			  {
			    if( cdp_mpos == cdp_rmdup_mpos_list[cdp_rmdup_a] && cdp_mchr == cdp_rmdup_mchr_list[cdp_rmdup_a] && cdp_rmdup_lseq_list[cdp_rmdup_a] == cdp_lseq && cdp_rmdup_tlen_list[cdp_rmdup_a] == cdp_tlen && cdp_mq >= g_min_mapq && cdp_rmdup_svtype_list[cdp_rmdup_a] == cdp_svtype )
			    {
			      cdp_add_to_list = 0;
			      break;
			    }
			  }
			}
			if( cdp_add_to_list == 1 && cdp_rmdup_index < g_rmdup_list_len )
			{
			  cdp_rmdup_mchr_list[cdp_rmdup_index] = cdp_mchr;
			  cdp_rmdup_mpos_list[cdp_rmdup_index] = cdp_mpos;
			  cdp_rmdup_lseq_list[cdp_rmdup_index] = cdp_lseq;
			  cdp_rmdup_tlen_list[cdp_rmdup_index] = cdp_tlen;
			  cdp_rmdup_svtype_list[cdp_rmdup_index] = cdp_svtype;
			  cdp_rmdup_index += 1;
			}
			caf_add_to_list = cdp_add_to_list;
		      }
		    }




#ifdef DO_TIMING
		    end_t = rdtsc();
		    timers_ss[1] += end_t - start_t;
#endif

		    if( caf_add_to_list == 1 )
		    {
#ifdef DO_TIMING
			    start_t = rdtsc();
#endif
			    caf_pos = cdp_pos;

			    for(caf_a_loop=0;caf_a_loop<(cdp_b->core.n_cigar);caf_a_loop++)
			    {
				    caf_c = cdp_cigar_ptr[caf_a_loop];
				    caf_c_type = bam_cigar_op(caf_c);
				    caf_c_len = bam_cigar_oplen(caf_c);

				    if( caf_c_type == BAM_CMATCH || caf_c_type == BAM_CEQUAL || caf_c_type == BAM_CDIFF )
				    {
					    if( cdp_mq >= g_rd_min_mapq )
					    {
						    if( caf_pos >=0 && caf_pos + caf_c_len < caf_chr_fasta_len )
						    {
						      for(caf_b_loop=caf_pos;caf_b_loop<caf_pos+caf_c_len;caf_b_loop++)
						      {
							      caf_rd_mq_list[caf_b_loop] += cdp_mq;
						      }
						      for(caf_b_loop=caf_pos;caf_b_loop<caf_pos+caf_c_len;caf_b_loop++)
						      {
							      caf_rd_rd_list[caf_b_loop] += 1;
						      }
						    }
					    }
					    else
					    {
						    if( caf_pos >=0 && caf_pos + caf_c_len < caf_chr_fasta_len )
						    {
						      for(caf_b_loop=caf_pos;caf_b_loop<caf_pos+caf_c_len;caf_b_loop++)
						      {
							      caf_rd_mq_list[caf_b_loop] += cdp_mq;
						      }
						      for(caf_b_loop=caf_pos;caf_b_loop<caf_pos+caf_c_len;caf_b_loop++)
						      {
							      caf_rd_low_mq_rd_list[caf_b_loop] += 1;
						      }
						    }
					    }
					    caf_pos += caf_c_len;
				    }
				    else if( caf_c_type == BAM_CDEL )
				    {
					    caf_pos += caf_c_len;
				    }
			    }
#ifdef DO_TIMING
			    end_t = rdtsc();
			    timers_ss[4] += end_t - start_t;
#endif
		    }

		    if( cdp_add_to_list == 1 )
		    {
#ifdef DO_TIMING
		      start_t = rdtsc();
#endif
		      cdp_aux_start_adj = 0;
		      cdp_aux_end_adj = 0;
		      cdp_aux_end_adj_indel = 0;
		      if( cdp_aux_pos >= 0 )
		      {

			cdp_aux_cigar_len = 0;
			cdp_aux_c_type_str_len = 0;
			cdp_aux_lp_len = strlen(cdp_aux_cigar);
			for(cdp_a_loop=0;cdp_a_loop<cdp_aux_lp_len;cdp_a_loop++)
			{
			  if( isdigit(cdp_aux_cigar[cdp_a_loop]) && cdp_aux_c_type_str_len < cdp_aux_c_type_len )
			  {
			    cdp_aux_c_type_str[cdp_aux_c_type_str_len] = cdp_aux_cigar[cdp_a_loop];
			    cdp_aux_c_type_str_len += 1;
			  }
			  else if( isalpha(cdp_aux_cigar[cdp_a_loop]) )
			  {
			    cdp_aux_c_type[cdp_aux_cigar_len] = cdp_aux_cigar[cdp_a_loop];
			    cdp_aux_c_type_str[cdp_aux_c_type_str_len] = cdp_aux_cigar[cdp_a_loop];
			    cdp_aux_c_len[cdp_aux_cigar_len] = (int) strtol(cdp_aux_c_type_str, &cdp_end_ptr, 10);
			    cdp_aux_cigar_len += 1;
			    cdp_aux_c_type_str_len = 0;;
			  }

			}



			if( cdp_aux_c_type[0] == BAM_CSOFT_CLIP_AUX )
			{
			  cdp_aux_start_adj = cdp_aux_c_len[0];
			}
			if( cdp_aux_c_type[cdp_aux_cigar_len-1] == BAM_CSOFT_CLIP_AUX )
			{
			  cdp_aux_end_adj = cdp_aux_c_len[cdp_aux_cigar_len-1];
			}
			for(cdp_a_loop=0;cdp_a_loop<cdp_aux_cigar_len;cdp_a_loop++)
			{
			  if( cdp_aux_c_type[cdp_a_loop] == BAM_CINS_AUX )
			  {
			    cdp_aux_end_adj_indel += cdp_aux_c_len[cdp_a_loop];
			  }
			  else if( cdp_aux_c_type[cdp_a_loop] == BAM_CDEL_AUX )
			  {
			    cdp_aux_end_adj_indel -= cdp_aux_c_len[cdp_a_loop];
			  }
			}

		      }


		      cdp_cigar_len = cdp_b->core.n_cigar;
		      if( cdp_cigar_len > cdp_c_type_len )
		      {
			cdp_cigar_len = cdp_c_type_len;
		      }
		      for(cdp_a_loop=0;cdp_a_loop<cdp_cigar_len;cdp_a_loop++)
		      {
			cdp_c = cdp_cigar_ptr[cdp_a_loop];
			cdp_c_type[cdp_a_loop] = bam_cigar_op(cdp_c);
			cdp_c_len[cdp_a_loop] = bam_cigar_oplen(cdp_c);
		      }



		      char *cdp_read_name = bam1_qname(cdp_b);
		      uint8_t *cdp_temp_seq = bam1_seq(cdp_b);
		      uint8_t *cdp_qual = bam1_qual(cdp_b);
		      for(cdp_a_int32_t=0;cdp_a_int32_t<cdp_lseq;cdp_a_int32_t++)
		      {
			uint8_t cdp_temp_base = bam1_seqi(cdp_temp_seq, cdp_a_int32_t);
			cdp_seq[cdp_a_int32_t] = bam_nt16_rev_table[cdp_temp_base];
		      }
		      int cdp_snv_base = 0;
		      int cdp_snv_ref_base = 0;
		      int cdp_last_cigar_id = 0;
		      for(cdp_a_loop=0;cdp_a_loop<cdp_cigar_len;cdp_a_loop++)
		      {
			if( cdp_c_type[cdp_a_loop] == BAM_CMATCH || cdp_c_type[cdp_a_loop] == BAM_CEQUAL || cdp_c_type[cdp_a_loop] == BAM_CDIFF )
			{


			  if( cdp_pos >= 0 && cdp_pos < cdp_chr_fasta_len )
			  {
			    cdp_72_loop_start = 0;
			    if( cdp_pos + cdp_snv_ref_base + cdp_c_len[cdp_a_loop] >= cdp_chr_fasta_len )
			    {
			      cdp_72_loop_end = cdp_chr_fasta_len - cdp_pos;
			    }
			    else if( cdp_pos + cdp_c_len[cdp_a_loop] < 0 )
			    {
			      cdp_72_loop_end = cdp_72_loop_start + 1;
			    }
			    else
			    {
			      cdp_72_loop_end = cdp_c_len[cdp_a_loop];
			    }

			    for(cdp_b_loop=cdp_72_loop_start;cdp_b_loop<cdp_72_loop_end;cdp_b_loop++)
			    {
			      if( cdp_mq >= g_min_mapq && cdp_qual[cdp_snv_base] >= g_min_base_qual )
			      {
				cdp_read_name_found = 0;
				if( toupper(cdp_chr_fasta[cdp_pos + cdp_snv_ref_base]) != cdp_seq[cdp_snv_base] )
				{
				  for(cdp_c_loop=0;cdp_c_loop<g_min_snv;cdp_c_loop++)
				  {
				    if( strcmp(cdp_one_base_snv_read_names[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start], "") == 0 )
				    {
				      if( strlen(cdp_read_name) < g_read_name_len )
				      {
						strcpy(cdp_one_base_snv_read_names[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start], cdp_read_name);
				      }
				      break;
				    }
				    else if( strcmp(cdp_one_base_snv_read_names[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start], cdp_read_name) == 0 )
				    {
				      cdp_read_name_found = 1;
				      break;
				    }
				  }
				}
				if( cdp_read_name_found == 0 )
				{

				  char cdp_upper_base = toupper(cdp_seq[cdp_snv_base]);
				  for(cdp_c_loop=0;cdp_c_loop<g_nucleotides;cdp_c_loop++)
				  {
				    if( cdp_upper_base == g_dna[cdp_c_loop] )
				    {
				      if( toupper(cdp_chr_fasta[cdp_pos + cdp_snv_ref_base]) == cdp_upper_base || g_tumor_sv == 1 )
				      {
					cdp_base_index = cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start;
					cdp_one_base_snv[cdp_c_loop][cdp_base_index] += 1;
					cdp_one_base_bq[cdp_base_index] += cdp_qual[cdp_snv_base];
					cdp_one_base_bq_all[cdp_base_index] += cdp_qual[cdp_snv_base];
					cdp_one_base_mq[cdp_base_index] += cdp_mq;
					cdp_one_base_mq_all[cdp_base_index] += cdp_mq;
					cdp_one_base_bq_read_count[cdp_base_index] += 1;
					cdp_one_base_mq_read_count[cdp_base_index] += 1;
					cdp_one_base_read_count_all[cdp_base_index] += 1;

					if( (cdp_flag & BAM_FREVERSE) == 0 )
					{
					  cdp_one_base_pos_in_read[cdp_c_loop][cdp_base_index] += cdp_snv_base;
					  cdp_one_base_fstrand[cdp_c_loop][cdp_base_index] += 1;
					}
					else
					{
					  cdp_one_base_pos_in_read[cdp_c_loop][cdp_base_index] += cdp_lseq - cdp_snv_base;
					}
				      }
				      else
				      {
					int cdp_next_cigar_id = 0;
					if( cdp_b_loop == cdp_c_len[cdp_a_loop] - 1 && cdp_a_loop < cdp_cigar_len - 1 )
					{
					  if( cdp_c_len[cdp_a_loop+1] == BAM_CINS || cdp_c_len[cdp_a_loop+1] == BAM_CDEL )
					  {
					    cdp_next_cigar_id = 1;
					  }
					}
					if( cdp_last_cigar_id >= 0 && cdp_next_cigar_id >= 0 )
					{
					  cdp_one_base_snv[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
					  cdp_one_base_bq[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_qual[cdp_snv_base];
					  cdp_one_base_bq_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_qual[cdp_snv_base];
					  cdp_one_base_mq[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_mq;
					  cdp_one_base_mq_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_mq;
					  cdp_one_base_bq_read_count[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
					  cdp_one_base_mq_read_count[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
					  cdp_one_base_read_count_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;

					  cdp_one_base_pos_in_read[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_snv_base;
					  if( (cdp_flag & BAM_FREVERSE) == 0 )
					  {
					    cdp_one_base_fstrand[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
					  }
					}
				      }
				      break;
				    }
				  }

				}
			      }
			      else
			      {
				char cdp_upper_base = toupper(cdp_seq[cdp_snv_base]);
				for(cdp_c_loop=0;cdp_c_loop<g_nucleotides;cdp_c_loop++)
				{
				  if( cdp_upper_base == g_dna[cdp_c_loop] )
				  {
				    if( toupper(cdp_chr_fasta[cdp_pos + cdp_snv_ref_base]) == cdp_seq[cdp_snv_base] || g_tumor_sv == 1 )
				    {
				      cdp_one_base_snv_lowmq[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
				      cdp_one_base_bq_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_qual[cdp_snv_base];
				      cdp_one_base_mq_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_mq;
				      cdp_one_base_read_count_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
				    }
				    else
				    {
				      int cdp_next_cigar_id = 0;
				      if( cdp_b_loop == cdp_c_len[cdp_a_loop] - 1 && cdp_a_loop < cdp_cigar_len - 1 )
				      {
					if( cdp_c_len[cdp_a_loop+1] == BAM_CINS || cdp_c_len[cdp_a_loop+1] == BAM_CDEL )
					{
					  cdp_next_cigar_id = 1;
					}
				      }
				      if( cdp_last_cigar_id >= 0 && cdp_next_cigar_id >= 0 )
				      {
						cdp_one_base_snv_lowmq[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
						cdp_one_base_bq_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_qual[cdp_snv_base];
						cdp_one_base_mq_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += cdp_mq;
						cdp_one_base_read_count_all[cdp_one_base_index + cdp_pos + cdp_snv_ref_base - cdp_pos_in_contig_start] += 1;
				      }
				    }
				    break;
				  }
				}
			      }
			      cdp_snv_base += 1;
			      cdp_snv_ref_base += 1;
			    }
			    cdp_last_cigar_id = 0;
			  }

			}

			else if( cdp_c_type[cdp_a_loop] == BAM_CSOFT_CLIP || cdp_c_type[cdp_a_loop] == BAM_CHARD_CLIP )
			{
			  if( cdp_c_type[cdp_a_loop] == BAM_CHARD_CLIP )
			  {
			    cdp_lseq += cdp_c_len[cdp_a_loop];
			  }
			  else if( g_tumor_sv == 1 && cdp_mq >= g_min_mapq )
			  {
			    int cdp_snv_sc_base = 0;
			    if( cdp_a_loop == 0 )
			    {
			      cdp_snv_sc_base = -cdp_c_len[cdp_a_loop];
			    }
			    if( cdp_pos >= 0 && cdp_pos < cdp_chr_fasta_len && cdp_pos + cdp_snv_ref_base + cdp_snv_sc_base >= 0 && cdp_pos + cdp_snv_ref_base + cdp_snv_sc_base < cdp_chr_fasta_len )
			    {
			      for(cdp_b_loop=0;cdp_b_loop<cdp_c_len[cdp_a_loop];cdp_b_loop++)
			      {
				if( (cdp_a_loop == 0 && cdp_c_len[cdp_a_loop] - cdp_b_loop <= g_sc_effect_range) || (cdp_a_loop == cdp_cigar_len - 1 && cdp_b_loop <= g_sc_effect_range - 1) )
				{
				  char cdp_upper_base = toupper(cdp_seq[cdp_snv_base]);
				  for(cdp_c_loop=0;cdp_c_loop<g_nucleotides;cdp_c_loop++)
				  {
				    if( cdp_upper_base == g_dna[cdp_c_loop] && cdp_one_base_index + cdp_pos + cdp_snv_ref_base + cdp_snv_sc_base - cdp_pos_in_contig_start > 0 && cdp_one_base_index + cdp_pos + cdp_snv_ref_base + cdp_snv_sc_base - cdp_pos_in_contig_start < g_one_base_rd_len )
				    {
				      cdp_one_base_snv_sc[cdp_c_loop][cdp_one_base_index + cdp_pos + cdp_snv_ref_base + cdp_snv_sc_base - cdp_pos_in_contig_start] += 1;
				      break;
				    }
				  }
				}
				cdp_snv_sc_base += 1;
				cdp_snv_base += 1;
			      }
			    }

			  }
			  else
			  {
			    cdp_snv_base += cdp_c_len[cdp_a_loop];
			  }
			  cdp_last_cigar_id = 0;

			}
			else if( cdp_c_type[cdp_a_loop] == BAM_CINS )
			{
			  cdp_snv_base += cdp_c_len[cdp_a_loop];
			  cdp_last_cigar_id = 1;
			}
			else if( cdp_c_type[cdp_a_loop] == BAM_CDEL )
			{
			  cdp_snv_ref_base += cdp_c_len[cdp_a_loop];
			  cdp_last_cigar_id = 1;
			}
			else if( cdp_c_type[cdp_a_loop] == BAM_CREF_SKIP )
			{
			  cdp_snv_ref_base += cdp_c_len[cdp_a_loop];
			  cdp_last_cigar_id = 0;
			}
			else
			{
			  cdp_last_cigar_id = 0;
			}
		      }

		      if( cdp_c_type[0] == BAM_CSOFT_CLIP || cdp_c_type[0] == BAM_CHARD_CLIP )
		      {
			cdp_start_adj = cdp_c_len[0];
		      }
		      else
		      {
			cdp_start_adj = 0;
		      }
		      cdp_end_adj_indel = 0;
		      if( cdp_c_type[cdp_cigar_len-1] == BAM_CSOFT_CLIP || cdp_c_type[cdp_cigar_len-1] == BAM_CHARD_CLIP )
		      {
			cdp_end_adj = cdp_c_len[cdp_cigar_len-1];
		      }
		      else
		      {
			cdp_end_adj = 0;
		      }

		      for(cdp_a_loop=0;cdp_a_loop<cdp_cigar_len;cdp_a_loop++)
		      {
			if( cdp_c_type[cdp_a_loop] == BAM_CINS )
			{
			  cdp_end_adj_indel += cdp_c_len[cdp_a_loop];
			}
			else if( cdp_c_type[cdp_a_loop] == BAM_CDEL )
			{
			  cdp_end_adj_indel -= cdp_c_len[cdp_a_loop];
			}
		      }
		      if( cdp_start_adj >= g_sc_min )
		      {

			if( (cdp_flag & BAM_FPAIRED) == 0 || ((cdp_flag & BAM_FREVERSE) == 0 && ((cdp_flag & BAM_FMUNMAP) != 0 || ((cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr && cdp_mpos > cdp_pos))) )
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start - 1;
			  cdp_one_base_sc_left[cdp_lp_start] += cdp_add;
			  cdp_one_base_sc_left_rd[cdp_lp_start] += 1;
			  cdp_one_base_sc_rd[cdp_lp_start] += 1;
			}



			if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr != cdp_mchr && (cdp_flag & BAM_FREVERSE) != 0 )
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start - 1;
			  cdp_one_base_ctx_sc_left[cdp_lp_start] += cdp_add;
			  cdp_one_base_ctx_sc_left_rd[cdp_lp_start] += 1;
			  cdp_one_base_ctx_sc_rd[cdp_lp_start] += 1;
			}



			if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr && (cdp_flag & BAM_FREVERSE) != 0  && abs(cdp_tlen) <= g_insert_max_size && cdp_mpos < cdp_pos )
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start - 1;
			  cdp_one_base_indel_sc_left[cdp_lp_start] += cdp_add;
			  cdp_one_base_indel_sc_left_rd[cdp_lp_start] += 1;
			  cdp_one_base_indel_sc_rd[cdp_lp_start] += 1;
			}

		      }

		      if( cdp_end_adj >= g_sc_min )
		      {

			if( (cdp_flag & BAM_FPAIRED) == 0 || ((cdp_flag & BAM_FREVERSE) != 0 && ((cdp_flag & BAM_FMUNMAP) != 0 || ((cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr && cdp_mpos < cdp_pos))) )
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_pos_in_contig_start + cdp_lseq - cdp_end_adj - cdp_end_adj_indel;
			  cdp_one_base_sc_right[cdp_lp_start] += cdp_add;
			  cdp_one_base_sc_right_rd[cdp_lp_start] += 1;
			  cdp_one_base_sc_rd[cdp_lp_start] += 1;
			}



			if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr != cdp_mchr && (cdp_flag & BAM_FREVERSE) == 0 )
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_pos_in_contig_start + cdp_lseq - cdp_end_adj - cdp_end_adj_indel;
			  cdp_one_base_ctx_sc_right[cdp_lp_start] += cdp_add;
			  cdp_one_base_ctx_sc_right_rd[cdp_lp_start] += 1;
			  cdp_one_base_ctx_sc_rd[cdp_lp_start] += 1;
			}



			if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr && (cdp_flag & BAM_FREVERSE) == 0  && abs(cdp_tlen) <= g_insert_max_size && cdp_mpos > cdp_pos )
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_pos_in_contig_start + cdp_lseq - cdp_end_adj - cdp_end_adj_indel;
			  cdp_one_base_indel_sc_right[cdp_lp_start] += cdp_add;
			  cdp_one_base_indel_sc_right_rd[cdp_lp_start] += 1;
			  cdp_one_base_indel_sc_rd[cdp_lp_start] += 1;
			}

		      }



		      cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
		      cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;
		      if( cdp_lp_end > cdp_lp_start )
		      {
			for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
			{
			  cdp_one_base_rd[cdp_a_loop] += 1;
			}
		      }



		      cdp_temp_pos = cdp_pos;
		      cdp_snv_base = 0;
		      for(cdp_c_loop=0;cdp_c_loop<cdp_cigar_len;cdp_c_loop++)
		      {
			if( cdp_c_type[cdp_c_loop] == BAM_CSOFT_CLIP )
			{
			  cdp_snv_base += cdp_c_len[cdp_c_loop];
			}
			else if( cdp_c_type[cdp_c_loop] == BAM_CMATCH || cdp_c_type[cdp_c_loop] == BAM_CREF_SKIP || cdp_c_type[cdp_c_loop] == BAM_CEQUAL || cdp_c_type[cdp_c_loop] == BAM_CDIFF )
			{
			  cdp_temp_pos += cdp_c_len[cdp_c_loop];
			  if( cdp_c_type[cdp_c_loop] != BAM_CREF_SKIP )
			  {
			    cdp_snv_base += cdp_c_len[cdp_c_loop];
			  }
			}


			else if( cdp_c_type[cdp_c_loop] == BAM_CINS )
			{
			  cdp_a_loop = cdp_one_base_index + cdp_temp_pos - cdp_pos_in_contig_start;
			  if( cdp_one_base_indel_i[cdp_a_loop] == 0 )
			  {
			    cdp_one_base_indel_i[cdp_a_loop] = cdp_add;
			    cdp_one_base_indel_idist[cdp_a_loop] = cdp_c_len[cdp_c_loop];
			    if( cdp_one_base_indel_idist[cdp_a_loop] <= g_indel_i_seq_len )
			    {
			      for(cdp_other_loop=0;cdp_other_loop<cdp_one_base_indel_idist[cdp_a_loop];cdp_other_loop++)
			      {
				cdp_one_base_indel_i_seq_set[cdp_other_loop][0] = 1;
				cdp_one_base_indel_i_seq_set[cdp_other_loop][1] = 1;
				cdp_one_base_indel_i_seq[cdp_other_loop][cdp_a_loop] = cdp_seq[cdp_snv_base+cdp_other_loop];
			      }
			    }
			  }
			  else if( cdp_c_len[cdp_c_loop] == cdp_one_base_indel_idist[cdp_a_loop] )
			  {
			    cdp_one_base_indel_i[cdp_a_loop] += cdp_add;
			  }
			  else
			  {
			    cdp_found_other = 0;
			    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			    {
			      if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_INDEL_I )
			      {
				if( cdp_c_len[cdp_c_loop] == (uint32_t)(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] + 0.5) )
				{
				  cdp_found_other = 1;
				  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
				  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_indel_i[cdp_a_loop] )
				  {
				    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
				    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
				    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_indel_i[cdp_a_loop];
				    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_indel_idist[cdp_a_loop];
				    cdp_one_base_indel_i[cdp_a_loop] = cdp_temp_other;
				    cdp_one_base_indel_idist[cdp_a_loop] = (uint32_t)(cdp_temp_other_dist + 0.5);
				  }
				  break;
				}
			      }
			      else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
			      {
				cdp_found_other = 1;
				cdp_one_base_other_set[cdp_other_loop][0] = 1;
				cdp_one_base_other_set[cdp_other_loop][1] = 1;
				cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INDEL_I;
				cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_c_len[cdp_c_loop];
				break;
			      }
			    }

			    if( cdp_found_other == 0 )
			    {
			      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			      {
				if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
				{
				  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INDEL_I;
				  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_c_len[cdp_c_loop];
				  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = 0;
				  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = 0;
				  break;
				}
			      }
			    }
			  }
			  cdp_snv_base += cdp_c_len[cdp_c_loop];
			}



			else if( cdp_c_type[cdp_c_loop] == BAM_CDEL )
			{
			  cdp_a_loop = cdp_one_base_index + cdp_temp_pos - cdp_pos_in_contig_start;
			  cdp_one_base_indel_d_f_rd[cdp_a_loop] += 1;
			  if( cdp_one_base_indel_d_f[cdp_a_loop] == 0 )
			  {
			    cdp_one_base_indel_d_f[cdp_a_loop] = cdp_add;
			    cdp_one_base_indel_d_fdist[cdp_a_loop] = cdp_c_len[cdp_c_loop];
			  }
			  else if( cdp_c_len[cdp_c_loop] == cdp_one_base_indel_d_fdist[cdp_a_loop] )
			  {
			    cdp_one_base_indel_d_f[cdp_a_loop] += cdp_add;
			  }
			  else
			  {
			    cdp_found_other = 0;
			    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			    {
			      if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_INDEL_D_F )
			      {
				if( cdp_c_len[cdp_c_loop] == (uint32_t)(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] + 0.5) )
				{
				  cdp_found_other = 1;
				  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
				  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_indel_d_f[cdp_a_loop] )
				  {
				    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
				    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
				    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_indel_d_f[cdp_a_loop];
				    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_indel_d_fdist[cdp_a_loop];
				    cdp_one_base_indel_d_f[cdp_a_loop] = cdp_temp_other;
				    cdp_one_base_indel_d_fdist[cdp_a_loop] = (uint32_t)(cdp_temp_other_dist + 0.5);
				  }
				  break;
				}
			      }
			      else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
			      {
				cdp_found_other = 1;
				cdp_one_base_other_set[cdp_other_loop][0] = 1;
				cdp_one_base_other_set[cdp_other_loop][1] = 1;
				cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INDEL_D_F;
				cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_c_len[cdp_c_loop];
				break;
			      }
			    }

			    if( cdp_found_other == 0 )
			    {
			      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			      {
				if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
				{
				  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INDEL_D_F;
				  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_c_len[cdp_c_loop];
				  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = 0;
				  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = 0;
				  break;
				}
			      }
			    }
			  }

			  cdp_a_loop += cdp_c_len[cdp_c_loop] - 1;
			  cdp_one_base_indel_d_r_rd[cdp_a_loop] += 1;
			  if( cdp_one_base_indel_d_r[cdp_a_loop] == 0 )
			  {
			    cdp_one_base_indel_d_r[cdp_a_loop] = cdp_add;
			    cdp_one_base_indel_d_rdist[cdp_a_loop] = cdp_c_len[cdp_c_loop];
			  }
			  else if( cdp_c_len[cdp_c_loop] == cdp_one_base_indel_d_rdist[cdp_a_loop] )
			  {
			    cdp_one_base_indel_d_r[cdp_a_loop] += cdp_add;
			  }
			  else
			  {
			    cdp_found_other = 0;
			    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			    {
			      if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_INDEL_D_R )
			      {
				if( cdp_c_len[cdp_c_loop] == (uint32_t)(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] + 0.5) )
				{
				  cdp_found_other = 1;
				  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
				  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_indel_d_r[cdp_a_loop] )
				  {
				    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
				    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
				    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_indel_d_r[cdp_a_loop];
				    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_indel_d_rdist[cdp_a_loop];
				    cdp_one_base_indel_d_r[cdp_a_loop] = cdp_temp_other;
				    cdp_one_base_indel_d_rdist[cdp_a_loop] = (uint32_t)(cdp_temp_other_dist + 0.5);
				  }
				  break;
				}
			      }
			      else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
			      {
				cdp_found_other = 1;
				cdp_one_base_other_set[cdp_other_loop][0] = 1;
				cdp_one_base_other_set[cdp_other_loop][1] = 1;
				cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INDEL_D_R;
				cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_c_len[cdp_c_loop];
				break;
			      }
			    }

			    if( cdp_found_other == 0 )
			    {
			      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			      {
				if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
				{
				  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INDEL_D_R;
				  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_c_len[cdp_c_loop];
				  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = 0;
				  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = 0;
				  break;
				}
			      }
			    }
			  }

			  cdp_temp_pos += cdp_c_len[cdp_c_loop];
			}

		      }

		      if( cdp_aux_pos >= 0 && strncmp(cdp_target_name, cdp_aux_chr, strlen(cdp_target_name)) == 0 )
		      {
			int cdp_sr_del = 0;
			if( cdp_aux_mq >= g_min_mapq && cdp_mq >= g_min_mapq )
			{

			  if( ((cdp_flag & BAM_FREVERSE) == 0 && cdp_aux_strand == 0) || ((cdp_flag & BAM_FREVERSE) != 0 && cdp_aux_strand == 1) )
			  {

				if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr )
			    {
			      if( (cdp_flag & BAM_FREVERSE) == 0 && cdp_aux_strand == 0 )
			      {
				if( cdp_pos < cdp_aux_pos && cdp_tlen <= g_insert_max_size && cdp_aux_pos < cdp_mpos )
				{
				  if( cdp_aux_pos - (cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel) < g_insert_max_size && cdp_aux_pos - (cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel) > 0 )
				  {
				    if( abs(cdp_lseq - cdp_end_adj - cdp_aux_start_adj) <= g_max_split_loss && cdp_lseq - cdp_start_adj - cdp_end_adj - cdp_end_adj_indel >= g_min_sr_len && cdp_lseq - cdp_aux_start_adj - cdp_aux_end_adj - cdp_aux_end_adj_indel >= g_min_sr_len )
				    {

				      cdp_sr_del = 1;
				      cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				      cdp_lp_end = cdp_one_base_index + cdp_aux_pos - cdp_pos_in_contig_start;
				    }
				  }
				}
			      }

				  else if( (cdp_flag & BAM_FREVERSE) != 0 && cdp_aux_strand == 1 )
			      {
				if( cdp_aux_pos < cdp_pos && abs(cdp_tlen) < g_insert_max_size && cdp_mpos < cdp_aux_pos )
				{
				  if( abs(cdp_lseq - cdp_start_adj - cdp_aux_end_adj) <= g_max_split_loss && cdp_lseq - cdp_start_adj - cdp_end_adj - cdp_end_adj_indel >= g_min_sr_len && cdp_lseq - cdp_aux_start_adj - cdp_aux_end_adj - cdp_aux_end_adj_indel >= g_min_sr_len )
				  {

				    cdp_lp_start = cdp_one_base_index + cdp_aux_pos - cdp_aux_start_adj + cdp_lseq - cdp_aux_end_adj - cdp_aux_end_adj_indel - cdp_pos_in_contig_start;
				    cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;
				    if( cdp_lp_start < cdp_lp_end )
				    {
				      cdp_sr_del = 1;
				    }
				  }
				}
			      }
			    }
			    else
			    {

			      if( (cdp_flag & BAM_FREVERSE) == 0 && cdp_aux_strand == 0 )
			      {
				if( cdp_pos < cdp_aux_pos )
				{
				  if( cdp_aux_pos - (cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel) < g_insert_max_size && cdp_aux_pos - (cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel) > 0 )
				  {

				    cdp_sr_del = 1;
				    cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				    cdp_lp_end = cdp_one_base_index + cdp_aux_pos - cdp_pos_in_contig_start;
				  }
				}
			      }

				  else if( (cdp_flag & BAM_FREVERSE) != 0 && cdp_aux_strand == 1 )
			      {
				if( cdp_aux_pos < cdp_pos && cdp_pos - (cdp_aux_pos - cdp_aux_start_adj + cdp_lseq - cdp_aux_end_adj - cdp_aux_end_adj_indel) < g_insert_max_size  )
				{

				  cdp_lp_start = cdp_one_base_index + cdp_aux_pos - cdp_aux_start_adj + cdp_lseq - cdp_aux_end_adj - cdp_aux_end_adj_indel - cdp_pos_in_contig_start;
				  cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;
				  if( cdp_lp_start < cdp_lp_end )
				  {
				    cdp_sr_del = 1;
				  }
				}
			      }

			    }

			  }


			  if( cdp_sr_del == 1 )
			  {

			    if( cdp_lp_end - cdp_lp_start < g_lseq && cdp_lp_end - cdp_lp_start < g_insert_max_size - g_insert_mean )
			    {
			      cdp_one_base_indel_d_f_rd[cdp_lp_start] += 1;
			      if( cdp_one_base_indel_d_f[cdp_lp_start] == 0 )
			      {
				cdp_one_base_indel_d_f[cdp_lp_start] = cdp_add;
				cdp_one_base_indel_d_fdist[cdp_lp_start] = cdp_lp_end - cdp_lp_start;
			      }
			      else if( cdp_lp_end - cdp_lp_start == cdp_one_base_indel_d_fdist[cdp_lp_start] )
			      {
				cdp_one_base_indel_d_f[cdp_lp_start] += cdp_add;
			      }
			      else
			      {
				cdp_found_other = 0;
				for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				{
				  if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] == OTHER_INDEL_D_F )
				  {
				    if( cdp_lp_end - cdp_lp_start == (uint32_t)(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] + 0.5) )
				    {
				      cdp_found_other = 1;
				      cdp_one_base_other[cdp_other_loop][cdp_lp_start] += cdp_add;
				      if( cdp_one_base_other[cdp_other_loop][cdp_lp_start] > cdp_one_base_indel_d_f[cdp_lp_start] )
				      {
					cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_start];
					cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start];
					cdp_one_base_other[cdp_other_loop][cdp_lp_start] = cdp_one_base_indel_d_f[cdp_lp_start];
					cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] = cdp_one_base_indel_d_fdist[cdp_lp_start];
					cdp_one_base_indel_d_f[cdp_lp_start] = cdp_temp_other;
					cdp_one_base_indel_d_fdist[cdp_lp_start] = (uint32_t)(cdp_temp_other_dist + 0.5);
				      }
				      break;
				    }
				  }
				  else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] == OTHER_EMPTY )
				  {
				    cdp_found_other = 1;
				    cdp_one_base_other_set[cdp_other_loop][0] = 1;
				    cdp_one_base_other_set[cdp_other_loop][1] = 1;
				    cdp_one_base_other[cdp_other_loop][cdp_lp_start] = cdp_add;
				    cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] = OTHER_INDEL_D_F;
				    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] = (double) (cdp_lp_end - cdp_lp_start);
				    break;
				  }
				}

				if( cdp_found_other == 0 )
				{
				  for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				  {
				    if( cdp_one_base_other[cdp_other_loop][cdp_lp_start] <= cdp_add )
				    {
				      cdp_one_base_other[cdp_other_loop][cdp_lp_start] = cdp_add;
				      cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] = OTHER_INDEL_D_F;
				      cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] = (double) (cdp_lp_end - cdp_lp_start);
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start] = 0;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = 0;
				      break;
				    }
				  }
				}

			      }

			      cdp_one_base_indel_d_r_rd[cdp_lp_end - 1] += 1;
			      if( cdp_one_base_indel_d_r[cdp_lp_end - 1] == 0 )
			      {
				cdp_one_base_indel_d_r[cdp_lp_end - 1] = cdp_add;
				cdp_one_base_indel_d_rdist[cdp_lp_end - 1] = cdp_lp_end - cdp_lp_start;
			      }
			      else if( cdp_lp_end - cdp_lp_start == cdp_one_base_indel_d_rdist[cdp_lp_end - 1] )
			      {
				cdp_one_base_indel_d_r[cdp_lp_end - 1] += cdp_add;
			      }
			      else
			      {
				cdp_found_other = 0;
				for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				{
				  if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] == OTHER_INDEL_D_R )
				  {
				    if( cdp_lp_end - cdp_lp_start == (uint32_t)(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] + 0.5) )
				    {
				      cdp_found_other = 1;
				      cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] += cdp_add;
				      if( cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] > cdp_one_base_indel_d_r[cdp_lp_end - 1] )
				      {
					cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1];
					cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1];
					cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] = cdp_one_base_indel_d_r[cdp_lp_end - 1];
					cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] = cdp_one_base_indel_d_rdist[cdp_lp_end - 1];
					cdp_one_base_indel_d_r[cdp_lp_end - 1] = cdp_temp_other;
					cdp_one_base_indel_d_rdist[cdp_lp_end - 1] = (uint32_t)(cdp_temp_other_dist + 0.5);
				      }
				      break;
				    }
				  }
				  else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] == OTHER_EMPTY )
				  {
				    cdp_found_other = 1;
				    cdp_one_base_other_set[cdp_other_loop][0] = 1;
				    cdp_one_base_other_set[cdp_other_loop][1] = 1;
				    cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] = cdp_add;
				    cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] = OTHER_INDEL_D_R;
				    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] = (double) (cdp_lp_end - cdp_lp_start);
				    break;
				  }
				}

				if( cdp_found_other == 0 )
				{
				  for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				  {
				    if( cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] <= cdp_add )
				    {
				      cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] = cdp_add;
				      cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] = OTHER_INDEL_D_R;
				      cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] = (double) (cdp_lp_end - cdp_lp_start);
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = 0;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = 0;
				      break;
				    }
				  }
				}

			      }
			    }


			    cdp_one_base_rd[cdp_lp_start] += 1;
			    if( cdp_one_base_del_f[cdp_lp_start] == 0 )
			    {
			      cdp_one_base_del_set[0] = 1;
			      cdp_one_base_del_set[1] = 1;
			      cdp_one_base_del_f[cdp_lp_start] = cdp_add;
			      cdp_one_base_del_fdist[cdp_lp_start] = cdp_lp_end - cdp_lp_start + g_insert_mean;
			      if( cdp_pos < cdp_aux_pos )
			      {
				cdp_one_base_del_f_read_start[cdp_lp_start] = cdp_pos;
				cdp_one_base_del_f_read_end[cdp_lp_start] = cdp_pos;
			      }
			      else
			      {
				cdp_one_base_del_f_read_start[cdp_lp_start] = cdp_aux_pos;
				cdp_one_base_del_f_read_end[cdp_lp_start] = cdp_aux_pos;
			      }
			    }
			    else if( abs(cdp_one_base_del_fdist[cdp_lp_start] - (double) (cdp_lp_end - cdp_lp_start + g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_del_f[cdp_lp_start])) )
			    {
			      cdp_one_base_del_set[0] = 1;
			      cdp_one_base_del_set[1] = 1;
			      cdp_one_base_del_f[cdp_lp_start] += cdp_add;
			      cdp_one_base_del_fdist[cdp_lp_start] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start + g_insert_mean) - cdp_one_base_del_fdist[cdp_lp_start]) / (double) cdp_one_base_del_f[cdp_lp_start];
			      if( cdp_pos < cdp_aux_pos )
			      {
				if( cdp_pos > cdp_one_base_del_f_read_end[cdp_lp_start] )
				{
				  cdp_one_base_del_f_read_end[cdp_lp_start] = cdp_pos;
				}
			      }
			      else
			      {
				if( cdp_aux_pos > cdp_one_base_del_f_read_end[cdp_lp_start] )
				{
				  cdp_one_base_del_f_read_end[cdp_lp_start] = cdp_aux_pos;
				}
			      }
			    }
			    else
			    {
			      cdp_found_other = 0;
			      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			      {
				if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] == OTHER_DEL_F )
				{
				  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] - (double) (cdp_lp_end - cdp_lp_start + g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_lp_start])) )
				  {
				    cdp_found_other = 1;
				    cdp_one_base_other[cdp_other_loop][cdp_lp_start] += cdp_add;
				    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start + g_insert_mean) - cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start]) / (double) cdp_one_base_other[cdp_other_loop][cdp_lp_start];
				    if( cdp_pos < cdp_aux_pos )
				    {
				      if( cdp_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] )
				      {
					cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = cdp_pos;
				      }
				    }
				    else
				    {
				      if( cdp_aux_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] )
				      {
					cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = cdp_aux_pos;
				      }
				    }
				    if( cdp_one_base_other[cdp_other_loop][cdp_lp_start] > cdp_one_base_del_f[cdp_lp_start] )
				    {
				      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_start];
				      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start];
				      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start];
				      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start];
				      cdp_one_base_other[cdp_other_loop][cdp_lp_start] = cdp_one_base_del_f[cdp_lp_start];
				      cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] = cdp_one_base_del_fdist[cdp_lp_start];
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start] = cdp_one_base_del_f_read_start[cdp_lp_start];
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = cdp_one_base_del_f_read_end[cdp_lp_start];
				      cdp_one_base_del_f[cdp_lp_start] = cdp_temp_other;
				      cdp_one_base_del_fdist[cdp_lp_start] = cdp_temp_other_dist;
				      cdp_one_base_del_f_read_start[cdp_lp_start] = cdp_temp_other_read_start;
				      cdp_one_base_del_f_read_end[cdp_lp_start] = cdp_temp_other_read_end;
				    }
				    break;
				  }
				}
				else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] == OTHER_EMPTY )
				{
				  cdp_found_other = 1;
				  cdp_one_base_other_set[cdp_other_loop][0] = 1;
				  cdp_one_base_other_set[cdp_other_loop][1] = 1;
				  cdp_one_base_other[cdp_other_loop][cdp_lp_start] = cdp_add;
				  cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] = OTHER_DEL_F;
				  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] = (double) (cdp_lp_end - cdp_lp_start + g_insert_mean);
				  if( cdp_pos < cdp_aux_pos )
				  {
				    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start] = cdp_pos;
				    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = cdp_pos;
				  }
				  else
				  {
				    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start] = cdp_aux_pos;
				    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = cdp_aux_pos;
				  }
				  break;
				}
			      }

			      if( cdp_found_other == 0 )
			      {
				for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				{
				  if( cdp_one_base_other[cdp_other_loop][cdp_lp_start] <= cdp_add )
				  {
				    cdp_one_base_other[cdp_other_loop][cdp_lp_start] = cdp_add;
				    cdp_one_base_other_type[cdp_other_loop][cdp_lp_start] = OTHER_DEL_F;
				    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start] = (double) (cdp_lp_end - cdp_lp_start + g_insert_mean);
				    if( cdp_pos < cdp_aux_pos )
				    {
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start] = cdp_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = cdp_pos;
				    }
				    else
				    {
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start] = cdp_aux_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start] = cdp_aux_pos;
				    }
				    break;
				  }
				}
			      }

			    }


			    cdp_one_base_rd[cdp_lp_end - 1] += 1;
			    if( cdp_one_base_del_r[cdp_lp_end - 1] == 0 )
			    {
			      cdp_one_base_del_set[0] = 1;
			      cdp_one_base_del_set[1] = 1;
			      cdp_one_base_del_r[cdp_lp_end - 1] = cdp_add;
			      cdp_one_base_del_rdist[cdp_lp_end - 1] = cdp_lp_end - cdp_lp_start + g_insert_mean;
			      if( cdp_pos < cdp_aux_pos )
			      {
				cdp_one_base_del_r_read_start[cdp_lp_end - 1] = cdp_aux_pos;
				cdp_one_base_del_r_read_end[cdp_lp_end - 1] = cdp_aux_pos;
			      }
			      else
			      {
				cdp_one_base_del_r_read_start[cdp_lp_end - 1] = cdp_pos;
				cdp_one_base_del_r_read_end[cdp_lp_end - 1] = cdp_pos;
			      }
			    }
			    else if( abs(cdp_one_base_del_rdist[cdp_lp_end - 1] - (double) (cdp_lp_end - cdp_lp_start + g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_del_r[cdp_lp_end - 1])) )
			    {
			      cdp_one_base_del_set[0] = 1;
			      cdp_one_base_del_set[1] = 1;
			      cdp_one_base_del_r[cdp_lp_end - 1] += cdp_add;
			      cdp_one_base_del_rdist[cdp_lp_end - 1] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start + g_insert_mean) - cdp_one_base_del_rdist[cdp_lp_end - 1]) / (double) cdp_one_base_del_r[cdp_lp_end - 1];
			      if( cdp_pos < cdp_aux_pos )
			      {
				if( cdp_aux_pos < cdp_one_base_del_r_read_start[cdp_lp_end - 1] )
				{
				  cdp_one_base_del_r_read_start[cdp_lp_end - 1] = cdp_aux_pos;
				}
				if( cdp_aux_pos > cdp_one_base_del_r_read_end[cdp_lp_end - 1] )
				{
				  cdp_one_base_del_r_read_end[cdp_lp_end - 1] = cdp_aux_pos;
				}
			      }
			      else
			      {
				if( cdp_pos < cdp_one_base_del_r_read_start[cdp_lp_end - 1] )
				{
				  cdp_one_base_del_r_read_start[cdp_lp_end - 1] = cdp_pos;
				}
				if( cdp_pos > cdp_one_base_del_r_read_end[cdp_lp_end - 1] )
				{
				  cdp_one_base_del_r_read_end[cdp_lp_end - 1] = cdp_pos;
				}
			      }
			    }
			    else
			    {
			      cdp_found_other = 0;
			      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
			      {
				if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] == OTHER_DEL_R)
				{
				  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] - (double) (cdp_lp_end - cdp_lp_start + g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1])) )
				  {
				    cdp_found_other = 1;
				    cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] += cdp_add;
				    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start + g_insert_mean) - cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1]) / (double) cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1];
				    if( cdp_pos < cdp_aux_pos )
				    {
				      if( cdp_aux_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] )
				      {
					cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = cdp_aux_pos;
				      }
				      if( cdp_aux_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] )
				      {
					cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = cdp_aux_pos;
				      }
				    }
				    else
				    {
				      if( cdp_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] )
				      {
					cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = cdp_pos;
				      }
				      if( cdp_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] )
				      {
					cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = cdp_pos;
				      }
				    }
				    if( cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] > cdp_one_base_del_r[cdp_lp_end - 1] )
				    {
				      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1];
				      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1];
				      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1];
				      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1];
				      cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] = cdp_one_base_del_r[cdp_lp_end - 1];
				      cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] = cdp_one_base_del_rdist[cdp_lp_end - 1];
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = cdp_one_base_del_r_read_start[cdp_lp_end - 1];
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = cdp_one_base_del_r_read_end[cdp_lp_end - 1];
				      cdp_one_base_del_r[cdp_lp_end - 1] = cdp_temp_other;
				      cdp_one_base_del_rdist[cdp_lp_end - 1] = cdp_temp_other_dist;
				      cdp_one_base_del_r_read_start[cdp_lp_end - 1] = cdp_temp_other_read_start;
				      cdp_one_base_del_r_read_end[cdp_lp_end - 1] = cdp_temp_other_read_end;
				    }
				    break;
				  }
				}
				else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] == OTHER_EMPTY )
				{
				  cdp_found_other = 1;
				  cdp_one_base_other_set[cdp_other_loop][0] = 1;
				  cdp_one_base_other_set[cdp_other_loop][1] = 1;
				  cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] = cdp_add;
				  cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] = OTHER_DEL_R;
				  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] = (double) (cdp_lp_end - cdp_lp_start + g_insert_mean);
				  if( cdp_pos < cdp_aux_pos )
				  {
				    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = cdp_aux_pos;
				    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = cdp_aux_pos;
				  }
				  else
				  {
				    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = cdp_pos;
				    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = cdp_pos;
				  }
				  break;
				}
			      }

			      if( cdp_found_other == 0 )
			      {
				for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				{
				  if( cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] <= cdp_add )
				  {
				    cdp_one_base_other[cdp_other_loop][cdp_lp_end - 1] = cdp_add;
				    cdp_one_base_other_type[cdp_other_loop][cdp_lp_end - 1] = OTHER_DEL_R;
				    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end - 1] = (double) (cdp_lp_end - cdp_lp_start + g_insert_mean);
				    if( cdp_pos < cdp_aux_pos )
				    {
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = cdp_aux_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = cdp_aux_pos;
				    }
				    else
				    {
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end - 1] = cdp_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end - 1] = cdp_pos;
				    }
				    break;
				  }
				}
			      }
			    }
			  }


			}

		      }


		      int cdp_insert_temp = 0;
		      if( g_insert_mean - 2*cdp_lseq > 0 )
		      {
			cdp_insert_temp = g_insert_mean - 2*cdp_lseq;
		      }


		      if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 )
		      {

			if( cdp_chr == cdp_mchr )
			{

			  if( cdp_mpos > cdp_pos )
			  {


			    if( (cdp_flag & BAM_FREVERSE) == 0 && (cdp_flag & BAM_FMREVERSE) != 0 )
			    {

			      if( cdp_tlen >= g_insert_min_size && cdp_tlen <= g_insert_max_size )
			      {
				int cdp_sr_dup = 0;

				if( cdp_aux_pos >= 0 && strncmp(cdp_target_name, cdp_aux_chr, strlen(cdp_target_name)) == 0 )
				{
				  if( cdp_aux_mq >= g_min_mapq && cdp_mq >= g_min_mapq )
				  {

					if( (cdp_flag & BAM_FREVERSE) == 0 && cdp_aux_strand == 0 )
				    {

					  if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr )
				      {
					if( cdp_pos < cdp_aux_pos && cdp_aux_pos < cdp_mpos )
					{
					  int cdp_end_adj_indel_temp = 0;

					  if( cdp_end_adj_indel > 0 )
					  {
					    cdp_end_adj_indel_temp = cdp_end_adj_indel;
					  }
					  int cdp_aux_end_adj_indel_temp = 0;

					  if( cdp_aux_end_adj_indel > 0 )
					  {
					    cdp_aux_end_adj_indel_temp = cdp_end_adj_indel;
					  }
					  if( abs(cdp_lseq - cdp_start_adj - cdp_aux_end_adj) <= g_max_split_loss && cdp_lseq - cdp_start_adj - cdp_end_adj - cdp_end_adj_indel_temp >= g_min_sr_len && cdp_lseq - cdp_aux_start_adj - cdp_aux_end_adj - cdp_aux_end_adj_indel_temp >= g_min_sr_len )
					  {

					    cdp_sr_dup = 1;
					    cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;
					    cdp_lp_end = cdp_one_base_index + cdp_aux_pos - cdp_aux_start_adj + cdp_lseq - cdp_aux_end_adj - cdp_aux_end_adj_indel - cdp_pos_in_contig_start;
					  }
					}
				      }
				    }
				  }
				}


				if( cdp_sr_dup == 1 )
				{
				  cdp_one_base_rd[cdp_lp_end] += 1;
				  if( cdp_one_base_dup_f[cdp_lp_end] == 0 )
				  {
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_f[cdp_lp_end] = cdp_add;
				    cdp_one_base_dup_fdist[cdp_lp_end] = cdp_lp_end - cdp_lp_start - g_insert_mean;
				    if( cdp_pos < cdp_aux_pos )
				    {
				      cdp_one_base_del_set[0] = 1;
				      cdp_one_base_del_set[1] = 1;
				      cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_aux_pos;
				      cdp_one_base_del_f_read_end[cdp_lp_end] = cdp_aux_pos;
				    }
				    else
				    {
				      cdp_one_base_del_set[0] = 1;
				      cdp_one_base_del_set[1] = 1;
				      cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_pos;
				      cdp_one_base_del_f_read_end[cdp_lp_end] = cdp_pos;
				    }
				  }
				  else if( abs(cdp_one_base_dup_fdist[cdp_lp_end] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_dup_f[cdp_lp_end])) )
				  {
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_f[cdp_lp_end] += cdp_add;
				    cdp_one_base_dup_fdist[cdp_lp_end] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_dup_fdist[cdp_lp_end]) / (double) cdp_one_base_dup_f[cdp_lp_end];
				    if( cdp_pos < cdp_aux_pos )
				    {
				      if( cdp_aux_pos > cdp_one_base_dup_f_read_end[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_end[cdp_lp_end] = cdp_aux_pos;
				      }
				      if( cdp_aux_pos < cdp_one_base_dup_f_read_start[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_aux_pos;
				      }
				    }
				    else
				    {
				      if( cdp_pos > cdp_one_base_dup_f_read_end[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_end[cdp_lp_end] = cdp_pos;
				      }
				      if( cdp_pos < cdp_one_base_dup_f_read_start[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_pos;
				      }
				    }
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] == OTHER_DUP_F )
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_lp_end])) )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other[cdp_other_loop][cdp_lp_end] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end]) / (double) cdp_one_base_other[cdp_other_loop][cdp_lp_end];
					  if( cdp_pos < cdp_aux_pos )
					  {
					    if( cdp_aux_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					    }
					    if( cdp_aux_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					    }
					  }
					  else
					  {
					    if( cdp_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_pos;
					    }
					    if( cdp_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_pos;
					    }
					  }
					  if( cdp_one_base_other[cdp_other_loop][cdp_lp_end] > cdp_one_base_dup_f[cdp_lp_end] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_end];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end];
					    cdp_one_base_other[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_f[cdp_lp_end];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_fdist[cdp_lp_end];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_f_read_start[cdp_lp_end];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_f_read_end[cdp_lp_end];
					    cdp_one_base_dup_f[cdp_lp_end] = cdp_temp_other;
					    cdp_one_base_dup_fdist[cdp_lp_end] = cdp_temp_other_dist;
					    cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_temp_other_read_start;
					    cdp_one_base_dup_f_read_end[cdp_lp_end] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					cdp_one_base_other[cdp_other_loop][cdp_lp_end] = cdp_add;
					cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] = OTHER_DUP_F;
					cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					if( cdp_pos < cdp_aux_pos )
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					}
					else
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_pos;
					}
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_lp_end] <= cdp_add )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_lp_end] = cdp_add;
					  cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] = OTHER_DUP_F;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					  if( cdp_pos < cdp_aux_pos )
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					  }
					  else
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_pos;
					  }
					  break;
					}
				      }
				    }

				  }

				  cdp_one_base_rd[cdp_lp_start - 1] += 1;
				  if( cdp_one_base_dup_r[cdp_lp_start - 1] == 0 )
				  {
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_r[cdp_lp_start - 1] = cdp_add;
				    cdp_one_base_dup_rdist[cdp_lp_start - 1] = cdp_lp_end - cdp_lp_start - g_insert_mean;
				    if( cdp_pos < cdp_aux_pos )
				    {
				      cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_pos;
				      cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_pos;
				    }
				    else
				    {
				      cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_aux_pos;
				      cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_aux_pos;
				    }
				  }
				  else if( abs(cdp_one_base_dup_rdist[cdp_lp_start - 1] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_dup_r[cdp_lp_start - 1])) )
				  {
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_r[cdp_lp_start - 1] += cdp_add;
				    cdp_one_base_dup_rdist[cdp_lp_start - 1] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_dup_rdist[cdp_lp_start - 1]) / (double) cdp_one_base_dup_r[cdp_lp_start - 1];
				    if( cdp_pos < cdp_aux_pos )
				    {
				      if( cdp_pos < cdp_one_base_dup_r_read_start[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_pos;
				      }
				      if( cdp_pos > cdp_one_base_dup_r_read_end[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_pos;
				      }
				    }
				    else
				    {
				      if( cdp_aux_pos < cdp_one_base_dup_r_read_start[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_aux_pos;
				      }
				      if( cdp_aux_pos > cdp_one_base_dup_r_read_end[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_aux_pos;
				      }
				    }
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] == OTHER_DUP_R)
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1])) )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1]) / (double) cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1];
					  if( cdp_pos < cdp_aux_pos )
					  {
					    if( cdp_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					    }
					    if( cdp_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					    }
					  }
					  else
					  {
					    if( cdp_aux_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					    }
					    if( cdp_aux_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					    }
					  }
					  if( cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] > cdp_one_base_dup_r[cdp_lp_start - 1] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1];
					    cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_r[cdp_lp_start - 1];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_rdist[cdp_lp_start - 1];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_r_read_start[cdp_lp_start - 1];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_r_read_end[cdp_lp_start - 1];
					    cdp_one_base_dup_r[cdp_lp_start - 1] = cdp_temp_other;
					    cdp_one_base_dup_rdist[cdp_lp_start - 1] = cdp_temp_other_dist;
					    cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_temp_other_read_start;
					    cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] = cdp_add;
					cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] = OTHER_DUP_R;
					cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					if( cdp_pos < cdp_aux_pos )
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					}
					else
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					}
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] <= cdp_add )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] = cdp_add;
					  cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] = OTHER_DUP_R;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					  if( cdp_pos < cdp_aux_pos )
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					  }
					  else
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					  }
					  break;
					}
				      }
				    }

				  }
				}



				if( cdp_sr_dup == 0 )
				{
				  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				  cdp_lp_end = cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start;
				  if( g_one_base_rd_len < cdp_lp_end )
				  {
				    cdp_lp_end = g_one_base_rd_len;
				  }

				  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				  {
				    cdp_one_base_rd[cdp_a_loop] += 1;
				  }
				  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				  {
				    cdp_one_base_conc[cdp_a_loop] += 1;
				  }
				}
			      }



			      else if( cdp_tlen > 2* g_insert_max_size )
			      {
				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				cdp_lp_end = cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start;
				if( g_one_base_rd_len < cdp_lp_end )
				{
				  cdp_lp_end = g_one_base_rd_len;
				}

				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_end_adj_indel + g_insert_max_size - cdp_lseq - cdp_pos_in_contig_start;
				if( g_one_base_rd_len < cdp_lp_end )
				{
				  cdp_lp_end = g_one_base_rd_len;
				}
				if( cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start < cdp_lp_end )
				{
				  cdp_lp_end = cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start;
				}

				for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				{
				  cdp_one_base_rd[cdp_a_loop] += 1;
				  if( cdp_one_base_del_f[cdp_a_loop] == 0 )
				  {
				    cdp_one_base_del_set[0] = 1;
				    cdp_one_base_del_set[1] = 1;
				    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				    {
				      cdp_one_base_del_f[cdp_a_loop] = cdp_add;
				    }
				    else
				    {
				      cdp_one_base_del_f[cdp_a_loop] = cdp_add/2;
				    }
				    cdp_one_base_del_fdist[cdp_a_loop] = (double) cdp_tlen;
				    cdp_one_base_del_f_read_start[cdp_a_loop] = cdp_pos;
				    cdp_one_base_del_f_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else if( abs(cdp_one_base_del_fdist[cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_del_f[cdp_a_loop])) )
				  {
				    cdp_one_base_del_set[0] = 1;
				    cdp_one_base_del_set[1] = 1;
				    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				    {
				      cdp_one_base_del_f[cdp_a_loop] += cdp_add;
				      cdp_one_base_del_fdist[cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_del_fdist[cdp_a_loop]) / (double) cdp_one_base_del_f[cdp_a_loop];
				    }
				    else
				    {
				      cdp_one_base_del_f[cdp_a_loop] += cdp_add/2;
				      cdp_one_base_del_fdist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_del_fdist[cdp_a_loop]) / (double) cdp_one_base_del_f[cdp_a_loop];
				    }
				    cdp_one_base_del_f_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_DEL_F )
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					{
					  cdp_found_other = 1;
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_del_f[cdp_a_loop] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_f[cdp_a_loop];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_fdist[cdp_a_loop];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_f_read_start[cdp_a_loop];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_f_read_end[cdp_a_loop];
					    cdp_one_base_del_f[cdp_a_loop] = cdp_temp_other;
					    cdp_one_base_del_fdist[cdp_a_loop] = cdp_temp_other_dist;
					    cdp_one_base_del_f_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					    cdp_one_base_del_f_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_F;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					{
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_F;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }
				    }

				  }
				}
			      }




			      else if( cdp_tlen > g_insert_max_size )
			      {
				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				cdp_lp_end = cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start;
				if( g_one_base_rd_len < cdp_lp_end )
				{
				  cdp_lp_end = g_one_base_rd_len;
				}
				for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				{
				  cdp_one_base_rd[cdp_a_loop] += 1;
				  if( cdp_a_loop - cdp_one_base_index + cdp_pos_in_contig_start < cdp_pos - cdp_start_adj - cdp_end_adj_indel + g_insert_max_size - cdp_lseq )
				  {
				    if( cdp_one_base_del_f[cdp_a_loop] == 0 )
				    {
				      cdp_one_base_del_set[0] = 1;
				      cdp_one_base_del_set[1] = 1;
				      if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				      {
					cdp_one_base_del_f[cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_del_f[cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_del_fdist[cdp_a_loop] = (double) cdp_tlen;
				      cdp_one_base_del_f_read_start[cdp_a_loop] = cdp_pos;
				      cdp_one_base_del_f_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else if( abs(cdp_one_base_del_fdist[cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_del_f[cdp_a_loop])) )
				    {
				      cdp_one_base_del_set[0] = 1;
				      cdp_one_base_del_set[1] = 1;
				      if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				      {
						cdp_one_base_del_f[cdp_a_loop] += cdp_add;
						cdp_one_base_del_fdist[cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_del_fdist[cdp_a_loop]) / (double) cdp_one_base_del_f[cdp_a_loop];
				      }
				      else
				      {
						cdp_one_base_del_f[cdp_a_loop] += cdp_add/2;
						cdp_one_base_del_fdist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_del_fdist[cdp_a_loop]) / (double) cdp_one_base_del_f[cdp_a_loop];
				      }
				      cdp_one_base_del_f_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else
				    {
				      cdp_found_other = 0;
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_DEL_F )
					{
					  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					  {
					    cdp_found_other = 1;
					    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_del_f[cdp_a_loop] )
					    {
					      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_f[cdp_a_loop];
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_fdist[cdp_a_loop];
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_f_read_start[cdp_a_loop];
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_f_read_end[cdp_a_loop];
					      cdp_one_base_del_f[cdp_a_loop] = cdp_temp_other;
					      cdp_one_base_del_fdist[cdp_a_loop] = cdp_temp_other_dist;
					      cdp_one_base_del_f_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					      cdp_one_base_del_f_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					    }
					    break;
					  }
					}
					else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other_set[cdp_other_loop][0] = 1;
					  cdp_one_base_other_set[cdp_other_loop][1] = 1;
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_F;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }

				      if( cdp_found_other == 0 )
				      {
					for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
					{
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					  {
					    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					    }
					    cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_F;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    break;
					  }
					}
				      }

				    }
				  }
				  if( abs(cdp_tlen) <= 2 * g_insert_max_size && cdp_a_loop - cdp_one_base_index + cdp_pos_in_contig_start > cdp_pos - cdp_start_adj + cdp_tlen - g_insert_max_size + cdp_lseq )
				  {
				    if( cdp_one_base_del_r[cdp_a_loop] == 0 )
				    {
				      cdp_one_base_del_set[0] = 1;
				      cdp_one_base_del_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_del_r[cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_del_r[cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_del_rdist[cdp_a_loop] = (double) cdp_tlen;
				      cdp_one_base_del_r_read_start[cdp_a_loop] = cdp_mpos;
				      cdp_one_base_del_r_read_end[cdp_a_loop] = cdp_mpos;
				    }
				    else if( abs(cdp_one_base_del_rdist[cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_del_r[cdp_a_loop])) )
				    {
				      cdp_one_base_del_set[0] = 1;
				      cdp_one_base_del_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_del_r[cdp_a_loop] += cdp_add;
						cdp_one_base_del_rdist[cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_del_rdist[cdp_a_loop]) / (double) cdp_one_base_del_r[cdp_a_loop];
				      }
				      else
				      {
						cdp_one_base_del_r[cdp_a_loop] += cdp_add/2;
						cdp_one_base_del_rdist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_del_rdist[cdp_a_loop]) / (double) cdp_one_base_del_r[cdp_a_loop];
				      }
				      if( cdp_mpos < cdp_one_base_del_r_read_start[cdp_a_loop] )
				      {
						cdp_one_base_del_r_read_start[cdp_a_loop] = cdp_mpos;
				      }
				      if( cdp_mpos > cdp_one_base_del_r_read_end[cdp_a_loop] )
				      {
						cdp_one_base_del_r_read_end[cdp_a_loop] = cdp_mpos;
				      }
				    }
				    else
				    {
				      cdp_found_other = 0;
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_DEL_R )
					{
					  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					  {
					    cdp_found_other = 1;
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    if( cdp_mpos < cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_mpos;
					    }
					    if( cdp_mpos > cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_mpos;
					    }
					    if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_del_r[cdp_a_loop] )
					    {
					      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_r[cdp_a_loop];
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_rdist[cdp_a_loop];
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_r_read_start[cdp_a_loop];
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_r_read_end[cdp_a_loop];
					      cdp_one_base_del_r[cdp_a_loop] = cdp_temp_other;
					      cdp_one_base_del_rdist[cdp_a_loop] = cdp_temp_other_dist;
					      cdp_one_base_del_r_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					      cdp_one_base_del_r_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					    }
					    break;
					  }
					}
					else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other_set[cdp_other_loop][0] = 1;
					  cdp_one_base_other_set[cdp_other_loop][1] = 1;
					  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_R;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_mpos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_mpos;
					  break;
					}
				      }

				      if( cdp_found_other == 0 )
				      {
					for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
					{
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					  {
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					    }
					    cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_R;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_mpos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_mpos;
					    break;
					  }
					}
				      }

				    }
				  }
				}
			      }



			      else if( cdp_tlen < g_insert_min_size )
			      {

				int cdp_no_ins = 0;

				if( cdp_aux_pos >= 0 && strncmp(cdp_target_name, cdp_aux_chr, strlen(cdp_target_name)) == 0 )
				{

				  if( ((cdp_flag & BAM_FREVERSE) == 0 && cdp_aux_strand == 0) || ((cdp_flag & BAM_FREVERSE) != 0 && cdp_aux_strand == 1) )
				  {

					if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr )
				    {
				      if( (cdp_flag & BAM_FREVERSE) == 0 && cdp_aux_strand == 0 )
				      {
					if( cdp_aux_pos < cdp_pos && cdp_pos < cdp_mpos )
					{
					  cdp_no_ins = 1;
					}

					else if( (cdp_flag & BAM_FREVERSE) != 0 && cdp_aux_strand == 1 )
					{
					  if( cdp_pos < cdp_aux_pos && cdp_mpos < cdp_pos )
					  {
					    cdp_no_ins = 1;
					  }
					}
				      }
				    }
				  }
				}

				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				cdp_lp_end = cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start;
				if( cdp_no_ins == 0 )
				{
				  if( g_one_base_rd_len < cdp_lp_end )
				  {
				    cdp_lp_end = g_one_base_rd_len;
				  }

				  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				  {
				    cdp_one_base_rd[cdp_a_loop] += 1;
				    cdp_one_base_ins[cdp_a_loop] += cdp_add;
				  }
				}
			      }

			    }



			    else if( (cdp_flag & BAM_FREVERSE) == 0 && (cdp_flag & BAM_FMREVERSE) == 0 )
			    {
			      if( (cdp_mpos - cdp_pos) >= min_dup_inv_pair_distance )
			      {

				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj + g_insert_max_size - cdp_lseq - cdp_end_adj_indel - cdp_pos_in_contig_start;
				if( g_one_base_rd_len < cdp_lp_end )
				{
				  cdp_lp_end = g_one_base_rd_len;
				}
				if( cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start < cdp_lp_end )
				{
				  cdp_lp_end = cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start;
				}

				for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				{
				  cdp_one_base_rd[cdp_a_loop] += 1;
				  if( cdp_one_base_inv_f1[cdp_a_loop] == 0 )
				  {
				    cdp_one_base_inv_f_set[0] = 1;
				    cdp_one_base_inv_f_set[1] = 1;
				    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				    {
				      cdp_one_base_inv_f1[cdp_a_loop] = cdp_add;
				    }
				    else
				    {
				      cdp_one_base_inv_f1[cdp_a_loop] = cdp_add/2;
				    }
				    cdp_one_base_inv_f1dist[cdp_a_loop] = (double) cdp_tlen;
				    cdp_one_base_inv_f1_read_start[cdp_a_loop] = cdp_pos;
				    cdp_one_base_inv_f1_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else if( abs(cdp_one_base_inv_f1dist[cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_inv_f1[cdp_a_loop])) )
				  {
				    cdp_one_base_inv_f_set[0] = 1;
				    cdp_one_base_inv_f_set[1] = 1;
				    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				    {
				      cdp_one_base_inv_f1[cdp_a_loop] += cdp_add;
				      cdp_one_base_inv_f1dist[cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_inv_f1dist[cdp_a_loop]) / (double) cdp_one_base_inv_f1[cdp_a_loop];
				    }
				    else
				    {
				      cdp_one_base_inv_f1[cdp_a_loop] += cdp_add/2;
				      cdp_one_base_inv_f1dist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_inv_f1dist[cdp_a_loop]) / (double) cdp_one_base_inv_f1[cdp_a_loop];
				    }
				    cdp_one_base_inv_f1_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_INV_F1 )
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					{
					  cdp_found_other = 1;
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_inv_f1[cdp_a_loop] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f1[cdp_a_loop];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f1dist[cdp_a_loop];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f1_read_start[cdp_a_loop];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f1_read_end[cdp_a_loop];
					    cdp_one_base_inv_f1[cdp_a_loop] = cdp_temp_other;
					    cdp_one_base_inv_f1dist[cdp_a_loop] = cdp_temp_other_dist;
					    cdp_one_base_inv_f1_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					    cdp_one_base_inv_f1_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_F1;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					{
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_F1;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }
				    }

				  }
				}
			      }
			    }



			    else if( (cdp_flag & BAM_FREVERSE) != 0 )
			    {
			      if( (cdp_mpos - cdp_pos) >= min_dup_inv_pair_distance )
			      {
				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				cdp_lp_end = cdp_one_base_index + cdp_mpos - cdp_pos_in_contig_start;
				if( g_one_base_rd_len < cdp_lp_end )
				{
				  cdp_lp_end = g_one_base_rd_len;
				}

				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj - g_insert_max_size + 2*cdp_lseq - cdp_pos_in_contig_start;
				if( cdp_lp_start < 0 )
				{
				  cdp_lp_start = 0;
				}
				cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;


				if( (cdp_flag & BAM_FMREVERSE) != 0 )
				{
				  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				  {
				    cdp_one_base_rd[cdp_a_loop] += 1;
				    if( cdp_one_base_inv_r1[cdp_a_loop] == 0 )
				    {
				      cdp_one_base_inv_r_set[0] = 1;
				      cdp_one_base_inv_r_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_inv_r1[cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_inv_r1[cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_inv_r1dist[cdp_a_loop] = (double) cdp_tlen;
				      cdp_one_base_inv_r1_read_start[cdp_a_loop] = cdp_pos;
				      cdp_one_base_inv_r1_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else if( abs(cdp_one_base_inv_r1dist[cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_inv_r1[cdp_a_loop])) )
				    {
				      cdp_one_base_inv_r_set[0] = 1;
				      cdp_one_base_inv_r_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_inv_r1[cdp_a_loop] += cdp_add;
						cdp_one_base_inv_r1dist[cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_inv_r1dist[cdp_a_loop]) / (double) cdp_one_base_inv_r1[cdp_a_loop];
				      }
				      else
				      {
						cdp_one_base_inv_r1[cdp_a_loop] += cdp_add/2;
						cdp_one_base_inv_r1dist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_inv_r1dist[cdp_a_loop]) / (double) cdp_one_base_inv_r1[cdp_a_loop];
				      }
				      cdp_one_base_inv_r1_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else
				    {
				      cdp_found_other = 0;
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_INV_R1 )
					{
					  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					  {
					    cdp_found_other = 1;
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_inv_r1[cdp_a_loop] )
					    {
					      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r1[cdp_a_loop];
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r1dist[cdp_a_loop];
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r1_read_start[cdp_a_loop];
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r1_read_end[cdp_a_loop];
					      cdp_one_base_inv_r1[cdp_a_loop] = cdp_temp_other;
					      cdp_one_base_inv_r1dist[cdp_a_loop] = cdp_temp_other_dist;
					      cdp_one_base_inv_r1_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					      cdp_one_base_inv_r1_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					    }
					    break;
					  }
					}
					else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other_set[cdp_other_loop][0] = 1;
					  cdp_one_base_other_set[cdp_other_loop][1] = 1;
					  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_R1;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }

				      if( cdp_found_other == 0 )
				      {
					for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
					{
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					  {
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					    }
					    cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_R1;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    break;
					  }
					}
				      }

				    }
				  }
				}



				else
				{
				  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				  {
				    cdp_one_base_rd[cdp_a_loop] += 1;
				    if( cdp_one_base_dup_r[cdp_a_loop] == 0 )
				    {
				      cdp_one_base_dup_set[0] = 1;
				      cdp_one_base_dup_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_dup_r[cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_dup_r[cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_dup_rdist[cdp_a_loop] = (double) cdp_tlen;
				      cdp_one_base_dup_r_read_start[cdp_a_loop] = cdp_pos;
				      cdp_one_base_dup_r_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else if( abs(cdp_one_base_dup_rdist[cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_dup_r[cdp_a_loop])) )
				    {
				      cdp_one_base_dup_set[0] = 1;
				      cdp_one_base_dup_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_dup_r[cdp_a_loop] += cdp_add;
						cdp_one_base_dup_rdist[cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_dup_rdist[cdp_a_loop]) / (double) cdp_one_base_dup_r[cdp_a_loop];
				      }
				      else
				      {
						cdp_one_base_dup_r[cdp_a_loop] += cdp_add/2;
						cdp_one_base_dup_rdist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_dup_rdist[cdp_a_loop]) / (double) cdp_one_base_dup_r[cdp_a_loop];
				      }
				      cdp_one_base_dup_r_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else
				    {
				      cdp_found_other = 0;
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_DUP_R )
					{
					  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_tlen) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					  {
					    cdp_found_other = 1;
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_tlen - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_dup_r[cdp_a_loop] )
					    {
					      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_r[cdp_a_loop];
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_rdist[cdp_a_loop];
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_r_read_start[cdp_a_loop];
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_r_read_end[cdp_a_loop];
					      cdp_one_base_dup_r[cdp_a_loop] = cdp_temp_other;
					      cdp_one_base_dup_rdist[cdp_a_loop] = cdp_temp_other_dist;
					      cdp_one_base_dup_r_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					      cdp_one_base_dup_r_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					    }
					    break;
					  }
					}
					else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other_set[cdp_other_loop][0] = 1;
					  cdp_one_base_other_set[cdp_other_loop][1] = 1;
					  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DUP_R;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }

				      if( cdp_found_other == 0 )
				      {
					for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
					{
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					  {
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					    }
					    cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DUP_R;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_tlen;
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    break;
					  }
					}
				      }

				    }
				  }
				}

			      }
			    }


			  }



			  else
			  {

			    if( (cdp_flag & BAM_FREVERSE) != 0 && (cdp_flag & BAM_FMREVERSE) == 0 )
			    {


			      if( abs(cdp_tlen) >= g_insert_min_size && abs(cdp_tlen) <= g_insert_max_size )
			      {
				int cdp_sr_dup = 0;

				if( cdp_aux_pos >= 0 && strncmp(cdp_target_name, cdp_aux_chr, strlen(cdp_target_name)) == 0 )
				{
				  if( cdp_aux_mq >= g_min_mapq && cdp_mq >= g_min_mapq )
				  {

					if( (cdp_flag & BAM_FREVERSE) != 0 && cdp_aux_strand == 1 )
				    {

					  if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) == 0 && cdp_chr == cdp_mchr )
				      {
					if( cdp_aux_pos < cdp_pos && cdp_mpos < cdp_aux_pos )
					{
					  int cdp_end_adj_indel_temp = 0;

					  if( cdp_end_adj_indel > 0 )
					  {
					    cdp_end_adj_indel_temp = cdp_end_adj_indel;
					  }
					  int cdp_aux_end_adj_indel_temp = 0;

					  if( cdp_aux_end_adj_indel > 0 )
					  {
					    cdp_aux_end_adj_indel_temp = cdp_end_adj_indel;
					  }
					  if( abs(cdp_lseq - cdp_aux_start_adj - cdp_end_adj) <= g_max_split_loss && cdp_lseq - cdp_start_adj - cdp_end_adj - cdp_end_adj_indel_temp >= g_min_sr_len && cdp_lseq - cdp_aux_start_adj - cdp_aux_end_adj - cdp_aux_end_adj_indel_temp >= g_min_sr_len )
					  {

					    cdp_lp_start = cdp_one_base_index + cdp_aux_pos - cdp_pos_in_contig_start;
					    cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
					    if( cdp_lp_start < cdp_lp_end )
					    {
					      cdp_sr_dup = 1;
					    }
					  }
					}
				      }
				    }
				  }
				}


				if( cdp_sr_dup == 1 )
				{
				  cdp_one_base_rd[cdp_lp_end] += 1;
				  if( cdp_one_base_dup_f[cdp_lp_end] == 0 )
				  {
				    cdp_one_base_del_set[0] = 1;
				    cdp_one_base_del_set[1] = 1;
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_f[cdp_lp_end] = cdp_add;
				    cdp_one_base_dup_fdist[cdp_lp_end] = cdp_lp_end - cdp_lp_start - g_insert_mean;

				    if( cdp_pos < cdp_aux_pos )
				    {
				      cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_aux_pos;
				      cdp_one_base_del_f_read_end[cdp_lp_end] = cdp_aux_pos;
				    }
				    else
				    {
				      cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_pos;
				      cdp_one_base_del_f_read_end[cdp_lp_end] = cdp_pos;
				    }
				  }
				  else if( abs(cdp_one_base_dup_fdist[cdp_lp_end] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_dup_f[cdp_lp_end])) )
				  {
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_f[cdp_lp_end] += cdp_add;
				    cdp_one_base_dup_fdist[cdp_lp_end] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_dup_fdist[cdp_lp_end]) / (double) cdp_one_base_dup_f[cdp_lp_end];
				    if( cdp_pos < cdp_aux_pos )
				    {
				      if( cdp_aux_pos > cdp_one_base_dup_f_read_end[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_end[cdp_lp_end] = cdp_aux_pos;
				      }
				      if( cdp_aux_pos < cdp_one_base_dup_f_read_start[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_aux_pos;
				      }
				    }
				    else
				    {
				      if( cdp_pos > cdp_one_base_dup_f_read_end[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_end[cdp_lp_end] = cdp_pos;
				      }
				      if( cdp_pos < cdp_one_base_dup_f_read_start[cdp_lp_end] )
				      {
					cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_pos;
				      }
				    }
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] == OTHER_DUP_F )
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_lp_end])) )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other[cdp_other_loop][cdp_lp_end] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end]) / (double) cdp_one_base_other[cdp_other_loop][cdp_lp_end];
					  if( cdp_pos < cdp_aux_pos )
					  {
					    if( cdp_aux_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					    }
					    if( cdp_aux_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					    }
					  }
					  else
					  {
					    if( cdp_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_pos;
					    }
					    if( cdp_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_pos;
					    }
					  }
					  if( cdp_one_base_other[cdp_other_loop][cdp_lp_end] > cdp_one_base_dup_f[cdp_lp_end] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_end];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end];
					    cdp_one_base_other[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_f[cdp_lp_end];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_fdist[cdp_lp_end];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_f_read_start[cdp_lp_end];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_one_base_dup_f_read_end[cdp_lp_end];
					    cdp_one_base_dup_f[cdp_lp_end] = cdp_temp_other;
					    cdp_one_base_dup_fdist[cdp_lp_end] = cdp_temp_other_dist;
					    cdp_one_base_dup_f_read_start[cdp_lp_end] = cdp_temp_other_read_start;
					    cdp_one_base_dup_f_read_end[cdp_lp_end] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					cdp_one_base_other[cdp_other_loop][cdp_lp_end] = cdp_add;
					cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] = OTHER_DUP_F;
					cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					if( cdp_pos < cdp_aux_pos )
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					}
					else
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_pos;
					}
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_lp_end] <= cdp_add )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_lp_end] = cdp_add;
					  cdp_one_base_other_type[cdp_other_loop][cdp_lp_end] = OTHER_DUP_F;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_end] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					  if( cdp_pos < cdp_aux_pos )
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_aux_pos;
					  }
					  else
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_end] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_end] = cdp_pos;
					  }
					  break;
					}
				      }
				    }

				  }

				  cdp_one_base_rd[cdp_lp_start - 1] += 1;
				  if( cdp_one_base_dup_r[cdp_lp_start - 1] == 0 )
				  {
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_r[cdp_lp_start - 1] = cdp_add;
				    cdp_one_base_dup_rdist[cdp_lp_start - 1] = cdp_lp_end - cdp_lp_start - g_insert_mean;
				    if( cdp_pos < cdp_aux_pos )
				    {
				      cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_pos;
				      cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_pos;
				    }
				    else
				    {
				      cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_aux_pos;
				      cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_aux_pos;
				    }
				  }
				  else if( abs(cdp_one_base_dup_rdist[cdp_lp_start - 1] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_dup_r[cdp_lp_start - 1])) )
				  {
				    cdp_one_base_dup_set[0] = 1;
				    cdp_one_base_dup_set[1] = 1;
				    cdp_one_base_dup_r[cdp_lp_start - 1] += cdp_add;
				    cdp_one_base_dup_rdist[cdp_lp_start - 1] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_dup_rdist[cdp_lp_start - 1]) / (double) cdp_one_base_dup_r[cdp_lp_start - 1];
				    if( cdp_pos < cdp_aux_pos )
				    {
				      if( cdp_pos < cdp_one_base_dup_r_read_start[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_pos;
				      }
				      if( cdp_pos > cdp_one_base_dup_r_read_end[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_pos;
				      }
				    }
				    else
				    {
				      if( cdp_aux_pos < cdp_one_base_dup_r_read_start[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_aux_pos;
				      }
				      if( cdp_aux_pos > cdp_one_base_dup_r_read_end[cdp_lp_start - 1] )
				      {
					cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_aux_pos;
				      }
				    }
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] == OTHER_DUP_R)
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] - (double) (cdp_lp_end - cdp_lp_start - g_insert_mean)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1])) )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] += cdp_add_factor_double * ((double) (cdp_lp_end - cdp_lp_start - g_insert_mean) - cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1]) / (double) cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1];
					  if( cdp_pos < cdp_aux_pos )
					  {
					    if( cdp_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					    }
					    if( cdp_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					    }
					  }
					  else
					  {
					    if( cdp_aux_pos < cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					    }
					    if( cdp_aux_pos > cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] )
					    {
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					    }
					  }
					  if( cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] > cdp_one_base_dup_r[cdp_lp_start - 1] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1];
					    cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_r[cdp_lp_start - 1];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_rdist[cdp_lp_start - 1];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_r_read_start[cdp_lp_start - 1];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_one_base_dup_r_read_end[cdp_lp_start - 1];
					    cdp_one_base_dup_r[cdp_lp_start - 1] = cdp_temp_other;
					    cdp_one_base_dup_rdist[cdp_lp_start - 1] = cdp_temp_other_dist;
					    cdp_one_base_dup_r_read_start[cdp_lp_start - 1] = cdp_temp_other_read_start;
					    cdp_one_base_dup_r_read_end[cdp_lp_start - 1] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] = cdp_add;
					cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] = OTHER_DUP_R;
					cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					if( cdp_pos < cdp_aux_pos )
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					}
					else
					{
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					}
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] <= cdp_add )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_lp_start - 1] = cdp_add;
					  cdp_one_base_other_type[cdp_other_loop][cdp_lp_start - 1] = OTHER_DUP_R;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_lp_start - 1] = (double) (cdp_lp_end - cdp_lp_start - g_insert_mean);
					  if( cdp_pos < cdp_aux_pos )
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_pos;
					  }
					  else
					  {
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_lp_start - 1] = cdp_aux_pos;
					  }
					  break;
					}
				      }
				    }

				  }
				}

			      }



			      else if( abs(cdp_tlen) > 2 * g_insert_max_size )
			      {
				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj - g_insert_max_size + 2*cdp_lseq - cdp_pos_in_contig_start;
				if( cdp_lp_start < 0 )
				{
				  cdp_lp_start = 0;
				}
				cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;

				for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				{
				  cdp_one_base_rd[cdp_a_loop] += 1;
				  if( cdp_one_base_del_r[cdp_a_loop] == 0 )
				  {
				    cdp_one_base_del_set[0] = 1;
				    cdp_one_base_del_set[1] = 1;
				    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				    {
				      cdp_one_base_del_r[cdp_a_loop] = cdp_add;
				    }
				    else
				    {
				      cdp_one_base_del_r[cdp_a_loop] = cdp_add/2;
				    }
				    cdp_one_base_del_rdist[cdp_a_loop] = (double) abs(cdp_tlen);
				    cdp_one_base_del_r_read_start[cdp_a_loop] = cdp_pos;
				    cdp_one_base_del_r_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else if( abs(cdp_one_base_del_rdist[cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_del_r[cdp_a_loop])) )
				  {
				    cdp_one_base_del_set[0] = 1;
				    cdp_one_base_del_set[1] = 1;
				    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				    {
				      cdp_one_base_del_r[cdp_a_loop] += cdp_add;
				      cdp_one_base_del_rdist[cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_del_rdist[cdp_a_loop]) / (double) cdp_one_base_del_r[cdp_a_loop];
				    }
				    else
				    {
				      cdp_one_base_del_r[cdp_a_loop] += cdp_add/2;
				      cdp_one_base_del_rdist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_del_rdist[cdp_a_loop]) / (double) cdp_one_base_del_r[cdp_a_loop];
				    }
				    cdp_one_base_del_r_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_DEL_R )
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					{
					  cdp_found_other = 1;
					  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_del_r[cdp_a_loop] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_r[cdp_a_loop];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_rdist[cdp_a_loop];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_r_read_start[cdp_a_loop];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_del_r_read_end[cdp_a_loop];
					    cdp_one_base_del_r[cdp_a_loop] = cdp_temp_other;
					    cdp_one_base_del_rdist[cdp_a_loop] = cdp_temp_other_dist;
					    cdp_one_base_del_r_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					    cdp_one_base_del_r_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_R;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					{
					  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DEL_R;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }
				    }

				  }
				}

			      }



			    }



			    else if( (cdp_flag & BAM_FREVERSE) == 0 && (cdp_flag & BAM_FMREVERSE) == 0 )
			    {
			      if( cdp_pos - cdp_mpos >= min_dup_inv_pair_distance )
			      {
				cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_end_adj_indel + g_insert_max_size - cdp_lseq - cdp_pos_in_contig_start;
				if( g_one_base_rd_len < cdp_lp_end )
				{
				  cdp_lp_end = g_one_base_rd_len;
				}
				for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				{
				  cdp_one_base_rd[cdp_a_loop] += 1;
				  if( cdp_one_base_inv_f2[cdp_a_loop] == 0 )
				  {
				    cdp_one_base_inv_f_set[0] = 1;
				    cdp_one_base_inv_f_set[1] = 1;
				    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				    {
				      cdp_one_base_inv_f2[cdp_a_loop] = cdp_add;
				    }
				    else
				    {
				      cdp_one_base_inv_f2[cdp_a_loop] = cdp_add/2;
				    }
				    cdp_one_base_inv_f2dist[cdp_a_loop] = (double) abs(cdp_tlen);
				    cdp_one_base_inv_f2_read_start[cdp_a_loop] = cdp_pos;
				    cdp_one_base_inv_f2_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else if( abs(cdp_one_base_inv_f2dist[cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_inv_f2[cdp_a_loop])) )
				  {
				    cdp_one_base_inv_f_set[0] = 1;
				    cdp_one_base_inv_f_set[1] = 1;
				    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				    {
				      cdp_one_base_inv_f2[cdp_a_loop] += cdp_add;
				      cdp_one_base_inv_f2dist[cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_inv_f2dist[cdp_a_loop]) / (double) cdp_one_base_inv_f2[cdp_a_loop];
				    }
				    else
				    {
				      cdp_one_base_inv_f2[cdp_a_loop] += cdp_add/2;
				      cdp_one_base_inv_f2dist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_inv_f2dist[cdp_a_loop]) / (double) cdp_one_base_inv_f2[cdp_a_loop];
				    }
				    cdp_one_base_inv_f2_read_end[cdp_a_loop] = cdp_pos;
				  }
				  else
				  {
				    cdp_found_other = 0;
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_INV_F2 )
				      {
					if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					{
					  cdp_found_other = 1;
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  }
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_inv_f2[cdp_a_loop] )
					  {
					    cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					    cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f2[cdp_a_loop];
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f2dist[cdp_a_loop];
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f2_read_start[cdp_a_loop];
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_f2_read_end[cdp_a_loop];
					    cdp_one_base_inv_f2[cdp_a_loop] = cdp_temp_other;
					    cdp_one_base_inv_f2dist[cdp_a_loop] = cdp_temp_other_dist;
					    cdp_one_base_inv_f2_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					    cdp_one_base_inv_f2_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					  }
					  break;
					}
				      }
				      else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				      {
					cdp_found_other = 1;
					cdp_one_base_other_set[cdp_other_loop][0] = 1;
					cdp_one_base_other_set[cdp_other_loop][1] = 1;
					if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_F2;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }

				    if( cdp_found_other == 0 )
				    {
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					{
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_F2;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }
				    }

				  }
				}
			      }
			    }



			    else if( (cdp_flag & BAM_FMREVERSE) != 0 )
			    {
			      if( cdp_pos - cdp_mpos >= min_dup_inv_pair_distance )
			      {

				if( (cdp_flag & BAM_FREVERSE) == 0 )
				{
				  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
				  cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_end_adj_indel + g_insert_max_size - cdp_lseq - cdp_pos_in_contig_start;
				  if( g_one_base_rd_len < cdp_lp_end )
				  {
				    cdp_lp_end = g_one_base_rd_len;
				  }

				  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				  {
				    cdp_one_base_rd[cdp_a_loop] += 1;
				    if( cdp_one_base_dup_f[cdp_a_loop] == 0 )
				    {
				      cdp_one_base_dup_set[0] = 1;
				      cdp_one_base_dup_set[1] = 1;
				      if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				      {
						cdp_one_base_dup_f[cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_dup_f[cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_dup_fdist[cdp_a_loop] = (double) abs(cdp_tlen);
				      cdp_one_base_dup_f_read_start[cdp_a_loop] = cdp_pos;
				      cdp_one_base_dup_f_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else if( abs(cdp_one_base_dup_fdist[cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_dup_f[cdp_a_loop])) )
				    {
				      cdp_one_base_dup_set[0] = 1;
				      cdp_one_base_dup_set[1] = 1;
				      if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				      {
						cdp_one_base_dup_f[cdp_a_loop] += cdp_add;
						cdp_one_base_dup_fdist[cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_dup_fdist[cdp_a_loop]) / (double) cdp_one_base_dup_f[cdp_a_loop];
				      }
				      else
				      {
						cdp_one_base_dup_f[cdp_a_loop] += cdp_add/2;
						cdp_one_base_dup_fdist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_dup_fdist[cdp_a_loop]) / (double) cdp_one_base_dup_f[cdp_a_loop];
				      }
				      cdp_one_base_dup_f_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else
				    {
				      cdp_found_other = 0;
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_DUP_F )
					{
					  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					  {
					    cdp_found_other = 1;
					    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_dup_f[cdp_a_loop] )
					    {
					      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_f[cdp_a_loop];
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_fdist[cdp_a_loop];
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_f_read_start[cdp_a_loop];
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_dup_f_read_end[cdp_a_loop];
					      cdp_one_base_dup_f[cdp_a_loop] = cdp_temp_other;
					      cdp_one_base_dup_fdist[cdp_a_loop] = cdp_temp_other_dist;
					      cdp_one_base_dup_f_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					      cdp_one_base_dup_f_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					    }
					    break;
					  }
					}
					else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other_set[cdp_other_loop][0] = 1;
					  cdp_one_base_other_set[cdp_other_loop][1] = 1;
					  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DUP_F;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }

				      if( cdp_found_other == 0 )
				      {
					for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
					{
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					  {
					    if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					    }
					    cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_DUP_F;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    break;
					  }
					}
				      }

				    }
				  }
				}



				else if( (cdp_flag & BAM_FREVERSE) != 0 )
				{
				  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj - g_insert_max_size + 2*cdp_lseq - cdp_pos_in_contig_start;
				  if( cdp_lp_start < cdp_one_base_index + cdp_mpos + cdp_lseq - cdp_pos_in_contig_start )
				  {
				    cdp_lp_start = cdp_one_base_index + cdp_mpos + cdp_lseq - cdp_pos_in_contig_start;
				  }
				  cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;

				  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
				  {
				    cdp_one_base_rd[cdp_a_loop] += 1;
				    if( cdp_one_base_inv_r2[cdp_a_loop] == 0 )
				    {
				      cdp_one_base_inv_r_set[0] = 1;
				      cdp_one_base_inv_r_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_inv_r2[cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_inv_r2[cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_inv_r2dist[cdp_a_loop] = (double) abs(cdp_tlen);
				      cdp_one_base_inv_r2_read_start[cdp_a_loop] = cdp_pos;
				      cdp_one_base_inv_r2_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else if( abs(cdp_one_base_inv_r2dist[cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_inv_r2[cdp_a_loop])) )
				    {
				      cdp_one_base_inv_r_set[0] = 1;
				      cdp_one_base_inv_r_set[1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_inv_r2[cdp_a_loop] += cdp_add;
						cdp_one_base_inv_r2dist[cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_inv_r2dist[cdp_a_loop]) / (double) cdp_one_base_inv_r2[cdp_a_loop];
				      }
				      else
				      {
						cdp_one_base_inv_r2[cdp_a_loop] += cdp_add/2;
						cdp_one_base_inv_r2dist[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_inv_r2dist[cdp_a_loop]) / (double) cdp_one_base_inv_r2[cdp_a_loop];
				      }
				      cdp_one_base_inv_r2_read_end[cdp_a_loop] = cdp_pos;
				    }
				    else
				    {
				      cdp_found_other = 0;
				      for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				      {
					if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_INV_R2 )
					{
					  if( abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) abs(cdp_tlen)) <= (double) (g_insert_max_size - g_insert_min_size + cdp_insert_temp) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) )
					  {
					    cdp_found_other = 1;
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) abs(cdp_tlen) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					    }
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_inv_r2[cdp_a_loop] )
					    {
					      cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					      cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r2[cdp_a_loop];
					      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r2dist[cdp_a_loop];
					      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r2_read_start[cdp_a_loop];
					      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_inv_r2_read_end[cdp_a_loop];
					      cdp_one_base_inv_r2[cdp_a_loop] = cdp_temp_other;
					      cdp_one_base_inv_r2dist[cdp_a_loop] = cdp_temp_other_dist;
					      cdp_one_base_inv_r2_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					      cdp_one_base_inv_r2_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					    }
					    break;
					  }
					}
					else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
					{
					  cdp_found_other = 1;
					  cdp_one_base_other_set[cdp_other_loop][0] = 1;
					  cdp_one_base_other_set[cdp_other_loop][1] = 1;
					  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					  }
					  else
					  {
					    cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					  }
					  cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_R2;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					  break;
					}
				      }

				      if( cdp_found_other == 0 )
				      {
					for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
					{
					  if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
					  {
					    if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					    }
					    else
					    {
					      cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					    }
					    cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_INV_R2;
					    cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) abs(cdp_tlen);
					    cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					    break;
					  }
					}
				      }

				    }
				  }
				}

			      }
			    }


			  }

			}



			else
			{

			  if( (cdp_flag & BAM_FREVERSE) == 0 )
			  {
			    cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
			    cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_end_adj_indel + g_insert_max_size - cdp_lseq - cdp_pos_in_contig_start;
			    if( g_one_base_rd_len < cdp_lp_end )
			    {
			      cdp_lp_end = g_one_base_rd_len;
			    }

			    if( (cdp_flag & BAM_FMREVERSE) == 0 )
			    {
			      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
			      {
				cdp_one_base_rd[cdp_a_loop] += 1;
				if( cdp_one_base_ctx_f[cdp_a_loop] == 0 )
				{
				  cdp_one_base_ctx_f_set[0] = 1;
				  cdp_one_base_ctx_f_set[1] = 1;
				  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] = cdp_add;
				  }
				  else
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] = cdp_add/2;
				  }
				  cdp_one_base_ctx_f_mchr[cdp_a_loop] = cdp_mchr;
				  cdp_one_base_ctx_f_mpos[cdp_a_loop] = (double)cdp_mpos;
				  cdp_one_base_ctx_f_read_start[cdp_a_loop] = cdp_pos;
				  cdp_one_base_ctx_f_read_end[cdp_a_loop] = cdp_pos;
				}
				else if( cdp_one_base_ctx_f_mchr[cdp_a_loop] == cdp_mchr && abs(cdp_one_base_ctx_f_mpos[cdp_a_loop] - (double)cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_ctx_f[cdp_a_loop])) && cdp_one_base_ctx_f_mpos[cdp_a_loop] > 0 )
				{
				  cdp_one_base_ctx_f_set[0] = 1;
				  cdp_one_base_ctx_f_set[1] = 1;
				  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] += cdp_add;
				    cdp_one_base_ctx_f_mpos[cdp_a_loop] += cdp_add_factor_double * ((double)cdp_mpos - (double)cdp_one_base_ctx_f_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_f[cdp_a_loop];
				  }
				  else
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] += cdp_add/2;
				    cdp_one_base_ctx_f_mpos[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double)cdp_mpos - (double)cdp_one_base_ctx_f_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_f[cdp_a_loop];
				  }
				  cdp_one_base_ctx_f_read_end[cdp_a_loop] = cdp_pos;
				}
				else
				{
				  cdp_found_other = 0;
				  for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				  {
				    if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_CTX_F )
				    {
				      if( cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] == cdp_mchr && abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) && cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] > 0 )
				      {
					cdp_found_other = 1;
					if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_mpos - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_mpos - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_ctx_f[cdp_a_loop] )
					{
					  cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_mchr = cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f[cdp_a_loop];
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_mpos[cdp_a_loop];
					  cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_mchr[cdp_a_loop];
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_read_start[cdp_a_loop];
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_read_end[cdp_a_loop];
					  cdp_one_base_ctx_f[cdp_a_loop] = cdp_temp_other;
					  cdp_one_base_ctx_f_mpos[cdp_a_loop] = cdp_temp_other_dist;
					  cdp_one_base_ctx_f_mchr[cdp_a_loop] = cdp_temp_other_mchr;
					  cdp_one_base_ctx_f_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					  cdp_one_base_ctx_f_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					}
					break;
				      }
				    }
				    else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				    {
				      cdp_found_other = 1;
				      cdp_one_base_other_set[cdp_other_loop][0] = 1;
				      cdp_one_base_other_set[cdp_other_loop][1] = 1;
				      if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_F;
				      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_mpos;
				      cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      break;
				    }
				  }

				  if( cdp_found_other == 0 )
				  {
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
				      {
					if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_F;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_mpos;
					cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }
				  }

				}
			      }
			    }

			    else
			    {

			      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
			      {
				cdp_one_base_rd[cdp_a_loop] += 1;
				if( cdp_one_base_ctx_f[cdp_a_loop] == 0 )
				{
				  cdp_one_base_ctx_f_set[0] = 1;
				  cdp_one_base_ctx_f_set[1] = 1;
				  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] = cdp_add;
				  }
				  else
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] = cdp_add/2;
				  }
				  cdp_one_base_ctx_f_mchr[cdp_a_loop] = cdp_mchr;
				  cdp_one_base_ctx_f_mpos[cdp_a_loop] = (double)(-cdp_mpos);
				  cdp_one_base_ctx_f_read_start[cdp_a_loop] = cdp_pos;
				  cdp_one_base_ctx_f_read_end[cdp_a_loop] = cdp_pos;
				}
				else if( cdp_one_base_ctx_f_mchr[cdp_a_loop] == cdp_mchr && abs(abs(cdp_one_base_ctx_f_mpos[cdp_a_loop]) - (double)cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_ctx_f[cdp_a_loop])) && cdp_one_base_ctx_f_mpos[cdp_a_loop] < 0 )
				{
				  cdp_one_base_ctx_f_set[0] = 1;
				  cdp_one_base_ctx_f_set[1] = 1;
				  if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] += cdp_add;
				    cdp_one_base_ctx_f_mpos[cdp_a_loop] += cdp_add_factor_double * ((double)(-cdp_mpos) - (double)cdp_one_base_ctx_f_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_f[cdp_a_loop];
				  }
				  else
				  {
				    cdp_one_base_ctx_f[cdp_a_loop] += cdp_add/2;
				    cdp_one_base_ctx_f_mpos[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double)(-cdp_mpos) - (double)cdp_one_base_ctx_f_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_f[cdp_a_loop];
				  }
				  cdp_one_base_ctx_f_read_end[cdp_a_loop] = cdp_pos;
				}
				else
				{
				  cdp_found_other = 0;
				  for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				  {
				    if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_CTX_F )
				    {
				      if( cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] == cdp_mchr && abs(abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) - (double) cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) && cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] < 0 )
				      {
					cdp_found_other = 1;
					if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) (-cdp_mpos) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) (-cdp_mpos) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_ctx_f[cdp_a_loop] )
					{
					  cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_mchr = cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f[cdp_a_loop];
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_mpos[cdp_a_loop];
					  cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_mchr[cdp_a_loop];
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_read_start[cdp_a_loop];
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_f_read_end[cdp_a_loop];
					  cdp_one_base_ctx_f[cdp_a_loop] = cdp_temp_other;
					  cdp_one_base_ctx_f_mpos[cdp_a_loop] = cdp_temp_other_dist;
					  cdp_one_base_ctx_f_mchr[cdp_a_loop] = cdp_temp_other_mchr;
					  cdp_one_base_ctx_f_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					  cdp_one_base_ctx_f_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					}
					break;
				      }
				    }
				    else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				    {
				      cdp_found_other = 1;
				      cdp_one_base_other_set[cdp_other_loop][0] = 1;
				      cdp_one_base_other_set[cdp_other_loop][1] = 1;
				      if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_F;
				      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) (-cdp_mpos);
				      cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      break;
				    }
				  }

				  if( cdp_found_other == 0 )
				  {
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
				      {
					if( cdp_end_adj < g_sc_min || cdp_a_loop == cdp_lp_start )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_F;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) (-cdp_mpos);
					cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }
				  }

				}
			      }
			    }
			  }



			  else
			  {
			    cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq -g_insert_max_size + cdp_lseq - cdp_pos_in_contig_start;
			    if( cdp_lp_start < 0 )
			    {
			      cdp_lp_start = 0;
			    }
			    cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;

			    if( (cdp_flag & BAM_FMREVERSE) == 0 )
			    {
			      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
			      {
				cdp_one_base_rd[cdp_a_loop] += 1;
				if( cdp_one_base_ctx_r[cdp_a_loop] == 0 )
				{
				  cdp_one_base_ctx_r_set[0] = 1;
				  cdp_one_base_ctx_r_set[1] = 1;
				  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] = cdp_add;
				  }
				  else
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] = cdp_add/2;
				  }
				  cdp_one_base_ctx_r_mchr[cdp_a_loop] = cdp_mchr;
				  cdp_one_base_ctx_r_mpos[cdp_a_loop] = (double)cdp_mpos;
				  cdp_one_base_ctx_r_read_start[cdp_a_loop] = cdp_pos;
				  cdp_one_base_ctx_r_read_end[cdp_a_loop] = cdp_pos;
				}
				else if( cdp_one_base_ctx_r_mchr[cdp_a_loop] == cdp_mchr && abs(cdp_one_base_ctx_r_mpos[cdp_a_loop] - (double)cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_ctx_r[cdp_a_loop])) && cdp_one_base_ctx_r_mpos[cdp_a_loop] > 0 )
				{
				  cdp_one_base_ctx_r_set[0] = 1;
				  cdp_one_base_ctx_r_set[1] = 1;
				  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] += cdp_add;
				    cdp_one_base_ctx_r_mpos[cdp_a_loop] += cdp_add_factor_double * ((double)cdp_mpos - (double)cdp_one_base_ctx_r_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_r[cdp_a_loop];
				  }
				  else
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] += cdp_add/2;
				    cdp_one_base_ctx_r_mpos[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double)cdp_mpos - (double)cdp_one_base_ctx_r_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_r[cdp_a_loop];
				  }
				  cdp_one_base_ctx_r_read_end[cdp_a_loop] = cdp_pos;
				}
				else
				{
				  cdp_found_other = 0;
				  for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				  {
				    if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_CTX_R )
				    {
				      if( cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] == cdp_mchr && abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] - (double) cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) && cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] > 0 )
				      {
					cdp_found_other = 1;
					if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) cdp_mpos - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) cdp_mpos - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_ctx_r[cdp_a_loop] )
					{
					  cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_mchr = cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r[cdp_a_loop];
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_mpos[cdp_a_loop];
					  cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_mchr[cdp_a_loop];
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_read_start[cdp_a_loop];
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_read_end[cdp_a_loop];
					  cdp_one_base_ctx_r[cdp_a_loop] = cdp_temp_other;
					  cdp_one_base_ctx_r_mpos[cdp_a_loop] = cdp_temp_other_dist;
					  cdp_one_base_ctx_r_mchr[cdp_a_loop] = cdp_temp_other_mchr;
					  cdp_one_base_ctx_r_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					  cdp_one_base_ctx_r_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					}
					break;
				      }
				    }
				    else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				    {
				      cdp_found_other = 1;
				      cdp_one_base_other_set[cdp_other_loop][0] = 1;
				      cdp_one_base_other_set[cdp_other_loop][1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_R;
				      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_mpos;
				      cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      break;
				    }
				  }

				  if( cdp_found_other == 0 )
				  {
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
				      {
					if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_R;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) cdp_mpos;
					cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }
				  }

				}
			      }
			    }
			    else
			    {

			      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
			      {
				cdp_one_base_rd[cdp_a_loop] += 1;
				if( cdp_one_base_ctx_r[cdp_a_loop] == 0 )
				{
				  cdp_one_base_ctx_r_set[0] = 1;
				  cdp_one_base_ctx_r_set[1] = 1;
				  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] = cdp_add;
				  }
				  else
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] = cdp_add/2;
				  }
				  cdp_one_base_ctx_r_mchr[cdp_a_loop] = cdp_mchr;
				  cdp_one_base_ctx_r_mpos[cdp_a_loop] = (double) (-cdp_mpos);
				  cdp_one_base_ctx_r_read_start[cdp_a_loop] = cdp_pos;
				  cdp_one_base_ctx_r_read_end[cdp_a_loop] = cdp_pos;
				}
				else if( cdp_one_base_ctx_r_mchr[cdp_a_loop] == cdp_mchr && abs(abs(cdp_one_base_ctx_r_mpos[cdp_a_loop]) - (double)cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_ctx_r[cdp_a_loop])) && cdp_one_base_ctx_r_mpos[cdp_a_loop] < 0 )
				{
				  cdp_one_base_ctx_r_set[0] = 1;
				  cdp_one_base_ctx_r_set[1] = 1;
				  if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] += cdp_add;
				    cdp_one_base_ctx_r_mpos[cdp_a_loop] += cdp_add_factor_double * ((double)(-cdp_mpos) - (double)cdp_one_base_ctx_r_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_r[cdp_a_loop];
				  }
				  else
				  {
				    cdp_one_base_ctx_r[cdp_a_loop] += cdp_add/2;
				    cdp_one_base_ctx_r_mpos[cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double)(-cdp_mpos) - (double)cdp_one_base_ctx_r_mpos[cdp_a_loop])/(double)cdp_one_base_ctx_r[cdp_a_loop];
				  }
				  cdp_one_base_ctx_r_read_end[cdp_a_loop] = cdp_pos;
				}
				else
				{
				  cdp_found_other = 0;
				  for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				  {
				    if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_CTX_R )
				    {
				      if( cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] == cdp_mchr && abs(abs(cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) - (double) cdp_mpos) <= (double) (g_insert_max_size - g_insert_min_size) * (1.0 + (1.0/(double) cdp_one_base_other[cdp_other_loop][cdp_a_loop])) && cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] < 0 )
				      {
					cdp_found_other = 1;
					if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += cdp_add_factor_double * ((double) (-cdp_mpos) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] += cdp_add/2;
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] += (cdp_add_factor_double/2.0) * ((double) (-cdp_mpos) - cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop]) / (double) cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					}
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] > cdp_one_base_ctx_r[cdp_a_loop] )
					{
					  cdp_temp_other = cdp_one_base_other[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_dist = cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_mchr = cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_start = cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop];
					  cdp_temp_other_read_end = cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop];
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r[cdp_a_loop];
					  cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_mpos[cdp_a_loop];
					  cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_mchr[cdp_a_loop];
					  cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_read_start[cdp_a_loop];
					  cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_one_base_ctx_r_read_end[cdp_a_loop];
					  cdp_one_base_ctx_r[cdp_a_loop] = cdp_temp_other;
					  cdp_one_base_ctx_r_mpos[cdp_a_loop] = cdp_temp_other_dist;
					  cdp_one_base_ctx_r_mchr[cdp_a_loop] = cdp_temp_other_mchr;
					  cdp_one_base_ctx_r_read_start[cdp_a_loop] = cdp_temp_other_read_start;
					  cdp_one_base_ctx_r_read_end[cdp_a_loop] = cdp_temp_other_read_end;
					}
					break;
				      }
				    }
				    else if( cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] == OTHER_EMPTY )
				    {
				      cdp_found_other = 1;
				      cdp_one_base_other_set[cdp_other_loop][0] = 1;
				      cdp_one_base_other_set[cdp_other_loop][1] = 1;
				      if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
				      }
				      else
				      {
						cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
				      }
				      cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_R;
				      cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) (-cdp_mpos);
				      cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
				      cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
				      break;
				    }
				  }

				  if( cdp_found_other == 0 )
				  {
				    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
				    {
				      if( cdp_one_base_other[cdp_other_loop][cdp_a_loop] <= cdp_add )
				      {
					if( cdp_start_adj < g_sc_min || cdp_a_loop == (cdp_lp_end - 1) )
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add;
					}
					else
					{
					  cdp_one_base_other[cdp_other_loop][cdp_a_loop] = cdp_add/2;
					}
					cdp_one_base_other_type[cdp_other_loop][cdp_a_loop] = OTHER_CTX_R;
					cdp_one_base_other_dist[cdp_other_loop][cdp_a_loop] = (double) (-cdp_mpos);
					cdp_one_base_other_mchr[cdp_other_loop][cdp_a_loop] = cdp_mchr;
					cdp_one_base_other_read_start[cdp_other_loop][cdp_a_loop] = cdp_pos;
					cdp_one_base_other_read_end[cdp_other_loop][cdp_a_loop] = cdp_pos;
					break;
				      }
				    }
				  }

				}
			      }
			    }
			  }

			}

		      }



		      else if( (cdp_flag & BAM_FPAIRED) != 0 && (cdp_flag & BAM_FMUNMAP) != 0 )
		      {

			if( (cdp_flag & BAM_FREVERSE) == 0 )
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq - cdp_end_adj - cdp_end_adj_indel - cdp_pos_in_contig_start;
			  cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_start_adj - cdp_end_adj_indel + g_insert_max_size - cdp_lseq - cdp_pos_in_contig_start;
			  if( g_one_base_rd_len < cdp_lp_end )
			  {
			    cdp_lp_end = g_one_base_rd_len;
			  }
			  cdp_one_base_munmapped_f_set[0] = 1;
			  cdp_one_base_munmapped_f_set[1] = 1;
			  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
			  {
			    cdp_one_base_rd[cdp_a_loop] += 1;
			    cdp_one_base_munmapped_f[cdp_a_loop] += cdp_add;
			  }
			}



			else
			{
			  cdp_lp_start = cdp_one_base_index + cdp_pos - cdp_start_adj + cdp_lseq + cdp_end_adj_indel - g_insert_max_size + cdp_lseq - cdp_pos_in_contig_start;
			  if( cdp_lp_start < 0 )
			  {
			    cdp_lp_start = 0;
			  }
			  cdp_lp_end = cdp_one_base_index + cdp_pos - cdp_pos_in_contig_start;
			  cdp_one_base_munmapped_r_set[0] = 1;
			  cdp_one_base_munmapped_r_set[1] = 1;
			  for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
			  {
			    cdp_one_base_rd[cdp_a_loop] += 1;
			    cdp_one_base_munmapped_r[cdp_a_loop] += cdp_add;
			  }
			}

		      }

#ifdef DO_TIMING
		      end_t = rdtsc();
		      timers_ss[2] += end_t - start_t;
#endif
		    }

		  }

		  if( my_samread(cdp_bam_file, cdp_b) > 0 )
		  {
		    cdp_pos = cdp_b->core.pos;
		    cdp_flag = cdp_b->core.flag;
		    cdp_mq = cdp_b->core.qual;
		    cdp_chr = cdp_b->core.tid;
		    cdp_mchr = cdp_b->core.mtid;
		    cdp_mpos = cdp_b->core.mpos;
		    cdp_tlen = cdp_b->core.isize;
		    cdp_lseq = cdp_b->core.l_qseq;
		    if( cdp_mq >= g_min_mapq )
		    {
		      cdp_add = cdp_add_factor;
		    }
		    else
		    {
		      cdp_add = cdp_add_factor_lowmq;
		    }
		    cdp_add_factor_double = (double) cdp_add;
		    cdp_aux_pos = -1;
		    cdp_aux_mq = -1;
		    cdp_l_aux = bam_get_l_aux(cdp_b);

		    if( (cdp_flag & BAM_FUNMAP) == 0 && (cdp_flag & BAM_FDUP) == 0 && (cdp_flag & BAM_FQCFAIL) == 0 )
		    {
		      if( g_splitread == 1 && cdp_l_aux > 0 && cdp_l_aux < aux_str_len )
		      {
			cdp_aux = bam_aux_get(cdp_b,"XP");
			if( cdp_aux )
			{
			  if( cdp_aux[0] == 'Z' )
			  {
			    memmove(cdp_aux_str, cdp_aux+1, cdp_l_aux);
			  }
			  else
			  {
			    memmove(cdp_aux_str, cdp_aux, cdp_l_aux+1);
			  }
			  cdp_aux_chr = strtok((char *) cdp_aux_str, g_aux_separator);
			  cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			  if( cdp_aux_str_temp[0] == '+' )
			  {
			    cdp_aux_strand = 0;
			  }
			  else
			  {
			    cdp_aux_strand = 1;
			  }
			  memmove(cdp_aux_str_temp, cdp_aux_str_temp+1, strlen(cdp_aux_str_temp));
			  cdp_aux_pos = atoi(cdp_aux_str_temp);
			  cdp_aux_cigar = strtok(NULL, g_aux_separator);
			  cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			  cdp_aux_mq = atoi(cdp_aux_str_temp);
			}

			else
			{
			  cdp_aux = bam_aux_get(cdp_b,"SA");
			  if( cdp_aux )
			  {
			    if( cdp_aux[0] == 'Z' )
			    {
			      memmove(cdp_aux_str, cdp_aux+1, cdp_l_aux);
			    }
			    else
			    {
			      memmove(cdp_aux_str, cdp_aux, cdp_l_aux+1);
			    }
			    cdp_aux_chr = strtok((char *) cdp_aux_str, g_aux_separator);
			    cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			    cdp_aux_pos = atoi(cdp_aux_str_temp);
			    cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			    if( cdp_aux_str_temp[0] == '+' )
			    {
			      cdp_aux_strand = 0;
			    }
			    else
			    {
			      cdp_aux_strand = 1;
			    }
			    cdp_aux_cigar = strtok(NULL, g_aux_separator);
			    cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			    cdp_aux_mq = atoi(cdp_aux_str_temp);
			  }
			}

		      }
		    }


		    if( cdp_chr != cdp_chr_match )
		    {
		      cdp_begin = 2;
		    }
		  }
		  else
		  {
		    cdp_begin = 2;
		  }

		}
	      }
	      if( cdp_pos_in_contig_start > 2*g_insert_max_size && g_tumor_sv == 0 )
	      {


#ifdef DO_TIMING
		start_t = rdtsc();
#endif
		if( cdp_one_base_rd[cdp_one_base_index] + cdp_one_base_indel_sc_rd[cdp_one_base_index] > 0 )
		{
#ifdef DO_OTHERLEN_STDEV
		  if( (cdp_pos_in_contig_start / g_other_len_sampling) * g_other_len_sampling == cdp_pos_in_contig_start )
		  {
		    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
		    {
		      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
		      {
			cdp_other_len_total_list[cdp_other_len_count] = cdp_tumor_loop2;
			break;
		      }
		      else if( cdp_tumor_loop2 == g_other_len - 1 )
		      {
			cdp_other_len_total_list[cdp_other_len_count] = cdp_tumor_loop2 + 1;
		      }
		    }
		    cdp_other_len_count += 1;
		  }
#endif

		  if( cdp_chr_fasta[cdp_pos_in_contig_start] != 'N' && cdp_chr_fasta[cdp_pos_in_contig_start] != 'n' )
		  {
		    cdp_snv_total = 0;
		    for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
		    {
		      cdp_snv_total += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index];
		    }
		    for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
		    {
		      cdp_snv_ratio[cdp_snv_a_loop] = (float) cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] / (float) cdp_snv_total;
		      if( cdp_snv_total > g_max_trials )
		      {
				cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] * g_max_trials/cdp_snv_total];
				cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] * g_max_trials/cdp_snv_total];
		      }
		      else
		      {
				cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_snv_total][cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index]];
				cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_snv_total][cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index]];
		      }

			  if( toupper(cdp_chr_fasta[cdp_pos_in_contig_start]) != g_dna[cdp_snv_a_loop] && cdp_snv_ratio[cdp_snv_a_loop] >= g_min_snv_ratio && cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] >= g_min_snv && (double)cdp_one_base_bq_all[cdp_one_base_index] / (double)cdp_one_base_read_count_all[cdp_one_base_index] >= g_min_ave_bq )
		      {
			if( cdp_snv_list_index > 0 && cdp_snv_pos_list[cdp_snv_list_index-1] == cdp_pos_in_contig_start )
			{
			  if( cdp_snv_ratio[cdp_snv_a_loop] > cdp_snv_ratio_list[cdp_snv_list_index-1] )
			  {
			    cdp_snv_ratio_list[cdp_snv_list_index-1] = cdp_snv_ratio[cdp_snv_a_loop];
			    cdp_snv_base_list[cdp_snv_list_index-1] = cdp_snv_a_loop;
			    cdp_snv_start_binom_cdf_list[cdp_snv_list_index-1] = cdp_binom_cdf;
			    cdp_snv_start_hez_binom_cdf_list[cdp_snv_list_index-1] = cdp_hez_binom_cdf;
			  }
			}
			else
			{
			  cdp_snv_pos_list[cdp_snv_list_index] = cdp_pos_in_contig_start;
			  for(cdp_snv_b_loop=0;cdp_snv_b_loop<g_nucleotides;cdp_snv_b_loop++)
			  {
			    cdp_snv_list[cdp_snv_b_loop][cdp_snv_list_index] = cdp_one_base_snv[cdp_snv_b_loop][cdp_one_base_index];
			    cdp_snv_lowmq_list[cdp_snv_b_loop][cdp_snv_list_index] = cdp_one_base_snv_lowmq[cdp_snv_b_loop][cdp_one_base_index];
			  }
			  cdp_snv_ratio_list[cdp_snv_list_index] = cdp_snv_ratio[cdp_snv_a_loop];
			  cdp_snv_base_list[cdp_snv_list_index] = cdp_snv_a_loop;
			  cdp_snv_start_binom_cdf_list[cdp_snv_list_index] = cdp_binom_cdf;
			  cdp_snv_start_hez_binom_cdf_list[cdp_snv_list_index] = cdp_hez_binom_cdf;
			  cdp_snv_bq_list[cdp_snv_list_index] = cdp_one_base_bq[cdp_one_base_index];
			  cdp_snv_bq_all_list[cdp_snv_list_index] = cdp_one_base_bq_all[cdp_one_base_index];
			  cdp_snv_mq_list[cdp_snv_list_index] = cdp_one_base_mq[cdp_one_base_index];
			  cdp_snv_mq_all_list[cdp_snv_list_index] = cdp_one_base_mq_all[cdp_one_base_index];
			  cdp_snv_bq_read_count_list[cdp_snv_list_index] = cdp_one_base_bq_read_count[cdp_one_base_index];
			  cdp_snv_mq_read_count_list[cdp_snv_list_index] = cdp_one_base_mq_read_count[cdp_one_base_index];
			  cdp_snv_read_count_all_list[cdp_snv_list_index] = cdp_one_base_read_count_all[cdp_one_base_index];

			  for(cdp_snv_b_loop=0;cdp_snv_b_loop<g_nucleotides;cdp_snv_b_loop++)
			  {
			    cdp_snv_pos_in_read_list[cdp_snv_b_loop][cdp_snv_list_index] = cdp_one_base_pos_in_read[cdp_snv_b_loop][cdp_one_base_index];
			    cdp_snv_fstrand_list[cdp_snv_b_loop][cdp_snv_list_index] = cdp_one_base_fstrand[cdp_snv_b_loop][cdp_one_base_index];
			  }
			  cdp_snv_list_index += 1;
			}
		      }
		    }


		    if( cdp_snv_list_index >= g_sv_list_len - 10 )
		    {
		      for(cdp_a_loop=caf_last_snv_group_pos;cdp_a_loop<cdp_pos_in_contig_start - cdp_one_base_index;cdp_a_loop++)
		      {
			if( cdp_chr_fasta[cdp_a_loop] != 'N' && cdp_chr_fasta[cdp_a_loop] != 'n' )
			{
			  cdp_snv_read_count_total += (long)caf_rd_rd_list[cdp_a_loop] + (long)caf_rd_low_mq_rd_list[cdp_a_loop];
			  cdp_snv_base_total += 1;
			}
		      }
		      cdp_snv_ave_rd = (double)cdp_snv_read_count_total / (double)cdp_snv_base_total;
		      caf_last_snv_group_pos = cdp_pos_in_contig_start - cdp_one_base_index;
		      if( g_internal == 1 )
		      {
			printf("1: chr, snv rd %s, %e\n", cdp_chr_name, cdp_snv_ave_rd);
		      }
		      if( g_normal == 0 )
		      {
			if( g_vcf == 1 )
			{
			  for(cdp_a_loop=0;cdp_a_loop<cdp_snv_list_index;cdp_a_loop++)
			  {
			    if( (cdp_snv_base_total > 0 && cdp_snv_read_count_all_list[cdp_a_loop] <= round(g_snv_rd_min_factor * cdp_snv_ave_rd)) || cdp_snv_ratio_list[cdp_a_loop] >= g_high_cov_min_snv_ratio )
			    {
			      cdp_snv_cn = round(cdp_snv_ratio_list[cdp_a_loop] * g_ploidy);
			      if( cdp_snv_cn == 0 )
			      {
				cdp_snv_cn = 1;
			      }
			      for(cdp_b_loop=0;cdp_b_loop<g_ploidy;cdp_b_loop++)
			      {
				if( cdp_b_loop < cdp_snv_cn )
				{
				  cdp_snv_gt_string[2*cdp_b_loop] = '1';
				}
				else
				{
				  cdp_snv_gt_string[2*cdp_b_loop] = '0';
				}
				if( cdp_b_loop < g_ploidy -1 )
				{
				  cdp_snv_gt_string[2*cdp_b_loop+1] = '/';
				}
				else
				{
				  cdp_snv_gt_string[2*cdp_b_loop+1] = '\0';
				}
			      }
			    }

				if (cdp_snv_start_binom_cdf_list[cdp_a_loop] != 0)
				{
					int qual_entry = (int) -10 * log10(cdp_snv_start_binom_cdf_list[cdp_a_loop]);
					fprintf(cdp_results_file_snv, "%s\t%d\t.\t%c\t%c\t%d\t.\t.\tGT:PR:AF:AMQ:CMQ:GMQ:TMQ:ALMQ:CLMQ:GLMQ:TLMQ:BQ:MQ:PIR:FS\t%s:%.2e:%.2e:%d:%d:%d:%d:%d:%d:%d:%d:%.2f:%.2f:%.2f:%.2f\n", cdp_chr_name, cdp_snv_pos_list[cdp_a_loop] + 1, cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop]], g_dna[cdp_snv_base_list[cdp_a_loop]], qual_entry, cdp_snv_gt_string, cdp_snv_start_binom_cdf_list[cdp_a_loop], cdp_snv_ratio_list[cdp_a_loop], cdp_snv_list[0][cdp_a_loop], cdp_snv_list[1][cdp_a_loop], cdp_snv_list[2][cdp_a_loop], cdp_snv_list[3][cdp_a_loop], cdp_snv_lowmq_list[0][cdp_a_loop], cdp_snv_lowmq_list[1][cdp_a_loop], cdp_snv_lowmq_list[2][cdp_a_loop], cdp_snv_lowmq_list[3][cdp_a_loop], (double)cdp_snv_bq_all_list[cdp_a_loop]/(double)cdp_snv_read_count_all_list[cdp_a_loop], (double)cdp_snv_mq_all_list[cdp_a_loop]/(double)cdp_snv_read_count_all_list[cdp_a_loop], (double)cdp_snv_pos_in_read_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], (double)cdp_snv_fstrand_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]);

				} else {
					fprintf(cdp_results_file_snv, "%s\t%d\t.\t%c\t%c\t%s\t.\t.\tGT:PR:AF:AMQ:CMQ:GMQ:TMQ:ALMQ:CLMQ:GLMQ:TLMQ:BQ:MQ:PIR:FS\t%s:%.2e:%.2e:%d:%d:%d:%d:%d:%d:%d:%d:%.2f:%.2f:%.2f:%.2f\n", cdp_chr_name, cdp_snv_pos_list[cdp_a_loop] + 1, cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop]], g_dna[cdp_snv_base_list[cdp_a_loop]], infinity_qual_entry, cdp_snv_gt_string, cdp_snv_start_binom_cdf_list[cdp_a_loop], cdp_snv_ratio_list[cdp_a_loop], cdp_snv_list[0][cdp_a_loop], cdp_snv_list[1][cdp_a_loop], cdp_snv_list[2][cdp_a_loop], cdp_snv_list[3][cdp_a_loop], cdp_snv_lowmq_list[0][cdp_a_loop], cdp_snv_lowmq_list[1][cdp_a_loop], cdp_snv_lowmq_list[2][cdp_a_loop], cdp_snv_lowmq_list[3][cdp_a_loop], (double)cdp_snv_bq_all_list[cdp_a_loop]/(double)cdp_snv_read_count_all_list[cdp_a_loop], (double)cdp_snv_mq_all_list[cdp_a_loop]/(double)cdp_snv_read_count_all_list[cdp_a_loop], (double)cdp_snv_pos_in_read_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], (double)cdp_snv_fstrand_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]);
				}
			  }
			}
			else
			{
			  for(cdp_a_loop=0;cdp_a_loop<cdp_snv_list_index;cdp_a_loop++)
			  {
			    if( (cdp_snv_base_total > 0 && cdp_snv_read_count_all_list[cdp_a_loop] <= round(g_snv_rd_min_factor * cdp_snv_ave_rd)) || cdp_snv_ratio_list[cdp_a_loop] >= g_high_cov_min_snv_ratio )
			    {
			      fprintf(cdp_results_file, "SNV\t%s\t%d\t%c\t%e\t%d\t%d", cdp_chr_name, cdp_snv_pos_list[cdp_a_loop] + 1, g_dna[cdp_snv_base_list[cdp_a_loop]], cdp_snv_ratio_list[cdp_a_loop], cdp_snv_rd_list[cdp_a_loop], cdp_snv_low_mq_rd_list[cdp_a_loop]);
			      for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
			      {
					fprintf(cdp_results_file, "\t%d", cdp_snv_list[cdp_b_loop][cdp_a_loop]);
			      }
			      for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
			      {
					fprintf(cdp_results_file, "\t%d", cdp_snv_lowmq_list[cdp_b_loop][cdp_a_loop]);
			      }
			      fprintf(cdp_results_file, "\t%d\t%d\t%d\t%d\t%d\t%d\t%d", cdp_snv_bq_list[cdp_a_loop], cdp_snv_bq_all_list[cdp_a_loop], cdp_snv_mq_list[cdp_a_loop], cdp_snv_mq_all_list[cdp_a_loop], cdp_snv_bq_read_count_list[cdp_a_loop], cdp_snv_mq_read_count_list[cdp_a_loop], cdp_snv_read_count_all_list[cdp_a_loop]);
			      if( cdp_snv_pos_list[cdp_a_loop] > 0 && cdp_snv_pos_list[cdp_a_loop] < cdp_chr_fasta_len - 1 )
			      {
					fprintf(cdp_results_file, "\t%.2f\t%.2f\t%c%c%c", (double)cdp_snv_pos_in_read_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], (double)cdp_snv_fstrand_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] - 1], cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop]], cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] + 1]);
			      }
			      else
			      {
					fprintf(cdp_results_file, "\t%.2f\t%.2f\t%c%c%c", (double)cdp_snv_pos_in_read_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], (double)cdp_snv_fstrand_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], '.','.','.');
			      }
			      fprintf(cdp_results_file, "\t");
			      for(cdp_b_loop=0;cdp_b_loop<cdp_lseq;cdp_b_loop++)
			      {
				if( cdp_snv_pos_list[cdp_a_loop] - cdp_lseq + 1 + cdp_b_loop < 0 )
				{
				  fprintf(cdp_results_file, "%c", 'N');
				}
				else
				{
				  fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] - cdp_lseq + 1 + cdp_b_loop]);
				}
			      }
			      for(cdp_b_loop=0;cdp_b_loop<cdp_lseq-1;cdp_b_loop++)
			      {
				if( cdp_snv_pos_list[cdp_a_loop] + cdp_lseq - 1 - cdp_b_loop >=  cdp_chr_fasta_len - 1 )
				{
				  fprintf(cdp_results_file, "%c", 'N');
				}
				else
				{
				  fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] + cdp_lseq - 1 - cdp_b_loop]);
				}
			      }


			      fprintf(cdp_results_file, "\t%e\t%e", cdp_snv_start_binom_cdf_list[cdp_a_loop], cdp_snv_start_hez_binom_cdf_list[cdp_a_loop]);
			      fprintf(cdp_results_file, "\n");
			    }
			  }
			}
		      }
		      cdp_snv_list_index = 0;
		    }


		  }



		  cdp_indel_rd_temp = 0;
		  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
		  {
		    cdp_indel_rd_temp += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] + cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
		  }
		  cdp_indel_i_temp = cdp_one_base_indel_i[cdp_one_base_index];
		  if( cdp_indel_i_temp / cdp_add_factor > cdp_indel_rd_temp )
		  {
		    cdp_indel_i_temp = cdp_indel_rd_temp * cdp_add_factor;
		  }
		  if( cdp_indel_i_temp/cdp_add_factor >= g_min_disc )
		  {
		    if( cdp_indel_rd_temp <= g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_indel_rd_temp][cdp_indel_i_temp/cdp_add_factor];
		      if( (cdp_indel_i_temp + cdp_one_base_indel_sc_left[cdp_one_base_index])/cdp_add_factor < cdp_indel_rd_temp )
		      {
			cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][(cdp_indel_i_temp + cdp_one_base_indel_sc_left[cdp_one_base_index])/cdp_add_factor];
			if( (cdp_indel_i_temp + cdp_one_base_indel_sc_right[cdp_one_base_index])/cdp_add_factor < cdp_indel_rd_temp )
			{
			  if( g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][(cdp_indel_i_temp + cdp_one_base_indel_sc_right[cdp_one_base_index])/cdp_add_factor] > cdp_hez_binom_cdf )
			  {
			    cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][(cdp_indel_i_temp + cdp_one_base_indel_sc_right[cdp_one_base_index])/cdp_add_factor];
			  }
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][cdp_indel_rd_temp];
			}
		      }
		      else
		      {
			cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][cdp_indel_rd_temp];
		      }
		      if( cdp_binom_cdf <= g_pval_threshold1 )
		      {
			if( cdp_indel_i_list_index < g_sv_list_len - 1 )
			{
			  cdp_indel_i_list_start[cdp_indel_i_list_index] = cdp_pos_in_contig_start;
			  cdp_indel_i_list_start_binom_cdf[cdp_indel_i_list_index] = cdp_binom_cdf;
			  cdp_indel_i_list_start_hez_binom_cdf[cdp_indel_i_list_index] = cdp_hez_binom_cdf;
			  cdp_indel_i_list_dist[cdp_indel_i_list_index] = cdp_one_base_indel_idist[cdp_one_base_index];
			  cdp_indel_i_list_start_conc[cdp_indel_i_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  cdp_indel_i_list_start_i[cdp_indel_i_list_index] = cdp_indel_i_temp;
			  cdp_indel_i_list_start_rd[cdp_indel_i_list_index] = cdp_one_base_sc_left_rd[cdp_one_base_index + 1] + cdp_one_base_sc_right_rd[cdp_one_base_index];
			  cdp_indel_i_list_start_sc[cdp_indel_i_list_index] = cdp_one_base_sc_left[cdp_one_base_index + 1] + cdp_one_base_sc_right[cdp_one_base_index];
			  cdp_indel_i_list_start_rd[cdp_indel_i_list_index] = cdp_indel_rd_temp;
			  if( cdp_indel_i_list_dist[cdp_indel_i_list_index] <= g_indel_i_seq_len )
			  {
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<cdp_indel_i_list_dist[cdp_indel_i_list_index];cdp_tumor_loop2++)
			    {
			      cdp_indel_i_list_seq[cdp_tumor_loop2][cdp_indel_i_list_index] = cdp_one_base_indel_i_seq[cdp_tumor_loop2][cdp_one_base_index];
			    }
			  }

			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_indel_i_list_start_other_len[cdp_indel_i_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_indel_i_list_start_other_len[cdp_indel_i_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }
			  cdp_indel_i_list_index += 1;
			}
			else
			{
			  g_sv_list_len_over = 1;
			}
		      }
		    }
		  }




		  cdp_indel_rd_temp = cdp_one_base_indel_d_f[cdp_one_base_index]/cdp_add_factor;
		  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
		  {
		    cdp_indel_rd_temp += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] + cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
		  }
		  cdp_indel_d_f_temp = cdp_one_base_indel_d_f[cdp_one_base_index];
		  if( cdp_indel_d_f_temp/cdp_add_factor >= g_min_disc )
		  {
		    if( cdp_indel_rd_temp <= g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_indel_rd_temp][cdp_indel_d_f_temp/cdp_add_factor];
		      if( (cdp_indel_d_f_temp + cdp_one_base_indel_sc_right[cdp_one_base_index])/cdp_add_factor < cdp_indel_rd_temp )
		      {
				cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][(cdp_indel_d_f_temp + cdp_one_base_indel_sc_right[cdp_one_base_index])/cdp_add_factor];
		      }
		      else
		      {
				cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][cdp_indel_rd_temp];
		      }
		      if( cdp_binom_cdf <= g_pval_threshold1 )
		      {

			if( cdp_indel_d_list_index == -1 )
			{
			  cdp_indel_d_list_index = 0;
			  cdp_indel_d_list_start[cdp_indel_d_list_index] = cdp_pos_in_contig_start;
			  cdp_indel_d_list_start_binom_cdf[cdp_indel_d_list_index] = cdp_binom_cdf;
			  cdp_indel_d_list_start_hez_binom_cdf[cdp_indel_d_list_index] = cdp_hez_binom_cdf;
			  cdp_indel_d_list_start_conc[cdp_indel_d_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  cdp_indel_d_list_start_f[cdp_indel_d_list_index] = cdp_indel_d_f_temp;
			  cdp_indel_d_list_start_rd[cdp_indel_d_list_index] = cdp_one_base_sc_right_rd[cdp_one_base_index] + cdp_one_base_indel_d_f_rd[cdp_one_base_index];
			  cdp_indel_d_list_start_sc[cdp_indel_d_list_index] = cdp_one_base_sc_right[cdp_one_base_index];
			  cdp_indel_d_list_start_rd[cdp_indel_d_list_index] = cdp_indel_rd_temp;

			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_indel_d_list_start_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_indel_d_list_start_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }

			}
			else if( cdp_indel_d_list_start[cdp_indel_d_list_index] != -1 && cdp_indel_d_list_end[cdp_indel_d_list_index] != -1 )
			{
			  if( cdp_indel_d_list_index < g_sv_list_len - 1 )
			  {
			    cdp_indel_d_list_index += 1;
			    cdp_indel_d_list_start[cdp_indel_d_list_index] = cdp_pos_in_contig_start;
			    cdp_indel_d_list_start_binom_cdf[cdp_indel_d_list_index] = cdp_binom_cdf;
			    cdp_indel_d_list_start_hez_binom_cdf[cdp_indel_d_list_index] = cdp_hez_binom_cdf;
			    cdp_indel_d_list_start_conc[cdp_indel_d_list_index] = cdp_one_base_conc[cdp_one_base_index];
			    cdp_indel_d_list_start_f[cdp_indel_d_list_index] = cdp_indel_d_f_temp;
			    cdp_indel_d_list_start_rd[cdp_indel_d_list_index] = cdp_one_base_sc_right_rd[cdp_one_base_index] + cdp_one_base_indel_d_f_rd[cdp_one_base_index];
			    cdp_indel_d_list_start_sc[cdp_indel_d_list_index] = cdp_one_base_sc_right[cdp_one_base_index];
			    cdp_indel_d_list_start_rd[cdp_indel_d_list_index] = cdp_indel_rd_temp;
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			      {
				cdp_indel_d_list_start_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2;
				break;
			      }
			      else if( cdp_tumor_loop2 == g_other_len - 1 )
			      {
				cdp_indel_d_list_start_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2 + 1;
			      }
			    }

			  }
			  else
			  {
			    g_sv_list_len_over = 1;
			  }

			}
			else if( (cdp_pos_in_contig_start - cdp_indel_d_list_start[cdp_indel_d_list_index] > g_lseq && cdp_indel_d_list_end[cdp_indel_d_list_index] == -1) || cdp_binom_cdf < cdp_indel_d_list_start_binom_cdf[cdp_indel_d_list_index] )
			{
			  cdp_indel_d_list_start[cdp_indel_d_list_index] = cdp_pos_in_contig_start;
			  cdp_indel_d_list_start_binom_cdf[cdp_indel_d_list_index] = cdp_binom_cdf;
			  cdp_indel_d_list_start_hez_binom_cdf[cdp_indel_d_list_index] = cdp_hez_binom_cdf;
			  cdp_indel_d_list_start_conc[cdp_indel_d_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  if( cdp_indel_d_list_end[cdp_indel_d_list_index] < cdp_indel_d_list_start[cdp_indel_d_list_index] )
			  {
			    cdp_indel_d_list_end[cdp_indel_d_list_index] = -1;
			  }
			  cdp_indel_d_list_start_f[cdp_indel_d_list_index] = cdp_indel_d_f_temp;
			  cdp_indel_d_list_start_rd[cdp_indel_d_list_index] = cdp_one_base_sc_right_rd[cdp_one_base_index] + cdp_one_base_indel_d_f_rd[cdp_one_base_index];
			  cdp_indel_d_list_start_sc[cdp_indel_d_list_index] = cdp_one_base_sc_right[cdp_one_base_index];
			  cdp_indel_d_list_start_rd[cdp_indel_d_list_index] = cdp_indel_rd_temp;

			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_indel_d_list_start_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_indel_d_list_start_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }

			}
		      }
		    }
		  }



		  cdp_indel_rd_temp = cdp_one_base_indel_d_r[cdp_one_base_index]/cdp_add_factor;
		  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
		  {
		    cdp_indel_rd_temp += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] + cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
		  }
		  cdp_indel_d_r_temp = cdp_one_base_indel_d_r[cdp_one_base_index];
		  if( cdp_indel_d_list_index >= 0 && cdp_indel_d_r_temp/cdp_add_factor >= g_min_disc )
		  {
		    if( cdp_indel_rd_temp <= g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_indel_rd_temp][cdp_indel_d_r_temp/cdp_add_factor];
		      if( (cdp_indel_d_r_temp + cdp_one_base_indel_sc_left[cdp_one_base_index])/cdp_add_factor < cdp_indel_rd_temp )
		      {
				cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][(cdp_indel_d_r_temp + cdp_one_base_indel_sc_left[cdp_one_base_index])/cdp_add_factor];
		      }
		      else
		      {
				cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_indel_rd_temp][cdp_indel_rd_temp];
		      }
		      if( cdp_binom_cdf <= g_pval_threshold1 )
		      {

			if( ((float)cdp_pos_in_contig_start - (float)cdp_indel_d_list_start[cdp_indel_d_list_index] - cdp_one_base_indel_d_rdist[cdp_one_base_index]) < g_indel_d_dist_range && cdp_indel_d_list_start[cdp_indel_d_list_index] != -1 && cdp_indel_d_list_end[cdp_indel_d_list_index] != -1 )
			{
			  cdp_indel_d_list_end[cdp_indel_d_list_index] = cdp_pos_in_contig_start;
			  cdp_indel_d_list_end_binom_cdf[cdp_indel_d_list_index] = cdp_binom_cdf;
			  cdp_indel_d_list_end_hez_binom_cdf[cdp_indel_d_list_index] = cdp_hez_binom_cdf;
			  cdp_indel_d_list_end_conc[cdp_indel_d_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  cdp_indel_d_list_end_r[cdp_indel_d_list_index] = cdp_indel_d_r_temp;
			  cdp_indel_d_list_end_rd[cdp_indel_d_list_index] = cdp_one_base_sc_left_rd[cdp_one_base_index] + cdp_one_base_indel_d_r_rd[cdp_one_base_index];
			  cdp_indel_d_list_end_sc[cdp_indel_d_list_index] = cdp_one_base_sc_left[cdp_one_base_index];
			  cdp_indel_d_list_end_rd[cdp_indel_d_list_index] = cdp_indel_rd_temp;
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_indel_d_list_end_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_indel_d_list_end_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }

			}
			else if( ((float)cdp_pos_in_contig_start - (float)cdp_indel_d_list_start[cdp_indel_d_list_index] - cdp_one_base_indel_d_rdist[cdp_one_base_index]) < g_indel_d_dist_range && (cdp_indel_d_list_end[cdp_indel_d_list_index] == -1 || cdp_binom_cdf < cdp_indel_d_list_end_binom_cdf[cdp_indel_d_list_index]) )
			{
			  cdp_indel_d_list_end[cdp_indel_d_list_index] = cdp_pos_in_contig_start;
			  cdp_indel_d_list_end_binom_cdf[cdp_indel_d_list_index] = cdp_binom_cdf;
			  cdp_indel_d_list_end_hez_binom_cdf[cdp_indel_d_list_index] = cdp_hez_binom_cdf;
			  cdp_indel_d_list_end_conc[cdp_indel_d_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  cdp_indel_d_list_end_r[cdp_indel_d_list_index] = cdp_indel_d_r_temp;
			  cdp_indel_d_list_end_rd[cdp_indel_d_list_index] = cdp_one_base_sc_left_rd[cdp_one_base_index] + cdp_one_base_indel_d_r_rd[cdp_one_base_index];
			  cdp_indel_d_list_end_sc[cdp_indel_d_list_index] = cdp_one_base_sc_left[cdp_one_base_index];
			  cdp_indel_d_list_end_rd[cdp_indel_d_list_index] = cdp_indel_rd_temp;

			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_indel_d_list_end_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_indel_d_list_end_other_len[cdp_indel_d_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }

			}
		      }
		    }
		  }

		}

		if( cdp_one_base_rd[cdp_one_base_index] + cdp_one_base_sc_rd[cdp_one_base_index] > 0 )
		{

		  cdp_sc_left_rd_temp = cdp_one_base_rd[cdp_one_base_index] + cdp_one_base_sc_left_rd[cdp_one_base_index];
		  if( (cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_ins[cdp_one_base_index])/cdp_add_factor >= g_min_disc )
		  {
		    if( cdp_sc_left_rd_temp <= g_max_trials )
		    {
		      if( (cdp_one_base_munmapped_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_ins[cdp_one_base_index])/cdp_add_factor < cdp_sc_left_rd_temp )
		      {
				cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_sc_left_rd_temp][(cdp_one_base_munmapped_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_ins[cdp_one_base_index])/cdp_add_factor];
		      }
		      else
		      {
				cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_sc_left_rd_temp][cdp_sc_left_rd_temp];
		      }

		      if( cdp_binom_cdf <= g_pval_insertion1 )
		      {

			if( cdp_ins_list_index == -1 )
			{
			  cdp_ins_list_index = 0;
			  cdp_ins_list_start[cdp_ins_list_index] = cdp_pos_in_contig_start;
			  cdp_ins_list_start_binom_cdf[cdp_ins_list_index] = cdp_binom_cdf;
			  cdp_ins_list_start_ins[cdp_ins_list_index] = cdp_one_base_ins[cdp_one_base_index];
			  cdp_ins_list_start_rd[cdp_ins_list_index] = cdp_one_base_rd[cdp_one_base_index];
			  cdp_ins_list_start_conc[cdp_ins_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_ins_list_start_other_len[cdp_ins_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_ins_list_start_other_len[cdp_ins_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }
			}
			else if( (cdp_pos_in_contig_start - cdp_ins_list_start[cdp_ins_list_index] > g_sc_range && cdp_ins_list_start[cdp_ins_list_index] != -1) || (cdp_pos_in_contig_start - cdp_ins_list_end[cdp_ins_list_index] > g_sc_range && cdp_ins_list_end[cdp_ins_list_index] != -1) )
			{
			  if( cdp_ins_list_index < g_sv_list_len - 1 )
			  {
			    cdp_ins_list_index += 1;
			    cdp_ins_list_start[cdp_ins_list_index] = cdp_pos_in_contig_start;
			    cdp_ins_list_start_binom_cdf[cdp_ins_list_index] = cdp_binom_cdf;
			    cdp_ins_list_start_ins[cdp_ins_list_index] = cdp_one_base_ins[cdp_one_base_index];
			    cdp_ins_list_start_rd[cdp_ins_list_index] = cdp_one_base_rd[cdp_one_base_index];
			    cdp_ins_list_start_conc[cdp_ins_list_index] = cdp_one_base_conc[cdp_one_base_index];
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			      {
				cdp_ins_list_start_other_len[cdp_ins_list_index] = cdp_tumor_loop2;
				break;
			      }
			      else if( cdp_tumor_loop2 == g_other_len - 1 )
			      {
				cdp_ins_list_start_other_len[cdp_ins_list_index] = cdp_tumor_loop2 + 1;
			      }
			    }
			  }
			  else
			  {
			    g_sv_list_len_over = 1;
			  }

			}
			else if( cdp_ins_list_start[cdp_ins_list_index] == -1 || cdp_binom_cdf < cdp_ins_list_start_binom_cdf[cdp_ins_list_index] )
			{
			  cdp_ins_list_start[cdp_ins_list_index] = cdp_pos_in_contig_start;
			  cdp_ins_list_start_binom_cdf[cdp_ins_list_index] = cdp_binom_cdf;
			  cdp_ins_list_start_ins[cdp_ins_list_index] = cdp_one_base_ins[cdp_one_base_index];
			  cdp_ins_list_start_rd[cdp_ins_list_index] = cdp_one_base_rd[cdp_one_base_index];
			  cdp_ins_list_start_conc[cdp_ins_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_ins_list_start_other_len[cdp_ins_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_ins_list_start_other_len[cdp_ins_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }
			}
		      }
		    }
		  }



		  cdp_sc_right_rd_temp = cdp_one_base_rd[cdp_one_base_index] + cdp_one_base_sc_right_rd[cdp_one_base_index];
		  if( (cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_ins[cdp_one_base_index])/cdp_add_factor >= g_min_disc )
		  {
		    if( cdp_sc_right_rd_temp <= g_max_trials )
		    {
		      if( (cdp_one_base_munmapped_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_ins[cdp_one_base_index])/cdp_add_factor < cdp_sc_right_rd_temp )
		      {
			cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_sc_right_rd_temp][(cdp_one_base_munmapped_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_ins[cdp_one_base_index])/cdp_add_factor];
		      }
		      else
		      {
			cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_sc_right_rd_temp][cdp_sc_right_rd_temp];
		      }
		      if( cdp_binom_cdf <= g_pval_insertion1 )
		      {

			if( cdp_ins_list_index == -1 )
			{
			  cdp_ins_list_index = 0;
			  cdp_ins_list_end[cdp_ins_list_index] = cdp_pos_in_contig_start;
			  cdp_ins_list_end_binom_cdf[cdp_ins_list_index] = cdp_binom_cdf;
			  cdp_ins_list_end_ins[cdp_ins_list_index] = cdp_one_base_ins[cdp_one_base_index];
			  cdp_ins_list_end_rd[cdp_ins_list_index] = cdp_one_base_rd[cdp_one_base_index];
			  cdp_ins_list_end_conc[cdp_ins_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_ins_list_end_other_len[cdp_ins_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_ins_list_end_other_len[cdp_ins_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }
			}
			else if( (cdp_pos_in_contig_start - cdp_ins_list_start[cdp_ins_list_index] > g_sc_range && cdp_ins_list_start[cdp_ins_list_index] != -1) || (cdp_pos_in_contig_start - cdp_ins_list_end[cdp_ins_list_index] > g_sc_range && cdp_ins_list_end[cdp_ins_list_index] != -1) )
			{
			  if( cdp_ins_list_index < g_sv_list_len - 1 )
			  {
			    cdp_ins_list_index += 1;
			    cdp_ins_list_end[cdp_ins_list_index] = cdp_pos_in_contig_start;
			    cdp_ins_list_end_binom_cdf[cdp_ins_list_index] = cdp_binom_cdf;
			    cdp_ins_list_end_ins[cdp_ins_list_index] = cdp_one_base_ins[cdp_one_base_index];
			    cdp_ins_list_end_rd[cdp_ins_list_index] = cdp_one_base_rd[cdp_one_base_index];
			    cdp_ins_list_end_conc[cdp_ins_list_index] = cdp_one_base_conc[cdp_one_base_index];
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			      {
				cdp_ins_list_end_other_len[cdp_ins_list_index] = cdp_tumor_loop2;
				break;
			      }
			      else if( cdp_tumor_loop2 == g_other_len - 1 )
			      {
				cdp_ins_list_end_other_len[cdp_ins_list_index] = cdp_tumor_loop2 + 1;
			      }
			    }
			  }
			  else
			  {
			    g_sv_list_len_over = 1;
			  }

			}
			else if( cdp_ins_list_end[cdp_ins_list_index] == -1 || cdp_binom_cdf < cdp_ins_list_end_binom_cdf[cdp_ins_list_index] )
			{
			  cdp_ins_list_end[cdp_ins_list_index] = cdp_pos_in_contig_start;
			  cdp_ins_list_end_binom_cdf[cdp_ins_list_index] = cdp_binom_cdf;
			  cdp_ins_list_end_ins[cdp_ins_list_index] = cdp_one_base_ins[cdp_one_base_index];
			  cdp_ins_list_end_rd[cdp_ins_list_index] = cdp_one_base_rd[cdp_one_base_index];
			  cdp_ins_list_end_conc[cdp_ins_list_index] = cdp_one_base_conc[cdp_one_base_index];
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_ins_list_end_other_len[cdp_ins_list_index] = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_ins_list_end_other_len[cdp_ins_list_index] = cdp_tumor_loop2 + 1;
			    }
			  }
			}
		      }
		    }
		  }

		}

		if( cdp_one_base_rd[cdp_one_base_index] > 0 )
		{

		  if( cdp_one_base_ctx_f[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_pos_in_contig_start - cdp_one_base_ctx_f_read_end[cdp_one_base_index] < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_ctx_f[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_ctx_f[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_ctx_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_ctx_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_ctx_f[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_ctx_f[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_ctx_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_ctx_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      if( cdp_ctx_f_list_index < g_sv_list_len - 1 )
		      {
			cdp_ctx_f_list[cdp_ctx_f_list_index] = cdp_pos_in_contig_start;
			cdp_ctx_f_list_binom_cdf[cdp_ctx_f_list_index] = cdp_binom_cdf;
			cdp_ctx_f_list_hez_binom_cdf[cdp_ctx_f_list_index] = cdp_hez_binom_cdf;
			cdp_ctx_f_list_mchr[cdp_ctx_f_list_index] = cdp_one_base_ctx_f_mchr[cdp_one_base_index];
			cdp_ctx_f_list_mpos[cdp_ctx_f_list_index] = cdp_one_base_ctx_f_mpos[cdp_one_base_index];
			cdp_ctx_f_list_conc[cdp_ctx_f_list_index] = cdp_one_base_conc[cdp_one_base_index];
			cdp_ctx_f_list_rd[cdp_ctx_f_list_index] = cdp_one_base_rd[cdp_one_base_index];
			cdp_ctx_f_list_ctx_f[cdp_ctx_f_list_index] = cdp_one_base_ctx_f[cdp_one_base_index];
			cdp_ctx_f_list_read_start[cdp_ctx_f_list_index] = cdp_one_base_ctx_f_read_start[cdp_one_base_index];
			cdp_ctx_f_list_read_end[cdp_ctx_f_list_index] = cdp_one_base_ctx_f_read_end[cdp_one_base_index];
			for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			{
			  if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			  {
			    cdp_ctx_f_list_other_len[cdp_ctx_f_list_index] = cdp_tumor_loop2;
			    break;
			  }
			  else if( cdp_tumor_loop2 == g_other_len - 1 )
			  {
			    cdp_ctx_f_list_other_len[cdp_ctx_f_list_index] = cdp_tumor_loop2 + 1;
			  }
			}
			cdp_ctx_f_list_index += 1;
		      }
		      else
		      {
			g_sv_list_len_over = 1;
		      }

		    }
		  }



		  if( cdp_one_base_ctx_r[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_one_base_ctx_r_read_start[cdp_one_base_index] + cdp_lseq - cdp_pos_in_contig_start < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_ctx_r[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_ctx_r[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_ctx_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_ctx_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_ctx_r[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_ctx_f[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_ctx_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_ctx_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      if( cdp_ctx_r_list_index < g_sv_list_len - 1 )
		      {
			cdp_ctx_r_list[cdp_ctx_r_list_index] = cdp_pos_in_contig_start;
			cdp_ctx_r_list_binom_cdf[cdp_ctx_r_list_index] = cdp_binom_cdf;
			cdp_ctx_r_list_hez_binom_cdf[cdp_ctx_r_list_index] = cdp_hez_binom_cdf;
			cdp_ctx_r_list_mchr[cdp_ctx_r_list_index] = cdp_one_base_ctx_r_mchr[cdp_one_base_index];
			cdp_ctx_r_list_mpos[cdp_ctx_r_list_index] = cdp_one_base_ctx_r_mpos[cdp_one_base_index];
			cdp_ctx_r_list_conc[cdp_ctx_r_list_index] = cdp_one_base_conc[cdp_one_base_index];
			cdp_ctx_r_list_rd[cdp_ctx_r_list_index] = cdp_one_base_rd[cdp_one_base_index];
			cdp_ctx_r_list_ctx_r[cdp_ctx_r_list_index] = cdp_one_base_ctx_r[cdp_one_base_index];
			cdp_ctx_r_list_read_start[cdp_ctx_r_list_index] = cdp_one_base_ctx_r_read_start[cdp_one_base_index];
			cdp_ctx_r_list_read_end[cdp_ctx_r_list_index] = cdp_one_base_ctx_r_read_end[cdp_one_base_index];
			for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			{
			  if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			  {
			    cdp_ctx_r_list_other_len[cdp_ctx_r_list_index] = cdp_tumor_loop2;
			    break;
			  }
			  else if( cdp_tumor_loop2 == g_other_len - 1 )
			  {
			    cdp_ctx_r_list_other_len[cdp_ctx_r_list_index] = cdp_tumor_loop2 + 1;
			  }
			}
			cdp_ctx_r_list_index += 1;
		      }
		      else
		      {
			g_sv_list_len_over = 1;
		      }

		    }
		  }




		  if( cdp_one_base_dup_r[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_one_base_dup_r_read_start[cdp_one_base_index] + cdp_lseq - cdp_pos_in_contig_start < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_dup_r[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_dup_r[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_dup_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_dup_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_dup_r[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_dup_r[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_dup_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_dup_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      if( cdp_dup_list_index < g_sv_list_len - 1 )
		      {
			cdp_dup_list_start[cdp_dup_list_index] = cdp_pos_in_contig_start;
			cdp_dup_list_start_binom_cdf[cdp_dup_list_index] = cdp_binom_cdf;
			cdp_dup_list_start_hez_binom_cdf[cdp_dup_list_index] = cdp_hez_binom_cdf;
			cdp_dup_list_dist[cdp_dup_list_index] = cdp_one_base_dup_rdist[cdp_one_base_index];
			cdp_dup_list_start_conc[cdp_dup_list_index] = cdp_one_base_conc[cdp_one_base_index];
			cdp_dup_list_start_rd[cdp_dup_list_index] = cdp_one_base_rd[cdp_one_base_index];
			cdp_dup_list_start_dup_r[cdp_dup_list_index] = cdp_one_base_dup_r[cdp_one_base_index];
			cdp_dup_list_start_read_start[cdp_dup_list_index] = cdp_one_base_dup_r_read_start[cdp_one_base_index];
			cdp_dup_list_start_read_end[cdp_dup_list_index] = cdp_one_base_dup_r_read_end[cdp_one_base_index];
			for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			{
			  if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			  {
			    cdp_dup_list_start_other_len[cdp_dup_list_index] = cdp_tumor_loop2;
			    break;
			  }
			  else if( cdp_tumor_loop2 == g_other_len - 1 )
			  {
			    cdp_dup_list_start_other_len[cdp_dup_list_index] = cdp_tumor_loop2 + 1;
			  }
			}
			cdp_dup_list_index += 1;
		      }
		      else
		      {
			g_sv_list_len_over = 1;
		      }

		    }
		  }



		  if( cdp_one_base_dup_f[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_pos_in_contig_start - cdp_one_base_dup_f_read_end[cdp_one_base_index] < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_dup_f[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_dup_f[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_dup_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_dup_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_dup_f[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_dup_f[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_dup_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_dup_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      cdp_dup_min = (int)(((double)(1.0)-g_range_mult) * (cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq) + (double)0.5);
		      cdp_dup_max = (int)(((double)(1.0)+g_range_mult) * (cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq) + (double)0.5);;
		      if( cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 > (double)cdp_dup_min )
		      {
			cdp_dup_min = (int)((cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      if( cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 < (double)cdp_dup_max )
		      {
			cdp_dup_max = (int)((cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      cdp_dup_min = (int)((cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);
		      cdp_dup_max = (int)((cdp_one_base_dup_fdist[cdp_one_base_index] + 2*g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);

		      cdp_bl_list = cdp_dup_list_start;
		      cdp_bl_pos = cdp_pos_in_contig_start - g_insert_mean + 2 * g_lseq - cdp_dup_min;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_dup_list_index;
		      cdp_bl_type = 0;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;
		      while( cdp_bl_index_found == 0 )
		      {

			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}

			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_start = cdp_bl_index;



		      cdp_bl_list = cdp_dup_list_start;
		      cdp_bl_pos = cdp_pos_in_contig_start - g_insert_mean + 2 * g_lseq - cdp_dup_max;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_dup_list_index;
		      cdp_bl_type = 1;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;
		      while( cdp_bl_index_found == 0 )
		      {

			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}

			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_end = cdp_bl_index;


		      if( cdp_lp_end < cdp_lp_start )
		      {
			cdp_lp_temp = cdp_lp_end;
			cdp_lp_end = cdp_lp_start;
			cdp_lp_start = cdp_lp_temp;
		      }
		      cdp_start_pos = cdp_pos_in_contig_start - g_insert_mean + 2 * g_lseq - cdp_dup_max;
		      cdp_end_pos = cdp_pos_in_contig_start - g_insert_mean + 2 * g_lseq - cdp_dup_min;
		      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
		      {
			if( cdp_dup_list_dist[cdp_a_loop] >= cdp_dup_min && cdp_dup_list_dist[cdp_a_loop] <= cdp_dup_max && cdp_dup_list_start[cdp_a_loop] >= cdp_start_pos && cdp_dup_list_start[cdp_a_loop] <= cdp_end_pos )
			{
			  if( (cdp_dup_list_end_binom_cdf[cdp_a_loop] > cdp_binom_cdf && cdp_one_base_dup_f[cdp_one_base_index] >= cdp_dup_list_end_dup_f[cdp_a_loop]) || cdp_dup_list_end[cdp_a_loop] == -1 || (cdp_dup_list_end_binom_cdf[cdp_a_loop] == cdp_binom_cdf && cdp_one_base_dup_f[cdp_one_base_index] > cdp_dup_list_end_dup_f[cdp_a_loop]) )
			  {
			    cdp_dup_list_end[cdp_a_loop] = cdp_pos_in_contig_start;
			    cdp_dup_list_end_binom_cdf[cdp_a_loop] = cdp_binom_cdf;
			    cdp_dup_list_end_hez_binom_cdf[cdp_a_loop] = cdp_hez_binom_cdf;
			    cdp_dup_list_end_conc[cdp_a_loop] = cdp_one_base_conc[cdp_one_base_index];
			    cdp_dup_list_end_rd[cdp_a_loop] = cdp_one_base_rd[cdp_one_base_index];
			    cdp_dup_list_end_dup_f[cdp_a_loop] = cdp_one_base_dup_f[cdp_one_base_index];
			    cdp_dup_list_end_read_start[cdp_a_loop] = cdp_one_base_dup_f_read_start[cdp_one_base_index];
			    cdp_dup_list_end_read_end[cdp_a_loop] = cdp_one_base_dup_f_read_end[cdp_one_base_index];
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			      {
				cdp_dup_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2;
				break;
			      }
			      else if( cdp_tumor_loop2 == g_other_len - 1 )
			      {
				cdp_dup_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2 + 1;
			      }
			    }
			  }
			}
		      }
		    }
		  }



		  if( cdp_one_base_del_f[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_pos_in_contig_start - cdp_one_base_del_f_read_end[cdp_one_base_index] < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_del_f[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_del_f[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_del_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_del_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_del_f[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_del_f[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_del_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_del_f[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }


		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      if( cdp_del_list_index < g_sv_list_len - 1 )
		      {
			cdp_del_list_start[cdp_del_list_index] = cdp_pos_in_contig_start;
			cdp_del_list_dist[cdp_del_list_index] = cdp_one_base_del_fdist[cdp_one_base_index];
			cdp_del_list_start_binom_cdf[cdp_del_list_index] = cdp_binom_cdf;
			cdp_del_list_start_hez_binom_cdf[cdp_del_list_index] = cdp_hez_binom_cdf;
			cdp_del_list_start_conc[cdp_del_list_index] = cdp_one_base_conc[cdp_one_base_index];
			cdp_del_list_start_rd[cdp_del_list_index] = cdp_one_base_rd[cdp_one_base_index];
			cdp_del_list_start_del_f[cdp_del_list_index] = cdp_one_base_del_f[cdp_one_base_index];
			cdp_del_list_start_read_start[cdp_del_list_index] = cdp_one_base_del_f_read_start[cdp_one_base_index];
			cdp_del_list_start_read_end[cdp_del_list_index] = cdp_one_base_del_f_read_end[cdp_one_base_index];
			for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			{
			  if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			  {
			    cdp_del_list_start_other_len[cdp_del_list_index] = cdp_tumor_loop2;
			    break;
			  }
			  else if( cdp_tumor_loop2 == g_other_len - 1 )
			  {
			    cdp_del_list_start_other_len[cdp_del_list_index] = cdp_tumor_loop2 + 1;
			  }
			}
			cdp_del_list_index += 1;
		      }
		      else
		      {
			g_sv_list_len_over = 1;
		      }

		    }
		  }



		  if( cdp_one_base_del_r[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_one_base_del_r_read_start[cdp_one_base_index] + cdp_lseq - cdp_pos_in_contig_start < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_del_r[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_del_r[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_del_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_del_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_del_r[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_del_r[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_del_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_del_r[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {

		      cdp_del_min = (int)(((double)(1.0)-g_range_mult) * cdp_one_base_del_rdist[cdp_one_base_index] + (double)0.5);
		      cdp_del_max = (int)(((double)(1.0)+g_range_mult) * cdp_one_base_del_rdist[cdp_one_base_index] + (double)0.5);
		      if( cdp_one_base_del_rdist[cdp_one_base_index] - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 > (double)cdp_del_min )
		      {
			cdp_del_min = (int)((cdp_one_base_del_rdist[cdp_one_base_index] - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      if( cdp_one_base_del_rdist[cdp_one_base_index] + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 < (double)cdp_del_max )
		      {
			cdp_del_max = (int)((cdp_one_base_del_rdist[cdp_one_base_index] + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      cdp_del_min = (int)((cdp_one_base_del_rdist[cdp_one_base_index] - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);
		      cdp_del_max = (int)((cdp_one_base_del_rdist[cdp_one_base_index] + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);

		      cdp_bl_list = cdp_del_list_start;
		      cdp_bl_pos = cdp_pos_in_contig_start + g_insert_mean - cdp_del_min;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_del_list_index;
		      cdp_bl_type = 0;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;

		      while( cdp_bl_index_found == 0 )
		      {

			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}

			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_start = cdp_bl_index;



		      cdp_bl_list = cdp_del_list_start;
		      cdp_bl_pos = cdp_pos_in_contig_start + g_insert_mean - cdp_del_max;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_del_list_index;
		      cdp_bl_type = 1;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;

		      while( cdp_bl_index_found == 0 )
		      {

			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}

			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_end = cdp_bl_index;


		      if( cdp_lp_end < cdp_lp_start )
		      {
			cdp_lp_temp = cdp_lp_end;
			cdp_lp_end = cdp_lp_start;
			cdp_lp_start = cdp_lp_temp;
		      }
		      cdp_start_pos = cdp_pos_in_contig_start + g_insert_mean - cdp_del_max;
		      cdp_end_pos = cdp_pos_in_contig_start + g_insert_mean - cdp_del_min;
		      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
		      {
			if( cdp_del_list_dist[cdp_a_loop] >= cdp_del_min && cdp_del_list_dist[cdp_a_loop] <= cdp_del_max && cdp_del_list_start[cdp_a_loop] >= cdp_start_pos && cdp_del_list_start[cdp_a_loop] <= cdp_end_pos )
			{
			  if( (cdp_del_list_end_binom_cdf[cdp_a_loop] > cdp_binom_cdf && cdp_one_base_del_r[cdp_one_base_index] >= cdp_del_list_end_del_r[cdp_a_loop]) || cdp_del_list_end[cdp_a_loop] == -1 || (cdp_del_list_end_binom_cdf[cdp_a_loop] == cdp_binom_cdf && cdp_one_base_del_r[cdp_one_base_index] >= cdp_del_list_end_del_r[cdp_a_loop]) )
			  {
			    cdp_del_list_end[cdp_a_loop] = cdp_pos_in_contig_start;
			    cdp_del_list_end_binom_cdf[cdp_a_loop] = cdp_binom_cdf;
			    cdp_del_list_end_hez_binom_cdf[cdp_a_loop] = cdp_hez_binom_cdf;
			    cdp_del_list_end_conc[cdp_a_loop] = cdp_one_base_conc[cdp_one_base_index];
			    cdp_del_list_end_rd[cdp_a_loop] = cdp_one_base_rd[cdp_one_base_index];
			    cdp_del_list_end_del_r[cdp_a_loop] = cdp_one_base_del_r[cdp_one_base_index];
			    cdp_del_list_end_read_start[cdp_a_loop] = cdp_one_base_del_r_read_start[cdp_one_base_index];
			    cdp_del_list_end_read_end[cdp_a_loop] = cdp_one_base_del_r_read_end[cdp_one_base_index];
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			      {
				cdp_del_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2;
				break;
			      }
			      else if( cdp_tumor_loop2 == g_other_len - 1 )
			      {
				cdp_del_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2 + 1;
			      }
			    }
			  }
			}
		      }
		    }
		  }



		  if( cdp_one_base_inv_f1[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_pos_in_contig_start - cdp_one_base_inv_f1_read_end[cdp_one_base_index] < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_inv_f1[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_inv_f1[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_f1[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_inv_f1[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_inv_f1[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_inv_f1[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_f1[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_inv_f1[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      if( cdp_inv_f_list_index < g_sv_list_len - 1 )
		      {
			cdp_inv_f_list_start[cdp_inv_f_list_index] = cdp_pos_in_contig_start;
			cdp_inv_f_list_dist[cdp_inv_f_list_index] = cdp_one_base_inv_f1dist[cdp_one_base_index];
			cdp_inv_f_list_start_binom_cdf[cdp_inv_f_list_index] = cdp_binom_cdf;
			cdp_inv_f_list_start_hez_binom_cdf[cdp_inv_f_list_index] = cdp_hez_binom_cdf;
			cdp_inv_f_list_start_conc[cdp_inv_f_list_index] = cdp_one_base_conc[cdp_one_base_index];
			cdp_inv_f_list_start_rd[cdp_inv_f_list_index] = cdp_one_base_rd[cdp_one_base_index];
			cdp_inv_f_list_start_inv[cdp_inv_f_list_index] = cdp_one_base_inv_f1[cdp_one_base_index];
			cdp_inv_f_list_start_read_start[cdp_inv_f_list_index] = cdp_one_base_inv_f1_read_start[cdp_one_base_index];
			cdp_inv_f_list_start_read_end[cdp_inv_f_list_index] = cdp_one_base_inv_f1_read_end[cdp_one_base_index];
			for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			{
			  if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			  {
			    cdp_inv_f_list_start_other_len[cdp_inv_f_list_index] = cdp_tumor_loop2;
			    break;
			  }
			  else if( cdp_tumor_loop2 == g_other_len - 1 )
			  {
			    cdp_inv_f_list_start_other_len[cdp_inv_f_list_index] = cdp_tumor_loop2 + 1;
			  }
			}


			cdp_inv_f_list_index += 1;
		      }
		      else
		      {
			g_sv_list_len_over = 1;
		      }

		    }
		  }



		  if( cdp_one_base_inv_f2[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_pos_in_contig_start - cdp_one_base_inv_f2_read_end[cdp_one_base_index] < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_inv_f2[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_inv_f2[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_f2[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_inv_f2[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_inv_f2[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/(float)cdp_one_base_inv_f2[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_f2[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_inv_f2[cdp_one_base_index] + cdp_one_base_sc_right[cdp_one_base_index] + cdp_one_base_munmapped_f[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      cdp_inv_temp_dist = cdp_one_base_inv_f2dist[cdp_one_base_index];
		      cdp_inv_min = (int)(((double)(1.0)-g_range_mult) * (cdp_inv_temp_dist + g_lseq) + (double)0.5);
		      cdp_inv_max = (int)(((double)(1.0)+g_range_mult) * (cdp_inv_temp_dist + g_lseq) + (double)0.5);
		      if( cdp_inv_temp_dist + g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 > (double)cdp_inv_min )
		      {
				cdp_inv_min = (int)((cdp_inv_temp_dist + g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      if( cdp_inv_temp_dist + g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 < (double)cdp_inv_max )
		      {
				cdp_inv_max = (int)((cdp_inv_temp_dist + g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      cdp_inv_min = (int)((cdp_inv_temp_dist + g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);
		      cdp_inv_max = (int)((cdp_inv_temp_dist + g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);


		      cdp_bl_list = cdp_inv_f_list_start;
		      cdp_bl_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_min;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_inv_f_list_index;
		      cdp_bl_type = 0;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;

		      while( cdp_bl_index_found == 0 )
		      {

			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}

			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_start = cdp_bl_index;



		      cdp_bl_list = cdp_inv_f_list_start;
		      cdp_bl_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_max;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_inv_f_list_index;
		      cdp_bl_type = 1;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;

		      while( cdp_bl_index_found == 0 )
		      {

			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}

			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] )
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_end = cdp_bl_index;


		      if( cdp_lp_end < cdp_lp_start )
		      {
			cdp_lp_temp = cdp_lp_end;
			cdp_lp_end = cdp_lp_start;
			cdp_lp_start = cdp_lp_temp;
		      }
		      cdp_start_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_max;
		      cdp_end_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_min;
		      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
		      {
			if( cdp_inv_f_list_dist[cdp_a_loop] >= cdp_inv_min && cdp_inv_f_list_dist[cdp_a_loop] <= cdp_inv_max && cdp_inv_f_list_start[cdp_a_loop] >= cdp_start_pos && cdp_inv_f_list_start[cdp_a_loop] <= cdp_end_pos )
			{
			  if( (cdp_inv_f_list_end_binom_cdf[cdp_a_loop] > cdp_binom_cdf && cdp_one_base_inv_f2[cdp_one_base_index] >= cdp_inv_f_list_end_inv[cdp_a_loop]) || cdp_inv_f_list_end[cdp_a_loop] == -1 || (cdp_inv_f_list_end_binom_cdf[cdp_a_loop] == cdp_binom_cdf && cdp_one_base_inv_f2[cdp_one_base_index] > cdp_inv_f_list_end_inv[cdp_a_loop]) )
			  {
			    cdp_inv_f_list_end[cdp_a_loop] = cdp_pos_in_contig_start;
			    cdp_inv_f_list_end_binom_cdf[cdp_a_loop] = cdp_binom_cdf;
			    cdp_inv_f_list_end_hez_binom_cdf[cdp_a_loop] = cdp_hez_binom_cdf;
			    cdp_inv_f_list_end_conc[cdp_a_loop] = cdp_one_base_conc[cdp_one_base_index];
			    cdp_inv_f_list_end_rd[cdp_a_loop] = cdp_one_base_rd[cdp_one_base_index];
			    cdp_inv_f_list_end_inv[cdp_a_loop] = cdp_one_base_inv_f2[cdp_one_base_index];
			    cdp_inv_f_list_end_read_start[cdp_a_loop] = cdp_one_base_inv_f2_read_start[cdp_one_base_index];
			    cdp_inv_f_list_end_read_end[cdp_a_loop] = cdp_one_base_inv_f2_read_end[cdp_one_base_index];


			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			      {
				cdp_inv_f_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2;
				break;
			      }
			      else if( cdp_tumor_loop2 == g_other_len - 1 )
			      {
				cdp_inv_f_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2 + 1;
			      }
			    }
			  }
			}
		      }
		    }
		  }



		  if( cdp_one_base_inv_r1[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_one_base_inv_r1_read_start[cdp_one_base_index] + cdp_lseq - cdp_pos_in_contig_start < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_inv_r1[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_inv_r1[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_r1[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_inv_r1[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_inv_r1[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_inv_r1[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_r1[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_inv_r1[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      if( cdp_inv_r_list_index < g_sv_list_len - 1 )
		      {
			cdp_inv_r_list_start[cdp_inv_r_list_index] = cdp_pos_in_contig_start;
			cdp_inv_r_list_dist[cdp_inv_r_list_index] = cdp_one_base_inv_r1dist[cdp_one_base_index];
			cdp_inv_r_list_start_binom_cdf[cdp_inv_r_list_index] = cdp_binom_cdf;
			cdp_inv_r_list_start_hez_binom_cdf[cdp_inv_r_list_index] = cdp_hez_binom_cdf;
			cdp_inv_r_list_start_conc[cdp_inv_r_list_index] = cdp_one_base_conc[cdp_one_base_index];
			cdp_inv_r_list_start_rd[cdp_inv_r_list_index] = cdp_one_base_rd[cdp_one_base_index];
			cdp_inv_r_list_start_inv[cdp_inv_r_list_index] = cdp_one_base_inv_r1[cdp_one_base_index];
			cdp_inv_r_list_start_read_start[cdp_inv_r_list_index] = cdp_one_base_inv_r1_read_start[cdp_one_base_index];
			cdp_inv_r_list_start_read_end[cdp_inv_r_list_index] = cdp_one_base_inv_r1_read_end[cdp_one_base_index];
			for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			{
			  if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			  {
			    cdp_inv_r_list_start_other_len[cdp_inv_r_list_index] = cdp_tumor_loop2;
			    break;
			  }
			  else if( cdp_tumor_loop2 == g_other_len - 1 )
			  {
			    cdp_inv_r_list_start_other_len[cdp_inv_r_list_index] = cdp_tumor_loop2 + 1;
			  }
			}
			cdp_inv_r_list_index += 1;
		      }
		      else
		      {
			g_sv_list_len_over = 1;
		      }

		    }
		  }



		  if( cdp_one_base_inv_r2[cdp_one_base_index]/cdp_add_factor >= g_min_disc && cdp_one_base_inv_r2_read_start[cdp_one_base_index] + cdp_lseq - cdp_pos_in_contig_start < g_insert_mean  )
		  {
		    cdp_hez_binom_cdf = 2.0;
		    if( cdp_one_base_rd[cdp_one_base_index] > g_max_trials )
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_inv_r2[cdp_one_base_index] * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_inv_r2[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_r2[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][(cdp_one_base_inv_r2[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index]) * g_max_trials/(cdp_add_factor*cdp_one_base_rd[cdp_one_base_index])];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[g_max_trials][g_max_trials];
			}
		      }

		    }
		    else
		    {
		      cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_inv_r2[cdp_one_base_index]/cdp_add_factor];
		      if( (float)(cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/(float)cdp_one_base_inv_r2[cdp_one_base_index] <= g_max_evidence_ratio )
		      {
			if( (cdp_one_base_inv_r2[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor < cdp_one_base_rd[cdp_one_base_index] )
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][(cdp_one_base_inv_r2[cdp_one_base_index] + cdp_one_base_sc_left[cdp_one_base_index] + cdp_one_base_munmapped_r[cdp_one_base_index])/cdp_add_factor];
			}
			else
			{
			  cdp_hez_binom_cdf = g_hez_prob_binom_cdf_table[cdp_one_base_rd[cdp_one_base_index]][cdp_one_base_rd[cdp_one_base_index]];
			}
		      }

		    }
		    if( cdp_binom_cdf <= g_pval_threshold1 )
		    {
		      cdp_inv_temp_dist = cdp_one_base_inv_r2dist[cdp_one_base_index];
		      cdp_inv_min = (int)(((double)(1.0)-g_range_mult) * (cdp_inv_temp_dist + g_lseq) + (double)0.5);
		      cdp_inv_max = (int)(((double)(1.0)+g_range_mult) * (cdp_inv_temp_dist + g_lseq) + (double)0.5);
		      if( cdp_inv_temp_dist + g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 > (double)cdp_inv_min )
		      {
				cdp_inv_min = (int)((cdp_inv_temp_dist + g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      if( cdp_inv_temp_dist + g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0 < (double)cdp_inv_max )
		      {
				cdp_inv_max = (int)((cdp_inv_temp_dist + g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)/(double)2.0) + (double)0.5);
		      }
		      cdp_inv_min = (int)((cdp_inv_temp_dist + g_lseq - g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);
		      cdp_inv_max = (int)((cdp_inv_temp_dist + g_lseq + g_range_mult * (double)(g_insert_max_size - g_insert_min_size)) + (double)0.5);


		      cdp_bl_list = cdp_inv_r_list_start;
		      cdp_bl_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_min;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_inv_r_list_index;
		      cdp_bl_type = 0;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;

		      while( cdp_bl_index_found == 0 )
		      {
			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )  
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] ) 
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_start = cdp_bl_index;



		      cdp_bl_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_max;
		      cdp_bl_start = 0;
		      cdp_bl_end = cdp_inv_r_list_index;
		      cdp_bl_type = 1;
		      cdp_bl_list = cdp_inv_r_list_start;
		      cdp_bl_range = cdp_bl_end/64;
		      if( cdp_bl_range < 4 )
		      {
			cdp_bl_range = 4;
		      }
		      else if( cdp_bl_range > 64 )
		      {
			cdp_bl_range = 64;
		      }
		      cdp_bl_low_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) - cdp_bl_range;
		      cdp_bl_high_index = round((double) cdp_bl_pos * (double) cdp_bl_end / (double) cdp_bl_list[cdp_bl_end]) + cdp_bl_range;
		      if( cdp_bl_low_index < cdp_bl_start || cdp_bl_low_index >= cdp_bl_end )
		      {
			cdp_bl_low_index = cdp_bl_start;
		      }
		      else if( cdp_bl_list[cdp_bl_low_index] > cdp_bl_pos )
		      {
			cdp_bl_high_index = cdp_bl_low_index;
			cdp_bl_low_index = cdp_bl_start;
		      }
		      if( cdp_bl_high_index > cdp_bl_end || cdp_bl_high_index < cdp_bl_start )
		      {
			cdp_bl_high_index = cdp_bl_end;
		      }
		      else if( cdp_bl_list[cdp_bl_high_index] < cdp_bl_pos )
		      {
			cdp_bl_low_index = cdp_bl_high_index;
			cdp_bl_high_index = cdp_bl_end;
		      }
		      cdp_bl_index = cdp_bl_low_index + (cdp_bl_high_index - cdp_bl_low_index)/2;
		      cdp_bl_index_found = 0;

		      while( cdp_bl_index_found == 0 )
		      {
			if( cdp_bl_pos < cdp_bl_list[cdp_bl_index] )  
			{
			  cdp_bl_high_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_low_index + (cdp_bl_index-cdp_bl_low_index)/2;
			  if( cdp_bl_high_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else if( cdp_bl_pos > cdp_bl_list[cdp_bl_index] ) 
			{
			  cdp_bl_low_index = cdp_bl_index;
			  cdp_bl_index = cdp_bl_index + (cdp_bl_high_index - cdp_bl_index)/2;
			  if( cdp_bl_low_index == cdp_bl_index )
			  {
			    cdp_bl_index_found = 1;
			  }
			}
			else
			{
			  cdp_bl_index_found = 1;
			}
		      }

		      if( cdp_bl_type == 0 && cdp_bl_pos > cdp_bl_list[cdp_bl_index] && cdp_bl_index < cdp_bl_end )
		      {
			cdp_bl_index += 1;
		      }
		      else if( cdp_bl_type == 1 && cdp_bl_pos < cdp_bl_list[cdp_bl_index] && cdp_bl_index > cdp_bl_start )
		      {
			cdp_bl_index -= 1;
		      }

		      cdp_lp_end = cdp_bl_index;


		      if( cdp_lp_end < cdp_lp_start )
		      {
			cdp_lp_temp = cdp_lp_end;
			cdp_lp_end = cdp_lp_start;
			cdp_lp_start = cdp_lp_temp;
		      }
		      cdp_start_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_max;
		      cdp_end_pos = cdp_pos_in_contig_start + g_lseq - cdp_inv_min;
		      for(cdp_a_loop=cdp_lp_start;cdp_a_loop<cdp_lp_end;cdp_a_loop++)
		      {
			if( cdp_inv_r_list_dist[cdp_a_loop] >= cdp_inv_min && cdp_inv_r_list_dist[cdp_a_loop] <= cdp_inv_max && cdp_inv_r_list_start[cdp_a_loop] >= cdp_start_pos && cdp_inv_r_list_start[cdp_a_loop] <= cdp_end_pos )
			{
			  if( (cdp_inv_r_list_end_binom_cdf[cdp_a_loop] > cdp_binom_cdf && cdp_one_base_inv_r2[cdp_one_base_index] >= cdp_inv_r_list_end_inv[cdp_a_loop]) || cdp_inv_r_list_end[cdp_a_loop] == -1 || (cdp_inv_r_list_end_binom_cdf[cdp_a_loop] == cdp_binom_cdf && cdp_one_base_inv_r2[cdp_one_base_index] > cdp_inv_r_list_end_inv[cdp_a_loop]) )
			  {
			    cdp_inv_r_list_end[cdp_a_loop] = cdp_pos_in_contig_start;
			    cdp_inv_r_list_end_binom_cdf[cdp_a_loop] = cdp_binom_cdf;
			    cdp_inv_r_list_end_hez_binom_cdf[cdp_a_loop] = cdp_hez_binom_cdf;
			    cdp_inv_r_list_end_conc[cdp_a_loop] = cdp_one_base_conc[cdp_one_base_index];
			    cdp_inv_r_list_end_rd[cdp_a_loop] = cdp_one_base_rd[cdp_one_base_index];
			    cdp_inv_r_list_end_inv[cdp_a_loop] = cdp_one_base_inv_r2[cdp_one_base_index];
			    cdp_inv_r_list_end_read_start[cdp_a_loop] = cdp_one_base_inv_r2_read_start[cdp_one_base_index];
			    cdp_inv_r_list_end_read_end[cdp_a_loop] = cdp_one_base_inv_r2_read_end[cdp_one_base_index];
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			      {
				cdp_inv_r_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2;
				break;
			      }
			      else if( cdp_tumor_loop2 == g_other_len - 1 )
			      {
				cdp_inv_r_list_end_other_len[cdp_a_loop] = cdp_tumor_loop2 + 1;
			      }
			    }
			  }
			}
		      }
		    }
		  }


		}
#ifdef DO_TIMING
		end_t = rdtsc();
		timers_ss[3] += end_t - start_t;
#endif


	      }


	      if( g_tumor_sv == 1 )
	      {

		double cdp_tumor_prob;
		while( cdp_tumor_start2_index < cdp_tumor_chr_len && cdp_tumor_start2[cdp_tumor_start2_index][0] < cdp_pos_in_contig_start )
		{
		  cdp_tumor_start2_index += 1;
		}
		while( cdp_tumor_end2_index < cdp_tumor_chr_len && cdp_tumor_end2[cdp_tumor_end2_index][0] < cdp_pos_in_contig_start )
		{
		  cdp_tumor_end2_index += 1;
		}

		while( (cdp_tumor_start2_index < cdp_tumor_chr_len && cdp_tumor_start2[cdp_tumor_start2_index][0] == cdp_pos_in_contig_start) || (cdp_tumor_end2_index < cdp_tumor_chr_len && cdp_tumor_end2[cdp_tumor_end2_index][0] == cdp_pos_in_contig_start) )
		{
		  cdp_tumor_loop = cdp_tumor_start2[cdp_tumor_start2_index][1];
		  if( cdp_tumor_start2[cdp_tumor_start2_index][0] == cdp_pos_in_contig_start )
		  {
		    cdp_snv_b_loop = 0;
		    for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
		    {
		      if( cdp_tumor_snv_base[cdp_tumor_loop] == g_dna[cdp_snv_a_loop] )
		      {
			cdp_snv_b_loop = cdp_snv_a_loop;
		      }
		    }
		    if( cdp_tumor_start[cdp_tumor_loop] == cdp_pos_in_contig_start )
		    {

		      for(cdp_a_loop=caf_last_snv_group_pos;cdp_a_loop<cdp_pos_in_contig_start - cdp_one_base_index;cdp_a_loop++)
		      {
			if( cdp_chr_fasta[cdp_a_loop] != 'N' && cdp_chr_fasta[cdp_a_loop] != 'n' )
			{
			  cdp_snv_read_count_total += (long)caf_rd_rd_list[cdp_a_loop] + (long)caf_rd_low_mq_rd_list[cdp_a_loop];
			  cdp_snv_base_total += 1;
			}
		      }
		      cdp_snv_ave_rd = (double)cdp_snv_read_count_total / (double)cdp_snv_base_total;
		      caf_last_snv_group_pos = cdp_pos_in_contig_start - cdp_one_base_index;

		      if( g_internal == 1 )
		      {
			if( cdp_chr_match >= 16 )
			{
			  printf("2: chr, snv rd %s, %e\n", cdp_chr_name, cdp_snv_ave_rd);
			  printf("2: %ld %d %d %d\n", cdp_snv_base_total, caf_last_snv_group_pos, cdp_pos_in_contig_start, cdp_one_base_index);
			}
		      }

		      if( g_vcf == 1 )
		      {
			if( g_normal == 1 )
			{

			}
			else
			{

			  cdp_snv_gt_string[0] = '1';
			  cdp_snv_gt_string[1] = '/';
			  cdp_snv_gt_string[2] = '1';
			  cdp_snv_gt_string[3] = '\0';

			  cdp_snv_total = 0;
			  cdp_snv_cn = g_ploidy;
			  cdp_binom_cdf = 1;
			  cdp_temp_snv_ratio = 0;
			  cdp_temp_snv_base_index = 0;
			  cdp_temp_snv_pir = 0;
			  cdp_temp_snv_fs = 0;
			  for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
			  {
			    cdp_snv_total += cdp_one_base_snv[cdp_b_loop][cdp_one_base_index];
			  }
			  for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
			  {
			    if( g_dna[cdp_b_loop] == cdp_tumor_snv_base[cdp_tumor_loop] )
			    {
			      cdp_temp_snv_base_index = cdp_b_loop;
			      if( cdp_snv_total > 0 )
			      {
					cdp_temp_snv_ratio = (float)cdp_one_base_snv[cdp_b_loop][cdp_one_base_index] / (float)cdp_snv_total;
					cdp_snv_cn = round((float)cdp_one_base_snv[cdp_b_loop][cdp_one_base_index] * (float)g_ploidy / (float)cdp_snv_total);
			      }
			      if( cdp_snv_total > g_max_trials )
			      {
					cdp_binom_cdf = g_mq_prob_binom_cdf_table[g_max_trials][cdp_one_base_snv[cdp_b_loop][cdp_one_base_index] * g_max_trials/cdp_snv_total];
			      }
			      else
			      {
					cdp_binom_cdf = g_mq_prob_binom_cdf_table[cdp_snv_total][cdp_one_base_snv[cdp_b_loop][cdp_one_base_index]];
			      }
			      if( cdp_one_base_snv[cdp_temp_snv_base_index][cdp_one_base_index] > 0 )
			      {
					cdp_temp_snv_pir = (double)cdp_one_base_pos_in_read[cdp_temp_snv_base_index][cdp_one_base_index]/(double)cdp_one_base_snv[cdp_temp_snv_base_index][cdp_one_base_index];
					cdp_temp_snv_fs = (double)cdp_one_base_fstrand[cdp_temp_snv_base_index][cdp_one_base_index]/(double)cdp_one_base_snv[cdp_temp_snv_base_index][cdp_one_base_index];
			      }
			      break;
			    }
			  }

			  int cdp_snv_indel_i_count = 0;
			  int cdp_snv_indel_d_f_count = 0;
			  int cdp_snv_indel_d_r_count = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_snv_indel_range;cdp_snv_a_loop++)
			  {
			    cdp_snv_indel_i_count += cdp_one_base_indel_i[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_i[cdp_one_base_index - cdp_snv_a_loop];
			    cdp_snv_indel_d_f_count += cdp_one_base_indel_d_f[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_d_f[cdp_one_base_index - cdp_snv_a_loop];
			    cdp_snv_indel_d_r_count += cdp_one_base_indel_d_r[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_d_r[cdp_one_base_index - cdp_snv_a_loop];
			  }

			  cdp_max_sc = 0;
			  for(cdp_tumor_loop2=(cdp_one_base_index-g_normal_sc_range);cdp_tumor_loop2<=(cdp_one_base_index+g_normal_sc_range);cdp_tumor_loop2++)
			  {
			    cdp_max_sc += cdp_one_base_sc_left[cdp_tumor_loop2] + cdp_one_base_sc_right[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_r[cdp_tumor_loop2] + cdp_one_base_indel_d_f[cdp_tumor_loop2];
			  }
			  cdp_temp_other_len = 0;
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			    {
			      cdp_temp_other_len = cdp_tumor_loop2;
			      break;
			    }
			    else if( cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      cdp_temp_other_len = cdp_tumor_loop2 + 1;
			    }
			  }


			  fprintf(cdp_results_file, "%s\t%d\t.\t%c\t%c\t.\t.\tGT:PR:AF:AMQ:CMQ:GMQ:TMQ:ALMQ:CLMQ:GLMQ:TLMQ:BQ:MQ:PIR:FS:OL:IN:SC:PRT:PRTL:SCA:SCC:SCG:SCT\t%s:%e:%e:%d:%d:%d:%d:%d:%d:%d:%d:%.2f:%.2f:%.2f:%.2f:%d:%d:%d:%e:%e:%d:%d:%d:%d\n", cdp_chr_name, cdp_tumor_start[cdp_tumor_loop] + 1, cdp_chr_fasta[cdp_tumor_start[cdp_tumor_loop]], cdp_tumor_snv_base[cdp_tumor_loop], cdp_snv_gt_string, cdp_binom_cdf, cdp_temp_snv_ratio, cdp_one_base_snv[0][cdp_one_base_index], cdp_one_base_snv[1][cdp_one_base_index], cdp_one_base_snv[2][cdp_one_base_index], cdp_one_base_snv[3][cdp_one_base_index], cdp_one_base_snv_lowmq[0][cdp_one_base_index], cdp_one_base_snv_lowmq[1][cdp_one_base_index], cdp_one_base_snv_lowmq[2][cdp_one_base_index], cdp_one_base_snv_lowmq[3][cdp_one_base_index], (double)cdp_one_base_bq_all[cdp_one_base_index]/(double)cdp_one_base_read_count_all[cdp_one_base_index], (double)cdp_one_base_mq_all[cdp_one_base_index]/(double)cdp_one_base_read_count_all[cdp_one_base_index], cdp_temp_snv_pir, cdp_temp_snv_fs, cdp_temp_other_len, cdp_snv_indel_i_count + cdp_snv_indel_d_f_count + cdp_snv_indel_d_r_count, cdp_max_sc, 0.0, 0.0, 0, 0, 0, 0);
			}

		      }
		      else
		      {

			if( cdp_tumor_type[cdp_tumor_loop] == DEL )
			{
			  if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_del_fdist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			  {
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_DEL_F && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			      {
				cdp_one_base_del_f[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_del_fdist[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_del_f_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_del_f_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
				break;
			      }
			      else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			      {
				break;
			      }
			    }
			  }
			}
			if( cdp_tumor_type[cdp_tumor_loop] == DUP )
			{
			  if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_dup_rdist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			  {
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_DUP_R && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			      {
				cdp_one_base_dup_r[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_dup_rdist[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_dup_r_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_dup_r_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
				break;
			      }
			      else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			      {
				break;
			      }
			    }
			  }
			}
			if( cdp_tumor_type[cdp_tumor_loop] == INV || cdp_tumor_type[cdp_tumor_loop] == INV_F || cdp_tumor_type[cdp_tumor_loop] == INV_R )
			{
			  if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_inv_f1dist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) ||  cdp_one_base_inv_f1[cdp_one_base_index] == 0 )
			  {
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_INV_F1 && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			      {
				cdp_one_base_inv_f1[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_inv_f1dist[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_inv_f1_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_inv_f1_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
				break;
			      }
			      else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			      {
				break;
			      }
			    }
			  }
			  if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_inv_r1dist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) || cdp_one_base_inv_r1[cdp_one_base_index] == 0 )
			  {
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_INV_R1 && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			      {
				cdp_one_base_inv_r1[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_inv_r1dist[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_inv_r1_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_inv_r1_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
				break;
			      }
			      else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			      {
				break;
			      }
			    }
			  }
			}

			if( cdp_tumor_type[cdp_tumor_loop] == CTX_F )
			{
			  if( cdp_tumor_mchr[cdp_tumor_loop] != cdp_one_base_ctx_f_mchr[cdp_one_base_index] || abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_ctx_f_mpos[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			  {
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_CTX_F && cdp_tumor_mchr[cdp_tumor_loop] == cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index] && abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			      {
				cdp_one_base_ctx_f[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_f_mchr[cdp_one_base_index] = cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_f_mpos[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_f_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_f_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
				break;
			      }
			      else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			      {
				break;
			      }
			    }
			  }
			}
			if( cdp_tumor_type[cdp_tumor_loop] == CTX_R )
			{
			  if( cdp_tumor_mchr[cdp_tumor_loop] != cdp_one_base_ctx_r_mchr[cdp_one_base_index] || abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_ctx_r_mpos[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			  {
			    for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			    {
			      if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_CTX_R && cdp_tumor_mchr[cdp_tumor_loop] == cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index] && abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			      {
				cdp_one_base_ctx_r[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_r_mchr[cdp_one_base_index] = cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_r_mpos[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_r_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
				cdp_one_base_ctx_r_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
				break;
			      }
			      else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			      {
				break;
			      }
			    }
			  }
			}

			fprintf(cdp_results_file, "%s\t", g_sv_types[cdp_tumor_type[cdp_tumor_loop]]); 
			fprintf(cdp_results_file, "%s\t", cdp_chr_name); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start[cdp_tumor_loop] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end[cdp_tumor_loop] + 1); 
			fprintf(cdp_results_file, "%e\t", cdp_tumor_dist[cdp_tumor_loop]); 

			if( cdp_tumor_type[cdp_tumor_loop] == SNV )
			{
			  cdp_tumor_read_count = 0;
			  cdp_tumor_snv_count = 0;
			  cdp_normal_read_count = 0;
			  cdp_total_read_count = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			  {
			    cdp_tumor_read_count += cdp_tumor_snv[cdp_snv_a_loop][cdp_tumor_loop];
			    cdp_normal_read_count += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index];
			    if( cdp_tumor_snv_base[cdp_tumor_loop] == g_dna[cdp_snv_a_loop] )
			    {
			      cdp_tumor_snv_count = cdp_tumor_snv[cdp_snv_a_loop][cdp_tumor_loop];
			    }
			  }
			  cdp_total_read_count = cdp_tumor_read_count + cdp_normal_read_count;

			  cdp_n1 = cdp_total_read_count;
			  cdp_k1 = cdp_tumor_read_count;
			  cdp_n2 = cdp_tumor_read_count;

			  cdp_k2 = cdp_tumor_snv_count;

			  if( cdp_n1 > g_max_combinations )
			  {
			    cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			    cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			    cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			    cdp_n1 = g_max_combinations;
			    if( cdp_k2 == 0 )
			    {
			      cdp_k2 = 1;
			    }
			  }

			  cdp_comb_div_list_len = 0;
			  cdp_comb_mult_list_len = 0;
			  for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			  {
			    cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			    cdp_comb_div_list_len += 1;
			  }
			  for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			  {
			    cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			    cdp_comb_mult_list_len += 1;
			  }

			  cdp_tumor_start_binom_cdf[cdp_tumor_loop] = 1;
			  cdp_div_index = 0;
			  cdp_mult_index = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			  {
			    if( cdp_tumor_start_binom_cdf[cdp_tumor_loop] > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			    {
			      cdp_tumor_start_binom_cdf[cdp_tumor_loop] = cdp_tumor_start_binom_cdf[cdp_tumor_loop] / cdp_comb_div_list[cdp_div_index];
			      cdp_div_index += 1;
			    }
			    else
			    {
			      cdp_tumor_start_binom_cdf[cdp_tumor_loop] = cdp_tumor_start_binom_cdf[cdp_tumor_loop] * cdp_comb_mult_list[cdp_mult_index];
			      cdp_mult_index += 1;
			      if( cdp_tumor_start_binom_cdf[cdp_tumor_loop] < 1e-50 )
			      {
				break;
			      }
			    }
			  }

			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			  {
			    cdp_tumor_read_count += cdp_tumor_snv_lowmq[cdp_snv_a_loop][cdp_tumor_loop];
			    cdp_normal_read_count += cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
			    if( cdp_tumor_snv_base[cdp_tumor_loop] == g_dna[cdp_snv_a_loop] )
			    {
			      cdp_tumor_snv_count += cdp_tumor_snv_lowmq[cdp_snv_a_loop][cdp_tumor_loop];
			    }
			  }
			  cdp_total_read_count = cdp_tumor_read_count + cdp_normal_read_count;

			  cdp_n1 = cdp_total_read_count;
			  cdp_k1 = cdp_tumor_read_count;
			  cdp_n2 = cdp_tumor_read_count;

			  cdp_k2 = cdp_tumor_snv_count;

			  if( cdp_n1 > g_max_combinations )
			  {
			    cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			    cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			    cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			    cdp_n1 = g_max_combinations;
			    if( cdp_k2 == 0 )
			    {
			      cdp_k2 = 1;
			    }
			  }

			  cdp_comb_div_list_len = 0;
			  cdp_comb_mult_list_len = 0;
			  for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			  {
			    cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			    cdp_comb_div_list_len += 1;
			  }
			  for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			  {
			    cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			    cdp_comb_mult_list_len += 1;
			  }

			  cdp_tumor_end_binom_cdf[cdp_tumor_loop] = 1;
			  cdp_div_index = 0;
			  cdp_mult_index = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			  {
			    if( cdp_tumor_end_binom_cdf[cdp_tumor_loop] > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			    {
			      cdp_tumor_end_binom_cdf[cdp_tumor_loop] = cdp_tumor_end_binom_cdf[cdp_tumor_loop] / cdp_comb_div_list[cdp_div_index];
			      cdp_div_index += 1;
			    }
			    else
			    {
			      cdp_tumor_end_binom_cdf[cdp_tumor_loop] = cdp_tumor_end_binom_cdf[cdp_tumor_loop] * cdp_comb_mult_list[cdp_mult_index];
			      cdp_mult_index += 1;
			      if( cdp_tumor_end_binom_cdf[cdp_tumor_loop] < 1e-50 )
			      {
				break;
			      }
			    }
			  }

			}

			else if( cdp_tumor_type[cdp_tumor_loop] == DEL || cdp_tumor_type[cdp_tumor_loop] == DUP || cdp_tumor_type[cdp_tumor_loop] == INV || cdp_tumor_type[cdp_tumor_loop] == CTX_F || cdp_tumor_type[cdp_tumor_loop] == CTX_R || cdp_tumor_type[cdp_tumor_loop] == INV_F || cdp_tumor_type[cdp_tumor_loop] == INV_R )
			{
			  cdp_n1 = cdp_tumor_start_rd[cdp_tumor_loop] + cdp_one_base_rd[cdp_one_base_index];
			  cdp_k1 = cdp_tumor_start_rd[cdp_tumor_loop];
			  cdp_n2 = cdp_tumor_start_rd[cdp_tumor_loop];

			  cdp_k2 = cdp_tumor_start_sv_evidence[cdp_tumor_loop] / cdp_add_factor;

			  if( cdp_n1 > g_max_combinations )
			  {
			    cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			    cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			    cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			    cdp_n1 = g_max_combinations;
			    if( cdp_k2 == 0 )
			    {
			      cdp_k2 = 1;
			    }
			  }


			  cdp_comb_div_list_len = 0;
			  cdp_comb_mult_list_len = 0;
			  for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			  {
			    cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			    cdp_comb_div_list_len += 1;
			  }
			  for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			  {
			    cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			    cdp_comb_mult_list_len += 1;
			  }

			  cdp_tumor_start_binom_cdf[cdp_tumor_loop] = 1;
			  cdp_div_index = 0;
			  cdp_mult_index = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			  {
			    if( cdp_tumor_start_binom_cdf[cdp_tumor_loop] > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			    {
			      cdp_tumor_start_binom_cdf[cdp_tumor_loop] = cdp_tumor_start_binom_cdf[cdp_tumor_loop] / cdp_comb_div_list[cdp_div_index];
			      cdp_div_index += 1;
			    }
			    else
			    {
			      cdp_tumor_start_binom_cdf[cdp_tumor_loop] = cdp_tumor_start_binom_cdf[cdp_tumor_loop] * cdp_comb_mult_list[cdp_mult_index];
			      cdp_mult_index += 1;
			      if( cdp_tumor_start_binom_cdf[cdp_tumor_loop] < 1e-50 )
			      {
				break;
			      }
			    }
			  }
			}

			fprintf(cdp_results_file, "%e\t", cdp_tumor_start_binom_cdf[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%e\t", cdp_tumor_end_binom_cdf[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_conc[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end_conc[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%s\t", "START");  
			fprintf(cdp_results_file, "%d\t", cdp_one_base_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_conc[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ins[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_del_f[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_del_r[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_del_fdist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_del_rdist[cdp_one_base_index]); 

			fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_r[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_f[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_dup_rdist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_dup_fdist[cdp_one_base_index]); 

			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f1[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r1[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f2[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r2[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_f1dist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_r1dist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_f2dist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_r2dist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_munmapped_f[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_munmapped_r[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_left[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_right[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_left_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_right_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_i[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_f[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_r[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_fdist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_rdist[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_left[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_right[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_left_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_right_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_left[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_right[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_left_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_right_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_rd[cdp_one_base_index]); 

			cdp_max_sc_left = 0;
			cdp_max_sc_right = 0;
			for(cdp_tumor_loop2=(cdp_one_base_index-g_normal_sc_range);cdp_tumor_loop2<=(cdp_one_base_index+g_normal_sc_range);cdp_tumor_loop2++)
			{
			  if( cdp_one_base_sc_left[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_r[cdp_tumor_loop2] > cdp_max_sc_left )
			  {
			    cdp_max_sc_left = cdp_one_base_sc_left[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_r[cdp_tumor_loop2];
			  }
			  if( cdp_one_base_sc_right[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_f[cdp_tumor_loop2] > cdp_max_sc_right )
			  {
			    cdp_max_sc_right = cdp_one_base_sc_right[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_f[cdp_tumor_loop2];
			  }
			}
			fprintf(cdp_results_file, "%d\t", cdp_max_sc_left); 
			fprintf(cdp_results_file, "%d\t", cdp_max_sc_right); 
			for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			{
			  if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			  {
			    fprintf(cdp_results_file, "%d\t", cdp_tumor_loop2); 
			    break;
			  }
			  else if( cdp_tumor_loop2 == g_other_len - 1 )
			  {
			    fprintf(cdp_results_file, "%d\t", (cdp_tumor_loop2+1) ); 
			  }
			}

			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_f[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_r[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", 0); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_other_len[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_read_start[cdp_tumor_loop] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_read_end[cdp_tumor_loop] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end_read_start[cdp_tumor_loop] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end_read_end[cdp_tumor_loop] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_del_f_read_start[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_del_f_read_end[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_r_read_start[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_r_read_end[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f1_read_start[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f1_read_end[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r1_read_start[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r1_read_end[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_f_read_start[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_f_read_end[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_r_read_start[cdp_one_base_index] + 1); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_r_read_end[cdp_one_base_index] + 1); 
			if( cdp_tumor_type[cdp_tumor_loop] != 6 && cdp_tumor_type[cdp_tumor_loop] != 7 )  
			{
			  fprintf(cdp_results_file, "-\t"); 
			  fprintf(cdp_results_file, "0\t"); 
			}
			else  
			{
			  fprintf(cdp_results_file, "%s\t", cdp_bam_file->header->target_name[cdp_tumor_mchr[cdp_tumor_loop]]);
			  fprintf(cdp_results_file, "%d\t", cdp_tumor_mpos[cdp_tumor_loop] + 1); 
			}
			fprintf(cdp_results_file, "%s\t", cdp_bam_file->header->target_name[cdp_one_base_ctx_f_mchr[cdp_one_base_index]]);  
			fprintf(cdp_results_file, "%d\t", (int) (cdp_one_base_ctx_f_mpos[cdp_one_base_index]) + 1);  
			fprintf(cdp_results_file, "%s\t", cdp_bam_file->header->target_name[cdp_one_base_ctx_r_mchr[cdp_one_base_index]]);  
			fprintf(cdp_results_file, "%d\t", (int) (cdp_one_base_ctx_r_mpos[cdp_one_base_index]) + 1);  

			
			fprintf(cdp_results_file, "%c\t", cdp_chr_fasta[cdp_tumor_start[cdp_tumor_loop]]);  
			fprintf(cdp_results_file, "%c\t", cdp_tumor_snv_base[cdp_tumor_loop]);  
			fprintf(cdp_results_file, "%e\t", cdp_tumor_snv_ratio[cdp_tumor_loop]);  
			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			{
			  fprintf(cdp_results_file, "%d\t", cdp_tumor_snv[cdp_snv_a_loop][cdp_tumor_loop]);  
			}

			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			{
			  fprintf(cdp_results_file, "%d\t", cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index]);  
			}

			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			{
			  fprintf(cdp_results_file, "%d\t", cdp_one_base_snv_sc[cdp_snv_a_loop][cdp_one_base_index]);  
			}

			int cdp_snv_indel_i_count = 0;
			int cdp_snv_indel_d_f_count = 0;
			int cdp_snv_indel_d_r_count = 0;
			double cdp_snv_indel_i_dist = 0;
			double cdp_snv_indel_d_f_dist = 0;
			double cdp_snv_indel_d_r_dist = 0;
			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_snv_indel_range;cdp_snv_a_loop++)
			{
			  cdp_snv_indel_i_count += cdp_one_base_indel_i[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_i[cdp_one_base_index - cdp_snv_a_loop];
			  cdp_snv_indel_d_f_count += cdp_one_base_indel_d_f[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_d_f[cdp_one_base_index - cdp_snv_a_loop];
			  cdp_snv_indel_d_r_count += cdp_one_base_indel_d_r[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_d_r[cdp_one_base_index - cdp_snv_a_loop];
			  if( cdp_snv_indel_i_count > 0 )
			  {
			    cdp_snv_indel_i_dist += ((float) cdp_snv_a_loop - cdp_snv_indel_i_dist) * (cdp_one_base_indel_i[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_i[cdp_one_base_index - cdp_snv_a_loop])/ (float) cdp_snv_indel_i_count;
			  }
			  if( cdp_snv_indel_d_f_count > 0 )
			  {
			    cdp_snv_indel_d_f_dist += ((float) cdp_snv_a_loop - cdp_snv_indel_d_f_dist) * (cdp_one_base_indel_d_f[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_d_f[cdp_one_base_index - cdp_snv_a_loop])/ (float) cdp_snv_indel_d_f_count;
			  }
			  if( cdp_snv_indel_d_r_count > 0 )
			  {
			    cdp_snv_indel_d_r_dist += ((float) cdp_snv_a_loop - cdp_snv_indel_d_r_dist) * (cdp_one_base_indel_d_r[cdp_one_base_index + cdp_snv_a_loop] + cdp_one_base_indel_d_r[cdp_one_base_index - cdp_snv_a_loop])/ (float) cdp_snv_indel_d_r_count;
			  }
			}
			fprintf(cdp_results_file, "%d\t", cdp_snv_indel_i_count); 
			fprintf(cdp_results_file, "%d\t", cdp_snv_indel_d_f_count); 
			fprintf(cdp_results_file, "%d\t", cdp_snv_indel_d_r_count); 
			fprintf(cdp_results_file, "%e\t", cdp_snv_indel_i_dist); 
			fprintf(cdp_results_file, "%e\t", cdp_snv_indel_d_f_dist); 
			fprintf(cdp_results_file, "%e\t", cdp_snv_indel_d_r_dist); 

			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			{
			  fprintf(cdp_results_file, "%d\t", cdp_tumor_snv_lowmq[cdp_snv_a_loop][cdp_tumor_loop]);  
			}
			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			{
			  fprintf(cdp_results_file, "%d\t", cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index]);  
			}

			if( cdp_tumor_type[cdp_tumor_loop] == SNV )
			{
			  cdp_tumor_read_count = 0;
			  cdp_tumor_snv_count = 0;
			  cdp_normal_read_count = 0;
			  cdp_total_read_count = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			  {
			    cdp_tumor_read_count += cdp_tumor_snv[cdp_snv_a_loop][cdp_tumor_loop];
			    cdp_normal_read_count += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] + cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
			    if( cdp_tumor_snv_base[cdp_tumor_loop] == g_dna[cdp_snv_a_loop] )
			    {
			      cdp_tumor_snv_count = cdp_tumor_snv[cdp_snv_a_loop][cdp_tumor_loop];
			    }
			  }
			  cdp_total_read_count = cdp_tumor_read_count + cdp_normal_read_count;

			  cdp_n1 = cdp_total_read_count;
			  cdp_k1 = cdp_tumor_read_count;
			  cdp_n2 = cdp_tumor_read_count;

			  cdp_k2 = cdp_tumor_snv_count;

			  if( cdp_n1 > g_max_combinations )
			  {
			    cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			    cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			    cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			    cdp_n1 = g_max_combinations;
			    if( cdp_k2 == 0 )
			    {
			      cdp_k2 = 1;
			    }
			  }

			  cdp_comb_div_list_len = 0;
			  cdp_comb_mult_list_len = 0;
			  for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			  {
			    cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			    cdp_comb_div_list_len += 1;
			  }
			  for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			  {
			    cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			    cdp_comb_mult_list_len += 1;
			  }

			  cdp_tumor_start_binom_cdf[cdp_tumor_loop] = 1;
			  cdp_div_index = 0;
			  cdp_mult_index = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			  {
			    if( cdp_tumor_start_binom_cdf[cdp_tumor_loop] > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			    {
			      cdp_tumor_start_binom_cdf[cdp_tumor_loop] = cdp_tumor_start_binom_cdf[cdp_tumor_loop] / cdp_comb_div_list[cdp_div_index];
			      cdp_div_index += 1;
			    }
			    else
			    {
			      cdp_tumor_start_binom_cdf[cdp_tumor_loop] = cdp_tumor_start_binom_cdf[cdp_tumor_loop] * cdp_comb_mult_list[cdp_mult_index];
			      cdp_mult_index += 1;
			      if( cdp_tumor_start_binom_cdf[cdp_tumor_loop] < 1e-50 )
			      {
				break;
			      }
			    }
			  }

			}
			fprintf(cdp_results_file, "%e\t", cdp_tumor_start_binom_cdf[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_f_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_r_rd[cdp_one_base_index]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_indel[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end_indel[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_rd[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end_rd[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_sc[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end_sc[cdp_tumor_loop]); 

			cdp_tumor_prob = 1.0;
			if( cdp_tumor_type[cdp_tumor_loop] == INDEL_DEL )
			{
			  int cdp_tumor_indel = (cdp_tumor_start_indel[cdp_tumor_loop] + cdp_one_base_indel_d_f[cdp_one_base_index])/cdp_add_factor;
			  cdp_total_read_count = cdp_tumor_start_rd[cdp_tumor_loop] + cdp_one_base_indel_d_f_rd[cdp_one_base_index] + cdp_one_base_sc_right_rd[cdp_one_base_index];
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			  {
			    cdp_total_read_count += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] + cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
			  }

			  cdp_n1 = cdp_total_read_count;
			  cdp_k1 = cdp_tumor_start_rd[cdp_tumor_loop];
			  cdp_n2 = cdp_tumor_start_rd[cdp_tumor_loop];

			  cdp_k2 = cdp_tumor_indel;

			  if( cdp_n1 > g_max_combinations )
			  {
			    cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			    cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			    cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			    cdp_n1 = g_max_combinations;
			    if( cdp_k2 == 0 )
			    {
			      cdp_k2 = 1;
			    }
			  }

			  cdp_comb_div_list_len = 0;
			  cdp_comb_mult_list_len = 0;
			  for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			  {
			    cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			    cdp_comb_div_list_len += 1;
			  }
			  for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			  {
			    cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			    cdp_comb_mult_list_len += 1;
			  }

			  cdp_div_index = 0;
			  cdp_mult_index = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			  {
			    if( cdp_tumor_prob > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			    {
			      cdp_tumor_prob = cdp_tumor_prob / cdp_comb_div_list[cdp_div_index];
			      cdp_div_index += 1;
			    }
			    else
			    {
			      cdp_tumor_prob = cdp_tumor_prob * cdp_comb_mult_list[cdp_mult_index];
			      cdp_mult_index += 1;
			      if( cdp_tumor_prob < 1e-50 )
			      {
				break;
			      }
			    }
			  }

			}
			else if( cdp_tumor_type[cdp_tumor_loop] == INDEL_INS )
			{
			  int cdp_tumor_indel = (cdp_tumor_start_indel[cdp_tumor_loop] + cdp_tumor_start_sc[cdp_tumor_loop])/cdp_add_factor;
			  cdp_total_read_count = cdp_tumor_start_rd[cdp_tumor_loop] + cdp_one_base_sc_left_rd[cdp_one_base_index + 1] + cdp_one_base_sc_right_rd[cdp_one_base_index];
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			  {
			    cdp_total_read_count += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] + cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
			  }

			  cdp_n1 = cdp_total_read_count;
			  cdp_k1 = cdp_tumor_start_rd[cdp_tumor_loop];
			  cdp_n2 = cdp_tumor_start_rd[cdp_tumor_loop];

			  cdp_k2 = cdp_tumor_indel;

			  if( cdp_n1 > g_max_combinations )
			  {
			    cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			    cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			    cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			    cdp_n1 = g_max_combinations;
			    if( cdp_k2 == 0 )
			    {
			      cdp_k2 = 1;
			    }
			  }

			  cdp_comb_div_list_len = 0;
			  cdp_comb_mult_list_len = 0;
			  for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			  {
			    cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			    cdp_comb_div_list_len += 1;
			  }
			  for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			  {
			    cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			    cdp_comb_mult_list_len += 1;
			  }

			  cdp_div_index = 0;
			  cdp_mult_index = 0;
			  for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			  {
			    if( cdp_tumor_prob > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			    {
			      cdp_tumor_prob = cdp_tumor_prob / cdp_comb_div_list[cdp_div_index];
			      cdp_div_index += 1;
			    }
			    else
			    {
			      cdp_tumor_prob = cdp_tumor_prob * cdp_comb_mult_list[cdp_mult_index];
			      cdp_mult_index += 1;
			      if( cdp_tumor_prob < 1e-50 )
			      {
				break;
			      }
			    }
			  }

			}
			fprintf(cdp_results_file, "%e\t", cdp_tumor_prob); 

			fprintf(cdp_results_file, "%d", cdp_tumor_snv_bq[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "\t%d", cdp_tumor_snv_bq_all[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "\t%d", cdp_tumor_snv_mq[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "\t%d", cdp_tumor_snv_mq_all[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "\t%d", cdp_tumor_snv_bq_read_count[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "\t%d", cdp_tumor_snv_mq_read_count[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "\t%d", cdp_tumor_snv_read_count_all[cdp_tumor_loop]); 

			fprintf(cdp_results_file, "\t%d", cdp_one_base_bq[cdp_one_base_index]); 
			fprintf(cdp_results_file, "\t%d", cdp_one_base_bq_all[cdp_one_base_index]); 
			fprintf(cdp_results_file, "\t%d", cdp_one_base_mq[cdp_one_base_index]); 
			fprintf(cdp_results_file, "\t%d", cdp_one_base_mq_all[cdp_one_base_index]); 
			fprintf(cdp_results_file, "\t%d", cdp_one_base_bq_read_count[cdp_one_base_index]); 
			fprintf(cdp_results_file, "\t%d", cdp_one_base_mq_read_count[cdp_one_base_index]); 
			fprintf(cdp_results_file, "\t%d", cdp_one_base_read_count_all[cdp_one_base_index]); 

			fprintf(cdp_results_file, "\t%.2f", cdp_tumor_snv_pos_in_read[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "\t%.2f", cdp_tumor_snv_fstrand[cdp_tumor_loop]); 

			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			{
			  if( toupper(cdp_tumor_snv_base[cdp_tumor_loop]) == g_dna[cdp_snv_a_loop] )
			  {
			    fprintf(cdp_results_file, "\t%.2f", (double)cdp_one_base_pos_in_read[cdp_snv_a_loop][cdp_one_base_index]/(double)cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index]); 
			    fprintf(cdp_results_file, "\t%.2f", (double)cdp_one_base_fstrand[cdp_snv_a_loop][cdp_one_base_index]/(double)cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index]); 
			    break;
			  }
			}

			if( cdp_tumor_start[cdp_tumor_loop] > 0 || cdp_tumor_start[cdp_tumor_loop] < cdp_chr_fasta_len - 1 )
			{
			  fprintf(cdp_results_file, "\t%c", cdp_chr_fasta[cdp_tumor_start[cdp_tumor_loop] -1]); 
			  fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_tumor_start[cdp_tumor_loop]]); 
			  fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_tumor_start[cdp_tumor_loop] + 1]); 
			}
			else
			{
			  fprintf(cdp_results_file, "\t%c%c%c", '.','.','.'); 
			}
			fprintf(cdp_results_file, "\t");
			for(cdp_b_loop=0;cdp_b_loop<cdp_lseq;cdp_b_loop++)
			{
			  if( cdp_tumor_start[cdp_tumor_loop] - cdp_lseq + 1 + cdp_b_loop < 0 )
			  {
			    fprintf(cdp_results_file, "%c", 'N');
			  }
			  else
			  {
			    fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_tumor_start[cdp_tumor_loop] - cdp_lseq + 1 + cdp_b_loop]);
			  }
			}
			for(cdp_b_loop=0;cdp_b_loop<cdp_lseq-1;cdp_b_loop++)
			{
			  if( cdp_tumor_start[cdp_tumor_loop] + cdp_lseq - 1 - cdp_b_loop >=  cdp_chr_fasta_len - 1 )
			  {
			    fprintf(cdp_results_file, "%c", 'N');
			  }
			  else
			  {
			    fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_tumor_start[cdp_tumor_loop] + cdp_lseq - 1 - cdp_b_loop]);
			  }
			}

			fprintf(cdp_results_file, "\t");
			fprintf(cdp_results_file, "%d\t", cdp_tumor_start_sv_evidence[cdp_tumor_loop]); 
			fprintf(cdp_results_file, "%d\t", cdp_tumor_end_sv_evidence[cdp_tumor_loop]); 

			fprintf(cdp_results_file, "%e\t", cdp_tumor_snv_start_binom_cdf[cdp_tumor_loop]);  
			fprintf(cdp_results_file, "%e\t", cdp_tumor_snv_start_hez_binom_cdf[cdp_tumor_loop]);  

			if( cdp_tumor_type[cdp_tumor_loop] == INDEL_INS )
			{
			  for(cdp_b_loop=0;cdp_b_loop<cdp_snv_gt_string_len;cdp_b_loop++)
			  {
			    cdp_snv_gt_string[cdp_b_loop] = cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_indel_seq_index[cdp_tumor_loop]];
			  }
			  fprintf(cdp_results_file, "%s", cdp_snv_gt_string);  
			}
			else if( cdp_tumor_type[cdp_tumor_loop] == INDEL_DEL )
			{
			  for(cdp_b_loop=0;cdp_b_loop<cdp_snv_gt_string_len;cdp_b_loop++)
			  {
			    cdp_snv_gt_string[cdp_b_loop] = cdp_tumor_indel_seq[cdp_b_loop][cdp_tumor_indel_seq_index[cdp_tumor_loop]];
			  }
			  fprintf(cdp_results_file, "%s", cdp_snv_gt_string);  
			}
			else
			{
			  fprintf(cdp_results_file, ".");  
			}

			fprintf(cdp_results_file, "\n");

		      }

		    }
		  }


		  cdp_tumor_loop = cdp_tumor_end2[cdp_tumor_end2_index][1];
		  if( cdp_tumor_end2[cdp_tumor_end2_index][0] == cdp_pos_in_contig_start )
		  {

		    if( cdp_tumor_end[cdp_tumor_loop] == cdp_pos_in_contig_start && cdp_tumor_type[cdp_tumor_loop] != 6 && cdp_tumor_type[cdp_tumor_loop] != 7 && cdp_tumor_type[cdp_tumor_loop] != SNV )
		    {
		      if( cdp_tumor_type[cdp_tumor_loop] == DEL )
		      {
			if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_del_rdist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			{
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_DEL_R && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			    {
			      cdp_one_base_del_r[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_del_rdist[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_del_r_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_del_r_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
			      break;
			    }
			    else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      break;
			    }
			  }
			}
		      }
		      if( cdp_tumor_type[cdp_tumor_loop] == DUP )
		      {
			if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_dup_fdist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			{
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_DUP_F && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			    {
			      cdp_one_base_dup_f[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_dup_fdist[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_dup_f_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_dup_f_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
			      break;
			    }
			    else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      break;
			    }
			  }
			}
		      }
		      if( cdp_tumor_type[cdp_tumor_loop] == INV || cdp_tumor_type[cdp_tumor_loop] == INV_F || cdp_tumor_type[cdp_tumor_loop] == INV_R )
		      {
			if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_inv_f2dist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) || cdp_one_base_inv_f2[cdp_one_base_index] == 0 )
			{
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_INV_F2 && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			    {
			      cdp_one_base_inv_f2[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_inv_f2dist[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_inv_f2_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_inv_f2_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
			      break;
			    }
			    else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      break;
			    }
			  }
			}
			if( abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_inv_r2dist[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) || cdp_one_base_inv_r2[cdp_one_base_index] == 0 )
			{
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_INV_R2 && abs(cdp_tumor_dist[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			    {
			      cdp_one_base_inv_r2[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_inv_r2dist[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_inv_r2_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_inv_r2_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
			      break;
			    }
			    else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      break;
			    }
			  }
			}
		      }

		      if( cdp_tumor_type[cdp_tumor_loop] == CTX_F )
		      {
			if( cdp_tumor_mchr[cdp_tumor_loop] != cdp_one_base_ctx_f_mchr[cdp_one_base_index] || abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_ctx_f_mpos[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			{
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_CTX_F && cdp_tumor_mchr[cdp_tumor_loop] == cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index] && abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			    {
			      cdp_one_base_ctx_f[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_f_mchr[cdp_one_base_index] = cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_f_mpos[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_f_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_f_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
			      break;
			    }
			    else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      break;
			    }
			  }
			}
		      }
		      if( cdp_tumor_type[cdp_tumor_loop] == CTX_R )
		      {
			if( cdp_tumor_mchr[cdp_tumor_loop] != cdp_one_base_ctx_r_mchr[cdp_one_base_index] || abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_ctx_r_mpos[cdp_one_base_index]) >= (g_insert_max_size - g_insert_min_size) )
			{
			  for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
			  {
			    if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_CTX_R && cdp_tumor_mchr[cdp_tumor_loop] == cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index] && abs(cdp_tumor_mpos[cdp_tumor_loop] - (int)cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index]) < (g_insert_max_size - g_insert_min_size) )
			    {
			      cdp_one_base_ctx_r[cdp_one_base_index] = cdp_one_base_other[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_r_mchr[cdp_one_base_index] = cdp_one_base_other_mchr[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_r_mpos[cdp_one_base_index] = cdp_one_base_other_dist[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_r_read_start[cdp_one_base_index] = cdp_one_base_other_read_start[cdp_tumor_loop2][cdp_one_base_index];
			      cdp_one_base_ctx_r_read_end[cdp_one_base_index] = cdp_one_base_other_read_end[cdp_tumor_loop2][cdp_one_base_index];
			      break;
			    }
			    else if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY || cdp_tumor_loop2 == g_other_len - 1 )
			    {
			      break;
			    }
			  }
			}
		      }

		      fprintf(cdp_results_file, "%s\t", g_sv_types[cdp_tumor_type[cdp_tumor_loop]]);
		      fprintf(cdp_results_file, "%s\t", cdp_chr_name);
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start[cdp_tumor_loop] + 1);
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end[cdp_tumor_loop] + 1);
		      fprintf(cdp_results_file, "%e\t", cdp_tumor_dist[cdp_tumor_loop]);

		      if( cdp_tumor_type[cdp_tumor_loop] == DEL || cdp_tumor_type[cdp_tumor_loop] == DUP || cdp_tumor_type[cdp_tumor_loop] == INV || cdp_tumor_type[cdp_tumor_loop] == INV_F || cdp_tumor_type[cdp_tumor_loop] == INV_R || cdp_tumor_type[cdp_tumor_loop] == INS )
		      {
			cdp_n1 = cdp_tumor_end_rd[cdp_tumor_loop] + cdp_one_base_rd[cdp_one_base_index];
			cdp_k1 = cdp_tumor_end_rd[cdp_tumor_loop];
			cdp_n2 = cdp_tumor_end_rd[cdp_tumor_loop];

			cdp_k2 = cdp_tumor_end_sv_evidence[cdp_tumor_loop];

			if( cdp_n1 > g_max_combinations )
			{
			  cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			  cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			  cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			  cdp_n1 = g_max_combinations;
			  if( cdp_k2 == 0 )
			  {
			    cdp_k2 = 1;
			  }
			}

			cdp_comb_div_list_len = 0;
			cdp_comb_mult_list_len = 0;
			for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			{
			  cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			  cdp_comb_div_list_len += 1;
			}
			for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			{
			  cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			  cdp_comb_mult_list_len += 1;
			}

			cdp_tumor_end_binom_cdf[cdp_tumor_loop] = 1;
			cdp_div_index = 0;
			cdp_mult_index = 0;
			for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			{
			  if( cdp_tumor_end_binom_cdf[cdp_tumor_loop] > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			  {
			    cdp_tumor_end_binom_cdf[cdp_tumor_loop] = cdp_tumor_end_binom_cdf[cdp_tumor_loop] / cdp_comb_div_list[cdp_div_index];
			    cdp_div_index += 1;
			  }
			  else
			  {
			    cdp_tumor_end_binom_cdf[cdp_tumor_loop] = cdp_tumor_end_binom_cdf[cdp_tumor_loop] * cdp_comb_mult_list[cdp_mult_index];
			    cdp_mult_index += 1;
			    if( cdp_tumor_end_binom_cdf[cdp_tumor_loop] < 1e-50 )
			    {
			      break;
			    }
			  }
			}
		      }

		      fprintf(cdp_results_file, "%e\t", cdp_tumor_start_binom_cdf[cdp_tumor_loop]);
		      fprintf(cdp_results_file, "%e\t", cdp_tumor_end_binom_cdf[cdp_tumor_loop]);
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start_conc[cdp_tumor_loop]);
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_conc[cdp_tumor_loop]);
		      fprintf(cdp_results_file, "%s\t", "END");
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_conc[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ins[cdp_one_base_index]);

		      fprintf(cdp_results_file, "%d\t", cdp_one_base_del_f[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_del_r[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_del_fdist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_del_rdist[cdp_one_base_index]);

		      fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_r[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_f[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_dup_rdist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_dup_fdist[cdp_one_base_index]);


		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f1[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r1[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f2[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r2[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_f1dist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_r1dist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_f2dist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%e\t", cdp_one_base_inv_r2dist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_munmapped_f[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_munmapped_r[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_left[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_right[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_left_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_right_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_sc_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_i[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_f[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_r[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_fdist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_rdist[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_left[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_right[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_left_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_right_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_sc_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_left[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_right[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_left_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_right_rd[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_sc_rd[cdp_one_base_index]);
		      cdp_max_sc_left = 0;
		      cdp_max_sc_right = 0;
		      for(cdp_tumor_loop2=(cdp_one_base_index-g_normal_sc_range);cdp_tumor_loop2<=(cdp_one_base_index+g_normal_sc_range);cdp_tumor_loop2++)
		      {
			if( cdp_one_base_sc_left[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_r[cdp_tumor_loop2] > cdp_max_sc_left )
			{
			  cdp_max_sc_left = cdp_one_base_sc_left[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_r[cdp_tumor_loop2];
			}
			if( cdp_one_base_sc_right[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_f[cdp_tumor_loop2] > cdp_max_sc_right )
			{
			  cdp_max_sc_right = cdp_one_base_sc_right[cdp_tumor_loop2] + cdp_one_base_indel_i[cdp_tumor_loop2] + cdp_one_base_indel_d_f[cdp_tumor_loop2];
			}
		      }
		      fprintf(cdp_results_file, "%d\t", cdp_max_sc_left);
		      fprintf(cdp_results_file, "%d\t", cdp_max_sc_right);
		      for(cdp_tumor_loop2=0;cdp_tumor_loop2<g_other_len;cdp_tumor_loop2++)
		      {
			if( cdp_one_base_other_type[cdp_tumor_loop2][cdp_one_base_index] == OTHER_EMPTY )
			{
			  fprintf(cdp_results_file, "%d\t", cdp_tumor_loop2);
			  break;
			}
			else if( cdp_tumor_loop2 == g_other_len - 1 )
			{
			  fprintf(cdp_results_file, "%d\t", (cdp_tumor_loop2+1) );
			}
		      }


		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_f[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_r[cdp_one_base_index]);
		      fprintf(cdp_results_file, "%d\t", 0); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_other_len[cdp_tumor_loop]); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start_read_start[cdp_tumor_loop] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start_read_end[cdp_tumor_loop] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_read_start[cdp_tumor_loop] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_read_end[cdp_tumor_loop] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_del_r_read_start[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_del_r_read_end[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_f_read_start[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_dup_f_read_end[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f2_read_start[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_f2_read_end[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r2_read_start[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_inv_r2_read_end[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_f_read_start[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_f_read_end[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_r_read_start[cdp_one_base_index] + 1); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_ctx_r_read_end[cdp_one_base_index] + 1); 

		      fprintf(cdp_results_file, "-\t"); 
		      fprintf(cdp_results_file, "0\t"); 
		      fprintf(cdp_results_file, "%s\t", cdp_bam_file->header->target_name[cdp_one_base_ctx_f_mchr[cdp_one_base_index]]);  
		      fprintf(cdp_results_file, "%d\t", (int) (cdp_one_base_ctx_f_mpos[cdp_one_base_index]));  
		      fprintf(cdp_results_file, "%s\t", cdp_bam_file->header->target_name[cdp_one_base_ctx_r_mchr[cdp_one_base_index]]);  
		      fprintf(cdp_results_file, "%d\t", (int) (cdp_one_base_ctx_r_mpos[cdp_one_base_index]));  

		      fprintf(cdp_results_file, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");  

		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_f_rd[cdp_one_base_index]); 
		      fprintf(cdp_results_file, "%d\t", cdp_one_base_indel_d_r_rd[cdp_one_base_index]); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start_indel[cdp_tumor_loop]); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_indel[cdp_tumor_loop]); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start_rd[cdp_tumor_loop]); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_rd[cdp_tumor_loop]); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start_sc[cdp_tumor_loop]); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_sc[cdp_tumor_loop]); 

		      cdp_tumor_prob = 1.0;
		      if( cdp_tumor_type[cdp_tumor_loop] == INDEL_DEL )
		      {
			int cdp_tumor_indel = (cdp_tumor_end_indel[cdp_tumor_loop] + cdp_one_base_indel_d_r[cdp_one_base_index])/cdp_add_factor;
			cdp_total_read_count = cdp_tumor_end_rd[cdp_tumor_loop] + cdp_one_base_indel_d_r_rd[cdp_one_base_index] + cdp_one_base_sc_left_rd[cdp_one_base_index];
			for(cdp_snv_a_loop=0;cdp_snv_a_loop<g_nucleotides;cdp_snv_a_loop++)
			{
			  cdp_total_read_count += cdp_one_base_snv[cdp_snv_a_loop][cdp_one_base_index] + cdp_one_base_snv_lowmq[cdp_snv_a_loop][cdp_one_base_index];
			}

			cdp_n1 = cdp_total_read_count;
			cdp_k1 = cdp_tumor_end_rd[cdp_tumor_loop];
			cdp_n2 = cdp_tumor_end_rd[cdp_tumor_loop];

			cdp_k2 = cdp_tumor_indel;

			if( cdp_n1 > g_max_combinations )
			{
			  cdp_k1 = cdp_k1 * g_max_combinations / cdp_n1;
			  cdp_n2 = cdp_n2 * g_max_combinations / cdp_n1;
			  cdp_k2 = cdp_k2 * g_max_combinations / cdp_n1;
			  cdp_n1 = g_max_combinations;
			  if( cdp_k2 == 0 )
			  {
			    cdp_k2 = 1;
			  }
			}

			cdp_comb_div_list_len = 0;
			cdp_comb_mult_list_len = 0;
			for(cdp_snv_a_loop=cdp_n1;cdp_snv_a_loop>cdp_n2;cdp_snv_a_loop--)
			{
			  cdp_comb_div_list[cdp_comb_div_list_len] = cdp_snv_a_loop;
			  cdp_comb_div_list_len += 1;
			}
			for(cdp_snv_a_loop=(cdp_n1-cdp_k2);cdp_snv_a_loop>(cdp_n2-cdp_k2);cdp_snv_a_loop--)
			{
			  cdp_comb_mult_list[cdp_comb_mult_list_len] = cdp_snv_a_loop;
			  cdp_comb_mult_list_len += 1;
			}

			cdp_div_index = 0;
			cdp_mult_index = 0;
			for(cdp_snv_a_loop=0;cdp_snv_a_loop<(cdp_comb_div_list_len+cdp_comb_mult_list_len);cdp_snv_a_loop++)
			{
			  if( cdp_tumor_prob > 1.0 || cdp_mult_index >= cdp_comb_mult_list_len )
			  {
			    cdp_tumor_prob = cdp_tumor_prob / cdp_comb_div_list[cdp_div_index];
			    cdp_div_index += 1;
			  }
			  else
			  {
			    cdp_tumor_prob = cdp_tumor_prob * cdp_comb_mult_list[cdp_mult_index];
			    cdp_mult_index += 1;
			    if( cdp_tumor_prob < 1e-50 )
			    {
			      break;
			    }
			  }
			}
		      }


		      fprintf(cdp_results_file, "%e", cdp_tumor_prob); 

		      fprintf(cdp_results_file, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t"); 

		      fprintf(cdp_results_file, "\t\t\t\t\t\t"); 

		      fprintf(cdp_results_file, "\t"); 
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_start_sv_evidence[cdp_tumor_loop]);
		      fprintf(cdp_results_file, "%d\t", cdp_tumor_end_sv_evidence[cdp_tumor_loop]);

		      fprintf(cdp_results_file, "\t\t\t."); 

		      fprintf(cdp_results_file, "\n");
		    }
		  }

		  if( cdp_tumor_start2[cdp_tumor_start2_index][0] == cdp_pos_in_contig_start )
		  {
		    cdp_tumor_start2_index += 1;
		  }
		  if( cdp_tumor_end2[cdp_tumor_end2_index][0] == cdp_pos_in_contig_start )
		  {
		    cdp_tumor_end2_index += 1;
		  }

		}
	      }

	      cdp_pos_in_contig_start += 1;
	    }
	    else
	    {
	      if( my_samread(cdp_bam_file, cdp_b) > 0 )
	      {
		cdp_pos = cdp_b->core.pos;
		cdp_flag = cdp_b->core.flag;
		cdp_mq = cdp_b->core.qual;
		cdp_chr = cdp_b->core.tid;
		cdp_mchr = cdp_b->core.mtid;
		cdp_mpos = cdp_b->core.mpos;
		cdp_tlen = cdp_b->core.isize;
		cdp_lseq = cdp_b->core.l_qseq;
		if( cdp_mq >= g_min_mapq )
		{
		  cdp_add = cdp_add_factor;
		}
		else
		{
		  cdp_add = cdp_add_factor_lowmq;
		}
		cdp_add_factor_double = (double) cdp_add;

		cdp_aux_pos = -1;
		cdp_aux_mq = -1;
		cdp_l_aux = bam_get_l_aux(cdp_b);

		if( (cdp_flag & BAM_FUNMAP) == 0 && (cdp_flag & BAM_FDUP) == 0 && (cdp_flag & BAM_FQCFAIL) == 0 )
		{
		  if( cdp_l_aux > 0 && cdp_l_aux < aux_str_len )
		  {
		    cdp_aux = bam_aux_get(cdp_b,"XP");
		    if( cdp_aux )
		    {
		      if( cdp_aux[0] == 'Z' )
		      {
			memmove(cdp_aux_str, cdp_aux+1, cdp_l_aux);
		      }
		      else
		      {
			memmove(cdp_aux_str, cdp_aux, cdp_l_aux + 1);
		      }
		      cdp_aux_chr = strtok((char *) cdp_aux_str, g_aux_separator);
		      cdp_aux_str_temp = strtok(NULL, g_aux_separator);
		      if( cdp_aux_str_temp[0] == '+' )
		      {
			cdp_aux_strand = 0;
		      }
		      else
		      {
			cdp_aux_strand = 1;
		      }
		      memmove(cdp_aux_str_temp, cdp_aux_str_temp+1, strlen(cdp_aux_str_temp));
		      cdp_aux_pos = atoi(cdp_aux_str_temp);
		      cdp_aux_cigar = strtok(NULL, g_aux_separator);
		      cdp_aux_str_temp = strtok(NULL, g_aux_separator);
		      cdp_aux_mq = atoi(cdp_aux_str_temp);
		    }
		    else
		    {
		      cdp_aux = bam_aux_get(cdp_b,"SA");
		      if( cdp_aux )
		      {
			if( cdp_aux[0] == 'Z' )
			{
			  memmove(cdp_aux_str, cdp_aux+1, cdp_l_aux);
			}
			else
			{
			  memmove(cdp_aux_str, cdp_aux, cdp_l_aux+1);
			}
			cdp_aux_chr = strtok((char *) cdp_aux_str, g_aux_separator);
			cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			cdp_aux_pos = atoi(cdp_aux_str_temp);
			cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			if( cdp_aux_str_temp[0] == '+' )
			{
			  cdp_aux_strand = 0;
			}
			else
			{
			  cdp_aux_strand = 1;
			}
			cdp_aux_cigar = strtok(NULL, g_aux_separator);
			cdp_aux_str_temp = strtok(NULL, g_aux_separator);
			cdp_aux_mq = atoi(cdp_aux_str_temp);

		      }
		    }
		  }
		}


		if( cdp_chr != cdp_chr_match )
		{
		  cdp_begin = 2;
		}
	      }
	      else
	      {
		cdp_begin = 2;
	      }
	    }

	  }

	}
	else if( cdp_begin == 1 )
	{
	  cdp_begin = 2;
	}
	  
	  }

#ifdef DO_OTHERLEN_STDEV
      qsort(cdp_other_len_total_list, cdp_other_len_count, sizeof(int), cmpfunc);

      cdp_other_len_stdev[0] = cdp_other_len_total_list[(cdp_other_len_count-1)-(cdp_other_len_count-1)/44];  
      cdp_other_len_stdev[1] = cdp_other_len_total_list[(cdp_other_len_count-1)-(cdp_other_len_count-1)/370];  
      cdp_other_len_stdev[2] = cdp_other_len_total_list[(cdp_other_len_count-1)-(cdp_other_len_count-1)/15787];  
      cdp_other_len_stdev[3] = cdp_other_len_total_list[(cdp_other_len_count-1)-(cdp_other_len_count-1)/1000];  
#endif
#ifdef DO_PRINT
      printf("chr, other_len_stdev[0] %s %d\n", cdp_chr_name, cdp_other_len_stdev[0]);
      printf("chr, other_len_stdev[1] %s %d\n", cdp_chr_name, cdp_other_len_stdev[1]);
      printf("chr, other_len_stdev[2] %s %d\n", cdp_chr_name, cdp_other_len_stdev[2]);
      printf("chr, other_len_stdev[3] %s %d\n", cdp_chr_name, cdp_other_len_stdev[3]);
#endif

      

#ifdef DO_TIMING
      start_t = rdtsc();
#endif
      

      for(cdp_a_loop=caf_last_snv_group_pos;cdp_a_loop<cdp_pos_in_contig_start - cdp_one_base_index;cdp_a_loop++)
      {
	if( cdp_chr_fasta[cdp_a_loop] != 'N' && cdp_chr_fasta[cdp_a_loop] != 'n' )
	{
	  cdp_snv_read_count_total += (long)caf_rd_rd_list[cdp_a_loop] + (long)caf_rd_low_mq_rd_list[cdp_a_loop];
	  cdp_snv_base_total += 1;
	}
      }
      cdp_snv_ave_rd = (double)cdp_snv_read_count_total / (double)cdp_snv_base_total;
      caf_last_snv_group_pos = cdp_pos_in_contig_start - cdp_one_base_index;
      if( g_internal == 1 )
      {
	printf("3: chr, snv rd %s, %e\n", cdp_chr_name, cdp_snv_ave_rd);
      }
      if( g_vcf == 1 )
      {
	for(cdp_a_loop=0;cdp_a_loop<cdp_snv_list_index;cdp_a_loop++)
	{
	  if( (cdp_snv_base_total > 0 && cdp_snv_read_count_all_list[cdp_a_loop] <= round(g_snv_rd_min_factor * cdp_snv_ave_rd)) || cdp_snv_ratio_list[cdp_a_loop] >= g_high_cov_min_snv_ratio )
	  {
	    cdp_snv_cn = round(cdp_snv_ratio_list[cdp_a_loop] * g_ploidy);
	    if( cdp_snv_cn == 0 )
	    {
	      cdp_snv_cn = 1;
	    }
	    for(cdp_b_loop=0;cdp_b_loop<g_ploidy;cdp_b_loop++)
	    {
	      if( cdp_b_loop < cdp_snv_cn )
	      {
		cdp_snv_gt_string[2*cdp_b_loop] = '1';
	      }
	      else
	      {
		cdp_snv_gt_string[2*cdp_b_loop] = '0';
	      }
	      if( cdp_b_loop < g_ploidy -1 )
	      {
		cdp_snv_gt_string[2*cdp_b_loop+1] = '/';
	      }
	      else
	      {
		cdp_snv_gt_string[2*cdp_b_loop+1] = '\0';
	      }
	    }


		if (cdp_snv_start_binom_cdf_list[cdp_a_loop] != 0)
		{
			int qual_entry = (int) -10 * log10(cdp_snv_start_binom_cdf_list[cdp_a_loop]);
			fprintf(cdp_results_file_snv, "%s\t%d\t.\t%c\t%c\t%d\t.\t.\tGT:PR:AF:AMQ:CMQ:GMQ:TMQ:ALMQ:CLMQ:GLMQ:TLMQ:BQ:MQ:PIR:FS\t%s:%.2e:%.2e", cdp_chr_name, cdp_snv_pos_list[cdp_a_loop] + 1, cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop]], g_dna[cdp_snv_base_list[cdp_a_loop]], qual_entry, cdp_snv_gt_string, cdp_snv_start_binom_cdf_list[cdp_a_loop], cdp_snv_ratio_list[cdp_a_loop]);
		
		} else {
			fprintf(cdp_results_file_snv, "%s\t%d\t.\t%c\t%c\t%s\t.\t.\tGT:PR:AF:AMQ:CMQ:GMQ:TMQ:ALMQ:CLMQ:GLMQ:TLMQ:BQ:MQ:PIR:FS\t%s:%.2e:%.2e", cdp_chr_name, cdp_snv_pos_list[cdp_a_loop] + 1, cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop]], g_dna[cdp_snv_base_list[cdp_a_loop]], infinity_qual_entry, cdp_snv_gt_string, cdp_snv_start_binom_cdf_list[cdp_a_loop], cdp_snv_ratio_list[cdp_a_loop]);
		}

	    for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
	    {
	      fprintf(cdp_results_file_snv, ":%d", cdp_snv_list[cdp_b_loop][cdp_a_loop]);
	    }
	    for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
	    {
	      fprintf(cdp_results_file_snv, ":%d", cdp_snv_lowmq_list[cdp_b_loop][cdp_a_loop]);
	    }
	    fprintf(cdp_results_file_snv, ":%.2f", (double)cdp_snv_bq_all_list[cdp_a_loop]/(double)cdp_snv_read_count_all_list[cdp_a_loop]);
	    fprintf(cdp_results_file_snv, ":%.2f", (double)cdp_snv_mq_all_list[cdp_a_loop]/(double)cdp_snv_read_count_all_list[cdp_a_loop]);
	    fprintf(cdp_results_file_snv, ":%.2f:%.2f\n", (double)cdp_snv_pos_in_read_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], (double)cdp_snv_fstrand_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]);
	  }
	}
      }
      else
      {
	for(cdp_a_loop=0;cdp_a_loop<cdp_snv_list_index;cdp_a_loop++)
	{
	  if( (cdp_snv_base_total > 0 && cdp_snv_read_count_all_list[cdp_a_loop] <= round(g_snv_rd_min_factor * cdp_snv_ave_rd)) || cdp_snv_ratio_list[cdp_a_loop] >= g_high_cov_min_snv_ratio )
	  {
	    fprintf(cdp_results_file, "SNV\t%s\t%d\t%c\t%e\t%d\t%d", cdp_chr_name, cdp_snv_pos_list[cdp_a_loop] + 1, g_dna[cdp_snv_base_list[cdp_a_loop]], cdp_snv_ratio_list[cdp_a_loop], cdp_snv_rd_list[cdp_a_loop], cdp_snv_low_mq_rd_list[cdp_a_loop]);
	    for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
	    {
	      fprintf(cdp_results_file, "\t%d", cdp_snv_list[cdp_b_loop][cdp_a_loop]);
	    }
	    for(cdp_b_loop=0;cdp_b_loop<g_nucleotides;cdp_b_loop++)
	    {
	      fprintf(cdp_results_file, "\t%d", cdp_snv_lowmq_list[cdp_b_loop][cdp_a_loop]);
	    }
	    fprintf(cdp_results_file, "\t%d\t%d\t%d\t%d\t%d\t%d\t%d", cdp_snv_bq_list[cdp_a_loop], cdp_snv_bq_all_list[cdp_a_loop], cdp_snv_mq_list[cdp_a_loop], cdp_snv_mq_all_list[cdp_a_loop], cdp_snv_bq_read_count_list[cdp_a_loop], cdp_snv_mq_read_count_list[cdp_a_loop], cdp_snv_read_count_all_list[cdp_a_loop]);
	    if( cdp_snv_pos_list[cdp_a_loop] > 0 && cdp_snv_pos_list[cdp_a_loop] < cdp_chr_fasta_len - 1 )
	    {
	      fprintf(cdp_results_file, "\t%.2f\t%.2f\t%c%c%c", (double)cdp_snv_pos_in_read_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], (double)cdp_snv_fstrand_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] - 1], cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop]], cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] + 1]);
	    }
	    else
	    {
	      fprintf(cdp_results_file, "\t%.2f\t%.2f\t%c%c%c", (double)cdp_snv_pos_in_read_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], (double)cdp_snv_fstrand_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop]/(double)cdp_snv_list[cdp_snv_base_list[cdp_a_loop]][cdp_a_loop], '.','.','.');
	    }
	    fprintf(cdp_results_file, "\t");
	    for(cdp_b_loop=0;cdp_b_loop<cdp_lseq;cdp_b_loop++)
	    {
	      if( cdp_snv_pos_list[cdp_a_loop] - cdp_lseq + 1 + cdp_b_loop < 0 )
	      {
		fprintf(cdp_results_file, "%c", 'N');
	      }
	      else
	      {
		fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] - cdp_lseq + 1 + cdp_b_loop]);
	      }
	    }
	    for(cdp_b_loop=0;cdp_b_loop<cdp_lseq-1;cdp_b_loop++)
	    {
	      if( cdp_snv_pos_list[cdp_a_loop] + cdp_lseq - 1 - cdp_b_loop >=  cdp_chr_fasta_len - 1 )
	      {
		fprintf(cdp_results_file, "%c", 'N');
	      }
	      else
	      {
		fprintf(cdp_results_file, "%c", cdp_chr_fasta[cdp_snv_pos_list[cdp_a_loop] + cdp_lseq - 1 - cdp_b_loop]);
	      }
	    }

	    fprintf(cdp_results_file, "\t%e\t%e", cdp_snv_start_binom_cdf_list[cdp_a_loop], cdp_snv_start_hez_binom_cdf_list[cdp_a_loop]);

	    fprintf(cdp_results_file, "\n");
	  }
	}
      }
      cdp_snv_list_index = 0;
      


      
      cdp_dup_list2_index = 0;
      cdp_dup_begin = 0;
      cdp_first_start = 0;
      cdp_last_start = 0;
      cdp_first_end = 0;
      cdp_last_end = 0;
      cdp_first_dist = 0;
      cdp_last_dist = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_dup_list_index;cdp_a_loop++)
      {
	if( cdp_dup_begin == 1 )
	{
	  if( cdp_dup_list_start[cdp_a_loop] > cdp_last_start + g_insert_max_size - 2*g_lseq )
	  {
	    cdp_dup_begin = 0;
	    cdp_first_start = 0;
	    cdp_last_start = 0;
	    cdp_first_end = 0;
	    cdp_last_end = 0;
	    cdp_first_dist = 0;
	    cdp_last_dist = 0;
	  }
	  else
	  {
	    cdp_max_binom_cdf = cdp_dup_list_start_binom_cdf[cdp_a_loop];
	    if( cdp_dup_list_end_binom_cdf[cdp_a_loop] > cdp_max_binom_cdf )
	    {
	      cdp_max_binom_cdf = cdp_dup_list_end_binom_cdf[cdp_a_loop];
	    }
	    cdp_max_binom_cdf2 = cdp_dup_list2_start_binom_cdf[cdp_dup_list2_index - 1];
	    if( cdp_dup_list2_end_binom_cdf[cdp_dup_list2_index - 1] > cdp_max_binom_cdf2 )
	    {
	      cdp_max_binom_cdf2 = cdp_dup_list2_end_binom_cdf[cdp_dup_list2_index - 1];
	    }
	    if( cdp_max_binom_cdf <= cdp_max_binom_cdf2 && cdp_dup_list_start[cdp_a_loop] >= 0 && cdp_dup_list_end[cdp_a_loop] >= 0 && cdp_dup_list2_start_dup_r[cdp_dup_list2_index - 1] <= cdp_dup_list_start_dup_r[cdp_a_loop] && cdp_dup_list2_end_dup_f[cdp_dup_list2_index - 1] <= cdp_dup_list_end_dup_f[cdp_a_loop] )
	    {
	      if( cdp_dup_list_start_binom_cdf[cdp_a_loop] == cdp_dup_list2_start_binom_cdf[cdp_dup_list2_index - 1] && cdp_dup_list_end_binom_cdf[cdp_a_loop] == cdp_dup_list2_end_binom_cdf[cdp_dup_list2_index - 1] )
	      {
		if( (cdp_dup_list2_start_dup_r[cdp_dup_list2_index - 1] < cdp_dup_list_start_dup_r[cdp_a_loop] && cdp_dup_list2_end_dup_f[cdp_dup_list2_index - 1] <= cdp_dup_list_end_dup_f[cdp_a_loop]) || (cdp_dup_list2_start_dup_r[cdp_dup_list2_index - 1] <= cdp_dup_list_start_dup_r[cdp_a_loop] && cdp_dup_list2_end_dup_f[cdp_dup_list2_index - 1] < cdp_dup_list_end_dup_f[cdp_a_loop]) )
		{
		  cdp_first_start = cdp_dup_list_start[cdp_a_loop];
		  cdp_last_start = cdp_dup_list_start[cdp_a_loop];
		  cdp_first_end = cdp_dup_list_end[cdp_a_loop];
		  cdp_last_end = cdp_dup_list_end[cdp_a_loop];
		  cdp_first_dist = cdp_dup_list_dist[cdp_a_loop];
		  cdp_last_dist = cdp_dup_list_dist[cdp_a_loop];
		  cdp_dup_list2_start[cdp_dup_list2_index - 1] = cdp_dup_list_start[cdp_a_loop];
		  cdp_dup_list2_end[cdp_dup_list2_index - 1] = cdp_dup_list_end[cdp_a_loop];
		  cdp_dup_list2_start_dup_r[cdp_dup_list2_index - 1] = cdp_dup_list_start_dup_r[cdp_a_loop];
		  cdp_dup_list2_end_dup_f[cdp_dup_list2_index - 1] = cdp_dup_list_end_dup_f[cdp_a_loop];
		  cdp_dup_list2_dist[cdp_dup_list2_index - 1] = cdp_dup_list_dist[cdp_a_loop];
		  cdp_dup_list2_start_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_start_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_start_hez_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_end_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_end_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_end_hez_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_start_rd[cdp_dup_list2_index - 1] = cdp_dup_list_start_rd[cdp_a_loop];
		  cdp_dup_list2_end_rd[cdp_dup_list2_index - 1] = cdp_dup_list_end_rd[cdp_a_loop];
		  cdp_dup_list2_start_conc[cdp_dup_list2_index - 1] = cdp_dup_list_start_conc[cdp_a_loop];
		  cdp_dup_list2_end_conc[cdp_dup_list2_index - 1] = cdp_dup_list_end_conc[cdp_a_loop];
		  cdp_dup_list2_start_other_len[cdp_dup_list2_index - 1] = cdp_dup_list_start_other_len[cdp_a_loop];
		  cdp_dup_list2_end_other_len[cdp_dup_list2_index - 1] = cdp_dup_list_end_other_len[cdp_a_loop];
		  cdp_dup_list2_start_read_start[cdp_dup_list2_index - 1] = cdp_dup_list_start_read_start[cdp_a_loop];
		  cdp_dup_list2_start_read_end[cdp_dup_list2_index - 1] = cdp_dup_list_start_read_end[cdp_a_loop];
		  cdp_dup_list2_end_read_start[cdp_dup_list2_index - 1] = cdp_dup_list_end_read_start[cdp_a_loop];
		  cdp_dup_list2_end_read_end[cdp_dup_list2_index - 1] = cdp_dup_list_end_read_end[cdp_a_loop];
		}
		else if( cdp_dup_list2_start_dup_r[cdp_dup_list2_index - 1] == cdp_dup_list_start_dup_r[cdp_a_loop] && cdp_dup_list2_end_dup_f[cdp_dup_list2_index - 1] == cdp_dup_list_end_dup_f[cdp_a_loop] )
		{
		  cdp_last_start = cdp_dup_list_start[cdp_a_loop];
		  cdp_last_end = cdp_dup_list_end[cdp_a_loop];
		  cdp_last_dist = cdp_dup_list_dist[cdp_a_loop];
		  cdp_dup_list2_start[cdp_dup_list2_index - 1] = (cdp_first_start + cdp_last_start)/2;
		  cdp_dup_list2_end[cdp_dup_list2_index - 1] = (cdp_first_end + cdp_last_end)/2;
		  cdp_dup_list2_dist[cdp_dup_list2_index - 1] = (cdp_first_dist + cdp_last_dist)/(double)(2.0);
		  cdp_dup_list2_start_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_start_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_start_hez_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_end_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_end_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_end_hez_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_dup_list2_start_rd[cdp_dup_list2_index - 1] = cdp_dup_list_start_rd[cdp_a_loop];
		  cdp_dup_list2_end_rd[cdp_dup_list2_index - 1] = cdp_dup_list_end_rd[cdp_a_loop];
		  cdp_dup_list2_start_conc[cdp_dup_list2_index - 1] = cdp_dup_list_start_conc[cdp_a_loop];
		  cdp_dup_list2_end_conc[cdp_dup_list2_index - 1] = cdp_dup_list_end_conc[cdp_a_loop];
		  cdp_dup_list2_start_other_len[cdp_dup_list2_index - 1] = cdp_dup_list_start_other_len[cdp_a_loop];
		  cdp_dup_list2_end_other_len[cdp_dup_list2_index - 1] = cdp_dup_list_end_other_len[cdp_a_loop];
		  cdp_dup_list2_start_read_start[cdp_dup_list2_index - 1] = cdp_dup_list_start_read_start[cdp_a_loop];
		  cdp_dup_list2_start_read_end[cdp_dup_list2_index - 1] = cdp_dup_list_start_read_end[cdp_a_loop];
		  cdp_dup_list2_end_read_start[cdp_dup_list2_index - 1] = cdp_dup_list_end_read_start[cdp_a_loop];
		  cdp_dup_list2_end_read_end[cdp_dup_list2_index - 1] = cdp_dup_list_end_read_end[cdp_a_loop];
		}
	      }
	      else
	      {
		cdp_first_start = cdp_dup_list_start[cdp_a_loop];
		cdp_last_start = cdp_dup_list_start[cdp_a_loop];
		cdp_first_end = cdp_dup_list_end[cdp_a_loop];
		cdp_last_end = cdp_dup_list_end[cdp_a_loop];
		cdp_first_dist = cdp_dup_list_dist[cdp_a_loop];
		cdp_last_dist = cdp_dup_list_dist[cdp_a_loop];
		cdp_dup_list2_start[cdp_dup_list2_index - 1] = cdp_dup_list_start[cdp_a_loop];
		cdp_dup_list2_end[cdp_dup_list2_index - 1] = cdp_dup_list_end[cdp_a_loop];
		cdp_dup_list2_start_dup_r[cdp_dup_list2_index - 1] = cdp_dup_list_start_dup_r[cdp_a_loop];
		cdp_dup_list2_end_dup_f[cdp_dup_list2_index - 1] = cdp_dup_list_end_dup_f[cdp_a_loop];
		cdp_dup_list2_dist[cdp_dup_list2_index - 1] = cdp_dup_list_dist[cdp_a_loop];
		cdp_dup_list2_start_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_start_binom_cdf[cdp_a_loop];
		cdp_dup_list2_start_hez_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_start_hez_binom_cdf[cdp_a_loop];
		cdp_dup_list2_end_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_end_binom_cdf[cdp_a_loop];
		cdp_dup_list2_end_hez_binom_cdf[cdp_dup_list2_index - 1] = cdp_dup_list_end_hez_binom_cdf[cdp_a_loop];
		cdp_dup_list2_start_rd[cdp_dup_list2_index - 1] = cdp_dup_list_start_rd[cdp_a_loop];
		cdp_dup_list2_end_rd[cdp_dup_list2_index - 1] = cdp_dup_list_end_rd[cdp_a_loop];
		cdp_dup_list2_start_conc[cdp_dup_list2_index - 1] = cdp_dup_list_start_conc[cdp_a_loop];
		cdp_dup_list2_end_conc[cdp_dup_list2_index - 1] = cdp_dup_list_end_conc[cdp_a_loop];
		cdp_dup_list2_start_other_len[cdp_dup_list2_index - 1] = cdp_dup_list_start_other_len[cdp_a_loop];
		cdp_dup_list2_end_other_len[cdp_dup_list2_index - 1] = cdp_dup_list_end_other_len[cdp_a_loop];
		cdp_dup_list2_start_read_start[cdp_dup_list2_index - 1] = cdp_dup_list_start_read_start[cdp_a_loop];
		cdp_dup_list2_start_read_end[cdp_dup_list2_index - 1] = cdp_dup_list_start_read_end[cdp_a_loop];
		cdp_dup_list2_end_read_start[cdp_dup_list2_index - 1] = cdp_dup_list_end_read_start[cdp_a_loop];
		cdp_dup_list2_end_read_end[cdp_dup_list2_index - 1] = cdp_dup_list_end_read_end[cdp_a_loop];
	      }
	    }
	  }
	}
	if( cdp_dup_begin == 0 )
	{
	  if( cdp_dup_list_start[cdp_a_loop] >= 0 && cdp_dup_list_end[cdp_a_loop] >= 0 )
	  {
	    if( cdp_dup_list2_index < g_sv_list2_len - 1 )
	    {
	      cdp_dup_begin = 1;
	      cdp_first_start = cdp_dup_list_start[cdp_a_loop];
	      cdp_last_start = cdp_dup_list_start[cdp_a_loop];
	      cdp_first_end = cdp_dup_list_end[cdp_a_loop];
	      cdp_last_end = cdp_dup_list_end[cdp_a_loop];
	      cdp_first_dist = cdp_dup_list_dist[cdp_a_loop];
	      cdp_last_dist = cdp_dup_list_dist[cdp_a_loop];
	      cdp_dup_list2_start[cdp_dup_list2_index] = cdp_dup_list_start[cdp_a_loop];
	      cdp_dup_list2_end[cdp_dup_list2_index] = cdp_dup_list_end[cdp_a_loop];
	      cdp_dup_list2_start_dup_r[cdp_dup_list2_index] = cdp_dup_list_start_dup_r[cdp_a_loop];
	      cdp_dup_list2_end_dup_f[cdp_dup_list2_index] = cdp_dup_list_end_dup_f[cdp_a_loop];
	      cdp_dup_list2_dist[cdp_dup_list2_index] = cdp_dup_list_dist[cdp_a_loop];
	      cdp_dup_list2_start_binom_cdf[cdp_dup_list2_index] = cdp_dup_list_start_binom_cdf[cdp_a_loop];
	      cdp_dup_list2_start_hez_binom_cdf[cdp_dup_list2_index] = cdp_dup_list_start_hez_binom_cdf[cdp_a_loop];
	      cdp_dup_list2_end_binom_cdf[cdp_dup_list2_index] = cdp_dup_list_end_binom_cdf[cdp_a_loop];
	      cdp_dup_list2_end_hez_binom_cdf[cdp_dup_list2_index] = cdp_dup_list_end_hez_binom_cdf[cdp_a_loop];
	      cdp_dup_list2_start_rd[cdp_dup_list2_index] = cdp_dup_list_start_rd[cdp_a_loop];
	      cdp_dup_list2_end_rd[cdp_dup_list2_index] = cdp_dup_list_end_rd[cdp_a_loop];
	      cdp_dup_list2_start_conc[cdp_dup_list2_index] = cdp_dup_list_start_conc[cdp_a_loop];
	      cdp_dup_list2_end_conc[cdp_dup_list2_index] = cdp_dup_list_end_conc[cdp_a_loop];
	      cdp_dup_list2_start_other_len[cdp_dup_list2_index] = cdp_dup_list_start_other_len[cdp_a_loop];
	      cdp_dup_list2_end_other_len[cdp_dup_list2_index] = cdp_dup_list_end_other_len[cdp_a_loop];
	      cdp_dup_list2_start_read_start[cdp_dup_list2_index] = cdp_dup_list_start_read_start[cdp_a_loop];
	      cdp_dup_list2_start_read_end[cdp_dup_list2_index] = cdp_dup_list_start_read_end[cdp_a_loop];
	      cdp_dup_list2_end_read_start[cdp_dup_list2_index] = cdp_dup_list_end_read_start[cdp_a_loop];
	      cdp_dup_list2_end_read_end[cdp_dup_list2_index] = cdp_dup_list_end_read_end[cdp_a_loop];
	      cdp_dup_list2_index += 1;
	    }
	    else
	    {
	      g_sv_list2_len_over = 1;
	    }

	  }
	}
      }

      for(cdp_a_loop=0;cdp_a_loop<cdp_dup_list2_index;cdp_a_loop++)
      {
	if( (cdp_dup_list2_start_binom_cdf[cdp_a_loop] <= g_pval_threshold || cdp_dup_list2_start_hez_binom_cdf[cdp_a_loop] <= g_pval_threshold) && (cdp_dup_list2_end_binom_cdf[cdp_a_loop] <= g_pval_threshold || cdp_dup_list2_end_hez_binom_cdf[cdp_a_loop] <= g_pval_threshold) && (double) cdp_dup_list2_start_dup_r[cdp_a_loop] / (double) cdp_dup_list2_start_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor && (double) cdp_dup_list2_end_dup_f[cdp_a_loop] / (double) cdp_dup_list2_end_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor )
	{
	  if( g_vcf == 1 )
	  {
	   
		float p_value_dup = cdp_dup_list2_end_binom_cdf[cdp_a_loop];
		if (cdp_dup_list2_start_binom_cdf[cdp_a_loop] > cdp_dup_list2_end_binom_cdf[cdp_a_loop]) {
			p_value_dup = cdp_dup_list2_start_binom_cdf[cdp_a_loop];
		}
		if (p_value_dup != 0)
		{
			int qual_entry = (int) -10 * log10(p_value_dup);
			vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
			vcf_sv_data_list[sort_sv_count].pos = cdp_dup_list2_start[cdp_a_loop];
			sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%d\t.\tSVTYPE=DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_dup_list2_start[cdp_a_loop] - 1]), qual_entry, cdp_dup_list2_end[cdp_a_loop] + 1, unknown_gt_entry, cdp_dup_list2_start_binom_cdf[cdp_a_loop], cdp_dup_list2_end_binom_cdf[cdp_a_loop], (double)cdp_dup_list2_start_dup_r[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_dup_list2_end_dup_f[cdp_a_loop]/(double)cdp_add_factor, cdp_dup_list2_start_rd[cdp_a_loop], cdp_dup_list2_end_rd[cdp_a_loop], cdp_dup_list2_start_conc[cdp_a_loop], cdp_dup_list2_end_conc[cdp_a_loop], cdp_dup_list2_start_other_len[cdp_a_loop], cdp_dup_list2_end_other_len[cdp_a_loop], cdp_dup_list2_start_read_start[cdp_a_loop] + 1, cdp_dup_list2_start_read_end[cdp_a_loop] + 1, cdp_dup_list2_end_read_start[cdp_a_loop] + 1, cdp_dup_list2_end_read_end[cdp_a_loop] + 1);
			sort_sv_count += 1;
		} else {
			vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
			vcf_sv_data_list[sort_sv_count].pos = cdp_dup_list2_start[cdp_a_loop];
			sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%s\t.\tSVTYPE=DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_dup_list2_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_dup_list2_end[cdp_a_loop] + 1, unknown_gt_entry, cdp_dup_list2_start_binom_cdf[cdp_a_loop], cdp_dup_list2_end_binom_cdf[cdp_a_loop], (double)cdp_dup_list2_start_dup_r[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_dup_list2_end_dup_f[cdp_a_loop]/(double)cdp_add_factor, cdp_dup_list2_start_rd[cdp_a_loop], cdp_dup_list2_end_rd[cdp_a_loop], cdp_dup_list2_start_conc[cdp_a_loop], cdp_dup_list2_end_conc[cdp_a_loop], cdp_dup_list2_start_other_len[cdp_a_loop], cdp_dup_list2_end_other_len[cdp_a_loop], cdp_dup_list2_start_read_start[cdp_a_loop] + 1, cdp_dup_list2_start_read_end[cdp_a_loop] + 1, cdp_dup_list2_end_read_start[cdp_a_loop] + 1, cdp_dup_list2_end_read_end[cdp_a_loop] + 1);
			sort_sv_count += 1;
		}
	  }
	  else
	  {
	    fprintf(cdp_results_file, "DUP\t%s\t%d\t%d\t%6.2f\t%e\t%e\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%e\t%e\n", cdp_chr_name, cdp_dup_list2_start[cdp_a_loop] + 1, cdp_dup_list2_end[cdp_a_loop] + 1, cdp_dup_list2_dist[cdp_a_loop], cdp_dup_list2_start_binom_cdf[cdp_a_loop], cdp_dup_list2_end_binom_cdf[cdp_a_loop], cdp_dup_list2_start_dup_r[cdp_a_loop], cdp_dup_list2_end_dup_f[cdp_a_loop], cdp_dup_list2_start_rd[cdp_a_loop], cdp_dup_list2_end_rd[cdp_a_loop], cdp_dup_list2_start_conc[cdp_a_loop], cdp_dup_list2_end_conc[cdp_a_loop], cdp_dup_list2_start_other_len[cdp_a_loop], cdp_dup_list2_end_other_len[cdp_a_loop], cdp_dup_list2_start_read_start[cdp_a_loop] + 1, cdp_dup_list2_start_read_end[cdp_a_loop] + 1, cdp_dup_list2_end_read_start[cdp_a_loop] + 1, cdp_dup_list2_end_read_end[cdp_a_loop] + 1, cdp_dup_list2_start_hez_binom_cdf[cdp_a_loop], cdp_dup_list2_end_hez_binom_cdf[cdp_a_loop]);
	  }
	}
      }
     


      
      cdp_del_list2_index = 0;
      cdp_del_begin = 0;
      cdp_first_start = 0;
      cdp_last_start = 0;
      cdp_first_end = 0;
      cdp_last_end = 0;
      cdp_first_dist = 0;
      cdp_last_dist = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_del_list_index;cdp_a_loop++)
      {
	if( cdp_del_begin == 1 )
	{
	  if( cdp_del_list_start[cdp_a_loop] > cdp_last_start + g_insert_max_size - 2*g_lseq )
	  {
	    cdp_del_begin = 0;
	    cdp_first_start = 0;
	    cdp_last_start = 0;
	    cdp_first_end = 0;
	    cdp_last_end = 0;
	    cdp_first_dist = 0;
	    cdp_last_dist = 0;
	  }
	  else
	  {
	    cdp_max_binom_cdf = cdp_del_list_start_binom_cdf[cdp_a_loop];
	    if( cdp_del_list_end_binom_cdf[cdp_a_loop] > cdp_max_binom_cdf )
	    {
	      cdp_max_binom_cdf = cdp_del_list_end_binom_cdf[cdp_a_loop];
	    }
	    cdp_max_binom_cdf2 = cdp_del_list2_start_binom_cdf[cdp_del_list2_index - 1];
	    if( cdp_del_list2_end_binom_cdf[cdp_del_list2_index - 1] > cdp_max_binom_cdf2 )
	    {
	      cdp_max_binom_cdf2 = cdp_del_list2_end_binom_cdf[cdp_del_list2_index - 1];
	    }
	    if( cdp_max_binom_cdf <= cdp_max_binom_cdf2 && cdp_del_list_start[cdp_a_loop] >= 0 && cdp_del_list_end[cdp_a_loop] >= 0 && cdp_del_list2_start_del_f[cdp_del_list2_index - 1] <= cdp_del_list_start_del_f[cdp_a_loop] && cdp_del_list2_end_del_r[cdp_del_list2_index - 1] <= cdp_del_list_end_del_r[cdp_a_loop] )
	    {
	      if( cdp_del_list_start_binom_cdf[cdp_a_loop] == cdp_del_list2_start_binom_cdf[cdp_del_list2_index - 1] && cdp_del_list_end_binom_cdf[cdp_a_loop] == cdp_del_list2_end_binom_cdf[cdp_del_list2_index - 1] )
	      {
		if( (cdp_del_list2_start_del_f[cdp_del_list2_index - 1] < cdp_del_list_start_del_f[cdp_a_loop] && cdp_del_list2_end_del_r[cdp_del_list2_index - 1] <= cdp_del_list_end_del_r[cdp_a_loop]) || (cdp_del_list2_start_del_f[cdp_del_list2_index - 1] <= cdp_del_list_start_del_f[cdp_a_loop] && cdp_del_list2_end_del_r[cdp_del_list2_index - 1] < cdp_del_list_end_del_r[cdp_a_loop]) )
		{
		  cdp_first_start = cdp_del_list_start[cdp_a_loop];
		  cdp_last_start = cdp_del_list_start[cdp_a_loop];
		  cdp_first_end = cdp_del_list_end[cdp_a_loop];
		  cdp_last_end = cdp_del_list_end[cdp_a_loop];
		  cdp_first_dist = cdp_del_list_dist[cdp_a_loop];
		  cdp_last_dist = cdp_del_list_dist[cdp_a_loop];
		  cdp_del_list2_start[cdp_del_list2_index - 1] = cdp_del_list_start[cdp_a_loop];
		  cdp_del_list2_end[cdp_del_list2_index - 1] = cdp_del_list_end[cdp_a_loop];
		  cdp_del_list2_start_del_f[cdp_del_list2_index - 1] = cdp_del_list_start_del_f[cdp_a_loop];
		  cdp_del_list2_end_del_r[cdp_del_list2_index - 1] = cdp_del_list_end_del_r[cdp_a_loop];
		  cdp_del_list2_dist[cdp_del_list2_index - 1] = cdp_del_list_dist[cdp_a_loop];
		  cdp_del_list2_start_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_start_binom_cdf[cdp_a_loop];
		  cdp_del_list2_start_hez_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_del_list2_end_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_end_binom_cdf[cdp_a_loop];
		  cdp_del_list2_end_hez_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_del_list2_start_rd[cdp_del_list2_index - 1] = cdp_del_list_start_rd[cdp_a_loop];
		  cdp_del_list2_end_rd[cdp_del_list2_index - 1] = cdp_del_list_end_rd[cdp_a_loop];
		  cdp_del_list2_start_conc[cdp_del_list2_index - 1] = cdp_del_list_start_conc[cdp_a_loop];
		  cdp_del_list2_end_conc[cdp_del_list2_index - 1] = cdp_del_list_end_conc[cdp_a_loop];
		  cdp_del_list2_start_other_len[cdp_del_list2_index - 1] = cdp_del_list_start_other_len[cdp_a_loop];
		  cdp_del_list2_end_other_len[cdp_del_list2_index - 1] = cdp_del_list_end_other_len[cdp_a_loop];
		  cdp_del_list2_start_read_start[cdp_del_list2_index - 1] = cdp_del_list_start_read_start[cdp_a_loop];
		  cdp_del_list2_start_read_end[cdp_del_list2_index - 1] = cdp_del_list_start_read_end[cdp_a_loop];
		  cdp_del_list2_end_read_start[cdp_del_list2_index - 1] = cdp_del_list_end_read_start[cdp_a_loop];
		  cdp_del_list2_end_read_end[cdp_del_list2_index - 1] = cdp_del_list_end_read_end[cdp_a_loop];
		}
		else if( cdp_del_list2_start_del_f[cdp_del_list2_index - 1] == cdp_del_list_start_del_f[cdp_a_loop] && cdp_del_list2_end_del_r[cdp_del_list2_index - 1] == cdp_del_list_end_del_r[cdp_a_loop] )
		{
		  cdp_last_start = cdp_del_list_start[cdp_a_loop];
		  cdp_last_end = cdp_del_list_end[cdp_a_loop];
		  cdp_last_dist = cdp_del_list_dist[cdp_a_loop];
		  cdp_del_list2_start[cdp_del_list2_index - 1] = (cdp_first_start + cdp_last_start)/2;
		  cdp_del_list2_end[cdp_del_list2_index - 1] = (cdp_first_end + cdp_last_end)/2;
		  cdp_del_list2_dist[cdp_del_list2_index - 1] = (cdp_first_dist + cdp_last_dist)/(double)(2.0);
		  cdp_del_list2_start_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_start_binom_cdf[cdp_a_loop];
		  cdp_del_list2_start_hez_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_del_list2_end_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_end_binom_cdf[cdp_a_loop];
		  cdp_del_list2_end_hez_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_del_list2_start_rd[cdp_del_list2_index - 1] = cdp_del_list_start_rd[cdp_a_loop];
		  cdp_del_list2_end_rd[cdp_del_list2_index - 1] = cdp_del_list_end_rd[cdp_a_loop];
		  cdp_del_list2_start_conc[cdp_del_list2_index - 1] = cdp_del_list_start_conc[cdp_a_loop];
		  cdp_del_list2_end_conc[cdp_del_list2_index - 1] = cdp_del_list_end_conc[cdp_a_loop];
		  cdp_del_list2_start_other_len[cdp_del_list2_index - 1] = cdp_del_list_start_other_len[cdp_a_loop];
		  cdp_del_list2_end_other_len[cdp_del_list2_index - 1] = cdp_del_list_end_other_len[cdp_a_loop];
		  cdp_del_list2_start_read_start[cdp_del_list2_index - 1] = cdp_del_list_start_read_start[cdp_a_loop];
		  cdp_del_list2_start_read_end[cdp_del_list2_index - 1] = cdp_del_list_start_read_end[cdp_a_loop];
		  cdp_del_list2_end_read_start[cdp_del_list2_index - 1] = cdp_del_list_end_read_start[cdp_a_loop];
		  cdp_del_list2_end_read_end[cdp_del_list2_index - 1] = cdp_del_list_end_read_end[cdp_a_loop];
		}
	      }
	      else
	      {
		cdp_first_start = cdp_del_list_start[cdp_a_loop];
		cdp_last_start = cdp_del_list_start[cdp_a_loop];
		cdp_first_end = cdp_del_list_end[cdp_a_loop];
		cdp_last_end = cdp_del_list_end[cdp_a_loop];
		cdp_first_dist = cdp_del_list_dist[cdp_a_loop];
		cdp_last_dist = cdp_del_list_dist[cdp_a_loop];
		cdp_del_list2_start[cdp_del_list2_index - 1] = cdp_del_list_start[cdp_a_loop];
		cdp_del_list2_end[cdp_del_list2_index - 1] = cdp_del_list_end[cdp_a_loop];
		cdp_del_list2_start_del_f[cdp_del_list2_index - 1] = cdp_del_list_start_del_f[cdp_a_loop];
		cdp_del_list2_end_del_r[cdp_del_list2_index - 1] = cdp_del_list_end_del_r[cdp_a_loop];
		cdp_del_list2_dist[cdp_del_list2_index - 1] = cdp_del_list_dist[cdp_a_loop];
		cdp_del_list2_start_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_start_binom_cdf[cdp_a_loop];
		cdp_del_list2_start_hez_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_start_hez_binom_cdf[cdp_a_loop];
		cdp_del_list2_end_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_end_binom_cdf[cdp_a_loop];
		cdp_del_list2_end_hez_binom_cdf[cdp_del_list2_index - 1] = cdp_del_list_end_hez_binom_cdf[cdp_a_loop];
		cdp_del_list2_start_rd[cdp_del_list2_index - 1] = cdp_del_list_start_rd[cdp_a_loop];
		cdp_del_list2_end_rd[cdp_del_list2_index - 1] = cdp_del_list_end_rd[cdp_a_loop];
		cdp_del_list2_start_conc[cdp_del_list2_index - 1] = cdp_del_list_start_conc[cdp_a_loop];
		cdp_del_list2_end_conc[cdp_del_list2_index - 1] = cdp_del_list_end_conc[cdp_a_loop];
		cdp_del_list2_start_other_len[cdp_del_list2_index - 1] = cdp_del_list_start_other_len[cdp_a_loop];
		cdp_del_list2_end_other_len[cdp_del_list2_index - 1] = cdp_del_list_end_other_len[cdp_a_loop];
		cdp_del_list2_start_read_start[cdp_del_list2_index - 1] = cdp_del_list_start_read_start[cdp_a_loop];
		cdp_del_list2_start_read_end[cdp_del_list2_index - 1] = cdp_del_list_start_read_end[cdp_a_loop];
		cdp_del_list2_end_read_start[cdp_del_list2_index - 1] = cdp_del_list_end_read_start[cdp_a_loop];
		cdp_del_list2_end_read_end[cdp_del_list2_index - 1] = cdp_del_list_end_read_end[cdp_a_loop];
	      }
	    }
	  }
	}
	if( cdp_del_begin == 0 )
	{
	  if( cdp_del_list_start[cdp_a_loop] >= 0 && cdp_del_list_end[cdp_a_loop] >= 0 )
	  {
	    if( cdp_del_list2_index < g_sv_list2_len - 1 )
	    {
	      cdp_del_begin = 1;
	      cdp_first_start = cdp_del_list_start[cdp_a_loop];
	      cdp_last_start = cdp_del_list_start[cdp_a_loop];
	      cdp_first_end = cdp_del_list_end[cdp_a_loop];
	      cdp_last_end = cdp_del_list_end[cdp_a_loop];
	      cdp_first_dist = cdp_del_list_dist[cdp_a_loop];
	      cdp_last_dist = cdp_del_list_dist[cdp_a_loop];
	      cdp_del_list2_start[cdp_del_list2_index] = cdp_del_list_start[cdp_a_loop];
	      cdp_del_list2_end[cdp_del_list2_index] = cdp_del_list_end[cdp_a_loop];
	      cdp_del_list2_start_del_f[cdp_del_list2_index] = cdp_del_list_start_del_f[cdp_a_loop];
	      cdp_del_list2_end_del_r[cdp_del_list2_index] = cdp_del_list_end_del_r[cdp_a_loop];
	      cdp_del_list2_dist[cdp_del_list2_index] = cdp_del_list_dist[cdp_a_loop];
	      cdp_del_list2_start_binom_cdf[cdp_del_list2_index] = cdp_del_list_start_binom_cdf[cdp_a_loop];
	      cdp_del_list2_start_hez_binom_cdf[cdp_del_list2_index] = cdp_del_list_start_hez_binom_cdf[cdp_a_loop];
	      cdp_del_list2_end_binom_cdf[cdp_del_list2_index] = cdp_del_list_end_binom_cdf[cdp_a_loop];
	      cdp_del_list2_end_hez_binom_cdf[cdp_del_list2_index] = cdp_del_list_end_hez_binom_cdf[cdp_a_loop];
	      cdp_del_list2_start_rd[cdp_del_list2_index] = cdp_del_list_start_rd[cdp_a_loop];
	      cdp_del_list2_end_rd[cdp_del_list2_index] = cdp_del_list_end_rd[cdp_a_loop];
	      cdp_del_list2_start_conc[cdp_del_list2_index] = cdp_del_list_start_conc[cdp_a_loop];
	      cdp_del_list2_end_conc[cdp_del_list2_index] = cdp_del_list_end_conc[cdp_a_loop];
	      cdp_del_list2_start_other_len[cdp_del_list2_index] = cdp_del_list_start_other_len[cdp_a_loop];
	      cdp_del_list2_end_other_len[cdp_del_list2_index] = cdp_del_list_end_other_len[cdp_a_loop];
	      cdp_del_list2_start_read_start[cdp_del_list2_index] = cdp_del_list_start_read_start[cdp_a_loop];
	      cdp_del_list2_start_read_end[cdp_del_list2_index] = cdp_del_list_start_read_end[cdp_a_loop];
	      cdp_del_list2_end_read_start[cdp_del_list2_index] = cdp_del_list_end_read_start[cdp_a_loop];
	      cdp_del_list2_end_read_end[cdp_del_list2_index] = cdp_del_list_end_read_end[cdp_a_loop];
	      cdp_del_list2_index += 1;
	    }
	    else
	    {
	      g_sv_list2_len_over = 1;
	    }

	  }
	}
      }

      

     
      cdp_inv_f_list2_index = 0;
      cdp_inv_begin = 0;
      cdp_first_start = 0;
      cdp_last_start = 0;
      cdp_first_end = 0;
      cdp_last_end = 0;
      cdp_first_dist = 0;
      cdp_last_dist = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_inv_f_list_index;cdp_a_loop++)
      {
	if( cdp_inv_begin == 1 )
	{
	  if( cdp_inv_f_list_start[cdp_a_loop] > cdp_last_start + g_insert_max_size - 2*g_lseq )
	  {
	    cdp_inv_begin = 0;
	    cdp_first_start = 0;
	    cdp_last_start = 0;
	    cdp_first_end = 0;
	    cdp_last_end = 0;
	    cdp_first_dist = 0;
	    cdp_last_dist = 0;
	  }
	  else
	  {
	    cdp_max_binom_cdf = cdp_inv_f_list_start_binom_cdf[cdp_a_loop];
	    if( cdp_inv_f_list_end_binom_cdf[cdp_a_loop] > cdp_max_binom_cdf )
	    {
	      cdp_max_binom_cdf = cdp_inv_f_list_end_binom_cdf[cdp_a_loop];
	    }
	    cdp_max_binom_cdf2 = cdp_inv_f_list2_start_binom_cdf[cdp_inv_f_list2_index - 1];
	    if( cdp_inv_f_list2_end_binom_cdf[cdp_inv_f_list2_index - 1] > cdp_max_binom_cdf2 )
	    {
	      cdp_max_binom_cdf2 = cdp_inv_f_list2_end_binom_cdf[cdp_inv_f_list2_index - 1];
	    }
	    if( cdp_max_binom_cdf <= cdp_max_binom_cdf2 && cdp_inv_f_list_start[cdp_a_loop] >= 0 && cdp_inv_f_list_end[cdp_a_loop] >= 0 && cdp_inv_f_list2_start_inv[cdp_inv_f_list2_index - 1] <= cdp_inv_f_list_start_inv[cdp_a_loop] && cdp_inv_f_list2_end_inv[cdp_inv_f_list2_index - 1] <= cdp_inv_f_list_end_inv[cdp_a_loop] )
	    {
	      if( cdp_inv_f_list_start_binom_cdf[cdp_a_loop] == cdp_inv_f_list2_start_binom_cdf[cdp_inv_f_list2_index - 1] && cdp_inv_f_list_end_binom_cdf[cdp_a_loop] == cdp_inv_f_list2_end_binom_cdf[cdp_inv_f_list2_index - 1] )
	      {
		if( (cdp_inv_f_list2_start_inv[cdp_inv_f_list2_index - 1] < cdp_inv_f_list_start_inv[cdp_a_loop] && cdp_inv_f_list2_end_inv[cdp_inv_f_list2_index - 1] <= cdp_inv_f_list_end_inv[cdp_a_loop]) || (cdp_inv_f_list2_start_inv[cdp_inv_f_list2_index - 1] <= cdp_inv_f_list_start_inv[cdp_a_loop] && cdp_inv_f_list2_end_inv[cdp_inv_f_list2_index - 1] < cdp_inv_f_list_end_inv[cdp_a_loop]) )
		{
		  cdp_first_start = cdp_inv_f_list_start[cdp_a_loop];
		  cdp_last_start = cdp_inv_f_list_start[cdp_a_loop];
		  cdp_first_end = cdp_inv_f_list_end[cdp_a_loop];
		  cdp_last_end = cdp_inv_f_list_end[cdp_a_loop];
		  cdp_first_dist = cdp_inv_f_list_dist[cdp_a_loop];
		  cdp_last_dist = cdp_inv_f_list_dist[cdp_a_loop];
		  cdp_inv_f_list2_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start[cdp_a_loop];
		  cdp_inv_f_list2_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end[cdp_a_loop];
		  cdp_inv_f_list2_start_inv[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_inv[cdp_a_loop];
		  cdp_inv_f_list2_end_inv[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_inv[cdp_a_loop];
		  cdp_inv_f_list2_dist[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_dist[cdp_a_loop];
		  cdp_inv_f_list2_start_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_start_hez_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_end_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_end_hez_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_start_rd[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_rd[cdp_a_loop];
		  cdp_inv_f_list2_end_rd[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_rd[cdp_a_loop];
		  cdp_inv_f_list2_start_conc[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_conc[cdp_a_loop];
		  cdp_inv_f_list2_end_conc[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_conc[cdp_a_loop];
		  cdp_inv_f_list2_start_other_len[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_other_len[cdp_a_loop];
		  cdp_inv_f_list2_end_other_len[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_other_len[cdp_a_loop];
		  cdp_inv_f_list2_start_read_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_read_start[cdp_a_loop];
		  cdp_inv_f_list2_start_read_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_read_end[cdp_a_loop];
		  cdp_inv_f_list2_end_read_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_read_start[cdp_a_loop];
		  cdp_inv_f_list2_end_read_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_read_end[cdp_a_loop];
		}
		else if( cdp_inv_f_list2_start_inv[cdp_inv_f_list2_index - 1] == cdp_inv_f_list_start_inv[cdp_a_loop] && cdp_inv_f_list2_end_inv[cdp_inv_f_list2_index - 1] == cdp_inv_f_list_end_inv[cdp_a_loop] )
		{
		  cdp_last_start = cdp_inv_f_list_start[cdp_a_loop];
		  cdp_last_end = cdp_inv_f_list_end[cdp_a_loop];
		  cdp_last_dist = cdp_inv_f_list_dist[cdp_a_loop];
		  cdp_inv_f_list2_start[cdp_inv_f_list2_index - 1] = (cdp_first_start + cdp_last_start)/2;
		  cdp_inv_f_list2_end[cdp_inv_f_list2_index - 1] = (cdp_first_end + cdp_last_end)/2;
		  cdp_inv_f_list2_dist[cdp_inv_f_list2_index - 1] = (cdp_first_dist + cdp_last_dist)/(double)(2.0);
		  cdp_inv_f_list2_start_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_start_hez_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_end_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_end_hez_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_f_list2_start_rd[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_rd[cdp_a_loop];
		  cdp_inv_f_list2_end_rd[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_rd[cdp_a_loop];
		  cdp_inv_f_list2_start_conc[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_conc[cdp_a_loop];
		  cdp_inv_f_list2_end_conc[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_conc[cdp_a_loop];
		  cdp_inv_f_list2_start_other_len[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_other_len[cdp_a_loop];
		  cdp_inv_f_list2_end_other_len[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_other_len[cdp_a_loop];
		  cdp_inv_f_list2_start_read_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_read_start[cdp_a_loop];
		  cdp_inv_f_list2_start_read_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_read_end[cdp_a_loop];
		  cdp_inv_f_list2_end_read_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_read_start[cdp_a_loop];
		  cdp_inv_f_list2_end_read_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_read_end[cdp_a_loop];
		}
	      }
	      else
	      {
		cdp_first_start = cdp_inv_f_list_start[cdp_a_loop];
		cdp_last_start = cdp_inv_f_list_start[cdp_a_loop];
		cdp_first_end = cdp_inv_f_list_end[cdp_a_loop];
		cdp_last_end = cdp_inv_f_list_end[cdp_a_loop];
		cdp_first_dist = cdp_inv_f_list_dist[cdp_a_loop];
		cdp_last_dist = cdp_inv_f_list_dist[cdp_a_loop];
		cdp_inv_f_list2_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start[cdp_a_loop];
		cdp_inv_f_list2_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end[cdp_a_loop];
		cdp_inv_f_list2_start_inv[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_inv[cdp_a_loop];
		cdp_inv_f_list2_end_inv[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_inv[cdp_a_loop];
		cdp_inv_f_list2_dist[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_dist[cdp_a_loop];
		cdp_inv_f_list2_start_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_binom_cdf[cdp_a_loop];
		cdp_inv_f_list2_start_hez_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_hez_binom_cdf[cdp_a_loop];
		cdp_inv_f_list2_end_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_binom_cdf[cdp_a_loop];
		cdp_inv_f_list2_end_hez_binom_cdf[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_hez_binom_cdf[cdp_a_loop];
		cdp_inv_f_list2_start_rd[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_rd[cdp_a_loop];
		cdp_inv_f_list2_end_rd[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_rd[cdp_a_loop];
		cdp_inv_f_list2_start_conc[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_conc[cdp_a_loop];
		cdp_inv_f_list2_end_conc[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_conc[cdp_a_loop];
		cdp_inv_f_list2_start_other_len[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_other_len[cdp_a_loop];
		cdp_inv_f_list2_end_other_len[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_other_len[cdp_a_loop];
		cdp_inv_f_list2_start_read_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_read_start[cdp_a_loop];
		cdp_inv_f_list2_start_read_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_start_read_end[cdp_a_loop];
		cdp_inv_f_list2_end_read_start[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_read_start[cdp_a_loop];
		cdp_inv_f_list2_end_read_end[cdp_inv_f_list2_index - 1] = cdp_inv_f_list_end_read_end[cdp_a_loop];
	      }
	    }
	  }
	}
	if( cdp_inv_begin == 0 )
	{
	  if( cdp_inv_f_list_start[cdp_a_loop] >= 0 && cdp_inv_f_list_end[cdp_a_loop] >= 0 )
	  {
	    if( cdp_inv_f_list2_index < g_sv_list2_len - 1 )
	    {
	      cdp_inv_begin = 1;
	      cdp_first_start = cdp_inv_f_list_start[cdp_a_loop];
	      cdp_last_start = cdp_inv_f_list_start[cdp_a_loop];
	      cdp_first_end = cdp_inv_f_list_end[cdp_a_loop];
	      cdp_last_end = cdp_inv_f_list_end[cdp_a_loop];
	      cdp_first_dist = cdp_inv_f_list_dist[cdp_a_loop];
	      cdp_last_dist = cdp_inv_f_list_dist[cdp_a_loop];
	      cdp_inv_f_list2_start[cdp_inv_f_list2_index] = cdp_inv_f_list_start[cdp_a_loop];
	      cdp_inv_f_list2_end[cdp_inv_f_list2_index] = cdp_inv_f_list_end[cdp_a_loop];
	      cdp_inv_f_list2_start_inv[cdp_inv_f_list2_index] = cdp_inv_f_list_start_inv[cdp_a_loop];
	      cdp_inv_f_list2_end_inv[cdp_inv_f_list2_index] = cdp_inv_f_list_end_inv[cdp_a_loop];
	      cdp_inv_f_list2_dist[cdp_inv_f_list2_index] = cdp_inv_f_list_dist[cdp_a_loop];
	      cdp_inv_f_list2_start_binom_cdf[cdp_inv_f_list2_index] = cdp_inv_f_list_start_binom_cdf[cdp_a_loop];
	      cdp_inv_f_list2_start_hez_binom_cdf[cdp_inv_f_list2_index] = cdp_inv_f_list_start_hez_binom_cdf[cdp_a_loop];
	      cdp_inv_f_list2_end_binom_cdf[cdp_inv_f_list2_index] = cdp_inv_f_list_end_binom_cdf[cdp_a_loop];
	      cdp_inv_f_list2_end_hez_binom_cdf[cdp_inv_f_list2_index] = cdp_inv_f_list_end_hez_binom_cdf[cdp_a_loop];
	      cdp_inv_f_list2_start_rd[cdp_inv_f_list2_index] = cdp_inv_f_list_start_rd[cdp_a_loop];
	      cdp_inv_f_list2_end_rd[cdp_inv_f_list2_index] = cdp_inv_f_list_end_rd[cdp_a_loop];
	      cdp_inv_f_list2_start_conc[cdp_inv_f_list2_index] = cdp_inv_f_list_start_conc[cdp_a_loop];
	      cdp_inv_f_list2_end_conc[cdp_inv_f_list2_index] = cdp_inv_f_list_end_conc[cdp_a_loop];
	      cdp_inv_f_list2_start_other_len[cdp_inv_f_list2_index] = cdp_inv_f_list_start_other_len[cdp_a_loop];
	      cdp_inv_f_list2_end_other_len[cdp_inv_f_list2_index] = cdp_inv_f_list_end_other_len[cdp_a_loop];
	      cdp_inv_f_list2_start_read_start[cdp_inv_f_list2_index] = cdp_inv_f_list_start_read_start[cdp_a_loop];
	      cdp_inv_f_list2_start_read_end[cdp_inv_f_list2_index] = cdp_inv_f_list_start_read_end[cdp_a_loop];
	      cdp_inv_f_list2_end_read_start[cdp_inv_f_list2_index] = cdp_inv_f_list_end_read_start[cdp_a_loop];
	      cdp_inv_f_list2_end_read_end[cdp_inv_f_list2_index] = cdp_inv_f_list_end_read_end[cdp_a_loop];
	      cdp_inv_f_list2_index += 1;
	    }
	    else
	    {
	      g_sv_list2_len_over = 1;
	    }

	  }
	}
      }
      


      
      cdp_inv_r_list2_index = 0;
      cdp_inv_begin = 0;
      cdp_first_start = 0;
      cdp_last_start = 0;
      cdp_first_end = 0;
      cdp_last_end = 0;
      cdp_first_dist = 0;
      cdp_last_dist = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_inv_r_list_index;cdp_a_loop++)
      {
	if( cdp_inv_begin == 1 )
	{
	  if( cdp_inv_r_list_start[cdp_a_loop] > cdp_last_start + g_insert_max_size - 2*g_lseq )
	  {
	    cdp_inv_begin = 0;
	    cdp_first_start = 0;
	    cdp_last_start = 0;
	    cdp_first_end = 0;
	    cdp_last_end = 0;
	    cdp_first_dist = 0;
	    cdp_last_dist = 0;
	  }
	  else
	  {
	    cdp_max_binom_cdf = cdp_inv_r_list_start_binom_cdf[cdp_a_loop];
	    if( cdp_inv_r_list_end_binom_cdf[cdp_a_loop] > cdp_max_binom_cdf )
	    {
	      cdp_max_binom_cdf = cdp_inv_r_list_end_binom_cdf[cdp_a_loop];
	    }
	    cdp_max_binom_cdf2 = cdp_inv_r_list2_start_binom_cdf[cdp_inv_r_list2_index - 1];
	    if( cdp_inv_r_list2_end_binom_cdf[cdp_inv_r_list2_index - 1] > cdp_max_binom_cdf2 )
	    {
	      cdp_max_binom_cdf2 = cdp_inv_r_list2_end_binom_cdf[cdp_inv_r_list2_index - 1];
	    }
	    if( cdp_max_binom_cdf <= cdp_max_binom_cdf2 && cdp_inv_r_list_start[cdp_a_loop] >= 0 && cdp_inv_r_list_end[cdp_a_loop] >= 0 && cdp_inv_r_list2_start_inv[cdp_inv_r_list2_index - 1] <= cdp_inv_r_list_start_inv[cdp_a_loop] && cdp_inv_r_list2_end_inv[cdp_inv_r_list2_index - 1] <= cdp_inv_r_list_end_inv[cdp_a_loop] )
	    {
	      if( cdp_inv_r_list_start_binom_cdf[cdp_a_loop] == cdp_inv_r_list2_start_binom_cdf[cdp_inv_r_list2_index - 1] && cdp_inv_r_list_end_binom_cdf[cdp_a_loop] == cdp_inv_r_list2_end_binom_cdf[cdp_inv_r_list2_index - 1] )
	      {
		if( (cdp_inv_r_list2_start_inv[cdp_inv_r_list2_index - 1] < cdp_inv_r_list_start_inv[cdp_a_loop] && cdp_inv_r_list2_end_inv[cdp_inv_r_list2_index - 1] <= cdp_inv_r_list_end_inv[cdp_a_loop]) || (cdp_inv_r_list2_start_inv[cdp_inv_r_list2_index - 1] <= cdp_inv_r_list_start_inv[cdp_a_loop] && cdp_inv_r_list2_end_inv[cdp_inv_r_list2_index - 1] < cdp_inv_r_list_end_inv[cdp_a_loop]) )
		{
		  cdp_first_start = cdp_inv_r_list_start[cdp_a_loop];
		  cdp_last_start = cdp_inv_r_list_start[cdp_a_loop];
		  cdp_first_end = cdp_inv_r_list_end[cdp_a_loop];
		  cdp_last_end = cdp_inv_r_list_end[cdp_a_loop];
		  cdp_first_dist = cdp_inv_r_list_dist[cdp_a_loop];
		  cdp_last_dist = cdp_inv_r_list_dist[cdp_a_loop];
		  cdp_inv_r_list2_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start[cdp_a_loop];
		  cdp_inv_r_list2_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end[cdp_a_loop];
		  cdp_inv_r_list2_start_inv[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_inv[cdp_a_loop];
		  cdp_inv_r_list2_end_inv[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_inv[cdp_a_loop];
		  cdp_inv_r_list2_dist[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_dist[cdp_a_loop];
		  cdp_inv_r_list2_start_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_start_hez_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_end_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_end_hez_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_start_rd[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_rd[cdp_a_loop];
		  cdp_inv_r_list2_end_rd[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_rd[cdp_a_loop];
		  cdp_inv_r_list2_start_conc[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_conc[cdp_a_loop];
		  cdp_inv_r_list2_end_conc[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_conc[cdp_a_loop];
		  cdp_inv_r_list2_start_other_len[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_other_len[cdp_a_loop];
		  cdp_inv_r_list2_end_other_len[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_other_len[cdp_a_loop];
		  cdp_inv_r_list2_start_read_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_read_start[cdp_a_loop];
		  cdp_inv_r_list2_start_read_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_read_end[cdp_a_loop];
		  cdp_inv_r_list2_end_read_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_read_start[cdp_a_loop];
		  cdp_inv_r_list2_end_read_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_read_end[cdp_a_loop];
		}
		else if( cdp_inv_r_list2_start_inv[cdp_inv_r_list2_index - 1] == cdp_inv_r_list_start_inv[cdp_a_loop] && cdp_inv_r_list2_end_inv[cdp_inv_r_list2_index - 1] == cdp_inv_r_list_end_inv[cdp_a_loop] )
		{
		  cdp_last_start = cdp_inv_r_list_start[cdp_a_loop];
		  cdp_last_end = cdp_inv_r_list_end[cdp_a_loop];
		  cdp_last_dist = cdp_inv_r_list_dist[cdp_a_loop];
		  cdp_inv_r_list2_start[cdp_inv_r_list2_index - 1] = (cdp_first_start + cdp_last_start)/2;
		  cdp_inv_r_list2_end[cdp_inv_r_list2_index - 1] = (cdp_first_end + cdp_last_end)/2;
		  cdp_inv_r_list2_dist[cdp_inv_r_list2_index - 1] = (cdp_first_dist + cdp_last_dist)/(double)(2.0);
		  cdp_inv_r_list2_start_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_start_hez_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_end_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_end_hez_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_hez_binom_cdf[cdp_a_loop];
		  cdp_inv_r_list2_start_rd[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_rd[cdp_a_loop];
		  cdp_inv_r_list2_end_rd[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_rd[cdp_a_loop];
		  cdp_inv_r_list2_start_conc[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_conc[cdp_a_loop];
		  cdp_inv_r_list2_end_conc[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_conc[cdp_a_loop];
		  cdp_inv_r_list2_start_other_len[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_other_len[cdp_a_loop];
		  cdp_inv_r_list2_end_other_len[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_other_len[cdp_a_loop];
		  cdp_inv_r_list2_start_read_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_read_start[cdp_a_loop];
		  cdp_inv_r_list2_start_read_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_read_end[cdp_a_loop];
		  cdp_inv_r_list2_end_read_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_read_start[cdp_a_loop];
		  cdp_inv_r_list2_end_read_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_read_end[cdp_a_loop];
		}
	      }
	      else
	      {
		cdp_first_start = cdp_inv_r_list_start[cdp_a_loop];
		cdp_last_start = cdp_inv_r_list_start[cdp_a_loop];
		cdp_first_end = cdp_inv_r_list_end[cdp_a_loop];
		cdp_last_end = cdp_inv_r_list_end[cdp_a_loop];
		cdp_first_dist = cdp_inv_r_list_dist[cdp_a_loop];
		cdp_last_dist = cdp_inv_r_list_dist[cdp_a_loop];
		cdp_inv_r_list2_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start[cdp_a_loop];
		cdp_inv_r_list2_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end[cdp_a_loop];
		cdp_inv_r_list2_start_inv[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_inv[cdp_a_loop];
		cdp_inv_r_list2_end_inv[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_inv[cdp_a_loop];
		cdp_inv_r_list2_dist[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_dist[cdp_a_loop];
		cdp_inv_r_list2_start_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_binom_cdf[cdp_a_loop];
		cdp_inv_r_list2_start_hez_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_hez_binom_cdf[cdp_a_loop];
		cdp_inv_r_list2_end_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_binom_cdf[cdp_a_loop];
		cdp_inv_r_list2_end_hez_binom_cdf[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_hez_binom_cdf[cdp_a_loop];
		cdp_inv_r_list2_start_rd[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_rd[cdp_a_loop];
		cdp_inv_r_list2_end_rd[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_rd[cdp_a_loop];
		cdp_inv_r_list2_start_conc[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_conc[cdp_a_loop];
		cdp_inv_r_list2_end_conc[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_conc[cdp_a_loop];
		cdp_inv_r_list2_start_other_len[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_other_len[cdp_a_loop];
		cdp_inv_r_list2_end_other_len[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_other_len[cdp_a_loop];
		cdp_inv_r_list2_start_read_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_read_start[cdp_a_loop];
		cdp_inv_r_list2_start_read_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_start_read_end[cdp_a_loop];
		cdp_inv_r_list2_end_read_start[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_read_start[cdp_a_loop];
		cdp_inv_r_list2_end_read_end[cdp_inv_r_list2_index - 1] = cdp_inv_r_list_end_read_end[cdp_a_loop];
	      }
	    }
	  }
	}
	if( cdp_inv_begin == 0 )
	{
	  if( cdp_inv_r_list_start[cdp_a_loop] >= 0 && cdp_inv_r_list_end[cdp_a_loop] >= 0 )
	  {
	    if( cdp_inv_r_list2_index < g_sv_list2_len - 1 )
	    {
	      cdp_inv_begin = 1;
	      cdp_first_start = cdp_inv_r_list_start[cdp_a_loop];
	      cdp_last_start = cdp_inv_r_list_start[cdp_a_loop];
	      cdp_first_end = cdp_inv_r_list_end[cdp_a_loop];
	      cdp_last_end = cdp_inv_r_list_end[cdp_a_loop];
	      cdp_first_dist = cdp_inv_r_list_dist[cdp_a_loop];
	      cdp_last_dist = cdp_inv_r_list_dist[cdp_a_loop];
	      cdp_inv_r_list2_start[cdp_inv_r_list2_index] = cdp_inv_r_list_start[cdp_a_loop];
	      cdp_inv_r_list2_end[cdp_inv_r_list2_index] = cdp_inv_r_list_end[cdp_a_loop];
	      cdp_inv_r_list2_start_inv[cdp_inv_r_list2_index] = cdp_inv_r_list_start_inv[cdp_a_loop];
	      cdp_inv_r_list2_end_inv[cdp_inv_r_list2_index] = cdp_inv_r_list_end_inv[cdp_a_loop];
	      cdp_inv_r_list2_dist[cdp_inv_r_list2_index] = cdp_inv_r_list_dist[cdp_a_loop];
	      cdp_inv_r_list2_start_binom_cdf[cdp_inv_r_list2_index] = cdp_inv_r_list_start_binom_cdf[cdp_a_loop];
	      cdp_inv_r_list2_start_hez_binom_cdf[cdp_inv_r_list2_index] = cdp_inv_r_list_start_hez_binom_cdf[cdp_a_loop];
	      cdp_inv_r_list2_end_binom_cdf[cdp_inv_r_list2_index] = cdp_inv_r_list_end_binom_cdf[cdp_a_loop];
	      cdp_inv_r_list2_end_hez_binom_cdf[cdp_inv_r_list2_index] = cdp_inv_r_list_end_hez_binom_cdf[cdp_a_loop];
	      cdp_inv_r_list2_start_rd[cdp_inv_r_list2_index] = cdp_inv_r_list_start_rd[cdp_a_loop];
	      cdp_inv_r_list2_end_rd[cdp_inv_r_list2_index] = cdp_inv_r_list_end_rd[cdp_a_loop];
	      cdp_inv_r_list2_start_conc[cdp_inv_r_list2_index] = cdp_inv_r_list_start_conc[cdp_a_loop];
	      cdp_inv_r_list2_end_conc[cdp_inv_r_list2_index] = cdp_inv_r_list_end_conc[cdp_a_loop];
	      cdp_inv_r_list2_start_other_len[cdp_inv_r_list2_index] = cdp_inv_r_list_start_other_len[cdp_a_loop];
	      cdp_inv_r_list2_end_other_len[cdp_inv_r_list2_index] = cdp_inv_r_list_end_other_len[cdp_a_loop];
	      cdp_inv_r_list2_start_read_start[cdp_inv_r_list2_index] = cdp_inv_r_list_start_read_start[cdp_a_loop];
	      cdp_inv_r_list2_start_read_end[cdp_inv_r_list2_index] = cdp_inv_r_list_start_read_end[cdp_a_loop];
	      cdp_inv_r_list2_end_read_start[cdp_inv_r_list2_index] = cdp_inv_r_list_end_read_start[cdp_a_loop];
	      cdp_inv_r_list2_end_read_end[cdp_inv_r_list2_index] = cdp_inv_r_list_end_read_end[cdp_a_loop];
	      cdp_inv_r_list2_index += 1;
	    }
	    else
	    {
	      g_sv_list2_len_over = 1;
	    }

	  }
	}
      }
     


      
      int cdp_overlap = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_inv_f_list2_index;cdp_a_loop++)
      {
	cdp_overlap = 0;
	if( cdp_inv_f_list2_start_binom_cdf[cdp_a_loop] <= g_pval_threshold && cdp_inv_f_list2_end_binom_cdf[cdp_a_loop] <= g_pval_threshold && (double) cdp_inv_f_list2_start_inv[cdp_a_loop] / (double) cdp_inv_f_list2_start_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor && (double) cdp_inv_f_list2_end_inv[cdp_a_loop] / (double) cdp_inv_f_list2_end_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor )
	{
	  for(cdp_b_loop=0;cdp_b_loop<cdp_inv_r_list2_index;cdp_b_loop++)
	  {
	    if( abs(cdp_inv_f_list2_start[cdp_a_loop] - cdp_inv_r_list2_start[cdp_b_loop]) <  + g_insert_max_size - 2*g_lseq && abs(cdp_inv_f_list2_end[cdp_a_loop] - cdp_inv_r_list2_end[cdp_b_loop]) <  + g_insert_max_size - 2*g_lseq )
	    {
	      if( (cdp_inv_f_list2_start[cdp_a_loop] >= cdp_inv_r_list2_start[cdp_b_loop] && cdp_inv_f_list2_start[cdp_a_loop] <= cdp_inv_r_list2_end[cdp_b_loop]) || (cdp_inv_r_list2_start[cdp_b_loop] >= cdp_inv_f_list2_start[cdp_a_loop] && cdp_inv_r_list2_start[cdp_b_loop] <= cdp_inv_f_list2_end[cdp_a_loop]) )
	      {
		if( cdp_inv_r_list2_start_binom_cdf[cdp_b_loop] * cdp_inv_r_list2_end_binom_cdf[cdp_b_loop] < cdp_inv_f_list2_start_binom_cdf[cdp_a_loop] * cdp_inv_f_list2_end_binom_cdf[cdp_a_loop] )
		{
		  cdp_overlap = 1;
		  break;
		}
	      }
	    }
	  }

	  cdp_inv_rd1_ave = 0;
	  for(cdp_b_loop=cdp_inv_f_list2_start_read_start[cdp_a_loop];cdp_b_loop<cdp_inv_f_list2_start_read_end[cdp_a_loop] + g_lseq;cdp_b_loop++)
	  {
	    cdp_inv_rd1_ave += caf_rd_rd_list[cdp_b_loop] + caf_rd_low_mq_rd_list[cdp_b_loop];
	  }
	  cdp_inv_rd1_ave = cdp_inv_rd1_ave / (cdp_inv_f_list2_start_read_end[cdp_a_loop] + g_lseq - cdp_inv_f_list2_start_read_start[cdp_a_loop]);
	  cdp_inv_rd2_ave = 0;
	  for(cdp_b_loop=cdp_inv_f_list2_end_read_start[cdp_a_loop];cdp_b_loop<cdp_inv_f_list2_end_read_end[cdp_a_loop] + g_lseq;cdp_b_loop++)
	  {
	    cdp_inv_rd2_ave += caf_rd_rd_list[cdp_b_loop] + caf_rd_low_mq_rd_list[cdp_b_loop];
	  }
	  cdp_inv_rd2_ave = cdp_inv_rd2_ave / (cdp_inv_f_list2_end_read_end[cdp_a_loop] + g_lseq - cdp_inv_f_list2_end_read_start[cdp_a_loop]);
	  if( cdp_overlap == 0 && cdp_inv_rd1_ave/cdp_inv_rd2_ave <= g_max_inv_rd_diff && cdp_inv_rd2_ave/cdp_inv_rd1_ave <= g_max_inv_rd_diff )
	  {
	    if( g_vcf == 1 )
	    {

			float p_value_inv_f = cdp_inv_f_list2_end_binom_cdf[cdp_a_loop];
			if (cdp_inv_f_list2_start_binom_cdf[cdp_a_loop] > cdp_inv_f_list2_end_binom_cdf[cdp_a_loop]) {
				p_value_inv_f = cdp_inv_f_list2_start_binom_cdf[cdp_a_loop];
			}
			if (p_value_inv_f != 0)
			{
				int qual_entry = (int) -10 * log10(p_value_inv_f);
				vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
				vcf_sv_data_list[sort_sv_count].pos = cdp_inv_f_list2_start[cdp_a_loop];
				sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INV>\t%d\t.\tSVTYPE=INV;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_inv_f_list2_start[cdp_a_loop] - 1]), qual_entry, cdp_inv_f_list2_end[cdp_a_loop], unknown_gt_entry, cdp_inv_f_list2_start_binom_cdf[cdp_a_loop], cdp_inv_f_list2_end_binom_cdf[cdp_a_loop], (double)cdp_inv_f_list2_start_inv[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_inv_f_list2_end_inv[cdp_a_loop]/(double)cdp_add_factor, cdp_inv_f_list2_start_rd[cdp_a_loop], cdp_inv_f_list2_end_rd[cdp_a_loop], cdp_inv_f_list2_start_conc[cdp_a_loop], cdp_inv_f_list2_end_conc[cdp_a_loop], cdp_inv_f_list2_start_other_len[cdp_a_loop], cdp_inv_f_list2_end_other_len[cdp_a_loop], cdp_inv_f_list2_start_read_start[cdp_a_loop] + 1, cdp_inv_f_list2_start_read_end[cdp_a_loop] + 1, cdp_inv_f_list2_end_read_start[cdp_a_loop] + 1, cdp_inv_f_list2_end_read_end[cdp_a_loop] + 1);
				sort_sv_count += 1;
			} else {
				vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
				vcf_sv_data_list[sort_sv_count].pos = cdp_inv_f_list2_start[cdp_a_loop];
				sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INV>\t%s\t.\tSVTYPE=INV;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_inv_f_list2_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_inv_f_list2_end[cdp_a_loop], unknown_gt_entry, cdp_inv_f_list2_start_binom_cdf[cdp_a_loop], cdp_inv_f_list2_end_binom_cdf[cdp_a_loop], (double)cdp_inv_f_list2_start_inv[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_inv_f_list2_end_inv[cdp_a_loop]/(double)cdp_add_factor, cdp_inv_f_list2_start_rd[cdp_a_loop], cdp_inv_f_list2_end_rd[cdp_a_loop], cdp_inv_f_list2_start_conc[cdp_a_loop], cdp_inv_f_list2_end_conc[cdp_a_loop], cdp_inv_f_list2_start_other_len[cdp_a_loop], cdp_inv_f_list2_end_other_len[cdp_a_loop], cdp_inv_f_list2_start_read_start[cdp_a_loop] + 1, cdp_inv_f_list2_start_read_end[cdp_a_loop] + 1, cdp_inv_f_list2_end_read_start[cdp_a_loop] + 1, cdp_inv_f_list2_end_read_end[cdp_a_loop] + 1);
				sort_sv_count += 1;
			}
	    }
	    else
	    {
	      fprintf(cdp_results_file, "INV_F\t%s\t%d\t%d\t%6.2f\t%e\t%e\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%e\t%e\n", cdp_chr_name, cdp_inv_f_list2_start[cdp_a_loop] + 1, cdp_inv_f_list2_end[cdp_a_loop] + 1, cdp_inv_f_list2_dist[cdp_a_loop], cdp_inv_f_list2_start_binom_cdf[cdp_a_loop], cdp_inv_f_list2_end_binom_cdf[cdp_a_loop], cdp_inv_f_list2_start_inv[cdp_a_loop], cdp_inv_f_list2_end_inv[cdp_a_loop], cdp_inv_f_list2_start_rd[cdp_a_loop], cdp_inv_f_list2_end_rd[cdp_a_loop], cdp_inv_f_list2_start_conc[cdp_a_loop], cdp_inv_f_list2_end_conc[cdp_a_loop], cdp_inv_f_list2_start_other_len[cdp_a_loop], cdp_inv_f_list2_end_other_len[cdp_a_loop], cdp_inv_f_list2_start_read_start[cdp_a_loop] + 1, cdp_inv_f_list2_start_read_end[cdp_a_loop] + 1, cdp_inv_f_list2_end_read_start[cdp_a_loop] + 1, cdp_inv_f_list2_end_read_end[cdp_a_loop] + 1, cdp_inv_f_list2_start_hez_binom_cdf[cdp_a_loop], cdp_inv_f_list2_end_hez_binom_cdf[cdp_a_loop]);
	    }
	  }
	}
      }


      for(cdp_a_loop=0;cdp_a_loop<cdp_inv_r_list2_index;cdp_a_loop++)
      {
	cdp_overlap = 0;
	if( cdp_inv_r_list2_start_binom_cdf[cdp_a_loop] <= g_pval_threshold && cdp_inv_r_list2_end_binom_cdf[cdp_a_loop] <= g_pval_threshold && (double) cdp_inv_r_list2_start_inv[cdp_a_loop] / (double) cdp_inv_r_list2_start_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor && (double) cdp_inv_r_list2_end_inv[cdp_a_loop] / (double) cdp_inv_r_list2_end_rd[cdp_a_loop] >= g_min_sv_ratio*cdp_add_factor )
	{
	  for(cdp_b_loop=0;cdp_b_loop<cdp_inv_f_list2_index;cdp_b_loop++)
	  {
	    if( abs(cdp_inv_r_list2_start[cdp_a_loop] - cdp_inv_f_list2_start[cdp_b_loop]) <  g_insert_max_size - 2*g_lseq && abs(cdp_inv_r_list2_end[cdp_a_loop] - cdp_inv_f_list2_end[cdp_b_loop]) <  g_insert_max_size - 2*g_lseq )
	    {
	      if( (cdp_inv_r_list2_start[cdp_a_loop] >= cdp_inv_f_list2_start[cdp_b_loop] && cdp_inv_r_list2_start[cdp_a_loop] <= cdp_inv_f_list2_end[cdp_b_loop]) || (cdp_inv_f_list2_start[cdp_b_loop] >= cdp_inv_r_list2_start[cdp_a_loop] && cdp_inv_f_list2_start[cdp_b_loop] <= cdp_inv_r_list2_end[cdp_a_loop]) )
	      {
		if( cdp_inv_f_list2_start_binom_cdf[cdp_b_loop] * cdp_inv_f_list2_end_binom_cdf[cdp_b_loop] <= cdp_inv_r_list2_start_binom_cdf[cdp_a_loop] * cdp_inv_r_list2_end_binom_cdf[cdp_a_loop] )
		{
		  cdp_overlap = 1;
		  break;
		}
	      }
	    }
	  }

	  cdp_inv_rd1_ave = 0;
	  for(cdp_b_loop=cdp_inv_r_list2_start_read_start[cdp_a_loop];cdp_b_loop<cdp_inv_r_list2_start_read_end[cdp_a_loop] + g_lseq;cdp_b_loop++)
	  {
	    cdp_inv_rd1_ave += caf_rd_rd_list[cdp_b_loop] + caf_rd_low_mq_rd_list[cdp_b_loop];
	  }
	  cdp_inv_rd1_ave = cdp_inv_rd1_ave / (cdp_inv_r_list2_start_read_end[cdp_a_loop] + g_lseq - cdp_inv_r_list2_start_read_start[cdp_a_loop]);
	  cdp_inv_rd2_ave = 0;
	  for(cdp_b_loop=cdp_inv_r_list2_end_read_start[cdp_a_loop];cdp_b_loop<cdp_inv_r_list2_end_read_end[cdp_a_loop] + g_lseq;cdp_b_loop++)
	  {
	    cdp_inv_rd2_ave += caf_rd_rd_list[cdp_b_loop] + caf_rd_low_mq_rd_list[cdp_b_loop];
	  }
	  cdp_inv_rd2_ave = cdp_inv_rd2_ave / (cdp_inv_r_list2_end_read_end[cdp_a_loop] + g_lseq - cdp_inv_r_list2_end_read_start[cdp_a_loop]);
	  if( cdp_overlap == 0 && cdp_inv_rd1_ave/cdp_inv_rd2_ave <= g_max_inv_rd_diff && cdp_inv_rd2_ave/cdp_inv_rd1_ave <= g_max_inv_rd_diff )
	  {
	    if( g_vcf == 1 )
	    {
			
			float p_value_inv_r = cdp_inv_r_list2_end_binom_cdf[cdp_a_loop];
			if (cdp_inv_r_list2_start_binom_cdf[cdp_a_loop] > cdp_inv_r_list2_end_binom_cdf[cdp_a_loop]) {
				p_value_inv_r = cdp_inv_r_list2_start_binom_cdf[cdp_a_loop];
			}
			if (p_value_inv_r != 0)
			{
				int qual_entry = (int) -10 * log10(p_value_inv_r);
				vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
				vcf_sv_data_list[sort_sv_count].pos = cdp_inv_r_list2_start[cdp_a_loop];
				sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INV>\t%d\t.\tSVTYPE=INV;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_inv_r_list2_start[cdp_a_loop] - 1]), qual_entry, cdp_inv_r_list2_end[cdp_a_loop] + 1, unknown_gt_entry, cdp_inv_r_list2_start_binom_cdf[cdp_a_loop], cdp_inv_r_list2_end_binom_cdf[cdp_a_loop], (double)cdp_inv_r_list2_start_inv[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_inv_r_list2_end_inv[cdp_a_loop]/(double)cdp_add_factor, cdp_inv_r_list2_start_rd[cdp_a_loop], cdp_inv_r_list2_end_rd[cdp_a_loop], cdp_inv_r_list2_start_conc[cdp_a_loop], cdp_inv_r_list2_end_conc[cdp_a_loop], cdp_inv_r_list2_start_other_len[cdp_a_loop], cdp_inv_r_list2_end_other_len[cdp_a_loop], cdp_inv_r_list2_start_read_start[cdp_a_loop] + 1, cdp_inv_r_list2_start_read_end[cdp_a_loop] + 1, cdp_inv_r_list2_end_read_start[cdp_a_loop] + 1, cdp_inv_r_list2_end_read_end[cdp_a_loop] + 1);
				sort_sv_count += 1;
			} else {
				vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
				vcf_sv_data_list[sort_sv_count].pos = cdp_inv_r_list2_start[cdp_a_loop];
				sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INV>\t%s\t.\tSVTYPE=INV;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_inv_r_list2_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_inv_r_list2_end[cdp_a_loop] + 1, unknown_gt_entry, cdp_inv_r_list2_start_binom_cdf[cdp_a_loop], cdp_inv_r_list2_end_binom_cdf[cdp_a_loop], (double)cdp_inv_r_list2_start_inv[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_inv_r_list2_end_inv[cdp_a_loop]/(double)cdp_add_factor, cdp_inv_r_list2_start_rd[cdp_a_loop], cdp_inv_r_list2_end_rd[cdp_a_loop], cdp_inv_r_list2_start_conc[cdp_a_loop], cdp_inv_r_list2_end_conc[cdp_a_loop], cdp_inv_r_list2_start_other_len[cdp_a_loop], cdp_inv_r_list2_end_other_len[cdp_a_loop], cdp_inv_r_list2_start_read_start[cdp_a_loop] + 1, cdp_inv_r_list2_start_read_end[cdp_a_loop] + 1, cdp_inv_r_list2_end_read_start[cdp_a_loop] + 1, cdp_inv_r_list2_end_read_end[cdp_a_loop] + 1);
				sort_sv_count += 1;
			}
	    }
	    else
	    {
	      fprintf(cdp_results_file, "INV_R\t%s\t%d\t%d\t%6.2f\t%e\t%e\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%e\t%e\n", cdp_chr_name, cdp_inv_r_list2_start[cdp_a_loop] + 1, cdp_inv_r_list2_end[cdp_a_loop] + 1, cdp_inv_r_list2_dist[cdp_a_loop], cdp_inv_r_list2_start_binom_cdf[cdp_a_loop], cdp_inv_r_list2_end_binom_cdf[cdp_a_loop], cdp_inv_r_list2_start_inv[cdp_a_loop], cdp_inv_r_list2_end_inv[cdp_a_loop], cdp_inv_r_list2_start_rd[cdp_a_loop], cdp_inv_r_list2_end_rd[cdp_a_loop], cdp_inv_r_list2_start_conc[cdp_a_loop], cdp_inv_r_list2_end_conc[cdp_a_loop], cdp_inv_r_list2_start_other_len[cdp_a_loop], cdp_inv_r_list2_end_other_len[cdp_a_loop], cdp_inv_r_list2_start_read_start[cdp_a_loop] + 1, cdp_inv_r_list2_start_read_end[cdp_a_loop] + 1, cdp_inv_r_list2_end_read_start[cdp_a_loop] + 1, cdp_inv_r_list2_end_read_end[cdp_a_loop] + 1, cdp_inv_r_list2_start_hez_binom_cdf[cdp_a_loop], cdp_inv_r_list2_end_hez_binom_cdf[cdp_a_loop]);
	    }
	  }
	}
      }



      cdp_ins_list2_index = 0;
      cdp_ins_begin = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_ins_list_index;cdp_a_loop++)
      {
	if( cdp_ins_begin == 1 )
	{
	  if( cdp_ins_list_start[cdp_a_loop] > cdp_ins_list2_start[cdp_ins_list2_index - 1] + g_insert_max_size - 2*g_lseq || cdp_ins_list_start[cdp_a_loop] > cdp_ins_list2_end[cdp_ins_list2_index - 1] + g_insert_max_size - 2*g_lseq || cdp_ins_list_end[cdp_a_loop] > cdp_ins_list2_start[cdp_ins_list2_index - 1] + g_insert_max_size - 2*g_lseq || cdp_ins_list_end[cdp_a_loop] > cdp_ins_list2_end[cdp_ins_list2_index - 1] + g_insert_max_size - 2*g_lseq )
	  {
	    cdp_ins_begin = 0;
	  }
	  else
	  {
	    if( cdp_ins_list_start_binom_cdf[cdp_a_loop] <= cdp_ins_list2_start_binom_cdf[cdp_ins_list2_index - 1] && cdp_ins_list_start[cdp_a_loop] >= 0 && cdp_ins_list_end_binom_cdf[cdp_a_loop] <= cdp_ins_list2_end_binom_cdf[cdp_ins_list2_index - 1] && cdp_ins_list_end[cdp_a_loop] >= 0 )
	    {
	      cdp_ins_list2_start[cdp_ins_list2_index - 1] = cdp_ins_list_start[cdp_a_loop];
	      cdp_ins_list2_end[cdp_ins_list2_index - 1] = cdp_ins_list_end[cdp_a_loop];
	      cdp_ins_list2_start_binom_cdf[cdp_ins_list2_index - 1] = cdp_ins_list_start_binom_cdf[cdp_a_loop];
	      cdp_ins_list2_end_binom_cdf[cdp_ins_list2_index - 1] = cdp_ins_list_end_binom_cdf[cdp_a_loop];
	      cdp_ins_list2_start_ins[cdp_ins_list2_index - 1] = cdp_ins_list_start_ins[cdp_a_loop];
	      cdp_ins_list2_end_ins[cdp_ins_list2_index - 1] = cdp_ins_list_end_ins[cdp_a_loop];
	      cdp_ins_list2_start_rd[cdp_ins_list2_index - 1] = cdp_ins_list_start_rd[cdp_a_loop];
	      cdp_ins_list2_end_rd[cdp_ins_list2_index - 1] = cdp_ins_list_end_rd[cdp_a_loop];
	      cdp_ins_list2_start_conc[cdp_ins_list2_index - 1] = cdp_ins_list_start_conc[cdp_a_loop];
	      cdp_ins_list2_end_conc[cdp_ins_list2_index - 1] = cdp_ins_list_end_conc[cdp_a_loop];
	      cdp_ins_list2_start_other_len[cdp_ins_list2_index - 1] = cdp_ins_list_start_other_len[cdp_a_loop];
	      cdp_ins_list2_end_other_len[cdp_ins_list2_index - 1] = cdp_ins_list_end_other_len[cdp_a_loop];
	    }
	  }
	}
	if( cdp_ins_begin == 0 )
	{
	  if( cdp_ins_list_start[cdp_a_loop] >= 0 && cdp_ins_list_end[cdp_a_loop] >= 0 )
	  {
	    if( cdp_ins_list_index < g_sv_list2_len - 1 )
	    {
	      cdp_ins_begin = 1;
	      cdp_ins_list2_start[cdp_ins_list2_index] = cdp_ins_list_start[cdp_a_loop];
	      cdp_ins_list2_end[cdp_ins_list2_index] = cdp_ins_list_end[cdp_a_loop];
	      cdp_ins_list2_start_binom_cdf[cdp_ins_list2_index] = cdp_ins_list_start_binom_cdf[cdp_a_loop];
	      cdp_ins_list2_end_binom_cdf[cdp_ins_list2_index] = cdp_ins_list_end_binom_cdf[cdp_a_loop];
	      cdp_ins_list2_start_ins[cdp_ins_list2_index] = cdp_ins_list_start_ins[cdp_a_loop];
	      cdp_ins_list2_end_ins[cdp_ins_list2_index] = cdp_ins_list_end_ins[cdp_a_loop];
	      cdp_ins_list2_start_rd[cdp_ins_list2_index] = cdp_ins_list_start_rd[cdp_a_loop];
	      cdp_ins_list2_end_rd[cdp_ins_list2_index] = cdp_ins_list_end_rd[cdp_a_loop];
	      cdp_ins_list2_start_conc[cdp_ins_list2_index] = cdp_ins_list_start_conc[cdp_a_loop];
	      cdp_ins_list2_end_conc[cdp_ins_list2_index] = cdp_ins_list_end_conc[cdp_a_loop];
	      cdp_ins_list2_start_other_len[cdp_ins_list2_index] = cdp_ins_list_start_other_len[cdp_a_loop];
	      cdp_ins_list2_end_other_len[cdp_ins_list2_index] = cdp_ins_list_end_other_len[cdp_a_loop];
	      cdp_ins_list2_index += 1;
	    }
	    else
	    {
	      g_sv_list2_len_over = 1;
	    }

	  }
	}
      }
      for(cdp_a_loop=0;cdp_a_loop<cdp_ins_list2_index;cdp_a_loop++)
      {
	if( cdp_ins_list2_start_binom_cdf[cdp_a_loop] <= g_pval_insertion && cdp_ins_list2_end_binom_cdf[cdp_a_loop] <= g_pval_insertion && abs(cdp_ins_list2_end[cdp_a_loop] - cdp_ins_list2_start[cdp_a_loop]) <= g_max_ins_range )
	{
	  if( g_vcf == 1 )
	  {

		float p_value_ins = cdp_ins_list2_end_binom_cdf[cdp_a_loop];
		if (cdp_ins_list2_start_binom_cdf[cdp_a_loop] > cdp_ins_list2_end_binom_cdf[cdp_a_loop]) {
			p_value_ins = cdp_ins_list2_start_binom_cdf[cdp_a_loop];
		}

		char gt_entry[10];
		if (p_value_ins < ins_homozygous_p_value_cutoff)
		{
			strcpy(gt_entry, homozygous_gt_entry);
		} else if (p_value_ins > ins_unknown_p_value_cutoff)
		{
			strcpy(gt_entry, unknown_gt_entry);
		} else {
			strcpy(gt_entry, heterozygous_gt_entry);
		}

		if (p_value_ins != 0)
		{
			int qual_entry = (int) -10 * log10(p_value_ins);
			vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
			vcf_sv_data_list[sort_sv_count].pos = cdp_ins_list2_start[cdp_a_loop];
			sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INS>\t%d\t.\tSVTYPE=INS;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_ins_list2_start[cdp_a_loop] - 1]), qual_entry, cdp_ins_list2_start[cdp_a_loop], gt_entry, cdp_ins_list2_start_binom_cdf[cdp_a_loop], cdp_ins_list2_end_binom_cdf[cdp_a_loop], (double)cdp_ins_list2_start_ins[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_ins_list2_end_ins[cdp_a_loop]/(double)cdp_add_factor, cdp_ins_list2_start_rd[cdp_a_loop], cdp_ins_list2_end_rd[cdp_a_loop], cdp_ins_list2_start_conc[cdp_a_loop], cdp_ins_list2_end_conc[cdp_a_loop], cdp_ins_list2_start_other_len[cdp_a_loop], cdp_ins_list2_end_other_len[cdp_a_loop]);
			sort_sv_count += 1;
		} else {
			vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
			vcf_sv_data_list[sort_sv_count].pos = cdp_ins_list2_start[cdp_a_loop];
			sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INS>\t%s\t.\tSVTYPE=INS;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_ins_list2_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_ins_list2_start[cdp_a_loop], gt_entry, cdp_ins_list2_start_binom_cdf[cdp_a_loop], cdp_ins_list2_end_binom_cdf[cdp_a_loop], (double)cdp_ins_list2_start_ins[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_ins_list2_end_ins[cdp_a_loop]/(double)cdp_add_factor, cdp_ins_list2_start_rd[cdp_a_loop], cdp_ins_list2_end_rd[cdp_a_loop], cdp_ins_list2_start_conc[cdp_a_loop], cdp_ins_list2_end_conc[cdp_a_loop], cdp_ins_list2_start_other_len[cdp_a_loop], cdp_ins_list2_end_other_len[cdp_a_loop]);
			sort_sv_count += 1;
		}
	  }
	  else
	  {
	    fprintf(cdp_results_file, "INS\t%s\t%d\t%d\t\t%e\t%e\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", cdp_chr_name, cdp_ins_list2_start[cdp_a_loop] + 1, cdp_ins_list2_end[cdp_a_loop] + 1, cdp_ins_list2_start_binom_cdf[cdp_a_loop], cdp_ins_list2_end_binom_cdf[cdp_a_loop], cdp_ins_list2_start_ins[cdp_a_loop], cdp_ins_list2_end_ins[cdp_a_loop], cdp_ins_list2_start_rd[cdp_a_loop], cdp_ins_list2_end_rd[cdp_a_loop], cdp_ins_list2_start_conc[cdp_a_loop], cdp_ins_list2_end_conc[cdp_a_loop], cdp_ins_list2_start_other_len[cdp_a_loop], cdp_ins_list2_end_other_len[cdp_a_loop]);
	  }
	}
      }



      cdp_ctx_f_list2_index = 0;
      cdp_ctx_f_begin = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_ctx_f_list_index;cdp_a_loop++)
      {
	if( cdp_ctx_f_begin == 1 )
	{
	  if( cdp_ctx_f_list[cdp_a_loop] > cdp_ctx_f_list2[cdp_ctx_f_list2_index - 1] + g_insert_max_size - 2*g_lseq )
	  {
	    cdp_ctx_f_begin = 0;
	  }
	  else
	  {
	    if( ((cdp_ctx_f_list_binom_cdf[cdp_a_loop] < cdp_ctx_f_list2_binom_cdf[cdp_ctx_f_list2_index - 1] && cdp_ctx_f_list2_ctx_f[cdp_ctx_f_list2_index - 1] <= cdp_ctx_f_list_ctx_f[cdp_a_loop]) || (cdp_ctx_f_list_binom_cdf[cdp_a_loop] == cdp_ctx_f_list2_binom_cdf[cdp_ctx_f_list2_index - 1] && cdp_ctx_f_list2_ctx_f[cdp_ctx_f_list2_index - 1] < cdp_ctx_f_list_ctx_f[cdp_a_loop])) && cdp_ctx_f_list[cdp_a_loop] >= 0 )
	    {
	      cdp_ctx_f_list2[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list[cdp_a_loop];
	      cdp_ctx_f_list2_ctx_f[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_ctx_f[cdp_a_loop];
	      cdp_ctx_f_list2_binom_cdf[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_binom_cdf[cdp_a_loop];
	      cdp_ctx_f_list2_hez_binom_cdf[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_hez_binom_cdf[cdp_a_loop];
	      cdp_ctx_f_list2_rd[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_rd[cdp_a_loop];
	      cdp_ctx_f_list2_conc[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_conc[cdp_a_loop];
	      cdp_ctx_f_list2_other_len[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_other_len[cdp_a_loop];
	      cdp_ctx_f_list2_mchr[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_mchr[cdp_a_loop];
	      cdp_ctx_f_list2_mpos[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_mpos[cdp_a_loop];
	      cdp_ctx_f_list2_read_start[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_read_start[cdp_a_loop];
	      cdp_ctx_f_list2_read_end[cdp_ctx_f_list2_index - 1] = cdp_ctx_f_list_read_end[cdp_a_loop];
	    }
	  }
	}
	if( cdp_ctx_f_begin == 0 )
	{
	  if( cdp_ctx_f_list[cdp_a_loop] >= 0 )
	  {
	    if( cdp_ctx_f_list2_index < g_sv_list2_len - 1 )
	    {
	      cdp_ctx_f_begin = 1;
	      cdp_ctx_f_list2[cdp_ctx_f_list2_index] = cdp_ctx_f_list[cdp_a_loop];
	      cdp_ctx_f_list2_ctx_f[cdp_ctx_f_list2_index] = cdp_ctx_f_list_ctx_f[cdp_a_loop];
	      cdp_ctx_f_list2_binom_cdf[cdp_ctx_f_list2_index] = cdp_ctx_f_list_binom_cdf[cdp_a_loop];
	      cdp_ctx_f_list2_hez_binom_cdf[cdp_ctx_f_list2_index] = cdp_ctx_f_list_hez_binom_cdf[cdp_a_loop];
	      cdp_ctx_f_list2_rd[cdp_ctx_f_list2_index] = cdp_ctx_f_list_rd[cdp_a_loop];
	      cdp_ctx_f_list2_conc[cdp_ctx_f_list2_index] = cdp_ctx_f_list_conc[cdp_a_loop];
	      cdp_ctx_f_list2_other_len[cdp_ctx_f_list2_index] = cdp_ctx_f_list_other_len[cdp_a_loop];
	      cdp_ctx_f_list2_mchr[cdp_ctx_f_list2_index] = cdp_ctx_f_list_mchr[cdp_a_loop];
	      cdp_ctx_f_list2_mpos[cdp_ctx_f_list2_index] = cdp_ctx_f_list_mpos[cdp_a_loop];
	      cdp_ctx_f_list2_read_start[cdp_ctx_f_list2_index] = cdp_ctx_f_list_read_start[cdp_a_loop];
	      cdp_ctx_f_list2_read_end[cdp_ctx_f_list2_index] = cdp_ctx_f_list_read_end[cdp_a_loop];
	      cdp_ctx_f_list2_index += 1;
	    }
	    else
	    {
	      g_sv_list2_len_over = 1;
	    }

	  }
	}
      }
      for(cdp_a_loop=0;cdp_a_loop<cdp_ctx_f_list2_index;cdp_a_loop++)
      {
	if( (cdp_ctx_f_list2_binom_cdf[cdp_a_loop] <= g_pval_threshold || cdp_ctx_f_list2_hez_binom_cdf[cdp_a_loop] <= g_pval_threshold) && (double) cdp_ctx_f_list2_ctx_f[cdp_a_loop] / (double) cdp_ctx_f_list2_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor )
	{
	  fprintf(cdp_results_file_ctx, "CTX_F\t%s\t%d\t%e\t%.1f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%e\n", cdp_chr_name, cdp_ctx_f_list2[cdp_a_loop], cdp_ctx_f_list2_binom_cdf[cdp_a_loop], (double)cdp_ctx_f_list2_ctx_f[cdp_a_loop]/(double)cdp_add_factor, cdp_ctx_f_list2_rd[cdp_a_loop], cdp_ctx_f_list2_conc[cdp_a_loop], cdp_ctx_f_list2_other_len[cdp_a_loop], cdp_ctx_f_list2_mchr[cdp_a_loop], cdp_ctx_f_list2_mpos[cdp_a_loop], cdp_ctx_f_list2_read_start[cdp_a_loop], cdp_ctx_f_list2_read_end[cdp_a_loop], cdp_ctx_f_list2_hez_binom_cdf[cdp_a_loop]);
	}
      }



      cdp_ctx_r_list2_index = 0;
      cdp_ctx_r_begin = 0;
      for(cdp_a_loop=0;cdp_a_loop<cdp_ctx_r_list_index;cdp_a_loop++)
      {
	if( cdp_ctx_r_begin == 1 )
	{
	  if( cdp_ctx_r_list[cdp_a_loop] > cdp_ctx_r_list2[cdp_ctx_r_list2_index - 1] + g_insert_max_size - 2*g_lseq )
	  {
	    cdp_ctx_r_begin = 0;
	  }
	  else
	  {
	    if( ((cdp_ctx_r_list_binom_cdf[cdp_a_loop] < cdp_ctx_r_list2_binom_cdf[cdp_ctx_r_list2_index - 1] && cdp_ctx_r_list2_ctx_r[cdp_ctx_r_list2_index - 1] <= cdp_ctx_r_list_ctx_r[cdp_a_loop]) || (cdp_ctx_r_list_binom_cdf[cdp_a_loop] == cdp_ctx_r_list2_binom_cdf[cdp_ctx_r_list2_index - 1] && cdp_ctx_r_list2_ctx_r[cdp_ctx_r_list2_index - 1] < cdp_ctx_r_list_ctx_r[cdp_a_loop])) && cdp_ctx_r_list[cdp_a_loop] >= 0 )
	    {
	      cdp_ctx_r_list2[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list[cdp_a_loop];
	      cdp_ctx_r_list2_ctx_r[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_ctx_r[cdp_a_loop];
	      cdp_ctx_r_list2_binom_cdf[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_binom_cdf[cdp_a_loop];
	      cdp_ctx_r_list2_hez_binom_cdf[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_hez_binom_cdf[cdp_a_loop];
	      cdp_ctx_r_list2_rd[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_rd[cdp_a_loop];
	      cdp_ctx_r_list2_conc[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_conc[cdp_a_loop];
	      cdp_ctx_r_list2_other_len[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_other_len[cdp_a_loop];
	      cdp_ctx_r_list2_mchr[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_mchr[cdp_a_loop];
	      cdp_ctx_r_list2_mpos[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_mpos[cdp_a_loop];
	      cdp_ctx_r_list2_read_start[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_read_start[cdp_a_loop];
	      cdp_ctx_r_list2_read_end[cdp_ctx_r_list2_index - 1] = cdp_ctx_r_list_read_end[cdp_a_loop];
	    }
	  }
	}
	if( cdp_ctx_r_begin == 0 )
	{
	  if( cdp_ctx_r_list[cdp_a_loop] >= 0 )
	  {
	    if( cdp_ctx_r_list2_index < g_sv_list2_len - 1 )
	    {
	      cdp_ctx_r_begin = 1;
	      cdp_ctx_r_list2[cdp_ctx_r_list2_index] = cdp_ctx_r_list[cdp_a_loop];
	      cdp_ctx_r_list2_ctx_r[cdp_ctx_r_list2_index] = cdp_ctx_r_list_ctx_r[cdp_a_loop];
	      cdp_ctx_r_list2_binom_cdf[cdp_ctx_r_list2_index] = cdp_ctx_r_list_binom_cdf[cdp_a_loop];
	      cdp_ctx_r_list2_hez_binom_cdf[cdp_ctx_r_list2_index] = cdp_ctx_r_list_hez_binom_cdf[cdp_a_loop];
	      cdp_ctx_r_list2_rd[cdp_ctx_r_list2_index] = cdp_ctx_r_list_rd[cdp_a_loop];
	      cdp_ctx_r_list2_conc[cdp_ctx_r_list2_index] = cdp_ctx_r_list_conc[cdp_a_loop];
	      cdp_ctx_r_list2_other_len[cdp_ctx_r_list2_index] = cdp_ctx_r_list_other_len[cdp_a_loop];
	      cdp_ctx_r_list2_mchr[cdp_ctx_r_list2_index] = cdp_ctx_r_list_mchr[cdp_a_loop];
	      cdp_ctx_r_list2_mpos[cdp_ctx_r_list2_index] = cdp_ctx_r_list_mpos[cdp_a_loop];
	      cdp_ctx_r_list2_read_start[cdp_ctx_r_list2_index] = cdp_ctx_r_list_read_start[cdp_a_loop];
	      cdp_ctx_r_list2_read_end[cdp_ctx_r_list2_index] = cdp_ctx_r_list_read_end[cdp_a_loop];
	      cdp_ctx_r_list2_index += 1;
	    }
	    else
	    {
	      g_sv_list2_len_over = 1;
	    }

	  }
	}
      }
      for(cdp_a_loop=0;cdp_a_loop<cdp_ctx_r_list2_index;cdp_a_loop++)
      {
	if( (cdp_ctx_r_list2_binom_cdf[cdp_a_loop] <= g_pval_threshold || cdp_ctx_r_list2_hez_binom_cdf[cdp_a_loop] <= g_pval_threshold) && (double) cdp_ctx_r_list2_ctx_r[cdp_a_loop] / (double) cdp_ctx_r_list2_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor )
	{
	  fprintf(cdp_results_file_ctx, "CTX_R\t%s\t%d\t%e\t%.1f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%e\n", cdp_chr_name, cdp_ctx_r_list2[cdp_a_loop], cdp_ctx_r_list2_binom_cdf[cdp_a_loop], (double)cdp_ctx_r_list2_ctx_r[cdp_a_loop]/(double)cdp_add_factor, cdp_ctx_r_list2_rd[cdp_a_loop], cdp_ctx_r_list2_conc[cdp_a_loop], cdp_ctx_r_list2_other_len[cdp_a_loop], cdp_ctx_r_list2_mchr[cdp_a_loop], cdp_ctx_r_list2_mpos[cdp_a_loop], cdp_ctx_r_list2_read_start[cdp_a_loop], cdp_ctx_r_list2_read_end[cdp_a_loop], cdp_ctx_r_list2_hez_binom_cdf[cdp_a_loop]);
	}
      }


      int cdp_homopolymer = 0;
      int cdp_homopolymer2 = 0;
      char cdp_homopolymer_char;

      for(cdp_a_loop=0;cdp_a_loop<cdp_indel_i_list_index;cdp_a_loop++)
      {
	if( cdp_indel_i_list_start_binom_cdf[cdp_a_loop] <= g_pval_threshold && (double)cdp_indel_i_list_start_i[cdp_a_loop]/(double)cdp_indel_i_list_start_rd[cdp_a_loop] > g_min_indel_ratio*(double)cdp_add_factor )
	{
	  cdp_homopolymer = 1;
	  cdp_homopolymer_char = cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop]];
	  for(cdp_b_loop=1;cdp_b_loop<20;cdp_b_loop++)
	  {
	    if( cdp_indel_i_list_start[cdp_a_loop] - cdp_b_loop >= 0 )
	    {
	      if( cdp_homopolymer_char == cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] - cdp_b_loop] )
	      {
		cdp_homopolymer += 1;
	      }
	      else
	      {
		break;
	      }
	    }
	    else
	    {
	      break;
	    }
	  }
	  cdp_homopolymer2 = 1;
	  if( cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop]] + 1 < cdp_chr_fasta_len )
	  {
	    cdp_homopolymer_char = cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop]] + 1;
	    for(cdp_b_loop=1;cdp_b_loop<20;cdp_b_loop++)
	    {
	      if( cdp_indel_i_list_start[cdp_a_loop] + cdp_b_loop + 1 < cdp_chr_fasta_len )
	      {
		if( cdp_homopolymer_char == cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] + cdp_b_loop + 1] )
		{
		  cdp_homopolymer2 += 1;
		}
		else
		{
		  break;
		}
	      }
	      else
	      {
		break;
	      }
	    }
	  }
	  if( cdp_homopolymer2 > cdp_homopolymer )
	  {
	    cdp_homopolymer = cdp_homopolymer2;
	  }
	  if( cdp_homopolymer <= g_max_homopolymer )
	  {


		if( cdp_indel_i_list_dist[cdp_a_loop] <= g_indel_i_seq_len )
	    {
	      for(cdp_b_loop=0;cdp_b_loop<cdp_indel_i_list_dist[cdp_a_loop];cdp_b_loop++)
	      {
		cdp_ins_string[cdp_b_loop] = toupper(cdp_indel_i_list_seq[cdp_b_loop][cdp_a_loop]);
	      }
	      cdp_ins_string[cdp_indel_i_list_dist[cdp_a_loop]] = '\0';
	    }

	    if( g_vcf == 1 )
	    {

			char gt_entry[10];
			if (cdp_indel_i_list_start_hez_binom_cdf[cdp_a_loop] > indel_ins_homozygous_cutoff)
			{
				strcpy(gt_entry, homozygous_gt_entry);
			} else if (cdp_indel_i_list_start_hez_binom_cdf[cdp_a_loop] < indel_ins_unknown_cutoff)
			{
				strcpy(gt_entry, unknown_gt_entry);
			} else {
				strcpy(gt_entry, heterozygous_gt_entry);
			}

			if (cdp_indel_i_list_start_binom_cdf[cdp_a_loop] != 0)
			{
				int qual_entry = (int) -10 * log10(cdp_indel_i_list_start_binom_cdf[cdp_a_loop]);
				if (cdp_indel_i_list_dist[cdp_a_loop] <= cdp_ins_string_len)
				{
					vcf_indel_data_list[sort_indel_count].chr_name = cdp_chr_name;
					vcf_indel_data_list[sort_indel_count].pos = cdp_indel_i_list_start[cdp_a_loop];
					sprintf(vcf_indel_data_list[sort_indel_count].vcf_data, ".\t%c\t%c%s\t%d\t.\tTYPE=INDEL_INS;END=%d\tGT:SPR:SEV:SRD:SCO:ECO:SOT:EOT:SSC:HP\t%s:%.2e:%.1f:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] - 1]), toupper(cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] - 1]), cdp_ins_string, qual_entry, cdp_indel_i_list_start[cdp_a_loop], gt_entry, cdp_indel_i_list_start_binom_cdf[cdp_a_loop], (double)cdp_indel_i_list_start_i[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_i_list_start_rd[cdp_a_loop], cdp_indel_i_list_start_conc[cdp_a_loop], cdp_indel_i_list_end_conc[cdp_a_loop], cdp_indel_i_list_start_other_len[cdp_a_loop], cdp_indel_i_list_end_other_len[cdp_a_loop], cdp_indel_i_list_start_sc[cdp_a_loop], cdp_homopolymer);
					sort_indel_count += 1;
				} else {
					vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
					vcf_sv_data_list[sort_sv_count].pos = cdp_indel_i_list_start[cdp_a_loop];
					sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INS>\t%d\t.\tSVTYPE=INS;END=%d\tGT:SPR:SEV:SRD:SCO:ECO:SOT:EOT:SSC\t%s:%.2e:%.1f:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] - 1]), qual_entry, cdp_indel_i_list_start[cdp_a_loop], gt_entry, cdp_indel_i_list_start_binom_cdf[cdp_a_loop], (double)cdp_indel_i_list_start_i[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_i_list_start_rd[cdp_a_loop], cdp_indel_i_list_start_conc[cdp_a_loop], cdp_indel_i_list_end_conc[cdp_a_loop], cdp_indel_i_list_start_other_len[cdp_a_loop], cdp_indel_i_list_end_other_len[cdp_a_loop], cdp_indel_i_list_start_sc[cdp_a_loop]);
					sort_sv_count += 1;
				}
			} else {
				if (cdp_indel_i_list_dist[cdp_a_loop] <= cdp_ins_string_len)
				{
					vcf_indel_data_list[sort_indel_count].chr_name = cdp_chr_name;
					vcf_indel_data_list[sort_indel_count].pos = cdp_indel_i_list_start[cdp_a_loop];
					sprintf(vcf_indel_data_list[sort_indel_count].vcf_data, ".\t%c\t%c%s\t%s\t.\tTYPE=INDEL_INS;END=%d\tGT:SPR:SEV:SRD:SCO:ECO:SOT:EOT:SSC:HP\t%s:%.2e:%.1f:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] - 1]), toupper(cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] - 1]), cdp_ins_string, infinity_qual_entry, cdp_indel_i_list_start[cdp_a_loop], gt_entry, cdp_indel_i_list_start_binom_cdf[cdp_a_loop], (double)cdp_indel_i_list_start_i[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_i_list_start_rd[cdp_a_loop], cdp_indel_i_list_start_conc[cdp_a_loop], cdp_indel_i_list_end_conc[cdp_a_loop], cdp_indel_i_list_start_other_len[cdp_a_loop], cdp_indel_i_list_end_other_len[cdp_a_loop], cdp_indel_i_list_start_sc[cdp_a_loop], cdp_homopolymer);
					sort_indel_count += 1;
				} else {
					vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
					vcf_sv_data_list[sort_sv_count].pos = cdp_indel_i_list_start[cdp_a_loop];
					sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<INS>\t%s\t.\tSVTYPE=INS;END=%d\tGT:SPR:SEV:SRD:SCO:ECO:SOT:EOT:SSC\t%s:%.2e:%.1f:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_i_list_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_indel_i_list_start[cdp_a_loop], gt_entry, cdp_indel_i_list_start_binom_cdf[cdp_a_loop], (double)cdp_indel_i_list_start_i[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_i_list_start_rd[cdp_a_loop], cdp_indel_i_list_start_conc[cdp_a_loop], cdp_indel_i_list_end_conc[cdp_a_loop], cdp_indel_i_list_start_other_len[cdp_a_loop], cdp_indel_i_list_end_other_len[cdp_a_loop], cdp_indel_i_list_start_sc[cdp_a_loop]);
					sort_sv_count += 1;
				}
			}
	    }
	    else
	    {
	      fprintf(cdp_results_file, "INDEL_INS\t%s\t%d\t%d\t%d\t%e\t%e\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n", cdp_chr_name, cdp_indel_i_list_start[cdp_a_loop] + 1, cdp_indel_i_list_end[cdp_a_loop] + 1, cdp_indel_i_list_dist[cdp_a_loop], cdp_indel_i_list_start_binom_cdf[cdp_a_loop], cdp_indel_i_list_start_hez_binom_cdf[cdp_a_loop], cdp_indel_i_list_start_conc[cdp_a_loop], cdp_indel_i_list_end_conc[cdp_a_loop], cdp_indel_i_list_start_other_len[cdp_a_loop], cdp_indel_i_list_end_other_len[cdp_a_loop], cdp_indel_i_list_start_i[cdp_a_loop], cdp_indel_i_list_start_rd[cdp_a_loop], cdp_indel_i_list_start_sc[cdp_a_loop], cdp_homopolymer, cdp_snv_gt_string);
	    }
	  }
	}
      }



      if( g_internal == 1 )
      {
	printf("cdp_indel_d_list_index %d\n", cdp_indel_d_list_index);
      }
      for(cdp_a_loop=0;cdp_a_loop<cdp_indel_d_list_index;cdp_a_loop++)
      {
	cdp_overlap = 0;
	if( cdp_indel_d_list_start_binom_cdf[cdp_a_loop] <= g_pval_threshold && cdp_indel_d_list_end_binom_cdf[cdp_a_loop] <= g_pval_threshold && (double)cdp_indel_d_list_start_f[cdp_a_loop]/(double)cdp_indel_d_list_start_rd[cdp_a_loop] > g_min_indel_ratio*(double)cdp_add_factor && (double)cdp_indel_d_list_end_r[cdp_a_loop]/(double)cdp_indel_d_list_end_rd[cdp_a_loop] > g_min_indel_ratio*(double)cdp_add_factor )
	{
	  for(cdp_b_loop=0;cdp_b_loop<cdp_del_list2_index;cdp_b_loop++)
	  {
	    if( abs(cdp_del_list2_start[cdp_b_loop] - cdp_indel_d_list_start[cdp_a_loop]) <  g_insert_max_size - 2*g_lseq && abs(cdp_del_list2_end[cdp_b_loop] - cdp_indel_d_list_end[cdp_a_loop]) <  g_insert_max_size - 2*g_lseq )
	    {
	      cdp_overlap_ratio_1 = 0;
	      cdp_overlap_ratio_2 = 0;
	      if( cdp_del_list2_start[cdp_b_loop] >= cdp_indel_d_list_start[cdp_a_loop] && cdp_del_list2_start[cdp_b_loop] <= cdp_indel_d_list_end[cdp_a_loop] )
	      {
		if( cdp_del_list2_end[cdp_b_loop] >= cdp_indel_d_list_end[cdp_a_loop] )
		{
		  cdp_overlap_ratio_1 = (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_del_list2_start[cdp_b_loop]) / (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_del_list2_start[cdp_b_loop]) / (double) (cdp_del_list2_end[cdp_b_loop] - cdp_del_list2_start[cdp_b_loop]);
		}
		else
		{
		  cdp_overlap_ratio_1 = (double) (cdp_del_list2_end[cdp_b_loop] - cdp_del_list2_start[cdp_b_loop]) / (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_del_list2_end[cdp_a_loop] - cdp_del_list2_start[cdp_b_loop]) / (double) (cdp_del_list2_end[cdp_b_loop] - cdp_del_list2_start[cdp_b_loop]);
		}
	      }
	      else if( cdp_indel_d_list_start[cdp_a_loop] >= cdp_del_list2_start[cdp_b_loop] && cdp_indel_d_list_start[cdp_a_loop] <= cdp_del_list2_end[cdp_b_loop] )
	      {
		if( cdp_del_list2_end[cdp_b_loop] >= cdp_indel_d_list_end[cdp_a_loop] )
		{
		  cdp_overlap_ratio_1 = (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop]) / (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop]) / (double) (cdp_del_list2_end[cdp_b_loop] - cdp_del_list2_start[cdp_b_loop]);
		}
		else
		{
		  cdp_overlap_ratio_1 = (double) (cdp_del_list2_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_a_loop]) / (double) (cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_del_list2_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_a_loop]) / (double) (cdp_del_list2_end[cdp_b_loop] - cdp_del_list2_start[cdp_b_loop]);
		}
	      }
	      if( cdp_overlap_ratio_1 >= g_min_overlap_ratio && cdp_overlap_ratio_2 >= g_min_overlap_ratio && cdp_del_list2_start_binom_cdf[cdp_b_loop] * cdp_del_list2_end_binom_cdf[cdp_b_loop] < cdp_indel_d_list_start_binom_cdf[cdp_a_loop] * cdp_indel_d_list_end_binom_cdf[cdp_a_loop] )
	      {
		cdp_overlap = 1;
		break;
	      }
	    }
	  }
	  if( cdp_overlap == 0 )
	  {
	    cdp_homopolymer = 1;
	    if( cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop]] - 1 >= 0 )
	    {
	      cdp_homopolymer_char = cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - 1];
	      for(cdp_b_loop=1;cdp_b_loop<20;cdp_b_loop++)
	      {
		if( cdp_indel_d_list_start[cdp_a_loop] - cdp_b_loop - 1 >= 0 )
		{
		  if( cdp_homopolymer_char == cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - cdp_b_loop - 1] )
		  {
		    cdp_homopolymer += 1;
		  }
		  else
		  {
		    break;
		  }
		}
		else
		{
		  break;
		}
	      }
	    }
	    cdp_homopolymer2 = 1;
	    if( cdp_chr_fasta[cdp_indel_d_list_end[cdp_a_loop]] + 1 < cdp_chr_fasta_len )
	    {
	      cdp_homopolymer_char = cdp_chr_fasta[cdp_indel_d_list_end[cdp_a_loop]] + 1;
	      for(cdp_b_loop=1;cdp_b_loop<20;cdp_b_loop++)
	      {
		if( cdp_indel_d_list_end[cdp_a_loop] + cdp_b_loop + 1 < cdp_chr_fasta_len )
		{
		  if( cdp_homopolymer_char == cdp_chr_fasta[cdp_indel_d_list_end[cdp_a_loop] + cdp_b_loop + 1] )
		  {
		    cdp_homopolymer2 += 1;
		  }
		  else
		  {
		    break;
		  }
		}
		else
		{
		  break;
		}
	      }
	    }
	    if( cdp_homopolymer2 > cdp_homopolymer )
	    {
	      cdp_homopolymer = cdp_homopolymer2;
	    }
	    if( cdp_homopolymer <= g_max_homopolymer )
	    {
	      cdp_snv_cn = cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop] + 1;
	      if( cdp_snv_cn > 0 && cdp_snv_cn < cdp_del_string_len - 1 )
	      {
		for(cdp_b_loop=0;cdp_b_loop<cdp_snv_cn;cdp_b_loop++)
		{

		  cdp_del_string[cdp_b_loop] = toupper(cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop]+cdp_b_loop]);
		}
		cdp_del_string[cdp_snv_cn] = '\0';
	      }



	      if( g_vcf == 1 )
	      {
			float p_value_indel_del = cdp_indel_d_list_end_binom_cdf[cdp_a_loop];
			if (cdp_indel_d_list_start_binom_cdf[cdp_a_loop] > cdp_indel_d_list_end_binom_cdf[cdp_a_loop]) {
				p_value_indel_del = cdp_indel_d_list_start_binom_cdf[cdp_a_loop];
			}
			float avg_hez_binom_cdf = (cdp_indel_d_list_start_hez_binom_cdf[cdp_a_loop] + cdp_indel_d_list_end_hez_binom_cdf[cdp_a_loop])/2.0;

			char gt_entry[10];
			if (avg_hez_binom_cdf > indel_del_homozygous_cutoff)
			{
				strcpy(gt_entry, homozygous_gt_entry);
			} else if (avg_hez_binom_cdf < indel_del_unknown_cutoff)
			{
				strcpy(gt_entry, unknown_gt_entry);
			} else {
				strcpy(gt_entry, heterozygous_gt_entry);
			}

			if (p_value_indel_del != 0)
			{
				int qual_entry = (int) -10 * log10(p_value_indel_del);
				if( cdp_snv_cn > 0 && cdp_snv_cn <= cdp_del_string_len )
				{

					vcf_indel_data_list[sort_indel_count].chr_name = cdp_chr_name;
					vcf_indel_data_list[sort_indel_count].pos = cdp_indel_d_list_start[cdp_a_loop];
					sprintf(vcf_indel_data_list[sort_indel_count].vcf_data, ".\t%c%s\t%c\t%d\t.\tTYPE=INDEL_DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SSC:ESC:HP\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - 1]), cdp_del_string, toupper(cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - 1]), qual_entry, cdp_indel_d_list_end[cdp_a_loop] + 1, gt_entry, cdp_indel_d_list_start_binom_cdf[cdp_a_loop], cdp_indel_d_list_end_binom_cdf[cdp_a_loop], (double)cdp_indel_d_list_start_f[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_indel_d_list_end_r[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_d_list_start_conc[cdp_a_loop], cdp_indel_d_list_end_conc[cdp_a_loop], cdp_indel_d_list_start_other_len[cdp_a_loop], cdp_indel_d_list_end_other_len[cdp_a_loop], cdp_indel_d_list_start_rd[cdp_a_loop], cdp_indel_d_list_end_rd[cdp_a_loop], cdp_indel_d_list_start_sc[cdp_a_loop], cdp_indel_d_list_end_sc[cdp_a_loop], cdp_homopolymer);
					sort_indel_count += 1;
				}
				else
				{

					vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
					vcf_sv_data_list[sort_sv_count].pos = cdp_indel_d_list_start[cdp_a_loop];
					sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%d\t.\tSVTYPE=DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SSC:ESC\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - 1]), qual_entry, cdp_indel_d_list_end[cdp_a_loop] + 1, gt_entry, cdp_indel_d_list_start_binom_cdf[cdp_a_loop], cdp_indel_d_list_end_binom_cdf[cdp_a_loop], (double)cdp_indel_d_list_start_f[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_indel_d_list_end_r[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_d_list_start_conc[cdp_a_loop], cdp_indel_d_list_end_conc[cdp_a_loop], cdp_indel_d_list_start_other_len[cdp_a_loop], cdp_indel_d_list_end_other_len[cdp_a_loop], cdp_indel_d_list_start_rd[cdp_a_loop], cdp_indel_d_list_end_rd[cdp_a_loop], cdp_indel_d_list_start_sc[cdp_a_loop], cdp_indel_d_list_end_sc[cdp_a_loop]);
					sort_sv_count += 1;
				}
			} else {
				if( cdp_snv_cn > 0 && cdp_snv_cn <= cdp_del_string_len )
				{
					vcf_indel_data_list[sort_indel_count].chr_name = cdp_chr_name;
					vcf_indel_data_list[sort_indel_count].pos = cdp_indel_d_list_start[cdp_a_loop];
					sprintf(vcf_indel_data_list[sort_indel_count].vcf_data, ".\t%c%s\t%c\t%s\t.\tTYPE=INDEL_DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SSC:ESC:HP\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - 1]), cdp_del_string, toupper(cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_indel_d_list_end[cdp_a_loop] + 1, gt_entry, cdp_indel_d_list_start_binom_cdf[cdp_a_loop], cdp_indel_d_list_end_binom_cdf[cdp_a_loop], (double)cdp_indel_d_list_start_f[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_indel_d_list_end_r[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_d_list_start_conc[cdp_a_loop], cdp_indel_d_list_end_conc[cdp_a_loop], cdp_indel_d_list_start_other_len[cdp_a_loop], cdp_indel_d_list_end_other_len[cdp_a_loop], cdp_indel_d_list_start_rd[cdp_a_loop], cdp_indel_d_list_end_rd[cdp_a_loop], cdp_indel_d_list_start_sc[cdp_a_loop], cdp_indel_d_list_end_sc[cdp_a_loop], cdp_homopolymer);
					sort_indel_count += 1;
				}
				else
				{
					vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
					vcf_sv_data_list[sort_sv_count].pos = cdp_indel_d_list_start[cdp_a_loop];
					sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%s\t.\tSVTYPE=DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SSC:ESC\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_indel_d_list_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_indel_d_list_end[cdp_a_loop] + 1, gt_entry, cdp_indel_d_list_start_binom_cdf[cdp_a_loop], cdp_indel_d_list_end_binom_cdf[cdp_a_loop], (double)cdp_indel_d_list_start_f[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_indel_d_list_end_r[cdp_a_loop]/(double)cdp_add_factor, cdp_indel_d_list_start_conc[cdp_a_loop], cdp_indel_d_list_end_conc[cdp_a_loop], cdp_indel_d_list_start_other_len[cdp_a_loop], cdp_indel_d_list_end_other_len[cdp_a_loop], cdp_indel_d_list_start_rd[cdp_a_loop], cdp_indel_d_list_end_rd[cdp_a_loop], cdp_indel_d_list_start_sc[cdp_a_loop], cdp_indel_d_list_end_sc[cdp_a_loop]);
					sort_sv_count += 1;
				}
			}
	      }
	      else
	      {
		fprintf(cdp_results_file, "INDEL_DEL\t%s\t%d\t%d\t%d\t%e\t%e\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%e\t%e\t%d\t%s\n", cdp_chr_name, cdp_indel_d_list_start[cdp_a_loop] + 1, cdp_indel_d_list_end[cdp_a_loop] + 1, (cdp_indel_d_list_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_a_loop] + 1), cdp_indel_d_list_start_binom_cdf[cdp_a_loop], cdp_indel_d_list_end_binom_cdf[cdp_a_loop], cdp_indel_d_list_start_conc[cdp_a_loop], cdp_indel_d_list_end_conc[cdp_a_loop], cdp_indel_d_list_start_other_len[cdp_a_loop], cdp_indel_d_list_end_other_len[cdp_a_loop], cdp_indel_d_list_start_f[cdp_a_loop], cdp_indel_d_list_end_r[cdp_a_loop], cdp_indel_d_list_start_rd[cdp_a_loop], cdp_indel_d_list_end_rd[cdp_a_loop], cdp_indel_d_list_start_sc[cdp_a_loop], cdp_indel_d_list_end_sc[cdp_a_loop], cdp_indel_d_list_start_hez_binom_cdf[cdp_a_loop], cdp_indel_d_list_end_hez_binom_cdf[cdp_a_loop], cdp_homopolymer, cdp_snv_gt_string);
	      }
	    }
	  }
	}
      }



      for(cdp_a_loop=0;cdp_a_loop<cdp_del_list2_index;cdp_a_loop++)
      {
	cdp_overlap = 0;
	if( (cdp_del_list2_start_binom_cdf[cdp_a_loop] <= g_pval_threshold || cdp_del_list2_start_hez_binom_cdf[cdp_a_loop] <= g_pval_threshold) && (cdp_del_list2_end_binom_cdf[cdp_a_loop] <= g_pval_threshold || cdp_del_list2_end_hez_binom_cdf[cdp_a_loop] <= g_pval_threshold) && (double) cdp_del_list2_start_del_f[cdp_a_loop] / (double) cdp_del_list2_start_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor && (double) cdp_del_list2_end_del_r[cdp_a_loop] / (double) cdp_del_list2_end_rd[cdp_a_loop] >= g_min_sv_ratio*(double)cdp_add_factor )
	{
	  for(cdp_b_loop=0;cdp_b_loop<cdp_indel_d_list_index;cdp_b_loop++)
	  {

	    if( cdp_indel_d_list_start_binom_cdf[cdp_b_loop] <= g_pval_threshold && cdp_indel_d_list_end_binom_cdf[cdp_b_loop] <= g_pval_threshold && (double)cdp_indel_d_list_start_f[cdp_b_loop]/(double)cdp_indel_d_list_start_rd[cdp_b_loop] > g_min_indel_ratio*(double)cdp_add_factor && (double)cdp_indel_d_list_end_r[cdp_b_loop]/(double)cdp_indel_d_list_end_rd[cdp_b_loop] > g_min_indel_ratio*(double)cdp_add_factor && abs(cdp_del_list2_start[cdp_a_loop] - cdp_indel_d_list_start[cdp_b_loop]) <  g_insert_max_size - 2*g_lseq && abs(cdp_del_list2_end[cdp_a_loop] - cdp_indel_d_list_end[cdp_b_loop]) <  g_insert_max_size - 2*g_lseq )
	    {
	      cdp_overlap_ratio_1 = 0;
	      cdp_overlap_ratio_2 = 0;
	      if( cdp_del_list2_start[cdp_a_loop] >= cdp_indel_d_list_start[cdp_b_loop] && cdp_del_list2_start[cdp_a_loop] <= cdp_indel_d_list_end[cdp_b_loop] )
	      {
		if( cdp_del_list2_end[cdp_a_loop] >= cdp_indel_d_list_end[cdp_b_loop] )
		{
		  cdp_overlap_ratio_1 = (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_del_list2_start[cdp_a_loop]) / (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_b_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_del_list2_start[cdp_a_loop]) / (double) (cdp_del_list2_end[cdp_a_loop] - cdp_del_list2_start[cdp_a_loop]);
		}
		else
		{
		  cdp_overlap_ratio_1 = (double) (cdp_del_list2_end[cdp_a_loop] - cdp_del_list2_start[cdp_a_loop]) / (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_b_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_del_list2_end[cdp_a_loop] - cdp_del_list2_start[cdp_a_loop]) / (double) (cdp_del_list2_end[cdp_a_loop] - cdp_del_list2_start[cdp_a_loop]);
		}
	      }
	      else if( cdp_indel_d_list_start[cdp_b_loop] >= cdp_del_list2_start[cdp_a_loop] && cdp_indel_d_list_start[cdp_b_loop] <= cdp_del_list2_end[cdp_a_loop] )
	      {
		if( cdp_del_list2_end[cdp_a_loop] >= cdp_indel_d_list_end[cdp_b_loop] )
		{
		  cdp_overlap_ratio_1 = (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_b_loop]) / (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_b_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_b_loop]) / (double) (cdp_del_list2_end[cdp_a_loop] - cdp_del_list2_start[cdp_a_loop]);
		}
		else
		{
		  cdp_overlap_ratio_1 = (double) (cdp_del_list2_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_b_loop]) / (double) (cdp_indel_d_list_end[cdp_b_loop] - cdp_indel_d_list_start[cdp_b_loop]);
		  cdp_overlap_ratio_2 = (double) (cdp_del_list2_end[cdp_a_loop] - cdp_indel_d_list_start[cdp_b_loop]) / (double) (cdp_del_list2_end[cdp_a_loop] - cdp_del_list2_start[cdp_a_loop]);
		}
	      }
	      if( cdp_overlap_ratio_1 >= g_min_overlap_ratio && cdp_overlap_ratio_2 >= g_min_overlap_ratio && cdp_indel_d_list_start_binom_cdf[cdp_b_loop] * cdp_indel_d_list_end_binom_cdf[cdp_b_loop] <= cdp_del_list2_start_binom_cdf[cdp_a_loop] * cdp_del_list2_end_binom_cdf[cdp_a_loop] )
	      {
		cdp_overlap = 1;
		break;
	      }
	    }
	  }
	  if( cdp_overlap == 0 )
	  {
	    if( g_vcf == 1 )
	    {

			float p_value_del_1 = cdp_del_list2_end_binom_cdf[cdp_a_loop];
			if (cdp_del_list2_start_binom_cdf[cdp_a_loop] > cdp_del_list2_end_binom_cdf[cdp_a_loop]) {
				p_value_del_1 = cdp_del_list2_start_binom_cdf[cdp_a_loop];
			}

			char gt_entry[10];
			if (p_value_del_1 < del_homozygous_p_value_cutoff)
			{
				strcpy(gt_entry, homozygous_gt_entry);
			} else {
				strcpy(gt_entry, unknown_gt_entry);
			}

			if (p_value_del_1 != 0)
			{
				int qual_entry = (int) -10 * log10(p_value_del_1);
				vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
				vcf_sv_data_list[sort_sv_count].pos = cdp_del_list2_start[cdp_a_loop];
				sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%d\t.\tSVTYPE=DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_del_list2_start[cdp_a_loop] - 1]), qual_entry, cdp_del_list2_end[cdp_a_loop] + 1, gt_entry, cdp_del_list2_start_binom_cdf[cdp_a_loop], cdp_del_list2_end_binom_cdf[cdp_a_loop], (double)cdp_del_list2_start_del_f[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_del_list2_end_del_r[cdp_a_loop]/(double)cdp_add_factor, cdp_del_list2_start_rd[cdp_a_loop], cdp_del_list2_end_rd[cdp_a_loop], cdp_del_list2_start_conc[cdp_a_loop], cdp_del_list2_end_conc[cdp_a_loop], cdp_del_list2_start_other_len[cdp_a_loop], cdp_del_list2_end_other_len[cdp_a_loop], cdp_del_list2_start_read_start[cdp_a_loop] + 1, cdp_del_list2_start_read_end[cdp_a_loop] + 1, cdp_del_list2_end_read_start[cdp_a_loop] + 1, cdp_del_list2_end_read_end[cdp_a_loop] + 1);
				sort_sv_count += 1;
			} else {
				vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
				vcf_sv_data_list[sort_sv_count].pos = cdp_del_list2_start[cdp_a_loop];
				sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%s\t.\tSVTYPE=DEL;END=%d\tGT:SPR:EPR:SEV:EEV:SRD:ERD:SCO:ECO:SOT:EOT:SFR:SLR:EFR:ELR\t%s:%.2e:%.2e:%.1f:%.1f:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", toupper(cdp_chr_fasta[cdp_del_list2_start[cdp_a_loop] - 1]), infinity_qual_entry, cdp_del_list2_end[cdp_a_loop] + 1, gt_entry, cdp_del_list2_start_binom_cdf[cdp_a_loop], cdp_del_list2_end_binom_cdf[cdp_a_loop], (double)cdp_del_list2_start_del_f[cdp_a_loop]/(double)cdp_add_factor, (double)cdp_del_list2_end_del_r[cdp_a_loop]/(double)cdp_add_factor, cdp_del_list2_start_rd[cdp_a_loop], cdp_del_list2_end_rd[cdp_a_loop], cdp_del_list2_start_conc[cdp_a_loop], cdp_del_list2_end_conc[cdp_a_loop], cdp_del_list2_start_other_len[cdp_a_loop], cdp_del_list2_end_other_len[cdp_a_loop], cdp_del_list2_start_read_start[cdp_a_loop] + 1, cdp_del_list2_start_read_end[cdp_a_loop] + 1, cdp_del_list2_end_read_start[cdp_a_loop] + 1, cdp_del_list2_end_read_end[cdp_a_loop] + 1);
				sort_sv_count += 1;
			}
		}
	    else
	    {
	      fprintf(cdp_results_file, "DEL\t%s\t%d\t%d\t%6.2f\t%e\t%e\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%e\t%e\n", cdp_chr_name, cdp_del_list2_start[cdp_a_loop] + 1, cdp_del_list2_end[cdp_a_loop] + 1, cdp_del_list2_dist[cdp_a_loop], cdp_del_list2_start_binom_cdf[cdp_a_loop], cdp_del_list2_end_binom_cdf[cdp_a_loop], cdp_del_list2_start_del_f[cdp_a_loop], cdp_del_list2_end_del_r[cdp_a_loop], cdp_del_list2_start_rd[cdp_a_loop], cdp_del_list2_end_rd[cdp_a_loop], cdp_del_list2_start_conc[cdp_a_loop], cdp_del_list2_end_conc[cdp_a_loop], cdp_del_list2_start_other_len[cdp_a_loop], cdp_del_list2_end_other_len[cdp_a_loop], cdp_del_list2_start_read_start[cdp_a_loop] + 1, cdp_del_list2_start_read_end[cdp_a_loop] + 1, cdp_del_list2_end_read_start[cdp_a_loop] + 1, cdp_del_list2_end_read_end[cdp_a_loop] + 1, cdp_del_list2_start_hez_binom_cdf[cdp_a_loop], cdp_del_list2_end_hez_binom_cdf[cdp_a_loop]);
	    }
	  }
	}
      }




#ifdef DO_TIMING
      end_t = rdtsc();
      timers_ss[9] += end_t - start_t;
#endif
    }

    bam_destroy1(cdp_b);

    if( g_internal == 1 )
    {
      printf("cdp_snv_list_index %d\n", cdp_snv_list_index);
      printf("cdp_ins_list_index %d\n", cdp_ins_list_index);
      printf("cdp_indel_i_list_index %d\n", cdp_indel_i_list_index);
      printf("cdp_indel_d_list_index %d\n", cdp_indel_d_list_index);
      printf("cdp_dup_list_index %d\n", cdp_dup_list_index);
      printf("cdp_del_list_index %d\n", cdp_del_list_index);
      printf("cdp_inv_f_list_index %d\n", cdp_inv_f_list_index);
      printf("cdp_inv_r_list_index %d\n", cdp_inv_r_list_index);
      printf("cdp_ins_list2_index %d\n", cdp_ins_list2_index);
      printf("cdp_dup_list2_index %d\n", cdp_dup_list2_index);
      printf("cdp_del_list2_index %d\n", cdp_del_list2_index);
      printf("cdp_inv_f_list2_index %d\n", cdp_inv_f_list2_index);
      printf("cdp_inv_r_list2_index %d\n", cdp_inv_r_list2_index);


      printf("before free SV lists\n");
    }

    if( g_sv_list_len_over == 1 )
    {
      printf("WARNING: Breakpoint array for chromosome is full. Use -G to increase array size.\n");
      g_sv_list_len_over = 0;
    }
    if( g_sv_list2_len_over == 1 )
    {
      printf("WARNING: SV array for chromosome is full. Use -G to increase array size.\n");
      g_sv_list2_len_over = 0;
    }

#ifdef DO_TIMING
    start_t = rdtsc();
#endif

    if( cdp_chr_match != -1 && g_do_rd == 0 )
    {


      for(caf_a_loop=0;caf_a_loop<caf_chr_fasta_len;caf_a_loop++)
      {
	      if( (caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop]) > 0 )
	      {
		      caf_rd_mq_list[caf_a_loop] = caf_rd_mq_list[caf_a_loop] / (caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop]);
	      }
      }



 
      double caf_repeat_chr_rd_ave = 0;
      double caf_repeat_chr_rd_stdev = 0;
      long caf_repeat_chr_rd_count = 0;
      for(caf_a_loop=g_insert_mean-1;caf_a_loop<caf_chr_fasta_len-g_one_base_window_size;caf_a_loop++)
      {
	      if( caf_one_base_rd_acgt_weighted[caf_a_loop] >= g_insert_min_acgt )
	      {
		      caf_repeat_chr_rd_ave += caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop];
		      caf_repeat_chr_rd_count += 1;
	      }
      }
      if( caf_repeat_chr_rd_count > 0 )
      {
	      caf_repeat_chr_rd_ave = caf_repeat_chr_rd_ave / caf_repeat_chr_rd_count;
      }

      for(caf_a_loop=g_insert_mean-1;caf_a_loop<caf_chr_fasta_len-g_one_base_window_size;caf_a_loop++)
      {
	      if( caf_one_base_rd_acgt_weighted[caf_a_loop] >= g_insert_min_acgt )
	      {
		      if( (caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop]) < 2*caf_repeat_chr_rd_ave )
		      {
			      caf_repeat_chr_rd_stdev += ((caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop]) - caf_repeat_chr_rd_ave)*((caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop]) - caf_repeat_chr_rd_ave);
		      }
		      else
		      {
		      caf_repeat_chr_rd_stdev += caf_repeat_chr_rd_ave*caf_repeat_chr_rd_ave;
		      }
	      }
      }

      if( caf_repeat_chr_rd_count > 1 )
      {
	      caf_repeat_chr_rd_stdev = sqrt(caf_repeat_chr_rd_stdev/((double)caf_repeat_chr_rd_count - 1.0));
      }
      else
      {
	      caf_repeat_chr_rd_stdev = 0;
      }

      if( g_internal == 1)
      {
	printf("chr rd, stdev %e\t%e\n", caf_repeat_chr_rd_ave, caf_repeat_chr_rd_stdev);
      }

      for(caf_a_loop=0;caf_a_loop<caf_repeat_types;caf_a_loop++)
      {
	      caf_repeat_rd_average[caf_a_loop] = 0;
	      caf_repeat_rd_stdev[caf_a_loop] = 0;
	      caf_repeat_rd_type_count[caf_a_loop] = 0;
      }

      long caf_repeat_rd = 0;
      double *caf_repeat_rd_list = (double *) malloc((caf_repeat_index) * sizeof(double));
      if( caf_repeat_rd_list == NULL )
      {
	printf("431 NULL\n");
	exit(0);
      }
      for(caf_a_loop=0;caf_a_loop<caf_repeat_index;caf_a_loop++)
      {
	      caf_repeat_rd = 0;
	      for(caf_b_loop=caf_repeat_start_list[caf_a_loop];caf_b_loop<caf_repeat_end_list[caf_a_loop];caf_b_loop++)
	      {
		      caf_repeat_rd += caf_rd_rd_list[caf_b_loop] + caf_rd_low_mq_rd_list[caf_b_loop];
	      }
	      caf_repeat_rd_list[caf_a_loop] = (double) caf_repeat_rd / (caf_repeat_end_list[caf_a_loop] - caf_repeat_start_list[caf_a_loop]);
	      if( caf_repeat_rd_list[caf_a_loop] < 2 * caf_repeat_chr_rd_ave )
	      {
		      caf_repeat_rd_average[caf_repeat_type_list[caf_a_loop]] += caf_repeat_rd_list[caf_a_loop];
	      }
	      else
	      {
		      caf_repeat_rd_average[caf_repeat_type_list[caf_a_loop]] += 2 * caf_repeat_chr_rd_ave;
	      }
	      caf_repeat_rd_type_count[caf_repeat_type_list[caf_a_loop]] += 1;
      }

      for(caf_a_loop=0;caf_a_loop<caf_repeat_types;caf_a_loop++)
      {
	      caf_repeat_rd_average[caf_a_loop] = caf_repeat_rd_average[caf_a_loop] / (double) caf_repeat_rd_type_count[caf_a_loop];
      }

      for(caf_a_loop=0;caf_a_loop<caf_repeat_index;caf_a_loop++)
      {
	      if( caf_repeat_rd_list[caf_a_loop] < 2 * caf_repeat_chr_rd_ave )
	      {
		      caf_repeat_rd_stdev[caf_repeat_type_list[caf_a_loop]] += (caf_repeat_rd_list[caf_a_loop] - caf_repeat_rd_average[caf_repeat_type_list[caf_a_loop]]) * (caf_repeat_rd_list[caf_a_loop] - caf_repeat_rd_average[caf_repeat_type_list[caf_a_loop]]);
	      }
	      else
	      {
		      caf_repeat_rd_stdev[caf_repeat_type_list[caf_a_loop]] += ((2 * caf_repeat_chr_rd_ave) - caf_repeat_rd_average[caf_repeat_type_list[caf_a_loop]]) * ((2 * caf_repeat_chr_rd_ave) - caf_repeat_rd_average[caf_repeat_type_list[caf_a_loop]]);
	      }
      }
      free(caf_repeat_rd_list);

      for(caf_a_loop=0;caf_a_loop<caf_repeat_types;caf_a_loop++)
      {
	      if( caf_repeat_rd_type_count[caf_a_loop] > 1 )
	      {
		      caf_repeat_rd_stdev[caf_a_loop] = sqrt(caf_repeat_rd_stdev[caf_a_loop] / ((double) caf_repeat_rd_type_count[caf_a_loop] - 1.0));
	      }
	      else
	      {
		      caf_repeat_rd_stdev[caf_a_loop] = 0;
	      }
      }

      g_most_biased_repeat = -1;
      g_most_biased_repeat_count = 0;
      for(caf_a_loop=0;caf_a_loop<caf_repeat_types;caf_a_loop++)
      {
	      if( caf_repeat_rd_type_count[caf_a_loop] > g_rd_no_combine_min_windows )
	      {
		      if( (caf_repeat_rd_average[caf_a_loop] + (g_min_repeat_stdev * caf_repeat_rd_stdev[caf_a_loop])) < caf_repeat_chr_rd_ave && (caf_repeat_chr_rd_ave - (g_min_repeat_stdev*caf_repeat_chr_rd_stdev)) > caf_repeat_rd_average[caf_a_loop] )
		      {
			      if( caf_repeat_rd_type_count[caf_a_loop] > g_most_biased_repeat_count )
			      {
				      g_most_biased_repeat = caf_a_loop;
				      g_most_biased_repeat_count = caf_repeat_rd_type_count[caf_a_loop];
			      }
		      }
	      }
      }


      if( g_internal == 1 )
      {
	printf("biased %d\t%ld\n", g_most_biased_repeat, g_most_biased_repeat_count);
      }



      long caf_num_blocks = caf_chr_fasta_len / g_block_unit_size;
      double *caf_block_rd_list = malloc(caf_num_blocks * sizeof(double));
      if( caf_block_rd_list == NULL )
      {
	printf("432 NULL\n");
	exit(0);
      }
      long *caf_block_over_pos_list = malloc(caf_num_blocks * sizeof(long));
      if( caf_block_over_pos_list == NULL )
      {
	printf("433 NULL\n");
	exit(0);
      }
      long *caf_block_over_dist_list = malloc(caf_num_blocks * sizeof(long));
      if( caf_block_over_dist_list == NULL )
      {
	printf("434 NULL\n");
	exit(0);
      }

      double caf_chr_rd_ave = 0.0;
      double caf_chr_rd_threshold = 0.0;
      long caf_block_rd_total = 0;
      long caf_chr_block_rd_total = 0;
      long caf_bin_count = 0;
      long caf_block_count = 0;
      long caf_num_blocks_count = 0;
      long caf_num_over_blocks_count = 0;
      for(caf_a_loop=0;caf_a_loop<caf_chr_fasta_len;caf_a_loop++)
      {

	if( strchr(caf_gc_chars, cdp_chr_fasta[caf_a_loop]) != NULL || strchr(caf_at_chars, cdp_chr_fasta[caf_a_loop]) != NULL )
	{
	      caf_chr_block_rd_total += caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop];
	      caf_block_count += 1;
	}

	      caf_block_rd_total += caf_rd_rd_list[caf_a_loop] + caf_rd_low_mq_rd_list[caf_a_loop];
	      caf_bin_count += 1;
	      if( caf_bin_count == g_block_unit_size )
	      {
		      caf_block_rd_list[caf_num_blocks_count] = caf_block_rd_total / (double) caf_bin_count;
		      caf_num_blocks_count += 1;
		      caf_bin_count = 0;
		      caf_block_rd_total = 0;
	      }

      }
      caf_chr_rd_ave = caf_chr_block_rd_total / (double) caf_block_count;
      caf_chr_rd_threshold = g_chr_rd_threshold_factor * caf_chr_rd_ave;

      if( g_internal == 1 )
      {
	printf("caf_chr_rd_ave, caf_chr_rd_threshold %e %e\n", caf_chr_rd_ave, caf_chr_rd_threshold);
      }


      for(caf_a_loop=0;caf_a_loop<caf_num_blocks_count;caf_a_loop++)
      {
	      if( caf_block_rd_list[caf_a_loop] > caf_chr_rd_threshold )
	      {
		      caf_block_over_pos_list[caf_num_over_blocks_count] = caf_a_loop;
		      if( caf_num_over_blocks_count > 0 )
		      {
			      caf_block_over_dist_list[caf_num_over_blocks_count - 1] = caf_block_over_pos_list[caf_num_over_blocks_count] - caf_block_over_pos_list[caf_num_over_blocks_count - 1];
		      }
		      caf_num_over_blocks_count += 1;
	      }
      }



      long caf_temp_blocks = 0;
      long caf_temp_block_start, caf_temp_block_end;
      g_block_index = 0;
      if( caf_num_over_blocks_count > 1 )
      {
	      for(caf_a_loop=1;caf_a_loop<caf_num_over_blocks_count;caf_a_loop++)
	      {
		      if( caf_temp_blocks == 0 )
		      {
			      if( (caf_temp_blocks + 1) > ((caf_block_over_pos_list[caf_a_loop] - caf_block_over_pos_list[caf_a_loop - 1]) / g_block_factor) )
			      {
				      caf_temp_block_end = caf_block_over_pos_list[caf_a_loop] + 1;
				      caf_temp_blocks += 1;
			      }
			      else
			      {
				      caf_temp_block_end = caf_block_over_pos_list[caf_a_loop - 1] + 1;
			      }
			      caf_temp_block_start = caf_block_over_pos_list[caf_a_loop - 1];
			      caf_temp_blocks += 1;
		      }
		      else
		      {
			      if( (caf_temp_blocks + 1) > ((caf_block_over_pos_list[caf_a_loop - 1] - caf_temp_block_start) / g_block_factor) )
			      {
				      caf_temp_block_end = caf_block_over_pos_list[caf_a_loop - 1] + 1;
				      caf_temp_blocks += 1;
			      }
			      else
			      {
				      if( caf_temp_blocks >= g_min_blocks )
				      {
					      g_block_index += 1;
				      }
				      caf_temp_blocks = 0;
				      caf_temp_block_start = caf_block_over_pos_list[caf_a_loop - 1];
				      caf_temp_block_end = caf_block_over_pos_list[caf_a_loop - 1] + 1;
				      caf_temp_blocks += 1;
			      }
			      if( caf_temp_blocks >= g_min_blocks && g_block_index < max_block_list_len )
			      {
				      g_block_start_list[g_block_index] = caf_temp_block_start * g_block_unit_size;
				      g_block_end_list[g_block_index] = caf_temp_block_end * g_block_unit_size;
			      }
		      }
	      }

      }
      if( caf_temp_blocks >= g_min_blocks )
      {
	      g_block_index += 1;
      }




      g_lowvar_block_index = 0;
      for(caf_a_loop=0;caf_a_loop<g_block_index;caf_a_loop++)
      {
	      if( (g_block_end_list[caf_a_loop] - g_block_start_list[caf_a_loop]) >= g_block_min )
	      {
		      g_lowvar_block_end_list[g_lowvar_block_index] = g_block_start_list[caf_a_loop];
		      g_lowvar_block_start_list[g_lowvar_block_index+1] = g_block_end_list[caf_a_loop];
		      g_lowvar_block_index += 1;
	      }
      }
      g_lowvar_block_index += 1;




      g_lowvar_block_end_list[g_lowvar_block_index-1] = caf_chr_fasta_len;

      if( g_internal == 1 )
      {
	printf("blocks %ld\n", g_lowvar_block_index);
	for(caf_block_loop=0;caf_block_loop<g_lowvar_block_index;caf_block_loop++)
	{
	  printf("block start, end %ld %ld\n", g_lowvar_block_start_list[caf_block_loop], g_lowvar_block_end_list[caf_block_loop]);
	}
      }

      for(caf_block_loop=0;caf_block_loop<g_lowvar_block_index;caf_block_loop++)
      {
	      if( g_lowvar_block_start_list[caf_block_loop] < (g_insert_mean-1) )
	      {
		      g_lowvar_block_start_list[caf_block_loop] = g_insert_mean-1;
	      }
	      else if( g_lowvar_block_start_list[caf_block_loop] >= (caf_chr_fasta_len-g_one_base_window_size) )
	      {
		      g_lowvar_block_start_list[caf_block_loop] = caf_chr_fasta_len-g_one_base_window_size;
	      }
	      if( g_lowvar_block_end_list[caf_block_loop] < (g_insert_mean-1) )
	      {
		      g_lowvar_block_end_list[caf_block_loop] = g_insert_mean-1;
	      }
	      else if( g_lowvar_block_end_list[caf_block_loop] >= (caf_chr_fasta_len-g_one_base_window_size) )
	      {
		      g_lowvar_block_end_list[caf_block_loop] = caf_chr_fasta_len-g_one_base_window_size;
	      }
      }
      caf_block_loop = 0;
      while(caf_block_loop < g_lowvar_block_index )
      {
	      if( (g_lowvar_block_end_list[caf_block_loop] - g_lowvar_block_start_list[caf_block_loop]) < g_min_rd_window_len )
	      {
		      if( caf_block_loop < (g_lowvar_block_index-1) )
		      {
			      for(caf_block_loop2=(caf_block_loop+1);caf_block_loop2<g_lowvar_block_index;caf_block_loop2++)
			      {
				      g_lowvar_block_chr_list[caf_block_loop2-1] = g_lowvar_block_chr_list[caf_block_loop2];
				      g_lowvar_block_start_list[caf_block_loop2-1] = g_lowvar_block_start_list[caf_block_loop2];
				      g_lowvar_block_end_list[caf_block_loop2-1] = g_lowvar_block_end_list[caf_block_loop2];
			      }
		      }
		      g_lowvar_block_index -= 1;
	      }
	      else
	      {
		      caf_block_loop += 1;
	      }
      }

      for(caf_block_loop=0;caf_block_loop<g_lowvar_block_index;caf_block_loop++)
      {
	g_lowvar_block_sample_start_list[caf_block_loop] = g_lowvar_block_start_list[caf_block_loop];
	g_lowvar_block_sample_end_list[caf_block_loop] = g_lowvar_block_end_list[caf_block_loop];
      }
      g_lowvar_block_sample_index = g_lowvar_block_index;

#ifdef DO_TIMING
      end_t = rdtsc();
      timers_ss[7] += end_t - start_t;
#endif

      if( g_internal == 1 )
      {
	for(caf_block_loop=0;caf_block_loop<g_lowvar_block_index;caf_block_loop++)
	{
	  printf("block_trimmed start, end %ld %ld\n", g_lowvar_block_start_list[caf_block_loop], g_lowvar_block_end_list[caf_block_loop]);
	}
      }

      long caf_ploidy_a_loop;

      for(caf_ploidy_a_loop=0;caf_ploidy_a_loop<1;caf_ploidy_a_loop++)
      {
	      if( caf_ploidy_a_loop == 0 )
	      {
		      caf_output_file = cdp_results_file;
		      if( caf_bam_name_len == g_chrx_len && strncmp(caf_bam_name, g_chrx, g_chrx_len) == 0 && g_gender == 1 )
		      {
			      caf_ploidy = g_ploidy/2;
		      }
		      else if( caf_bam_name_len == g_x_len && strncmp(caf_bam_name, g_x, g_x_len) == 0 && g_gender == 1 )
		      {
			      caf_ploidy = g_ploidy/2;
		      }
		      else
		      {
			      caf_ploidy = g_ploidy;
		      }

	      }


		    if (g_troubleshooting == 1)
			{
				printf("before ploidy a loop = 0 start\n");
			}
	      if( caf_ploidy_a_loop == 0 )
	      {

		      long caf_del_list_index = 0;
		      long caf_old_del_list_index = 0;
		      long caf_del_dup_list_len = caf_chr_fasta_len / g_min_rd_window_len + 1;
		      int *caf_del_list_ref = malloc(caf_del_dup_list_len * sizeof(int));
		      if( caf_del_list_ref == NULL )
		      {
			printf("435 NULL\n");
			exit(0);
		      }
		      long *caf_del_list_start = malloc(caf_del_dup_list_len * sizeof(long));
		      if( caf_del_list_start == NULL )
		      {
			printf("436 NULL\n");
			exit(0);
		      }
		      long *caf_del_list_end = malloc(caf_del_dup_list_len * sizeof(long));
		      if( caf_del_list_end == NULL )
		      {
			printf("437 NULL\n");
			exit(0);
		      }
		      double *caf_del_list_stdev = malloc(caf_del_dup_list_len * sizeof(double));
		      if( caf_del_list_stdev == NULL )
		      {
			printf("438 NULL\n");
			exit(0);
		      }
		      double *caf_del_list_cn = malloc(caf_del_dup_list_len * sizeof(int));
		      if( caf_del_list_cn == NULL )
		      {
			printf("439 NULL\n");
			exit(0);
		      }

		      double *caf_del_list_cn_stdev = malloc(caf_del_dup_list_len * sizeof(int));
		      if( caf_del_list_cn_stdev == NULL )
		      {
			printf("440 NULL\n");
			exit(0);
		      }

		      long caf_dup_list_index = 0;
		      long caf_old_dup_list_index = 0;
		      int *caf_dup_list_ref = malloc(caf_del_dup_list_len * sizeof(int));
		      if( caf_dup_list_ref == NULL )
		      {
			printf("441 NULL\n");
			exit(0);
		      }
		      long *caf_dup_list_start = malloc(caf_del_dup_list_len * sizeof(long));
		      if( caf_dup_list_start == NULL )
		      {
			printf("442 NULL\n");
			exit(0);
		      }
		      long *caf_dup_list_end = malloc(caf_del_dup_list_len * sizeof(long));
		      if( caf_dup_list_end == NULL )
		      {
			printf("443 NULL\n");
			exit(0);
		      }
		      double *caf_dup_list_stdev = malloc(caf_del_dup_list_len * sizeof(double));
		      if( caf_dup_list_stdev == NULL )
		      {
			printf("444 NULL\n");
			exit(0);
		      }
		      double *caf_dup_list_cn = malloc(caf_del_dup_list_len * sizeof(int));
		      if( caf_dup_list_cn == NULL )
		      {
			printf("445 NULL\n");
			exit(0);
		      }
		      double *caf_dup_list_cn_stdev = malloc(caf_del_dup_list_len * sizeof(int));
		      if( caf_dup_list_cn_stdev == NULL )
		      {
			printf("446 NULL\n");
			exit(0);
		      }

		      g_lowvar_block_start_list[0] = g_insert_mean-1;
		      g_lowvar_block_end_list[0] = caf_chr_fasta_len-g_one_base_window_size;
		      g_lowvar_block_index = 1;

#ifdef DO_TIMING
		      start_t = rdtsc();
#endif

			  if (g_troubleshooting == 1)
			  {
				  printf("before detect del dup called\n");
			  }
		      detect_del_dup(cdp_chr, cdp_begin, caf_chr_fasta_len, caf_one_base_rd_gc_weighted, caf_one_base_rd_acgt_weighted, caf_rd_mq_list, caf_rd_rd_list, caf_rd_low_mq_rd_list, caf_sample_high_mq_rd_list, caf_sample_low_mq_rd_list, caf_sample_repeat_rd_list, &caf_low_mq_index[0], &caf_high_mq_index[0], &caf_low_mq_index_all[0], &caf_high_mq_index_all[0], caf_pval2sd_pval_list, caf_pval2sd_sd_list, caf_pval2sd_list_len, &caf_del_list_index, caf_del_list_ref, caf_del_list_start, caf_del_list_end, caf_del_list_stdev, caf_del_list_cn, caf_del_list_cn_stdev, &caf_dup_list_index, caf_dup_list_ref, caf_dup_list_start, caf_dup_list_end, caf_dup_list_stdev, caf_dup_list_cn, caf_dup_list_cn_stdev, caf_ploidy, caf_repeat_type_list, caf_repeat_start_list, caf_repeat_end_list, caf_repeat_index, caf_results_file_name, cdp_chr_name);
 
			  if (g_troubleshooting == 1)
			  {
				  printf("after detect del dup run\n");
			  }
#ifdef DO_TIMIN
		      end_t = rdtsc();
		      timers_ss[8] += end_t - start_t;
#endif


		      double caf_p = 0.3275911;
		      double caf_a1 = 0.254829592;
		      double caf_a2 = -0.284496736;
		      double caf_a3 = 1.421413741;
		      double caf_a4 = -1.453152027;
		      double caf_a5 = 1.061405429;
		      double caf_x, caf_t, caf_erf;

		      double caf_prob;

		      double *caf_del_list_pvalue = malloc(caf_del_list_index * sizeof(double));
		      if( caf_del_list_pvalue == NULL )
		      {
			printf("447 NULL\n");
			exit(0);
		      }

		      for(caf_a_loop=0;caf_a_loop<caf_del_list_index;caf_a_loop++)
		      {
			      caf_x = fabs(caf_del_list_stdev[caf_a_loop]) / sqrt_2;
			      caf_t = 1.0 / (1.0 + caf_p + caf_x);
			      caf_erf = 1.0 - ((caf_a1*caf_t + caf_a2 * pow(caf_t,2) + caf_a3 * pow(caf_t,3) + caf_a4 * pow(caf_t,4) + caf_a5 * pow(caf_t,5)) * exp(-pow(caf_x,2)));
			      caf_prob = (1.0-caf_erf)/2.0;
			      caf_del_list_pvalue[caf_a_loop] = caf_prob;

		      }

		      double *caf_dup_list_pvalue = malloc(caf_dup_list_index * sizeof(double));
		      if( caf_dup_list_pvalue == NULL )
		      {
			printf("448 NULL\n");
			exit(0);
		      }

		      for(caf_a_loop=0;caf_a_loop<caf_dup_list_index;caf_a_loop++)
		      {
			      caf_x = fabs(caf_dup_list_stdev[caf_a_loop]) / sqrt_2;
			      caf_t = 1.0 / (1.0 + caf_p + caf_x);
			      caf_erf = 1.0 - ((caf_a1*caf_t + caf_a2 * pow(caf_t,2) + caf_a3 * pow(caf_t,3) + caf_a4 * pow(caf_t,4) + caf_a5 * pow(caf_t,5)) * exp(-pow(caf_x,2)));
			      caf_prob = (1.0-caf_erf)/2.0;
			      caf_dup_list_pvalue[caf_a_loop] = caf_prob;

		      }


		      if( g_normal == 0 )
		      {
			caf_b_loop = 0;
			caf_c_loop = 0;
			for(caf_a_loop=0;caf_a_loop<caf_del_list_index;caf_a_loop++)
			{
			  if( caf_del_list_pvalue[caf_a_loop] < g_rd_pval_threshold )
			  {
			    caf_del_list_ref[caf_b_loop] = caf_del_list_ref[caf_a_loop];
			    caf_del_list_start[caf_b_loop] = caf_del_list_start[caf_a_loop];
			    caf_del_list_end[caf_b_loop] = caf_del_list_end[caf_a_loop];
			    caf_del_list_stdev[caf_b_loop] = caf_del_list_stdev[caf_a_loop];
			    caf_del_list_pvalue[caf_b_loop] = caf_del_list_pvalue[caf_a_loop];
			    caf_del_list_cn[caf_b_loop] = caf_del_list_cn[caf_a_loop];
			    caf_del_list_cn_stdev[caf_b_loop] = caf_del_list_cn_stdev[caf_a_loop];
			    caf_b_loop += 1;
			    if( caf_a_loop < caf_old_del_list_index )
			    {
			      caf_c_loop += 1;
			    }
			  }
			}
			caf_del_list_index = caf_b_loop;
			caf_old_del_list_index = caf_c_loop;


			caf_b_loop = 0;
			caf_c_loop = 0;
			for(caf_a_loop=0;caf_a_loop<caf_dup_list_index;caf_a_loop++)
			{
			  if( caf_dup_list_pvalue[caf_a_loop] < g_rd_pval_threshold )
			  {
			    caf_dup_list_ref[caf_b_loop] = caf_dup_list_ref[caf_a_loop];
			    caf_dup_list_start[caf_b_loop] = caf_dup_list_start[caf_a_loop];
			    caf_dup_list_end[caf_b_loop] = caf_dup_list_end[caf_a_loop];
			    caf_dup_list_stdev[caf_b_loop] = caf_dup_list_stdev[caf_a_loop];
			    caf_dup_list_pvalue[caf_b_loop] = caf_dup_list_pvalue[caf_a_loop];
			    caf_dup_list_cn[caf_b_loop] = caf_dup_list_cn[caf_a_loop];
			    caf_dup_list_cn_stdev[caf_b_loop] = caf_dup_list_cn_stdev[caf_a_loop];
			    caf_b_loop += 1;
			    if( caf_a_loop < caf_old_dup_list_index )
			    {
			      caf_c_loop += 1;
			    }
			  }
			}
			caf_dup_list_index = caf_b_loop;
			caf_old_dup_list_index = caf_c_loop;


			int caf_output = 1;

			if( g_vcf == 0 )
			{
			  fprintf(caf_output_file, "SV Type\tChromosome\tStart\tEnd\tStdev from mean\tP Value\tCopy Number\n");
			}
			if( caf_old_del_list_index > 0 )
			{
			  for(caf_a_loop=0;caf_a_loop<caf_old_del_list_index;caf_a_loop++)
			  {
			    caf_output = 1;
			    for(caf_b_loop=caf_old_del_list_index;caf_b_loop<caf_del_list_index;caf_b_loop++)
			    {
			      if( (caf_del_list_start[caf_a_loop] <= caf_del_list_start[caf_b_loop] && caf_del_list_end[caf_a_loop] > caf_del_list_start[caf_b_loop]) || (caf_del_list_start[caf_b_loop] <= caf_del_list_start[caf_a_loop] && caf_del_list_end[caf_b_loop] > caf_del_list_start[caf_a_loop]) )
			      {
				if( (caf_del_list_end[caf_a_loop] - caf_del_list_start[caf_a_loop]) < (caf_del_list_end[caf_b_loop] - caf_del_list_start[caf_b_loop]) )
				{
				  caf_output = 0;
				}
				else if( (caf_del_list_end[caf_a_loop] - caf_del_list_start[caf_a_loop]) == (caf_del_list_end[caf_b_loop] - caf_del_list_start[caf_b_loop]) && caf_del_list_pvalue[caf_a_loop] > caf_del_list_pvalue[caf_b_loop] )
				{
				  caf_output = 0;
				}

			      }
			    }

				if (g_troubleshooting == 1)
				{
					printf("caf_output=%d DEL 1", caf_output);
				}
			    if( caf_output == 1 )
			    {
			      if( g_vcf == 1 )
			      {

					char gt_entry[10];
					if (caf_del_list_cn[caf_a_loop] < 0.5)
					{
						strcpy(gt_entry, homozygous_gt_entry);
					} else {
						strcpy(gt_entry, heterozygous_gt_entry);
					}
					if (caf_del_list_pvalue[caf_a_loop] != 0)
					{
						int qual_entry = (int) -10 * log10(caf_del_list_pvalue[caf_a_loop]);
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_del_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%d\t.\tSVTYPE=DEL;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_del_list_start[caf_a_loop] - 1]), qual_entry, caf_del_list_end[caf_a_loop] + 1, gt_entry, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					} else {
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_del_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%s\t.\tSVTYPE=DEL;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_del_list_start[caf_a_loop] - 1]), infinity_qual_entry, caf_del_list_end[caf_a_loop] + 1, gt_entry, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					}
				  }
			      else
			      {
					fprintf(caf_output_file, "%s\t%s\t%ld\t%ld\t%e\t%e\t%e\t%e\n", caf_del_text, cdp_chr_name, caf_del_list_start[caf_a_loop] + 1, caf_del_list_end[caf_a_loop] + 1, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
			      }
			    }
			  }

			  for(caf_a_loop=caf_old_del_list_index;caf_a_loop<caf_del_list_index;caf_a_loop++)
			  {
			    caf_output = 1;
			    for(caf_b_loop=0;caf_b_loop<caf_old_del_list_index;caf_b_loop++)
			    {
			      if( (caf_del_list_start[caf_a_loop] <= caf_del_list_start[caf_b_loop] && caf_del_list_end[caf_a_loop] > caf_del_list_start[caf_b_loop]) || (caf_del_list_start[caf_b_loop] <= caf_del_list_start[caf_a_loop] && caf_del_list_end[caf_b_loop] > caf_del_list_start[caf_a_loop]) )
			      {
				if( (caf_del_list_end[caf_a_loop] - caf_del_list_start[caf_a_loop]) < (caf_del_list_end[caf_b_loop] - caf_del_list_start[caf_b_loop]) )
				{
				  caf_output = 0;
				}
				else if( (caf_del_list_end[caf_a_loop] - caf_del_list_start[caf_a_loop]) == (caf_del_list_end[caf_b_loop] - caf_del_list_start[caf_b_loop]) && caf_del_list_pvalue[caf_a_loop] >= caf_del_list_pvalue[caf_b_loop] )
				{
				  caf_output = 0;
				}

			      }
			    }

				if (g_troubleshooting == 1)
				{
					printf("caf_output=%d DEL 2", caf_output);
				}
			    if( caf_output == 1 )
			    {
			      if( g_vcf == 1 )
			      {

					char gt_entry[10];
					if (caf_del_list_cn[caf_a_loop] < 0.5)
					{
						strcpy(gt_entry, homozygous_gt_entry);
					} else {
						strcpy(gt_entry, heterozygous_gt_entry);
					}
					if (caf_del_list_pvalue[caf_a_loop] != 0)
					{
						int qual_entry = (int) -10 * log10(caf_del_list_pvalue[caf_a_loop]);
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_del_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%d\t.\tSVTYPE=DEL;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_del_list_start[caf_a_loop] - 1]), qual_entry, caf_del_list_end[caf_a_loop] + 1, gt_entry, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					} else {
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_del_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%s\t.\tSVTYPE=DEL;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_del_list_start[caf_a_loop] - 1]), infinity_qual_entry, caf_del_list_end[caf_a_loop] + 1, gt_entry, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					}
				  }
			      else
			      {
					fprintf(caf_output_file, "%s\t%s\t%ld\t%ld\t%e\t%e\t%e\t%e\n", caf_del_text, cdp_chr_name, caf_del_list_start[caf_a_loop] + 1, caf_del_list_end[caf_a_loop] + 1, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
			      }
			    }
			  }

			}
			else
			{

			  for(caf_a_loop=0;caf_a_loop<caf_del_list_index;caf_a_loop++)
			  {
			    caf_output = 1;

			    if( caf_output == 1 )
			    {
			      if( g_vcf == 1 )
			      {

					char gt_entry[10];
					if (caf_del_list_cn[caf_a_loop] < 0.5)
					{
						strcpy(gt_entry, homozygous_gt_entry);
					} else {
						strcpy(gt_entry, heterozygous_gt_entry);
					}
					if (caf_del_list_pvalue[caf_a_loop] != 0)
					{
						int qual_entry = (int) -10 * log10(caf_del_list_pvalue[caf_a_loop]);
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_del_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%d\t.\tSVTYPE=DEL;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_del_list_start[caf_a_loop] - 1]), qual_entry, caf_del_list_end[caf_a_loop] + 1, gt_entry, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					} else {
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_del_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DEL>\t%s\t.\tSVTYPE=DEL;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_del_list_start[caf_a_loop] - 1]), infinity_qual_entry, caf_del_list_end[caf_a_loop] + 1, gt_entry, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					}
				  }
			      else
			      {
					fprintf(caf_output_file, "%s\t%s\t%ld\t%ld\t%e\t%e\t%e\t%e\n", caf_del_text, cdp_chr_name, caf_del_list_start[caf_a_loop] + 1, caf_del_list_end[caf_a_loop] + 1, caf_del_list_stdev[caf_a_loop], caf_del_list_pvalue[caf_a_loop], caf_del_list_cn[caf_a_loop], caf_del_list_cn_stdev[caf_a_loop]);
			      }
			    }
			  }
			}


			if( g_vcf == 0 )
			{
			  fprintf(caf_output_file, "SV Type\tChromosome\tStart\tEnd\tStdev from mean\tP Value\tCopy Number\n");
			}
			if( caf_old_dup_list_index > 0 )
			{

			  for(caf_a_loop=0;caf_a_loop<caf_old_dup_list_index;caf_a_loop++)
			  {
			    caf_output = 1;
			    for(caf_b_loop=caf_old_dup_list_index;caf_b_loop<caf_dup_list_index;caf_b_loop++)
			    {
			      if( (caf_dup_list_start[caf_a_loop] <= caf_dup_list_start[caf_b_loop] && caf_dup_list_end[caf_a_loop] > caf_dup_list_start[caf_b_loop]) || (caf_dup_list_start[caf_b_loop] <= caf_dup_list_start[caf_a_loop] && caf_dup_list_end[caf_b_loop] > caf_dup_list_start[caf_a_loop]) )
			      {
				if( (caf_dup_list_end[caf_a_loop] - caf_dup_list_start[caf_a_loop]) < (caf_dup_list_end[caf_b_loop] - caf_dup_list_start[caf_b_loop]) )
				{
				  caf_output = 0;
				}
				else if( (caf_dup_list_end[caf_a_loop] - caf_dup_list_start[caf_a_loop]) == (caf_dup_list_end[caf_b_loop] - caf_dup_list_start[caf_b_loop]) && caf_dup_list_pvalue[caf_a_loop] > caf_dup_list_pvalue[caf_b_loop] )
				{
				  caf_output = 0;
				}

			      }
			    }

				if (g_troubleshooting == 1)
				{
					printf("caf_output=%d DUP 1", caf_output);
				}
			    if( caf_output == 1 )
			    {
			      if( g_vcf == 1 )
			      {

					if (caf_dup_list_pvalue[caf_a_loop] != 0)
					{
						int qual_entry = (int) -10 * log10(caf_dup_list_pvalue[caf_a_loop]);
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_dup_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%d\t.\tSVTYPE=DUP;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_dup_list_start[caf_a_loop] - 1]), qual_entry, caf_dup_list_end[caf_a_loop] + 1, unknown_gt_entry, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					} else {
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_dup_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%s\t.\tSVTYPE=DUP;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_dup_list_start[caf_a_loop] - 1]), infinity_qual_entry, caf_dup_list_end[caf_a_loop] + 1, unknown_gt_entry, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					}
				  }
			      else
			      {
					fprintf(caf_output_file, "%s\t%s\t%ld\t%ld\t%e\t%e\t%e\t%e\n", caf_dup_text, cdp_chr_name, caf_dup_list_start[caf_a_loop] + 1, caf_dup_list_end[caf_a_loop] + 1, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
			      }
			    }
			  }

			  for(caf_a_loop=caf_old_dup_list_index;caf_a_loop<caf_dup_list_index;caf_a_loop++)
			  {
			    caf_output = 1;
			    for(caf_b_loop=0;caf_b_loop<caf_old_dup_list_index;caf_b_loop++)
			    {
			      if( (caf_dup_list_start[caf_a_loop] <= caf_dup_list_start[caf_b_loop] && caf_dup_list_end[caf_a_loop] > caf_dup_list_start[caf_b_loop]) || (caf_dup_list_start[caf_b_loop] <= caf_dup_list_start[caf_a_loop] && caf_dup_list_end[caf_b_loop] > caf_dup_list_start[caf_a_loop]) )
			      {
				if( (caf_dup_list_end[caf_a_loop] - caf_dup_list_start[caf_a_loop]) < (caf_dup_list_end[caf_b_loop] - caf_dup_list_start[caf_b_loop]) )
				{
				  caf_output = 0;
				}
				else if( (caf_dup_list_end[caf_a_loop] - caf_dup_list_start[caf_a_loop]) == (caf_dup_list_end[caf_b_loop] - caf_dup_list_start[caf_b_loop]) && caf_dup_list_pvalue[caf_a_loop] >= caf_dup_list_pvalue[caf_b_loop] )
				{
				  caf_output = 0;
				}

			      }
			    }

				if (g_troubleshooting == 1)
				{
					printf("caf_output=%d DUP 2", caf_output);
				}
			    if( caf_output == 1 )
			    {
			      if( g_vcf == 1 )
			      {

					if (caf_dup_list_pvalue[caf_a_loop] != 0)
					{
						int qual_entry = (int) -10 * log10(caf_dup_list_pvalue[caf_a_loop]);
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_dup_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%d\t.\tSVTYPE=DUP;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_dup_list_start[caf_a_loop] - 1]), qual_entry, caf_dup_list_end[caf_a_loop] + 1, unknown_gt_entry, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					} else {
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_dup_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%s\t.\tSVTYPE=DUP;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_dup_list_start[caf_a_loop] - 1]), infinity_qual_entry, caf_dup_list_end[caf_a_loop] + 1, unknown_gt_entry, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					}
				  }
			      else
			      {
					fprintf(caf_output_file, "%s\t%s\t%ld\t%ld\t%e\t%e\t%e\t%e\n", caf_dup_text, cdp_chr_name, caf_dup_list_start[caf_a_loop] + 1, caf_dup_list_end[caf_a_loop] + 1, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
			      }
			    }
			  }

			}
			else
			{
			  for(caf_a_loop=0;caf_a_loop<caf_dup_list_index;caf_a_loop++)
			  {
			    caf_output = 1;

			    if( caf_output == 1 )
			    {
			      if( g_vcf == 1 )
			      {

					if (caf_dup_list_pvalue[caf_a_loop] != 0)
					{
						int qual_entry = (int) -10 * log10(caf_dup_list_pvalue[caf_a_loop]);
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_dup_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%d\t.\tSVTYPE=DUP;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_dup_list_start[caf_a_loop] - 1]), qual_entry, caf_dup_list_end[caf_a_loop] + 1, unknown_gt_entry, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					} else {
						vcf_sv_data_list[sort_sv_count].chr_name = cdp_chr_name;
						vcf_sv_data_list[sort_sv_count].pos = caf_dup_list_start[caf_a_loop];
						sprintf(vcf_sv_data_list[sort_sv_count].vcf_data, ".\t%c\t<DUP>\t%s\t.\tSVTYPE=DUP;END=%ld\tGT:SD:Z:CN:CS\t%s:%.2e:%.2e:%.2f:%.2e\n", toupper(cdp_chr_fasta[caf_dup_list_start[caf_a_loop] - 1]), infinity_qual_entry, caf_dup_list_end[caf_a_loop] + 1, unknown_gt_entry, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
						sort_sv_count += 1;
					}
				  }
			      else
			      {
					fprintf(caf_output_file, "%s\t%s\t%ld\t%ld\t%e\t%e\t%e\t%e\n", caf_dup_text, cdp_chr_name, caf_dup_list_start[caf_a_loop] + 1, caf_dup_list_end[caf_a_loop] + 1, caf_dup_list_stdev[caf_a_loop], caf_dup_list_pvalue[caf_a_loop], caf_dup_list_cn[caf_a_loop], caf_dup_list_cn_stdev[caf_a_loop]);
			      }
			    }
			  }
			}
		      }


		      free(caf_del_list_start);
		      free(caf_del_list_end);
		      free(caf_del_list_ref);
		      free(caf_del_list_stdev);
		      free(caf_del_list_pvalue);
		      free(caf_del_list_cn);
		      free(caf_del_list_cn_stdev);

		      free(caf_dup_list_start);
		      free(caf_dup_list_end);
		      free(caf_dup_list_ref);
		      free(caf_dup_list_stdev);
		      free(caf_dup_list_pvalue);
		      free(caf_dup_list_cn);
		      free(caf_dup_list_cn_stdev);
	      }
      }

      free(caf_block_rd_list);
      free(caf_block_over_pos_list);
      free(caf_block_over_dist_list);
    }

    free(cdp_rmdup_svtype_list);
    free(cdp_rmdup_tlen_list);
    free(cdp_rmdup_lseq_list);
    free(cdp_rmdup_mpos_list);
    free(cdp_rmdup_mchr_list);


    free(cdp_ctx_r_list2_read_end);
    free(cdp_ctx_r_list2_read_start);
    free(cdp_ctx_r_list2_other_len);
    free(cdp_ctx_r_list2_mpos);
    free(cdp_ctx_r_list2_mchr);
    free(cdp_ctx_r_list2_hez_binom_cdf);
    free(cdp_ctx_r_list2_binom_cdf);
    free(cdp_ctx_r_list2_conc);
    free(cdp_ctx_r_list2_rd);
    free(cdp_ctx_r_list2_ctx_r);
    free(cdp_ctx_r_list2);

    free(cdp_ctx_f_list2_read_end);
    free(cdp_ctx_f_list2_read_start);
    free(cdp_ctx_f_list2_other_len);
    free(cdp_ctx_f_list2_mpos);
    free(cdp_ctx_f_list2_mchr);
    free(cdp_ctx_f_list2_hez_binom_cdf);
    free(cdp_ctx_f_list2_binom_cdf);
    free(cdp_ctx_f_list2_conc);
    free(cdp_ctx_f_list2_rd);
    free(cdp_ctx_f_list2_ctx_f);
    free(cdp_ctx_f_list2);

    free(cdp_inv_r_list2_end_read_end);
    free(cdp_inv_r_list2_end_read_start);
    free(cdp_inv_r_list2_start_read_end);
    free(cdp_inv_r_list2_start_read_start);
    free(cdp_inv_r_list2_end_other_len);
    free(cdp_inv_r_list2_start_other_len);
    free(cdp_inv_r_list2_dist);
    free(cdp_inv_r_list2_end_hez_binom_cdf);
    free(cdp_inv_r_list2_end_binom_cdf);
    free(cdp_inv_r_list2_end_conc);
    free(cdp_inv_r_list2_end_rd);
    free(cdp_inv_r_list2_end_inv);
    free(cdp_inv_r_list2_end);
    free(cdp_inv_r_list2_start_hez_binom_cdf);
    free(cdp_inv_r_list2_start_binom_cdf);
    free(cdp_inv_r_list2_start_conc);
    free(cdp_inv_r_list2_start_rd);
    free(cdp_inv_r_list2_start_inv);
    free(cdp_inv_r_list2_start);

    free(cdp_inv_f_list2_end_read_end);
    free(cdp_inv_f_list2_end_read_start);
    free(cdp_inv_f_list2_start_read_end);
    free(cdp_inv_f_list2_start_read_start);
    free(cdp_inv_f_list2_end_other_len);
    free(cdp_inv_f_list2_start_other_len);
    free(cdp_inv_f_list2_dist);
    free(cdp_inv_f_list2_end_hez_binom_cdf);
    free(cdp_inv_f_list2_end_binom_cdf);
    free(cdp_inv_f_list2_end_conc);
    free(cdp_inv_f_list2_end_rd);
    free(cdp_inv_f_list2_end_inv);
    free(cdp_inv_f_list2_end);
    free(cdp_inv_f_list2_start_hez_binom_cdf);
    free(cdp_inv_f_list2_start_binom_cdf);
    free(cdp_inv_f_list2_start_conc);
    free(cdp_inv_f_list2_start_rd);
    free(cdp_inv_f_list2_start_inv);
    free(cdp_inv_f_list2_start);

    free(cdp_del_list2_end_read_end);
    free(cdp_del_list2_end_read_start);
    free(cdp_del_list2_start_read_end);
    free(cdp_del_list2_start_read_start);
    free(cdp_del_list2_end_other_len);
    free(cdp_del_list2_start_other_len);
    free(cdp_del_list2_dist);
    free(cdp_del_list2_end_hez_binom_cdf);
    free(cdp_del_list2_end_binom_cdf);
    free(cdp_del_list2_end_conc);
    free(cdp_del_list2_end_rd);
    free(cdp_del_list2_end_del_r);
    free(cdp_del_list2_end);
    free(cdp_del_list2_start_hez_binom_cdf);
    free(cdp_del_list2_start_binom_cdf);
    free(cdp_del_list2_start_conc);
    free(cdp_del_list2_start_rd);
    free(cdp_del_list2_start_del_f);
    free(cdp_del_list2_start);

    free(cdp_dup_list2_end_read_end);
    free(cdp_dup_list2_end_read_start);
    free(cdp_dup_list2_start_read_end);
    free(cdp_dup_list2_start_read_start);
    free(cdp_dup_list2_end_other_len);
    free(cdp_dup_list2_start_other_len);
    free(cdp_dup_list2_dist);
    free(cdp_dup_list2_end_hez_binom_cdf);
    free(cdp_dup_list2_end_binom_cdf);
    free(cdp_dup_list2_end_conc);
    free(cdp_dup_list2_end_rd);
    free(cdp_dup_list2_end_dup_f);
    free(cdp_dup_list2_end);
    free(cdp_dup_list2_start_hez_binom_cdf);
    free(cdp_dup_list2_start_binom_cdf);
    free(cdp_dup_list2_start_conc);
    free(cdp_dup_list2_start_rd);
    free(cdp_dup_list2_start_dup_r);
    free(cdp_dup_list2_start);

    free(cdp_ins_list2_end_other_len);
    free(cdp_ins_list2_start_other_len);
    free(cdp_ins_list2_end_binom_cdf);
    free(cdp_ins_list2_end_conc);
    free(cdp_ins_list2_end_rd);
    free(cdp_ins_list2_end_ins);
    free(cdp_ins_list2_end);
    free(cdp_ins_list2_start_binom_cdf);
    free(cdp_ins_list2_start_conc);
    free(cdp_ins_list2_start_rd);
    free(cdp_ins_list2_start_ins);
    free(cdp_ins_list2_start);

    free(cdp_ctx_r_list_read_end);
    free(cdp_ctx_r_list_read_start);
    free(cdp_ctx_r_list_other_len);
    free(cdp_ctx_r_list_mpos);
    free(cdp_ctx_r_list_mchr);
    free(cdp_ctx_r_list_hez_binom_cdf);
    free(cdp_ctx_r_list_binom_cdf);
    free(cdp_ctx_r_list_conc);
    free(cdp_ctx_r_list_rd);
    free(cdp_ctx_r_list_ctx_r);
    free(cdp_ctx_r_list);

    free(cdp_ctx_f_list_read_end);
    free(cdp_ctx_f_list_read_start);
    free(cdp_ctx_f_list_other_len);
    free(cdp_ctx_f_list_mpos);
    free(cdp_ctx_f_list_mchr);
    free(cdp_ctx_f_list_hez_binom_cdf);
    free(cdp_ctx_f_list_binom_cdf);
    free(cdp_ctx_f_list_conc);
    free(cdp_ctx_f_list_rd);
    free(cdp_ctx_f_list_ctx_f);
    free(cdp_ctx_f_list);

    free(cdp_indel_d_list_end_sc);
    free(cdp_indel_d_list_start_sc);
    free(cdp_indel_d_list_end_rd);
    free(cdp_indel_d_list_start_rd);
    free(cdp_indel_d_list_end_r);
    free(cdp_indel_d_list_start_f);
    free(cdp_indel_d_list_end_other_len);
    free(cdp_indel_d_list_start_other_len);
    free(cdp_indel_d_list_end_hez_binom_cdf);
    free(cdp_indel_d_list_end_binom_cdf);
    free(cdp_indel_d_list_end_conc);
    free(cdp_indel_d_list_end);
    free(cdp_indel_d_list_start_hez_binom_cdf);
    free(cdp_indel_d_list_start_binom_cdf);
    free(cdp_indel_d_list_start_conc);
    free(cdp_indel_d_list_start);

    free(cdp_indel_i_list_start_sc);
    free(cdp_indel_i_list_start_rd);
    free(cdp_indel_i_list_start_i);
    free(cdp_indel_i_list_end_other_len);
    free(cdp_indel_i_list_start_other_len);
    free(cdp_indel_i_list_dist);
    free(cdp_indel_i_list_end_conc);
    free(cdp_indel_i_list_end);
    free(cdp_indel_i_list_start_hez_binom_cdf);
    free(cdp_indel_i_list_start_binom_cdf);
    free(cdp_indel_i_list_start_conc);
    free(cdp_indel_i_list_start);

    for(cdp_a_loop=0;cdp_a_loop<g_indel_i_seq_len;cdp_a_loop++)
    {
      free(cdp_indel_i_list_seq[cdp_a_loop]);
    }
    free(cdp_indel_i_list_seq);

    free(cdp_inv_r_list_end_read_end);
    free(cdp_inv_r_list_end_read_start);
    free(cdp_inv_r_list_start_read_end);
    free(cdp_inv_r_list_start_read_start);
    free(cdp_inv_r_list_end_other_len);
    free(cdp_inv_r_list_start_other_len);
    free(cdp_inv_r_list_dist);
    free(cdp_inv_r_list_end_hez_binom_cdf);
    free(cdp_inv_r_list_end_binom_cdf);
    free(cdp_inv_r_list_end_conc);
    free(cdp_inv_r_list_end_rd);
    free(cdp_inv_r_list_end_inv);
    free(cdp_inv_r_list_end);
    free(cdp_inv_r_list_start_hez_binom_cdf);
    free(cdp_inv_r_list_start_binom_cdf);
    free(cdp_inv_r_list_start_conc);
    free(cdp_inv_r_list_start_rd);
    free(cdp_inv_r_list_start_inv);
    free(cdp_inv_r_list_start);

    free(cdp_inv_f_list_end_read_end);
    free(cdp_inv_f_list_end_read_start);
    free(cdp_inv_f_list_start_read_end);
    free(cdp_inv_f_list_start_read_start);
    free(cdp_inv_f_list_end_other_len);
    free(cdp_inv_f_list_start_other_len);
    free(cdp_inv_f_list_dist);
    free(cdp_inv_f_list_end_hez_binom_cdf);
    free(cdp_inv_f_list_end_binom_cdf);
    free(cdp_inv_f_list_end_conc);
    free(cdp_inv_f_list_end_rd);
    free(cdp_inv_f_list_end_inv);
    free(cdp_inv_f_list_end);
    free(cdp_inv_f_list_start_hez_binom_cdf);
    free(cdp_inv_f_list_start_binom_cdf);
    free(cdp_inv_f_list_start_conc);
    free(cdp_inv_f_list_start_rd);
    free(cdp_inv_f_list_start_inv);
    free(cdp_inv_f_list_start);

    free(cdp_del_list_end_read_end);
    free(cdp_del_list_end_read_start);
    free(cdp_del_list_start_read_end);
    free(cdp_del_list_start_read_start);
    free(cdp_del_list_end_other_len);
    free(cdp_del_list_start_other_len);
    free(cdp_del_list_dist);
    free(cdp_del_list_end_hez_binom_cdf);
    free(cdp_del_list_end_binom_cdf);
    free(cdp_del_list_end_conc);
    free(cdp_del_list_end_rd);
    free(cdp_del_list_end_del_r);
    free(cdp_del_list_end);
    free(cdp_del_list_start_hez_binom_cdf);
    free(cdp_del_list_start_binom_cdf);
    free(cdp_del_list_start_conc);
    free(cdp_del_list_start_rd);
    free(cdp_del_list_start_del_f);
    free(cdp_del_list_start);

    free(cdp_dup_list_end_read_end);
    free(cdp_dup_list_end_read_start);
    free(cdp_dup_list_start_read_end);
    free(cdp_dup_list_start_read_start);
    free(cdp_dup_list_end_other_len);
    free(cdp_dup_list_start_other_len);
    free(cdp_dup_list_dist);
    free(cdp_dup_list_end_hez_binom_cdf);
    free(cdp_dup_list_end_binom_cdf);
    free(cdp_dup_list_end_conc);
    free(cdp_dup_list_end_rd);
    free(cdp_dup_list_end_dup_f);
    free(cdp_dup_list_end);
    free(cdp_dup_list_start_hez_binom_cdf);
    free(cdp_dup_list_start_binom_cdf);
    free(cdp_dup_list_start_conc);
    free(cdp_dup_list_start_rd);
    free(cdp_dup_list_start_dup_r);
    free(cdp_dup_list_start);

    free(cdp_ins_list_end_other_len);
    free(cdp_ins_list_start_other_len);
    free(cdp_ins_list_end_binom_cdf);
    free(cdp_ins_list_end_conc);
    free(cdp_ins_list_end_rd);
    free(cdp_ins_list_end_ins);
    free(cdp_ins_list_end);
    free(cdp_ins_list_start_binom_cdf);
    free(cdp_ins_list_start_conc);
    free(cdp_ins_list_start_rd);
    free(cdp_ins_list_start_ins);
    free(cdp_ins_list_start);

    free(cdp_snv_start_hez_binom_cdf_list);
    free(cdp_snv_start_binom_cdf_list);

    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      free(cdp_snv_fstrand_list[cdp_a_loop]);
    }
    free(cdp_snv_fstrand_list);
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      free(cdp_snv_pos_in_read_list[cdp_a_loop]);
    }
    free(cdp_snv_pos_in_read_list);

    free(cdp_snv_bq_list);
    free(cdp_snv_bq_all_list);
    free(cdp_snv_mq_list);
    free(cdp_snv_mq_all_list);
    free(cdp_snv_bq_read_count_list);
    free(cdp_snv_mq_read_count_list);
    free(cdp_snv_read_count_all_list);

    free(cdp_snv_low_mq_rd_list);
    free(cdp_snv_rd_list);
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      free(cdp_snv_lowmq_list[cdp_a_loop]);
    }
    free(cdp_snv_lowmq_list);
    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      free(cdp_snv_list[cdp_a_loop]);
    }
    free(cdp_snv_list);
    free(cdp_snv_ratio_list);
    free(cdp_snv_base_list);
    free(cdp_snv_pos_list);


	qsort(vcf_indel_data_list, sort_indel_count, sizeof(vcf_chr_pos_data_t), compare_chr_pos);
	qsort(vcf_sv_data_list, sort_sv_count, sizeof(vcf_chr_pos_data_t), compare_chr_pos);

	int sort_indel_index;
	for (sort_indel_index = 0; sort_indel_index < sort_indel_count; sort_indel_index++)
	{
		fprintf(cdp_results_file_ind, "%s\t%d\t%s", vcf_indel_data_list[sort_indel_index].chr_name, vcf_indel_data_list[sort_indel_index].pos, vcf_indel_data_list[sort_indel_index].vcf_data);
	}
	int sort_sv_index;
	for (sort_sv_index = 0; sort_sv_index < sort_sv_count; sort_sv_index++)
	{
		fprintf(cdp_results_file_sv, "%s\t%d\t%s", vcf_sv_data_list[sort_sv_index].chr_name, vcf_sv_data_list[sort_sv_index].pos, vcf_sv_data_list[sort_sv_index].vcf_data);
	}


	free(vcf_indel_data_list);
	free(vcf_sv_data_list);

    if( g_internal == 1 )
    {
      printf("before free counters\n");
    }


    for(cdp_a_loop=0;cdp_a_loop<g_min_snv;cdp_a_loop++)
    {
      for(cdp_b_loop=0;cdp_b_loop<g_one_base_rd_len;cdp_b_loop++)
      {
	free(cdp_one_base_snv_read_names[cdp_a_loop][cdp_b_loop]);
      }
      free(cdp_one_base_snv_read_names[cdp_a_loop]);
    }
    free(cdp_one_base_snv_read_names);

    for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
    {
      free(cdp_one_base_snv_sc[cdp_other_loop]);
    }
    if( g_internal == 1 )
    {
      printf("before free SNV_sc pointer\n");
    }
    free(cdp_one_base_snv_sc);

    for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
    {
      free(cdp_one_base_snv_lowmq[cdp_other_loop]);
    }
    free(cdp_one_base_snv_lowmq);

    for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
    {
      free(cdp_one_base_snv[cdp_other_loop]);
    }
    free(cdp_one_base_snv);

    if( g_internal == 1 )
    {
      printf("after free SNV counters\n");
    }

    for(cdp_other_loop=0;cdp_other_loop<g_other_len;cdp_other_loop++)
    {
      free(cdp_one_base_other_read_end[cdp_other_loop]);
      free(cdp_one_base_other_read_start[cdp_other_loop]);
      free(cdp_one_base_other_dist[cdp_other_loop]);
      free(cdp_one_base_other_mchr[cdp_other_loop]);
      free(cdp_one_base_other_type[cdp_other_loop]);
      free(cdp_one_base_other[cdp_other_loop]);
    }

    free(cdp_one_base_other_read_end);
    free(cdp_one_base_other_read_start);
    free(cdp_one_base_other_dist);
    free(cdp_one_base_other_mchr);
    free(cdp_one_base_other_type);
    free(cdp_one_base_other);

    for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
    {
      free(cdp_one_base_fstrand[cdp_other_loop]);
    }
    free(cdp_one_base_fstrand);
    for(cdp_other_loop=0;cdp_other_loop<g_nucleotides;cdp_other_loop++)
    {
      free(cdp_one_base_pos_in_read[cdp_other_loop]);
    }
    free(cdp_one_base_pos_in_read);

    free(cdp_one_base_read_count_all);
    free(cdp_one_base_mq_read_count);
    free(cdp_one_base_bq_read_count);
    free(cdp_one_base_mq_all);
    free(cdp_one_base_mq);
    free(cdp_one_base_bq_all);
    free(cdp_one_base_bq);

    free(cdp_one_base_indel_sc_rd);
    free(cdp_one_base_indel_sc_right_rd);
    free(cdp_one_base_indel_sc_left_rd);
    free(cdp_one_base_indel_sc_right);
    free(cdp_one_base_indel_sc_left);

    free(cdp_one_base_ctx_sc_rd);
    free(cdp_one_base_ctx_sc_right_rd);
    free(cdp_one_base_ctx_sc_left_rd);
    free(cdp_one_base_ctx_sc_right);
    free(cdp_one_base_ctx_sc_left);

    free(cdp_one_base_indel_d_r_rd);
    free(cdp_one_base_indel_d_rdist);
    free(cdp_one_base_indel_d_r);
    free(cdp_one_base_indel_d_f_rd);
    free(cdp_one_base_indel_d_fdist);
    free(cdp_one_base_indel_d_f);
    free(cdp_one_base_indel_idist);

    for(cdp_a_loop=0;cdp_a_loop<g_indel_i_seq_len;cdp_a_loop++)
    {
      free(cdp_one_base_indel_i_seq[cdp_a_loop]);
    }
    free(cdp_one_base_indel_i_seq);

    free(cdp_one_base_indel_i);

    free(cdp_one_base_munmapped_r);
    free(cdp_one_base_munmapped_f);
    free(cdp_one_base_ctx_r_read_end);
    free(cdp_one_base_ctx_f_read_end);
    free(cdp_one_base_ctx_r_read_start);
    free(cdp_one_base_ctx_f_read_start);
    free(cdp_one_base_ctx_r_mpos);
    free(cdp_one_base_ctx_f_mpos);
    free(cdp_one_base_ctx_r_mchr);
    free(cdp_one_base_ctx_f_mchr);
    free(cdp_one_base_ctx_r);
    free(cdp_one_base_ctx_f);

    free(cdp_one_base_inv_r2_read_end);
    free(cdp_one_base_inv_f2_read_end);
    free(cdp_one_base_inv_r2_read_start);
    free(cdp_one_base_inv_f2_read_start);
    free(cdp_one_base_inv_r2dist);
    free(cdp_one_base_inv_f2dist);
    free(cdp_one_base_inv_r2);
    free(cdp_one_base_inv_f2);
    free(cdp_one_base_inv_r1_read_end);
    free(cdp_one_base_inv_f1_read_end);
    free(cdp_one_base_inv_r1_read_start);
    free(cdp_one_base_inv_f1_read_start);
    free(cdp_one_base_inv_r1dist);
    free(cdp_one_base_inv_f1dist);
    free(cdp_one_base_inv_r1);
    free(cdp_one_base_inv_f1);

    if( g_internal == 1 )
    {
      printf("after free inv counters\n");
    }

    free(cdp_one_base_dup_r_read_end);
    free(cdp_one_base_dup_f_read_end);
    free(cdp_one_base_dup_r_read_start);
    free(cdp_one_base_dup_f_read_start);
    free(cdp_one_base_dup_rdist);
    free(cdp_one_base_dup_fdist);
    free(cdp_one_base_dup_r);
    free(cdp_one_base_dup_f);

    free(cdp_one_base_del_r_read_end);
    free(cdp_one_base_del_f_read_end);
    free(cdp_one_base_del_r_read_start);
    free(cdp_one_base_del_f_read_start);
    free(cdp_one_base_del_rdist);
    free(cdp_one_base_del_fdist);
    free(cdp_one_base_del_r);
    free(cdp_one_base_del_f);

    free(cdp_one_base_ins);
    free(cdp_one_base_sc_rd);
    free(cdp_one_base_sc_right_rd);
    free(cdp_one_base_sc_left_rd);
    free(cdp_one_base_sc_right);
    free(cdp_one_base_sc_left);

    free(cdp_one_base_conc);
    free(cdp_one_base_rd);
 

    free(caf_repeat_type_list);
    free(caf_repeat_start_list);
    free(caf_repeat_end_list);

    free(caf_n_blocks_end);
    free(caf_n_blocks_start);
    free(caf_one_base_rd_gc_weighted);
    free(caf_one_base_rd_acgt_weighted);
    free(caf_rd_mq_list);
    free(caf_rd_rd_list);
    free(caf_rd_low_mq_rd_list);

    if( g_internal == 1 )
    {
      printf("before free tumor SV predictions\n");
    }

 
    for(cdp_a_loop=0;cdp_a_loop<cdp_snv_gt_string_len;cdp_a_loop++)
    {
      free(cdp_tumor_indel_seq[cdp_a_loop]);
    }
    free(cdp_tumor_indel_seq);
    free(cdp_tumor_indel_seq_index);
    free(cdp_tumor_hp);

    free(cdp_tumor_snv_start_hez_binom_cdf);
    free(cdp_tumor_snv_start_binom_cdf);

    free(cdp_tumor_snv_fstrand);
    free(cdp_tumor_snv_pos_in_read);

    free(cdp_tumor_snv_read_count_all);
    free(cdp_tumor_snv_mq_read_count);
    free(cdp_tumor_snv_bq_read_count);
    free(cdp_tumor_snv_mq_all);
    free(cdp_tumor_snv_mq);
    free(cdp_tumor_snv_bq_all);
    free(cdp_tumor_snv_bq);

    free(cdp_tumor_end_sc);
    free(cdp_tumor_start_sc);
    free(cdp_tumor_end_rd);
    free(cdp_tumor_start_rd);
    free(cdp_tumor_end_indel);
    free(cdp_tumor_start_indel);

    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      free(cdp_tumor_snv_lowmq[cdp_a_loop]);
    }
    free(cdp_tumor_snv_lowmq);

    for(cdp_a_loop=0;cdp_a_loop<g_nucleotides;cdp_a_loop++)
    {
      free(cdp_tumor_snv[cdp_a_loop]);
    }
    free(cdp_tumor_snv);
    free(cdp_tumor_snv_ratio);
    free(cdp_tumor_snv_base);

    free(cdp_tumor_end_hez_binom_cdf);
    free(cdp_tumor_start_hez_binom_cdf);
    free(cdp_tumor_end_read_end);
    free(cdp_tumor_end_read_start);
    free(cdp_tumor_start_read_end);
    free(cdp_tumor_start_read_start);
    free(cdp_tumor_mpos);
    free(cdp_tumor_mchr);
    free(cdp_tumor_end_other_len);
    free(cdp_tumor_start_other_len);
    free(cdp_tumor_end_conc);
    free(cdp_tumor_start_conc);
    free(cdp_tumor_end_sv_evidence);
    free(cdp_tumor_start_sv_evidence);
    free(cdp_tumor_end_binom_cdf);
    free(cdp_tumor_start_binom_cdf);
    free(cdp_tumor_dist);

    for(cdp_a_loop=0;cdp_a_loop<cdp_tumor_len;cdp_a_loop++)
    {
      free(cdp_tumor_end2[cdp_a_loop]);
    }

    free(cdp_tumor_end2);

    for(cdp_a_loop=0;cdp_a_loop<cdp_tumor_len;cdp_a_loop++)
    {
      free(cdp_tumor_start2[cdp_a_loop]);
    }

    free(cdp_tumor_start2);
    free(cdp_tumor_end);
    free(cdp_tumor_start);
    free(cdp_tumor_type);


    if( g_internal == 1 )
    {
      printf("after free tumor SV predictions\n");
    }

  }


#ifdef DO_TIMING
  int cdp_loop;
  for(cdp_loop=0;cdp_loop<10;cdp_loop++)
  {
    printf("timer %d %lld\n", cdp_loop, timers_ss[cdp_loop]);
  }
#endif

  return;
}

void detect_del_dup(int ddd_chr, int ddd_begin, long ddd_chr_fasta_len, int *ddd_one_base_rd_gc_weighted, int *ddd_one_base_rd_acgt_weighted, int *ddd_rd_mq_list, int *ddd_rd_rd_list, int *ddd_rd_low_mq_rd_list, int **ddd_sample_high_mq_rd_list, int **ddd_sample_low_mq_rd_list, int **ddd_sample_repeat_rd_list, long *ddd_low_mq_index, long *ddd_high_mq_index, long *ddd_low_mq_index_all, long *ddd_high_mq_index_all, double *ddd_pval2sd_pval_list, double *ddd_pval2sd_sd_list, int ddd_pval2sd_list_len, long *ddd_del_list_index, int *ddd_del_list_ref, long *ddd_del_list_start, long *ddd_del_list_end, double *ddd_del_list_stdev, double *ddd_del_list_cn, double *ddd_del_list_cn_stdev, long *ddd_dup_list_index, int *ddd_dup_list_ref, long *ddd_dup_list_start, long *ddd_dup_list_end, double *ddd_dup_list_stdev, double *ddd_dup_list_cn, double *ddd_dup_list_cn_stdev, int ddd_ploidy, int *ddd_repeat_type_list, long *ddd_repeat_start_list, long *ddd_repeat_end_list, long ddd_repeat_index, char *ddd_results_file_name, char *ddd_chr_name)
{

	if (g_troubleshooting == 1)
	{
		printf("detect del dup start\n");
	}
	int32_t ddd_pos = 0;

	int ddd_last_low_mq = 0;
	long ddd_block_loop;
	long ddd_a_loop, ddd_b_loop, ddd_c_loop;
	long ddd_repeat_loop;
	int ddd_repeat_segment;

	double ddd_del_ploidy_threshold, ddd_dup_ploidy_threshold;

	ddd_del_ploidy_threshold = (1.0 - g_ploidy_threshold_numerator/ddd_ploidy);
	ddd_dup_ploidy_threshold = (1.0 + g_ploidy_threshold_numerator/ddd_ploidy);


	for(ddd_a_loop=0;ddd_a_loop<g_num_gc_bins;ddd_a_loop++)
	{
		ddd_low_mq_index[ddd_a_loop] = 0;
		ddd_high_mq_index[ddd_a_loop] = 0;
		ddd_low_mq_index_all[ddd_a_loop] = 0;
		ddd_high_mq_index_all[ddd_a_loop] = 0;
	}



	long ddd_most_biased_index[g_repeat_segments];
	long ddd_most_biased_index_all[g_repeat_segments];
	for(ddd_a_loop=0;ddd_a_loop<g_repeat_segments;ddd_a_loop++)
	{
	  ddd_most_biased_index[ddd_a_loop] = 0;
	  ddd_most_biased_index_all[ddd_a_loop] = 0;
	}
	if( g_most_biased_repeat != -1 )
	{
	  for(ddd_repeat_loop=0;ddd_repeat_loop<ddd_repeat_index;ddd_repeat_loop++)
	  {
	    if( ddd_repeat_type_list[ddd_repeat_loop] == g_most_biased_repeat )
	    {
	      for(ddd_pos=(ddd_repeat_start_list[ddd_repeat_loop]-(g_insert_mean/2));ddd_pos<(ddd_repeat_end_list[ddd_repeat_loop]+(g_insert_mean/2));ddd_pos++)
	      {
		if( ddd_one_base_rd_acgt_weighted[ddd_pos] >= g_insert_min_acgt )
		{
		  if( ddd_pos < ddd_repeat_start_list[ddd_repeat_loop] )
		  {
		    ddd_repeat_segment = (g_repeat_segments - 1) * (ddd_pos - (ddd_repeat_start_list[ddd_repeat_loop]-(g_insert_mean/2)))/(g_insert_mean/2);
		  }
		  else if( ddd_pos >= ddd_repeat_end_list[ddd_repeat_loop] )
		  {
		    ddd_repeat_segment = (g_repeat_segments - 1) * ((ddd_repeat_end_list[ddd_repeat_loop]+(g_insert_mean/2)) - ddd_pos)/(g_insert_mean/2);
		  }
		  else
		  {
		    ddd_repeat_segment = g_repeat_segments - 1;
		  }
		  if( ddd_most_biased_index[ddd_repeat_segment] < g_sample_lists_len )
		  {
		    ddd_sample_repeat_rd_list[ddd_repeat_segment][ddd_most_biased_index[ddd_repeat_segment]] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
		    ddd_most_biased_index[ddd_repeat_segment] += 1;
		    ddd_most_biased_index_all[ddd_repeat_segment] += 1;
		  }
		  else
		  {
		    if( grom_rand(ddd_most_biased_index_all[ddd_repeat_segment]) == 0 )
		    {
		      ddd_sample_repeat_rd_list[ddd_repeat_segment][grom_rand(ddd_most_biased_index[ddd_repeat_segment])] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
		    }
		    ddd_most_biased_index_all[ddd_repeat_segment] += 1;
		  }
		}
	      }
	    }
	  }
	}



	for(ddd_repeat_loop=0;ddd_repeat_loop<g_repeat_segments;ddd_repeat_loop++)
	{
		if( ddd_most_biased_index[ddd_repeat_loop] > 1 )
		{
			qsort(ddd_sample_repeat_rd_list[ddd_repeat_loop], ddd_most_biased_index[ddd_repeat_loop], sizeof(int), cmpfunc);
		}
	}




	long ddd_repeat_segment_len, ddd_repeat_segment_start_for_ave, ddd_repeat_segment_end_for_ave;
	double ddd_rd_ave_by_repeat_segment[g_repeat_segments];
	double ddd_rd_stdev_by_repeat_segment[g_repeat_segments];

	if( g_most_biased_repeat != -1 )
	{
		for(ddd_repeat_loop=0;ddd_repeat_loop<g_repeat_segments;ddd_repeat_loop++)
		{
			ddd_rd_stdev_by_repeat_segment[ddd_repeat_loop] = 0.0;

			ddd_rd_ave_by_repeat_segment[ddd_repeat_loop] = 0.0;
			if( ddd_most_biased_index[ddd_repeat_loop] > 0 )
			{
				ddd_repeat_segment_start_for_ave = ddd_most_biased_index[ddd_repeat_loop]/20;
				ddd_repeat_segment_end_for_ave = ddd_most_biased_index[ddd_repeat_loop] - ddd_repeat_segment_start_for_ave;
				ddd_repeat_segment_len = ddd_repeat_segment_end_for_ave - ddd_repeat_segment_start_for_ave;
				for( ddd_a_loop=ddd_repeat_segment_start_for_ave;ddd_a_loop<ddd_repeat_segment_end_for_ave;ddd_a_loop++)
				{
					ddd_rd_ave_by_repeat_segment[ddd_repeat_loop] += ddd_sample_repeat_rd_list[ddd_repeat_loop][ddd_a_loop];
				}
				ddd_rd_ave_by_repeat_segment[ddd_repeat_loop] = ddd_rd_ave_by_repeat_segment[ddd_repeat_loop]/ddd_repeat_segment_len;
				for(ddd_a_loop=ddd_repeat_segment_start_for_ave;ddd_a_loop<ddd_repeat_segment_end_for_ave;ddd_a_loop++)
				{
					ddd_rd_stdev_by_repeat_segment[ddd_repeat_loop] += pow((ddd_sample_repeat_rd_list[ddd_repeat_loop][ddd_a_loop] - ddd_rd_ave_by_repeat_segment[ddd_repeat_loop]), 2);
				}
				if( ddd_repeat_segment_len > 1 )
				{
					ddd_rd_stdev_by_repeat_segment[ddd_repeat_loop] = sqrt(ddd_rd_stdev_by_repeat_segment[ddd_repeat_loop]/(ddd_repeat_segment_len-1));
				}
			}
			else
			{
				ddd_rd_ave_by_repeat_segment[ddd_repeat_loop] = 0.0;
			}

		}
	}


	for(ddd_block_loop=0;ddd_block_loop<g_lowvar_block_sample_index;ddd_block_loop++)
	{
    for(ddd_pos=g_lowvar_block_sample_start_list[ddd_block_loop];ddd_pos<g_lowvar_block_sample_end_list[ddd_block_loop];ddd_pos+=(g_insert_mean/2))
		{
			if( ddd_one_base_rd_acgt_weighted[ddd_pos] >= g_insert_min_acgt )
			{
				if( ddd_rd_rd_list[ddd_pos] == 0 && ddd_rd_low_mq_rd_list[ddd_pos] == 0 )
				{
					if( ddd_last_low_mq == 0 )
					{
						if( ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] < g_sample_lists_len )
						{
							ddd_sample_high_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]]] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
							ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
							ddd_high_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
						}
						else
						{
							if( grom_rand(ddd_high_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]]) == 0 )
							{
								ddd_sample_high_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][grom_rand(ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]])] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
							}
							ddd_high_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
						}
					}
					else
					{
						if( ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] < g_sample_lists_len )
						{
							ddd_sample_low_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]]] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
							ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
							ddd_low_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
						}
						else
						{
							if( grom_rand(ddd_low_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]]) == 0 )
							{
								ddd_sample_low_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][grom_rand(ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]])] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
							}
							ddd_low_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
						}
					}
				}
				else if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
				{
					if( ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] < g_sample_lists_len )
					{
						ddd_sample_high_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]]] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
						ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
						ddd_high_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
					}
					else
					{
						if( grom_rand(ddd_high_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]]) == 0 )
						{
							ddd_sample_high_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][grom_rand(ddd_high_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]])] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
						}
						ddd_high_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
					}
					ddd_last_low_mq = 0;
				}
				else
				{
					if( ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] < g_sample_lists_len )
					{
						ddd_sample_low_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]]] = 	ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
						ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
						ddd_low_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
					}
					else
					{
						if( grom_rand(ddd_low_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]]) == 0 )
						{
							ddd_sample_low_mq_rd_list[ddd_one_base_rd_gc_weighted[ddd_pos]][grom_rand(ddd_low_mq_index[ddd_one_base_rd_gc_weighted[ddd_pos]])] = ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos];
						}
						ddd_low_mq_index_all[ddd_one_base_rd_gc_weighted[ddd_pos]] += 1;
					}
					ddd_last_low_mq = 1;
				}
			}
		}
	}




	int ddd_gc_bin;
	for(ddd_gc_bin=0;ddd_gc_bin<g_num_gc_bins;ddd_gc_bin++)
	{

		if( ddd_high_mq_index[ddd_gc_bin] > 1 )
		{
			qsort(ddd_sample_high_mq_rd_list[ddd_gc_bin], ddd_high_mq_index[ddd_gc_bin], sizeof(int), cmpfunc);
		}

		if( ddd_low_mq_index[ddd_gc_bin] > 1 )
		{
			qsort(ddd_sample_low_mq_rd_list[ddd_gc_bin], ddd_low_mq_index[ddd_gc_bin], sizeof(int), cmpfunc);
		}
	}




	long ddd_temp_low_mq_index[g_num_gc_bins];
	long ddd_temp_high_mq_index[g_num_gc_bins];
	for(ddd_a_loop=0;ddd_a_loop<g_num_gc_bins;ddd_a_loop++)
	{
		ddd_temp_low_mq_index[ddd_a_loop] = ddd_low_mq_index[ddd_a_loop];
		ddd_temp_high_mq_index[ddd_a_loop] = ddd_high_mq_index[ddd_a_loop];
	}
	for(ddd_gc_bin=0;ddd_gc_bin<g_num_gc_bins;ddd_gc_bin++)
	{

		if( ddd_gc_bin >= 2 && ddd_gc_bin < (g_num_gc_bins - 2) && ddd_high_mq_index[ddd_gc_bin] >= g_rd_min_windows && ddd_high_mq_index[ddd_gc_bin] < g_rd_no_combine_min_windows )
		{
			for(ddd_a_loop=(ddd_gc_bin-2);ddd_a_loop<=(ddd_gc_bin+2);ddd_a_loop++)
			{
				if( ddd_a_loop != ddd_gc_bin )
				{
					for(ddd_b_loop=0;ddd_b_loop<ddd_high_mq_index[ddd_a_loop];ddd_b_loop++)
					{
						if( ddd_temp_high_mq_index[ddd_gc_bin] < g_sample_lists_len )
						{
							ddd_sample_high_mq_rd_list[ddd_gc_bin][ddd_temp_high_mq_index[ddd_gc_bin]] = ddd_sample_high_mq_rd_list[ddd_a_loop][ddd_b_loop];
							ddd_temp_high_mq_index[ddd_gc_bin] += 1;
						}
					}
				}
			}
		}



		if( ddd_gc_bin >= 2 && ddd_gc_bin < (g_num_gc_bins - 2) && ddd_low_mq_index[ddd_gc_bin] >= g_rd_min_windows && ddd_low_mq_index[ddd_gc_bin] < g_rd_no_combine_min_windows )
		{
			for(ddd_a_loop=(ddd_gc_bin-2);ddd_a_loop<=(ddd_gc_bin+2);ddd_a_loop++)
			{
				if( ddd_a_loop != ddd_gc_bin )
				{
					for(ddd_b_loop=0;ddd_b_loop<ddd_low_mq_index[ddd_a_loop];ddd_b_loop++)
					{
						if( ddd_temp_low_mq_index[ddd_gc_bin] < g_sample_lists_len )
						{
							ddd_sample_low_mq_rd_list[ddd_gc_bin][ddd_temp_low_mq_index[ddd_gc_bin]] = ddd_sample_low_mq_rd_list[ddd_a_loop][ddd_b_loop];
							ddd_temp_low_mq_index[ddd_gc_bin] += 1;
						}
					}
				}
			}
		}

	}


	for(ddd_gc_bin=0;ddd_gc_bin<g_num_gc_bins;ddd_gc_bin++)
	{

		if( ddd_gc_bin >= 2 && ddd_gc_bin < (g_num_gc_bins - 2) && ddd_high_mq_index[ddd_gc_bin] >= g_rd_min_windows && ddd_high_mq_index[ddd_gc_bin] < g_rd_no_combine_min_windows )
		{
			ddd_high_mq_index[ddd_gc_bin] = ddd_temp_high_mq_index[ddd_gc_bin];
			qsort(ddd_sample_high_mq_rd_list[ddd_gc_bin], ddd_high_mq_index[ddd_gc_bin], sizeof(int), cmpfunc);
		}


		if( ddd_gc_bin >= 2 && ddd_gc_bin < (g_num_gc_bins - 2) && ddd_low_mq_index[ddd_gc_bin] >= g_rd_min_windows && ddd_low_mq_index[ddd_gc_bin] < g_rd_no_combine_min_windows )
		{
			ddd_low_mq_index[ddd_gc_bin] = ddd_temp_low_mq_index[ddd_gc_bin];
			qsort(ddd_sample_low_mq_rd_list[ddd_gc_bin], ddd_low_mq_index[ddd_gc_bin], sizeof(int), cmpfunc);
		}

	}





	int ddd_index = 0;
	long ddd_gc_len, ddd_gc_start_for_ave, ddd_gc_end_for_ave;
	double ddd_rd_ave_by_gc[3][g_num_gc_bins];
	double ddd_rd_stdev_by_gc[3][g_num_gc_bins];
	double ddd_rd_ave_by_gc_del_threshold[2][g_num_gc_bins];
	double ddd_rd_ave_by_gc_dup_threshold[2][g_num_gc_bins];
	long ddd_rd_windows_by_gc[3][g_num_gc_bins];

	for(ddd_gc_bin=0;ddd_gc_bin<g_num_gc_bins;ddd_gc_bin++)
	{
		ddd_rd_stdev_by_gc[0][ddd_gc_bin] = 0.0;
		ddd_rd_stdev_by_gc[1][ddd_gc_bin] = 0.0;
		ddd_rd_stdev_by_gc[2][ddd_gc_bin] = 0.0;


		ddd_index = 0;
		ddd_rd_ave_by_gc[2][ddd_gc_bin] = 0.0;
		if( ddd_high_mq_index[ddd_gc_bin] > 0 )
		{
			ddd_gc_start_for_ave = 0;
			ddd_gc_end_for_ave = ddd_high_mq_index[ddd_gc_bin] - ddd_gc_start_for_ave;
			ddd_gc_len = ddd_gc_end_for_ave - ddd_gc_start_for_ave;
			ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] = 0.0;
			for( ddd_a_loop=ddd_gc_start_for_ave;ddd_a_loop<ddd_gc_end_for_ave;ddd_a_loop++)
			{
				ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] += ddd_sample_high_mq_rd_list[ddd_gc_bin][ddd_a_loop];
			}
			ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] = ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin]/ddd_gc_len;
			ddd_rd_ave_by_gc_del_threshold[ddd_index][ddd_gc_bin] = ddd_del_ploidy_threshold * ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin];
			ddd_rd_ave_by_gc_dup_threshold[ddd_index][ddd_gc_bin] = ddd_dup_ploidy_threshold * ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin];
			ddd_rd_windows_by_gc[ddd_index][ddd_gc_bin] = ddd_high_mq_index[ddd_gc_bin];
			for(ddd_a_loop=ddd_gc_start_for_ave;ddd_a_loop<ddd_gc_end_for_ave;ddd_a_loop++)
			{
				ddd_rd_stdev_by_gc[0][ddd_gc_bin] += pow((ddd_sample_high_mq_rd_list[ddd_gc_bin][ddd_a_loop] - ddd_rd_ave_by_gc[0][ddd_gc_bin]), 2);
			}
			if( ddd_gc_len > 1 )
			{
				ddd_rd_stdev_by_gc[0][ddd_gc_bin] = sqrt(ddd_rd_stdev_by_gc[0][ddd_gc_bin]/(ddd_gc_len-1));
			}
		}
		else
		{
			ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] = 0.0;
			ddd_rd_ave_by_gc_del_threshold[ddd_index][ddd_gc_bin] = 0.0;
			ddd_rd_ave_by_gc_dup_threshold[ddd_index][ddd_gc_bin] = 0.0;
			ddd_rd_windows_by_gc[ddd_index][ddd_gc_bin] = 0;
		}


		ddd_index = 1;
		if( ddd_low_mq_index[ddd_gc_bin] > 0 )
		{
			ddd_gc_start_for_ave = 0;
			ddd_gc_end_for_ave = ddd_low_mq_index[ddd_gc_bin] - ddd_gc_start_for_ave;
			ddd_gc_len = ddd_gc_end_for_ave - ddd_gc_start_for_ave;
			ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] = 0.0;
			for( ddd_a_loop=ddd_gc_start_for_ave;ddd_a_loop<ddd_gc_end_for_ave;ddd_a_loop++)
			{
				ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] += ddd_sample_low_mq_rd_list[ddd_gc_bin][ddd_a_loop];
			}
			ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] = ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin]/ddd_gc_len;
			ddd_rd_ave_by_gc_del_threshold[ddd_index][ddd_gc_bin] = ddd_del_ploidy_threshold * ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin];
			ddd_rd_ave_by_gc_dup_threshold[ddd_index][ddd_gc_bin] = ddd_dup_ploidy_threshold * ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin];
			ddd_rd_windows_by_gc[ddd_index][ddd_gc_bin] = ddd_low_mq_index[ddd_gc_bin];
			for(ddd_a_loop=ddd_gc_start_for_ave;ddd_a_loop<ddd_gc_end_for_ave;ddd_a_loop++)
			{
				ddd_rd_stdev_by_gc[1][ddd_gc_bin] += pow((ddd_sample_low_mq_rd_list[ddd_gc_bin][ddd_a_loop] - ddd_rd_ave_by_gc[1][ddd_gc_bin]), 2);
			}
			if( ddd_gc_len > 1 )
			{
				ddd_rd_stdev_by_gc[1][ddd_gc_bin] = sqrt(ddd_rd_stdev_by_gc[1][ddd_gc_bin]/(ddd_gc_len-1));
			}
		}
		else
		{
			ddd_rd_ave_by_gc[ddd_index][ddd_gc_bin] = 0.0;
			ddd_rd_ave_by_gc_del_threshold[ddd_index][ddd_gc_bin] = 0.0;
			ddd_rd_ave_by_gc_dup_threshold[ddd_index][ddd_gc_bin] = 0.0;
			ddd_rd_windows_by_gc[ddd_index][ddd_gc_bin] = 0;
		}

	}


	long ddd_rd_windows_count[g_max_rd_window_len+1];
	for(ddd_a_loop=0;ddd_a_loop<g_max_rd_window_len+1;ddd_a_loop++)
	{
		ddd_rd_windows_count[ddd_a_loop] = 0;
	}

	long ddd_num_windows_for_genome;
	ddd_num_windows_for_genome = (g_windows_sampling_factor*ddd_chr_fasta_len)/(g_max_rd_window_len*g_genome_reduction_factor) + g_windows_sampling_factor;
	double **ddd_rd_windows_low;
	ddd_rd_windows_low = malloc((g_max_rd_window_len+1)*sizeof(double *));
	if( ddd_rd_windows_low == NULL )
	{
	  printf("449 NULL\n");
	  exit(0);
	}
	for(ddd_a_loop=0;ddd_a_loop<(g_max_rd_window_len+1);ddd_a_loop++)
	{
		ddd_rd_windows_low[ddd_a_loop] = malloc(ddd_num_windows_for_genome*sizeof(double));
		for(ddd_b_loop=0;ddd_b_loop<ddd_num_windows_for_genome;ddd_b_loop++)
		{
			ddd_rd_windows_low[ddd_a_loop][ddd_b_loop] = 0.0;
		}
	}



	ddd_last_low_mq = 0;
	int ddd_mq_index = 0;
	int *ddd_rd_low_acgt_or_windows_list = malloc(ddd_chr_fasta_len * sizeof(int));
	if( ddd_rd_low_acgt_or_windows_list == NULL )
	{
	  printf("450 NULL\n");
	  exit(0);
	}
	for(ddd_pos=0;ddd_pos<g_insert_mean-1;ddd_pos++)
	{
		ddd_rd_low_acgt_or_windows_list[ddd_pos] = 1;
	}
	for(ddd_pos=ddd_chr_fasta_len-g_one_base_window_size;ddd_pos<ddd_chr_fasta_len;ddd_pos++)
	{
		ddd_rd_low_acgt_or_windows_list[ddd_pos] = 1;
	}
	for(ddd_pos=g_insert_mean-1;ddd_pos<ddd_chr_fasta_len-g_one_base_window_size;ddd_pos++)
	{
		if( ddd_one_base_rd_acgt_weighted[ddd_pos] >= g_insert_min_acgt )
		{
			if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) == 0 )
			{
				ddd_mq_index = ddd_last_low_mq;
			}
			else if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
			{
				ddd_mq_index = 0;
				ddd_last_low_mq = 0;
			}
			else
			{
				ddd_mq_index = 1;
				ddd_last_low_mq = 1;
			}
			if( ddd_rd_windows_by_gc[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos]] < g_rd_no_combine_min_windows )
			{
				ddd_rd_low_acgt_or_windows_list[ddd_pos] = 1;
			}
			else
			{
				ddd_rd_low_acgt_or_windows_list[ddd_pos] = 0;
			}
		}
		else
		{
			ddd_rd_low_acgt_or_windows_list[ddd_pos] = 1;
		}
	}



	long ddd_gc_start, ddd_gc_end;
	double ddd_prob;
	ddd_last_low_mq = 0;
	ddd_mq_index = 0;
	long ddd_window_len = 0;
	double ddd_window_len_double = 0.0;
	long ddd_bisect_index = 0;
	long ddd_bisect_index2 = 0;
	long ddd_window_rd_low_acgt_or_windows_total = 0;
	long ddd_temp_low_count = 0;
	double ddd_temp_low_count_double = 0.0;
	int ddd_temp_win_count = 0;
	double ddd_double_index = 0.0;
	double ddd_double_index2 = 0.0;
	double ddd_window_rd_low_total = 0.0;
	double *ddd_stdev_list = malloc(ddd_chr_fasta_len * sizeof(double));
	if( ddd_stdev_list == NULL )
	{
	  printf("451 NULL\n");
	  exit(0);
	}
	for(ddd_pos=0;ddd_pos<ddd_chr_fasta_len;ddd_pos++)
	{
		ddd_stdev_list[ddd_pos] = 0.0;
	}

	long ddd_win_sampling_loop, ddd_win_sampling_adj;

	for(ddd_block_loop=0;ddd_block_loop<g_lowvar_block_index;ddd_block_loop++)
	{
		ddd_window_len = 0;
		ddd_window_len_double = 0.0;
		ddd_window_rd_low_total = 0;
		ddd_window_rd_low_acgt_or_windows_total = 0;
		ddd_temp_low_count = 0;


		for(ddd_pos=g_lowvar_block_start_list[ddd_block_loop];ddd_pos<g_lowvar_block_end_list[ddd_block_loop];ddd_pos++)
		{
		  if( ddd_rd_low_acgt_or_windows_list[ddd_pos] == 0 && ((ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq && ddd_rd_windows_by_gc[0][ddd_one_base_rd_gc_weighted[ddd_pos]] > 1) || (ddd_rd_mq_list[ddd_pos] < g_rd_min_mapq && ddd_rd_windows_by_gc[1][ddd_one_base_rd_gc_weighted[ddd_pos]] > 1)) )
		  {
		    if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
		    {
		      ddd_mq_index = 0;
		      ddd_last_low_mq = 0;
		    }
		    else if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) == 0 )
		    {
		      ddd_mq_index = ddd_last_low_mq;
		    }
		    else
		    {
		      ddd_mq_index = 1;
		      ddd_last_low_mq = 1;
		    }

		    ddd_gc_bin = ddd_one_base_rd_gc_weighted[ddd_pos];
		    ddd_gc_start = 0;
		    if( ddd_mq_index == 0 )
		    {
		      ddd_gc_end = ddd_high_mq_index[ddd_gc_bin];
		    }
		    else
		    {
		      ddd_gc_end = ddd_low_mq_index[ddd_gc_bin];
		    }
		    if( ddd_gc_end > ddd_gc_start )
		    {
		      if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) < ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin] )
		      {
			if( ddd_mq_index == 0 )
			{

			  ddd_bisect_index = bisect_right(ddd_sample_high_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			  ddd_bisect_index2 = bisect_left(ddd_sample_high_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);

			}
			else
			{
			  ddd_bisect_index = bisect_right(ddd_sample_low_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			  ddd_bisect_index2 = bisect_left(ddd_sample_low_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			}
			ddd_bisect_index = ddd_bisect_index - ddd_gc_start;
			ddd_bisect_index2 = ddd_bisect_index2 - ddd_gc_start;
			if( ddd_bisect_index <= 0 )
			{
			  ddd_double_index = 0.5;
			}
			else
			{
			  ddd_double_index = (double) ddd_bisect_index;
			}
			if( ddd_bisect_index2 <= 0 )
			{
			  ddd_double_index2 = 0.5;
			}
			else
			{
			  ddd_double_index2 = (double) ddd_bisect_index2;
			}
			ddd_prob = (ddd_double_index + ddd_double_index2) / (2* (ddd_gc_end-ddd_gc_start));
			ddd_bisect_index = bisect_right_double(ddd_pval2sd_pval_list, ddd_prob, 0, ddd_pval2sd_list_len);
			if( ddd_bisect_index < 0 )
			{
			  ddd_bisect_index = 0;
			}
			else if( ddd_bisect_index >= ddd_pval2sd_list_len )
			{
			  ddd_bisect_index = ddd_pval2sd_list_len - 1;
			}
			if( g_ranks_stdev == 0 )
			{
			  if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
			  {
			    ddd_stdev_list[ddd_pos] = (g_mapq_factor + (1.0-g_mapq_factor)*(ddd_rd_mq_list[ddd_pos] - g_rd_min_mapq)/(double)(g_rd_max_mapq-g_rd_min_mapq))*(ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin] - ddd_rd_rd_list[ddd_pos] - ddd_rd_low_mq_rd_list[ddd_pos]) / ddd_rd_stdev_by_gc[ddd_mq_index][ddd_gc_bin];
			  }
			  else
			  {
			    ddd_stdev_list[ddd_pos] = g_mapq_factor*(ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin] - ddd_rd_rd_list[ddd_pos] - ddd_rd_low_mq_rd_list[ddd_pos]) / ddd_rd_stdev_by_gc[ddd_mq_index][ddd_gc_bin];
			  }
			}
			else
			{
			  if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
			  {
			    ddd_stdev_list[ddd_pos] = (g_mapq_factor + (1.0-g_mapq_factor)*(ddd_rd_mq_list[ddd_pos] - g_rd_min_mapq)/(double)(g_rd_max_mapq-g_rd_min_mapq))*ddd_pval2sd_sd_list[ddd_bisect_index];
			  }
			  else
			  {
			    ddd_stdev_list[ddd_pos] = g_mapq_factor*ddd_pval2sd_sd_list[ddd_bisect_index];
			  }
			}
		      }
		      else
		      {

			if( ddd_mq_index == 0 )
			{
			  if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > g_dup_threshold_factor*ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin] )
			  {
			   
				ddd_bisect_index = bisect_left(ddd_sample_high_mq_rd_list[ddd_gc_bin], g_dup_threshold_factor*ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin], ddd_gc_start, ddd_gc_end);
			    
				ddd_bisect_index2 = bisect_right(ddd_sample_high_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			  }
			  else
			  {
			    
				ddd_bisect_index = bisect_left(ddd_sample_high_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			    
				ddd_bisect_index2 = bisect_right(ddd_sample_high_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			  }
			}
			else
			{
			  if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > g_dup_threshold_factor*ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin] )
			  {
			    ddd_bisect_index = bisect_left(ddd_sample_low_mq_rd_list[ddd_gc_bin], g_dup_threshold_factor*ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin], ddd_gc_start, ddd_gc_end);
			    ddd_bisect_index2 = bisect_right(ddd_sample_low_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			  }
			  else
			  {
			    ddd_bisect_index = bisect_left(ddd_sample_low_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			    ddd_bisect_index2 = bisect_right(ddd_sample_low_mq_rd_list[ddd_gc_bin], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], ddd_gc_start, ddd_gc_end);
			  }
			}
			ddd_bisect_index = ddd_gc_end - ddd_bisect_index;
			ddd_bisect_index2 = ddd_gc_end - ddd_bisect_index2;
			if( ddd_bisect_index <= 0 )
			{
			  ddd_double_index = 0.5;
			}
			else
			{
			  ddd_double_index = (double) ddd_bisect_index;
			}
			if( ddd_bisect_index2 <= 0 )
			{
			  ddd_double_index2 = 0.5;
			}
			else
			{
			  ddd_double_index2 = (double) ddd_bisect_index2;
			}
			ddd_prob = (ddd_double_index + ddd_double_index2) / (2* (ddd_gc_end-ddd_gc_start));
			ddd_bisect_index = bisect_right_double(ddd_pval2sd_pval_list, ddd_prob, 0, ddd_pval2sd_list_len);
			if( ddd_bisect_index < 0 )
			{
			  ddd_bisect_index = 0;
			}
			else if( ddd_bisect_index >= ddd_pval2sd_list_len )
			{
			  ddd_bisect_index = ddd_pval2sd_list_len - 1;
			}
			if( g_ranks_stdev == 0 )
			{
			  if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > (g_dup_threshold_factor*ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin]) )
			  {
			    if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
			    {
			      ddd_stdev_list[ddd_pos] = (g_mapq_factor + (1.0-g_mapq_factor)*(ddd_rd_mq_list[ddd_pos] - g_rd_min_mapq)/(double)(g_rd_max_mapq-g_rd_min_mapq))*(g_dup_threshold_factor-1)*(-ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin]) / ddd_rd_stdev_by_gc[ddd_mq_index][ddd_gc_bin];
			    }
			    else
			    {
			      ddd_stdev_list[ddd_pos] = g_mapq_factor*(g_dup_threshold_factor-1)*(-ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin]) / ddd_rd_stdev_by_gc[ddd_mq_index][ddd_gc_bin];
			    }
			  }
			  else
			  {
			    if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
			    {
			      ddd_stdev_list[ddd_pos] = (g_mapq_factor + (1.0-g_mapq_factor)*(ddd_rd_mq_list[ddd_pos] - g_rd_min_mapq)/(double)(g_rd_max_mapq-g_rd_min_mapq))*(ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin] - ddd_rd_rd_list[ddd_pos] - ddd_rd_low_mq_rd_list[ddd_pos]) / ddd_rd_stdev_by_gc[ddd_mq_index][ddd_gc_bin];
			    }
			    else
			    {
			      ddd_stdev_list[ddd_pos] = g_mapq_factor*(ddd_rd_ave_by_gc[ddd_mq_index][ddd_gc_bin] - ddd_rd_rd_list[ddd_pos] - ddd_rd_low_mq_rd_list[ddd_pos]) / ddd_rd_stdev_by_gc[ddd_mq_index][ddd_gc_bin];
			    }
			  }
			}
			else
			{
			  if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
			  {
			    ddd_stdev_list[ddd_pos] = -(g_mapq_factor + (1.0-g_mapq_factor)*(ddd_rd_mq_list[ddd_pos] - g_rd_min_mapq)/(double)(g_rd_max_mapq-g_rd_min_mapq))*ddd_pval2sd_sd_list[ddd_bisect_index];
			  }
			  else
			  {
			    ddd_stdev_list[ddd_pos] = -g_mapq_factor*ddd_pval2sd_sd_list[ddd_bisect_index];
			  }
			}
		      }

		    }

		  }
		}

	}



	for(ddd_block_loop=0;ddd_block_loop<g_lowvar_block_sample_index;ddd_block_loop++)
	{
		ddd_window_len = 0;
		ddd_window_len_double = 0.0;
		ddd_window_rd_low_total = 0;
		ddd_window_rd_low_acgt_or_windows_total = 0;
		ddd_temp_low_count = 0;

		for(ddd_win_sampling_loop=0;ddd_win_sampling_loop<g_windows_sampling_factor;ddd_win_sampling_loop++)
		{
			ddd_win_sampling_adj = ddd_win_sampling_loop * g_max_rd_window_len / g_windows_sampling_factor;

	    for(ddd_pos=g_lowvar_block_sample_start_list[ddd_block_loop]+ddd_win_sampling_adj;ddd_pos<g_lowvar_block_sample_end_list[ddd_block_loop];ddd_pos++)
			{
				if( ddd_rd_low_acgt_or_windows_list[ddd_pos] == 0 && ((ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq && ddd_rd_windows_by_gc[0][ddd_one_base_rd_gc_weighted[ddd_pos]] > 1) || (ddd_rd_mq_list[ddd_pos] < g_rd_min_mapq && ddd_rd_windows_by_gc[1][ddd_one_base_rd_gc_weighted[ddd_pos]] > 1)) )
				{

					ddd_window_rd_low_total += ddd_stdev_list[ddd_pos];
					ddd_temp_low_count += 1;

				}
				ddd_window_rd_low_acgt_or_windows_total += ddd_rd_low_acgt_or_windows_list[ddd_pos];
				ddd_window_len += 1;
				if( ddd_window_len >= g_min_rd_window_len && ddd_temp_win_count == 0 )
				{
					ddd_window_len_double = ddd_window_len;
					if( (ddd_window_rd_low_acgt_or_windows_total/ddd_window_len_double) < g_max_rd_low_acgt_or_windows )
					{
						if( ddd_temp_low_count > 0 )
						{
							ddd_temp_low_count_double = ddd_temp_low_count;
							ddd_rd_windows_low[ddd_window_len][ddd_rd_windows_count[ddd_window_len]] = ddd_window_rd_low_total / ddd_temp_low_count_double;
							ddd_rd_windows_count[ddd_window_len] += 1;
						}
					}
				}
				if( ddd_window_len == g_max_rd_window_len )
				{
					ddd_temp_win_count += 1;
					if( ddd_temp_win_count == g_genome_reduction_factor )
					{
						ddd_temp_win_count = 0;
					}
					ddd_window_len = 0;
					ddd_window_rd_low_total = 0;
					ddd_window_rd_low_acgt_or_windows_total = 0;
					ddd_temp_low_count = 0;
				}
			}

		}
	}


	if( g_most_biased_repeat != -1 )
	{
		for(ddd_repeat_loop=0;ddd_repeat_loop<ddd_repeat_index;ddd_repeat_loop++)
		{
			if( ddd_repeat_type_list[ddd_repeat_loop] == g_most_biased_repeat )
			{
				for(ddd_pos=(ddd_repeat_start_list[ddd_repeat_loop]-(g_insert_mean/2));ddd_pos<(ddd_repeat_end_list[ddd_repeat_loop]+(g_insert_mean/2));ddd_pos++)
				{

					if( ddd_pos < ddd_repeat_start_list[ddd_repeat_loop] )
					{
						ddd_repeat_segment = (g_repeat_segments - 1) * (ddd_pos - (ddd_repeat_start_list[ddd_repeat_loop]-(g_insert_mean/2)))/(g_insert_mean/2);
					}
					else if( ddd_pos >= ddd_repeat_end_list[ddd_repeat_loop] )
					{
						ddd_repeat_segment = (g_repeat_segments - 1) * ((ddd_repeat_end_list[ddd_repeat_loop]+(g_insert_mean/2)) - ddd_pos)/(g_insert_mean/2);
					}
					else
					{
						ddd_repeat_segment = g_repeat_segments - 1;
					}

					if( ddd_rd_low_acgt_or_windows_list[ddd_pos] == 0 )
					{

						if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) < ddd_rd_ave_by_repeat_segment[ddd_repeat_segment] )
						{

							ddd_bisect_index = bisect_right(ddd_sample_repeat_rd_list[ddd_repeat_segment], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], 0, ddd_most_biased_index[ddd_repeat_segment]);
							ddd_bisect_index2 = bisect_left(ddd_sample_repeat_rd_list[ddd_repeat_segment], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], 0, ddd_most_biased_index[ddd_repeat_segment]);

							if( ddd_bisect_index <= 0 )
							{
								ddd_double_index = 0.5;
							}
							else
							{
								ddd_double_index = (double) ddd_bisect_index;
							}
							if( ddd_bisect_index2 <= 0 )
							{
								ddd_double_index2 = 0.5;
							}
							else
							{
								ddd_double_index2 = (double) ddd_bisect_index2;
							}
							ddd_prob = (ddd_double_index + ddd_double_index2) / (2* ddd_most_biased_index[ddd_repeat_segment]);
							ddd_bisect_index = bisect_right_double(ddd_pval2sd_pval_list, ddd_prob, 0, ddd_pval2sd_list_len);
							if( ddd_bisect_index < 0 )
							{
								ddd_bisect_index = 0;
							}
							else if( ddd_bisect_index >= ddd_pval2sd_list_len )
							{
								ddd_bisect_index = ddd_pval2sd_list_len - 1;
							}
							if( g_ranks_stdev == 0 )
							{
								ddd_stdev_list[ddd_pos] = (ddd_rd_ave_by_repeat_segment[ddd_repeat_segment] - ddd_rd_rd_list[ddd_pos] - ddd_rd_low_mq_rd_list[ddd_pos]) / ddd_rd_stdev_by_repeat_segment[ddd_repeat_segment];
							}
							else
							{
								ddd_stdev_list[ddd_pos] = ddd_pval2sd_sd_list[ddd_bisect_index];
							}
						}
						else
						{

							if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > g_dup_threshold_factor*ddd_rd_ave_by_repeat_segment[ddd_repeat_segment] )
							{
								ddd_bisect_index = bisect_left(ddd_sample_repeat_rd_list[ddd_repeat_segment], g_dup_threshold_factor*ddd_rd_ave_by_repeat_segment[ddd_repeat_segment], 0, ddd_most_biased_index[ddd_repeat_segment]);
								ddd_bisect_index2 = bisect_right(ddd_sample_repeat_rd_list[ddd_repeat_segment], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], 0, ddd_most_biased_index[ddd_repeat_segment]);
							}
							else
							{
								ddd_bisect_index = bisect_left(ddd_sample_repeat_rd_list[ddd_repeat_segment], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], 0, ddd_most_biased_index[ddd_repeat_segment]);
								ddd_bisect_index2 = bisect_right(ddd_sample_repeat_rd_list[ddd_repeat_segment], ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos], 0, ddd_most_biased_index[ddd_repeat_segment]);
							}

							ddd_bisect_index = ddd_most_biased_index[ddd_repeat_segment] - ddd_bisect_index;
							ddd_bisect_index2 = ddd_most_biased_index[ddd_repeat_segment] - ddd_bisect_index2;
							if( ddd_bisect_index <= 0 )
							{
								ddd_double_index = 0.5;
							}
							else
							{
								ddd_double_index = (double) ddd_bisect_index;
							}
							if( ddd_bisect_index2 <= 0 )
							{
								ddd_double_index2 = 0.5;
							}
							else
							{
								ddd_double_index2 = (double) ddd_bisect_index2;
							}
							ddd_prob = (ddd_double_index + ddd_double_index2) / (2* ddd_most_biased_index[ddd_repeat_segment]);
							ddd_bisect_index = bisect_right_double(ddd_pval2sd_pval_list, ddd_prob, 0, ddd_pval2sd_list_len);
							if( ddd_bisect_index < 0 )
							{
								ddd_bisect_index = 0;
							}
							else if( ddd_bisect_index >= ddd_pval2sd_list_len )
							{
								ddd_bisect_index = ddd_pval2sd_list_len - 1;
							}
							if( g_ranks_stdev == 0 )
							{
								if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > g_dup_threshold_factor*ddd_rd_ave_by_repeat_segment[ddd_repeat_segment] )
								{
									ddd_stdev_list[ddd_pos] = (g_dup_threshold_factor-1)*(-ddd_rd_ave_by_repeat_segment[ddd_repeat_segment]) / ddd_rd_stdev_by_repeat_segment[ddd_repeat_segment];
								}
								else
								{
									ddd_stdev_list[ddd_pos] = (ddd_rd_ave_by_repeat_segment[ddd_repeat_segment] - ddd_rd_rd_list[ddd_pos] - ddd_rd_low_mq_rd_list[ddd_pos]) / ddd_rd_stdev_by_repeat_segment[ddd_repeat_segment];
								}
							}
							else
							{
								ddd_stdev_list[ddd_pos] = -ddd_pval2sd_sd_list[ddd_bisect_index];
							}
						}

					}
				}
			}
		}
	}



	long ddd_window_a;
	double ddd_stdev_total;
	double ddd_rd_windows_low_stdev[g_max_rd_window_len+1];
	for(ddd_window_a=0;ddd_window_a<g_min_rd_window_len;ddd_window_a++)
	{
		ddd_rd_windows_low_stdev[ddd_window_a] = 0.0;
	}
	for(ddd_window_a=g_min_rd_window_len;ddd_window_a<=g_max_rd_window_len;ddd_window_a++)
	{
		ddd_stdev_total = 0.0;
		if( ddd_rd_windows_count[ddd_window_a] > 1 )
		{
			for(ddd_a_loop=0;ddd_a_loop<ddd_rd_windows_count[ddd_window_a];ddd_a_loop++)
			{
				ddd_stdev_total += ddd_rd_windows_low[ddd_window_a][ddd_a_loop]*ddd_rd_windows_low[ddd_window_a][ddd_a_loop];
			}
			ddd_rd_windows_low_stdev[ddd_window_a] = sqrt(ddd_stdev_total/(ddd_rd_windows_count[ddd_window_a] - 1));
		}
		else
		{
			ddd_rd_windows_low_stdev[ddd_window_a] = 0.0;
		}
	}



	for(ddd_a_loop=0;ddd_a_loop<(g_max_rd_window_len+1);ddd_a_loop++)
	{
		free(ddd_rd_windows_low[ddd_a_loop]);
	}

	free(ddd_rd_windows_low);



	int ddd_stop_base, ddd_stop_while;
	int ddd_mq_index_a = 0;
	int ddd_mq_index_b = 0;
	int ddd_del_begin = 0;
	int ddd_del_ref = 0;
	long ddd_del_start = 0;
	long ddd_del_end = 0;
	long ddd_del_last_good = 0;
	double ddd_del_stdevs = 0.0;
	double ddd_temp_stdevs = 0.0;
	double ddd_temp_low_total = 0.0;
	long ddd_start, ddd_end;
	long ddd_pos_a, ddd_pos_b;
	long ddd_temp_pos, ddd_temp_low_count_2, ddd_temp_low_count_3;



	  for(ddd_block_loop=0;ddd_block_loop<g_lowvar_block_index;ddd_block_loop++)
	  {
	    ddd_start = g_lowvar_block_start_list[ddd_block_loop];
	    ddd_end = g_lowvar_block_end_list[ddd_block_loop] - g_min_rd_window_len;
	    ddd_pos = ddd_start;
	    ddd_mq_index = 0;
	    ddd_last_low_mq = 0;


	    while( ddd_pos < ddd_end )
	    {
	      ddd_stop_base = 0;
	      if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
	      {
		ddd_mq_index = 0;
		ddd_last_low_mq = 0;
	      }
	      else if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > 0 )
	      {
		ddd_mq_index = 1;
		ddd_last_low_mq = 1;
	      }
	      else
	      {
		ddd_mq_index = ddd_last_low_mq;
	      }


	      if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) <= ddd_rd_ave_by_gc_del_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos]] )
	      {
		ddd_temp_pos = ddd_pos;
		ddd_temp_low_total = 0;
		ddd_temp_low_count = 0;
		ddd_temp_low_count_2 = 0;
		ddd_window_len = 0;
		for(ddd_pos_a=ddd_pos;ddd_pos_a<ddd_pos+g_min_rd_window_len;ddd_pos_a++)
		{
		  ddd_window_len += 1;
		  if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 )
		  {
		    if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
		    {
		      ddd_mq_index = 0;
		    }
		    else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
		    {
		      ddd_mq_index = 1;
		    }
		    if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) <= ddd_rd_ave_by_gc_del_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos_a]] )
		    {
		      ddd_temp_low_count_2 += 1;
		    }
		    else if( (2*ddd_temp_low_count_2) < ddd_window_len )
		    {
		      ddd_stop_base = 1;
		      ddd_temp_pos = ddd_pos_a;
		      break;
		    }
		  }
		  else if( (2*ddd_temp_low_count_2) < ddd_window_len )
		  {
		    ddd_stop_base = 1;
		    ddd_temp_pos = ddd_pos_a;
		    break;
		  }
		}


		if( ddd_stop_base == 0 )
		{
		  ddd_temp_low_count = g_min_rd_window_len;
		  ddd_temp_low_total = 0;
		  for(ddd_a_loop=ddd_pos;ddd_a_loop<(ddd_pos+g_min_rd_window_len);ddd_a_loop++)
		  {
		    ddd_temp_low_count -= ddd_rd_low_acgt_or_windows_list[ddd_a_loop];
		    ddd_temp_low_total += ddd_stdev_list[ddd_a_loop];
		  }
		}


		if( ddd_stop_base == 0 && ddd_temp_low_count > 0 && ddd_rd_windows_low_stdev[g_min_rd_window_len] > 0 && (ddd_temp_low_total/(ddd_temp_low_count*ddd_rd_windows_low_stdev[g_min_rd_window_len])) >= g_one_base_read_depth_min_rd_low_stdev && ((g_min_rd_window_len-ddd_temp_low_count) / ((double)g_min_rd_window_len)) <= g_max_rd_low_acgt_or_windows )
		{
		  ddd_del_begin = 1;
		  ddd_del_start = ddd_pos;
		  ddd_del_ref = ddd_chr - ddd_begin + 1;

		  ddd_del_last_good = ddd_pos + g_min_rd_window_len;
		  ddd_del_end = ddd_pos + g_min_rd_window_len;
		  ddd_del_stdevs = ddd_temp_low_total / (ddd_temp_low_count*ddd_rd_windows_low_stdev[g_min_rd_window_len]);
		}



		if( ddd_stop_base == 0 )
		{
		  for(ddd_pos_a=(ddd_pos+g_min_rd_window_len);ddd_pos_a<ddd_pos+g_max_rd_window_len;ddd_pos_a++)
		  {
		    ddd_window_len += 1;
		    if( ddd_pos_a < ddd_end )
		    {
		      if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 )
		      {
			if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
			{
			  ddd_mq_index = 0;
			}
			else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
			{
			  ddd_mq_index = 1;
			}
			ddd_temp_low_total += ddd_stdev_list[ddd_pos_a];
			ddd_temp_low_count += 1;
			if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) <= ddd_rd_ave_by_gc_del_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos_a]] )
			{
			  ddd_temp_low_count_2 += 1;
			  if( ddd_rd_windows_low_stdev[ddd_window_len] > 0 && (ddd_temp_low_total/(ddd_temp_low_count*ddd_rd_windows_low_stdev[ddd_window_len])) >= g_one_base_read_depth_min_rd_low_stdev && ((ddd_window_len - ddd_temp_low_count) / ((double)ddd_window_len)) <= g_max_rd_low_acgt_or_windows )
			  {
			    ddd_del_last_good = ddd_pos_a;
			    if( ddd_del_begin == 0 )
			    {
			      ddd_del_begin = 1;
			      ddd_del_start = ddd_pos;
			      ddd_del_ref = ddd_chr - ddd_begin + 1;
			      ddd_del_end = ddd_pos_a;
			      ddd_del_stdevs = ddd_temp_low_total / (ddd_temp_low_count * ddd_rd_windows_low_stdev[ddd_window_len]);
			    }
			    else
			    {
			      ddd_temp_stdevs = ddd_temp_low_total / (ddd_temp_low_count * ddd_rd_windows_low_stdev[ddd_window_len]);
			      ddd_del_end = ddd_pos_a;
			      if( ddd_temp_stdevs > ddd_del_stdevs )
			      {
				ddd_del_stdevs = ddd_temp_stdevs;
			      }
			    }

			  }
			}
			else if( (2 * ddd_temp_low_count_2) < ddd_window_len )
			{
			  ddd_stop_base = 1;
			  break;
			}

		      }
		      else if( (2*ddd_temp_low_count_2) < ddd_window_len )
		      {
			ddd_stop_base = 1;
			break;
		      }
		    }
		    else
		    {
		      ddd_stop_base = 1;
		      break;
		    }
		  }
		}




		if( ddd_stop_base == 0 && ddd_del_begin == 1 )
		{
		  ddd_pos_a = ddd_pos + g_max_rd_window_len;
		  ddd_temp_low_total = 0;
		  ddd_temp_low_count = 0;
		  ddd_mq_index_b = ddd_mq_index;
		  while( ddd_pos_a < ddd_chr_fasta_len && (ddd_pos_a - ddd_del_last_good) <= g_max_distance_since_last_del_good )
		  {
		    if( ddd_pos_a == (ddd_pos + g_max_rd_window_len) )
		    {
		      for(ddd_pos_b=(ddd_pos_a-g_max_rd_window_len+1);ddd_pos_b<(ddd_pos_a+1);ddd_pos_b++)
		      {
			if( ddd_rd_mq_list[ddd_pos_b] >= g_rd_min_mapq )
			{
			  ddd_mq_index_b = 0;
			}
			else if( (ddd_rd_rd_list[ddd_pos_b] + ddd_rd_low_mq_rd_list[ddd_pos_b]) > 0 )
			{
			  ddd_mq_index_b = 1;
			}
			if( ddd_rd_low_acgt_or_windows_list[ddd_pos_b] == 0 && ddd_rd_windows_by_gc[ddd_mq_index_b][ddd_one_base_rd_gc_weighted[ddd_pos_b]] > 1 )
			{
			  ddd_temp_low_total += ddd_stdev_list[ddd_pos_b];
			  ddd_temp_low_count += 1;
			}
		      }
		    }
		    else
		    {
		      ddd_pos_b = ddd_pos_a - g_max_rd_window_len;
		      if( ddd_rd_mq_list[ddd_pos_b] >= g_rd_min_mapq )
		      {
			ddd_mq_index_b = 0;
		      }
		      else if( (ddd_rd_rd_list[ddd_pos_b] + ddd_rd_low_mq_rd_list[ddd_pos_b]) > 0 )
		      {
			ddd_mq_index_b = 1;
		      }
		      if( ddd_rd_low_acgt_or_windows_list[ddd_pos_b] == 0 && ddd_rd_windows_by_gc[ddd_mq_index_b][ddd_one_base_rd_gc_weighted[ddd_pos_b]] > 1 )
		      {
			ddd_temp_low_total -= ddd_stdev_list[ddd_pos_b];
			ddd_temp_low_count -= 1;
		      }
		      if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
		      {
			ddd_mq_index = 0;
		      }
		      else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
		      {
			ddd_mq_index = 1;
		      }
		      if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 && ddd_rd_windows_by_gc[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos_a]] > 1 )
		      {
			ddd_temp_low_total += ddd_stdev_list[ddd_pos_a];
			ddd_temp_low_count += 1;
		      }

		    }

		    if( ddd_temp_low_count > 0 && ddd_rd_windows_low_stdev[g_max_rd_window_len] > 0 && (ddd_temp_low_total/(ddd_temp_low_count*ddd_rd_windows_low_stdev[g_max_rd_window_len])) >= g_one_base_read_depth_min_rd_low_stdev && ((g_max_rd_window_len-ddd_temp_low_count) / ((double)g_max_rd_window_len)) <= g_max_rd_low_acgt_or_windows )
		    {
		      ddd_del_last_good = ddd_pos_a;
		      ddd_del_end = ddd_pos_a;
		      ddd_temp_stdevs = ddd_temp_low_total / (ddd_temp_low_count*ddd_rd_windows_low_stdev[g_max_rd_window_len]);
		      if( ddd_temp_stdevs > ddd_del_stdevs )
		      {
			ddd_del_stdevs = ddd_temp_stdevs;
		      }
		    }
		    ddd_pos_a +=1;
		  }
		}




		if( ddd_del_begin == 1 )
		{
		  ddd_pos = ddd_del_end;
		  while( ddd_pos > (ddd_del_start + g_min_rd_window_len) )
		  {
		    if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
		    {
		      ddd_mq_index = 0;
		    }
		    else if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > 0 )
		    {
		      ddd_mq_index = 1;
		    }

		    if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > ddd_rd_ave_by_gc_del_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos]] )
		    {
		      ddd_pos -= 1;
		      ddd_del_end = ddd_pos;
		    }
		    else
		    {
		      ddd_temp_low_count_2 = 0;
		      ddd_temp_low_count_3 = 0;
		      ddd_pos_a = ddd_del_end;
		      ddd_stop_while = 0;
		      ddd_mq_index_a = ddd_mq_index;
		      while( ddd_pos_a > (ddd_del_start + g_min_rd_window_len) && ddd_stop_while == 0 )
		      {
			if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 )
			{
			  if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
			  {
			    ddd_mq_index_a = 0;
			  }
			  else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
			  {
			    ddd_mq_index_a = 1;
			  }
			  ddd_temp_low_count_3 += 1;
			  if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) <= ddd_rd_ave_by_gc_del_threshold[ddd_mq_index_a][ddd_one_base_rd_gc_weighted[ddd_pos_a]] )
			  {
			    ddd_temp_low_count_2 += 1;
			  }
			}
			if( ddd_temp_low_count_3 == 0 || (ddd_temp_low_count_3 > 0 && (ddd_temp_low_count_2 / ((double)ddd_temp_low_count_3)) < 0.5) || ((ddd_del_end - ddd_pos_a + 1 - ddd_temp_low_count_3) / ((double)ddd_del_end - (double)ddd_pos_a + 1.0)) > g_max_rd_low_acgt_or_windows )
			{
			  ddd_del_end = ddd_pos_a - 1;
			  ddd_stop_while = 1;
			}
			ddd_pos_a -= 1;
		      }
		      ddd_pos = ddd_pos_a;
		    }
		  }
		  ddd_pos = ddd_del_end + 1;
		  ddd_del_list_start[*ddd_del_list_index] = ddd_del_start;
		  ddd_del_list_end[*ddd_del_list_index] = ddd_del_end;
		  ddd_del_list_ref[*ddd_del_list_index] = ddd_del_ref;
		  ddd_del_list_stdev[*ddd_del_list_index] = ddd_del_stdevs;
		  *ddd_del_list_index += 1;
		  ddd_del_start = 0;
		  ddd_del_end = 0;
		  ddd_del_ref = 0;
		  ddd_del_stdevs = 0;
		  ddd_del_last_good = 0;
		  ddd_del_begin = 0;


		}



		else if( ddd_stop_base == 1 )
		{
		  ddd_pos = ddd_temp_pos;
		}
	      }
	      ddd_pos += 1;

	    }
	  }



	  ddd_mq_index_a = 0;
	  ddd_mq_index_b = 0;
	  int ddd_dup_begin = 0;
	  int ddd_dup_ref = 0;
	  long ddd_dup_start = 0;
	  long ddd_dup_end = 0;
	  long ddd_dup_last_good = 0;
	  double ddd_dup_stdevs = 0.0;
	  ddd_temp_stdevs = 0.0;
	  ddd_temp_low_total = 0.0;

	  ddd_start = g_insert_mean - 1;
	  ddd_end = ddd_chr_fasta_len - g_one_base_window_size - g_min_rd_window_len;
	  ddd_pos = g_insert_mean - 1;
	  ddd_mq_index = 0;
	  ddd_last_low_mq = 0;


	  for(ddd_block_loop=0;ddd_block_loop<g_lowvar_block_index;ddd_block_loop++)
	  {
	    ddd_start = g_lowvar_block_start_list[ddd_block_loop];
	    ddd_end = g_lowvar_block_end_list[ddd_block_loop] - g_min_rd_window_len;
	    ddd_pos = ddd_start;
	    ddd_mq_index = 0;
	    ddd_last_low_mq = 0;


	    while( ddd_pos < ddd_end )
	    {
	      ddd_stop_base = 0;
	      if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
	      {
		ddd_mq_index = 0;
		ddd_last_low_mq = 0;
	      }
	      else if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > 0 )
	      {
		ddd_mq_index = 1;
		ddd_last_low_mq = 1;
	      }
	      else
	      {
		ddd_mq_index = ddd_last_low_mq;
	      }

	      if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) >= ddd_rd_ave_by_gc_dup_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos]] )
	      {
		ddd_temp_pos = ddd_pos;
		ddd_temp_low_total = 0;
		ddd_temp_low_count = 0;
		ddd_temp_low_count_2 = 0;
		ddd_window_len = 0;
		for(ddd_pos_a=ddd_pos;ddd_pos_a<ddd_pos+g_min_rd_window_len;ddd_pos_a++)
		{
		  ddd_window_len += 1;
		  if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 )
		  {
		    if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
		    {
		      ddd_mq_index = 0;
		    }
		    else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
		    {
		      ddd_mq_index = 1;
		    }
		    if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) >= ddd_rd_ave_by_gc_dup_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos_a]] )
		    {
		      ddd_temp_low_count_2 += 1;
		    }
		    else if( (2*ddd_temp_low_count_2) < ddd_window_len )
		    {
		      ddd_stop_base = 1;
		      ddd_temp_pos = ddd_pos_a;
		      break;
		    }
		  }
		  else if( (2*ddd_temp_low_count_2) < ddd_window_len )
		  {
		    ddd_stop_base = 1;
		    ddd_temp_pos = ddd_pos_a;
		    break;
		  }
		}

		if( ddd_stop_base == 0 )
		{
		  ddd_temp_low_count = g_min_rd_window_len;
		  ddd_temp_low_total = 0;
		  for(ddd_a_loop=ddd_pos;ddd_a_loop<(ddd_pos+g_min_rd_window_len);ddd_a_loop++)
		  {
		    ddd_temp_low_count -= ddd_rd_low_acgt_or_windows_list[ddd_a_loop];
		    ddd_temp_low_total -= ddd_stdev_list[ddd_a_loop];
		  }
		}
		if( ddd_stop_base == 0 && ddd_temp_low_count > 0 && ddd_rd_windows_low_stdev[g_min_rd_window_len] > 0 && (ddd_temp_low_total/(ddd_temp_low_count*ddd_rd_windows_low_stdev[g_min_rd_window_len])) >= g_one_base_read_depth_min_rd_low_stdev && ((g_min_rd_window_len-ddd_temp_low_count) / ((double)g_min_rd_window_len)) <= g_max_rd_low_acgt_or_windows )
		{
		  ddd_dup_begin = 1;
		  ddd_dup_start = ddd_pos;
		  ddd_dup_ref = ddd_chr - ddd_begin + 1;
		  ddd_dup_last_good = ddd_pos + g_min_rd_window_len;
		  ddd_dup_end = ddd_pos + g_min_rd_window_len;
		  ddd_dup_stdevs = ddd_temp_low_total / (ddd_temp_low_count*ddd_rd_windows_low_stdev[g_min_rd_window_len]);
		}


		if( ddd_stop_base == 0 )
		{
		  for(ddd_pos_a=(ddd_pos+g_min_rd_window_len);ddd_pos_a<ddd_pos+g_max_rd_window_len;ddd_pos_a++)
		  {
		    ddd_window_len += 1;
		    if( ddd_pos_a < ddd_end )
		    {
		      if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 )
		      {
			if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
			{
			  ddd_mq_index = 0;
			}
			else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
			{
			  ddd_mq_index = 1;
			}
			ddd_temp_low_total -= ddd_stdev_list[ddd_pos_a];
			ddd_temp_low_count += 1;
			if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) >= ddd_rd_ave_by_gc_dup_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos_a]] )
			{
			  ddd_temp_low_count_2 += 1;
			  if( ddd_rd_windows_low_stdev[ddd_window_len] > 0 && (ddd_temp_low_total/(ddd_temp_low_count*ddd_rd_windows_low_stdev[ddd_window_len])) >= g_one_base_read_depth_min_rd_low_stdev && ((ddd_window_len - ddd_temp_low_count) / ((double)ddd_window_len)) <= g_max_rd_low_acgt_or_windows )
			  {
			    ddd_dup_last_good = ddd_pos_a;
			    if( ddd_dup_begin == 0 )
			    {
			      ddd_dup_begin = 1;
			      ddd_dup_start = ddd_pos;
			      ddd_dup_ref = ddd_chr - ddd_begin + 1;
			      ddd_dup_end = ddd_pos_a;
			      ddd_dup_stdevs = ddd_temp_low_total / (ddd_temp_low_count * ddd_rd_windows_low_stdev[ddd_window_len]);
			    }
			    else
			    {
			      ddd_temp_stdevs = ddd_temp_low_total / (ddd_temp_low_count * ddd_rd_windows_low_stdev[ddd_window_len]);
			      ddd_dup_end = ddd_pos_a;
			      if( ddd_temp_stdevs > ddd_dup_stdevs )
			      {
				ddd_dup_stdevs = ddd_temp_stdevs;
			      }
			    }

			  }
			}
			else if( (2 * ddd_temp_low_count_2) < ddd_window_len )
			{
			  ddd_stop_base = 1;
			  break;
			}

		      }
		      else if( (2*ddd_temp_low_count_2) < ddd_window_len )
		      {
			ddd_stop_base = 1;
			break;
		      }
		    }
		    else
		    {
		      ddd_stop_base = 1;
		      break;
		    }
		  }
		}



		if( ddd_stop_base == 0 && ddd_dup_begin == 1 )
		{
		  ddd_pos_a = ddd_pos + g_max_rd_window_len;
		  ddd_temp_low_total = 0;
		  ddd_temp_low_count = 0;
		  ddd_mq_index_b = ddd_mq_index;
		  while( ddd_pos_a < ddd_chr_fasta_len && (ddd_pos_a - ddd_dup_last_good) <= g_max_distance_since_last_del_good )
		  {
		    if( ddd_pos_a == (ddd_pos + g_max_rd_window_len) )
		    {
		      for(ddd_pos_b=(ddd_pos_a-g_max_rd_window_len+1);ddd_pos_b<(ddd_pos_a+1);ddd_pos_b++)
		      {
			if( ddd_rd_mq_list[ddd_pos_b] >= g_rd_min_mapq )
			{
			  ddd_mq_index_b = 0;
			}
			else if( (ddd_rd_rd_list[ddd_pos_b] + ddd_rd_low_mq_rd_list[ddd_pos_b]) > 0 )
			{
			  ddd_mq_index_b = 1;
			}
			if( ddd_rd_low_acgt_or_windows_list[ddd_pos_b] == 0 && ddd_rd_windows_by_gc[ddd_mq_index_b][ddd_one_base_rd_gc_weighted[ddd_pos_b]] > 1 )
			{
			  ddd_temp_low_total -= ddd_stdev_list[ddd_pos_b];
			  ddd_temp_low_count += 1;
			}
		      }
		    }
		    else
		    {
		      ddd_pos_b = ddd_pos_a - g_max_rd_window_len;
		      if( ddd_rd_mq_list[ddd_pos_b] >= g_rd_min_mapq )
		      {
			ddd_mq_index_b = 0;
		      }
		      else if( (ddd_rd_rd_list[ddd_pos_b] + ddd_rd_low_mq_rd_list[ddd_pos_b]) > 0 )
		      {
			ddd_mq_index_b = 1;
		      }
		      if( ddd_rd_low_acgt_or_windows_list[ddd_pos_b] == 0 && ddd_rd_windows_by_gc[ddd_mq_index_b][ddd_one_base_rd_gc_weighted[ddd_pos_b]] > 1 )
		      {
			ddd_temp_low_total += ddd_stdev_list[ddd_pos_b];
			ddd_temp_low_count -= 1;
		      }
		      if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
		      {
			ddd_mq_index = 0;
		      }
		      else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
		      {
			ddd_mq_index = 1;
		      }
		      if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 && ddd_rd_windows_by_gc[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos_a]] > 1 )
		      {
			ddd_temp_low_total -= ddd_stdev_list[ddd_pos_a];
			ddd_temp_low_count += 1;
		      }

		    }

		    if( ddd_temp_low_count > 0 && ddd_rd_windows_low_stdev[g_max_rd_window_len] > 0 && (ddd_temp_low_total/(ddd_temp_low_count*ddd_rd_windows_low_stdev[g_max_rd_window_len])) >= g_one_base_read_depth_min_rd_low_stdev && ((g_max_rd_window_len-ddd_temp_low_count) / ((double)g_max_rd_window_len)) <= g_max_rd_low_acgt_or_windows )
		    {
		      ddd_dup_last_good = ddd_pos_a;
		      ddd_dup_end = ddd_pos_a;
		      ddd_temp_stdevs = ddd_temp_low_total / (ddd_temp_low_count*ddd_rd_windows_low_stdev[g_max_rd_window_len]);
		      if( ddd_temp_stdevs > ddd_dup_stdevs )
		      {
			ddd_dup_stdevs = ddd_temp_stdevs;
		      }
		    }
		    ddd_pos_a +=1;
		  }
		}



		if( ddd_dup_begin == 1 )
		{
		  ddd_pos = ddd_dup_end;
		  while( ddd_pos > (ddd_dup_start + g_min_rd_window_len) )
		  {
		    if( ddd_rd_mq_list[ddd_pos] >= g_rd_min_mapq )
		    {
		      ddd_mq_index = 0;
		    }
		    else if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) > 0 )
		    {
		      ddd_mq_index = 1;
		    }

		    if( (ddd_rd_rd_list[ddd_pos] + ddd_rd_low_mq_rd_list[ddd_pos]) < ddd_rd_ave_by_gc_dup_threshold[ddd_mq_index][ddd_one_base_rd_gc_weighted[ddd_pos]] )
		    {
		      ddd_pos -= 1;
		      ddd_dup_end = ddd_pos;
		    }
		    else
		    {
		      ddd_temp_low_count_2 = 0;
		      ddd_temp_low_count_3 = 0;
		      ddd_pos_a = ddd_dup_end;
		      ddd_stop_while = 0;
		      ddd_mq_index_a = ddd_mq_index;
		      while( ddd_pos_a > (ddd_dup_start + g_min_rd_window_len) && ddd_stop_while == 0 )
		      {
			if( ddd_rd_low_acgt_or_windows_list[ddd_pos_a] == 0 )
			{
			  if( ddd_rd_mq_list[ddd_pos_a] >= g_rd_min_mapq )
			  {
			    ddd_mq_index_a = 0;
			  }
			  else if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) > 0 )
			  {
			    ddd_mq_index_a = 1;
			  }
			  ddd_temp_low_count_3 += 1;
			  if( (ddd_rd_rd_list[ddd_pos_a] + ddd_rd_low_mq_rd_list[ddd_pos_a]) >= ddd_rd_ave_by_gc_dup_threshold[ddd_mq_index_a][ddd_one_base_rd_gc_weighted[ddd_pos_a]] )
			  {
			    ddd_temp_low_count_2 += 1;
			  }
			}
			if( ddd_temp_low_count_3 == 0 || (ddd_temp_low_count_3 > 0 && (ddd_temp_low_count_2 / ((double)ddd_temp_low_count_3)) < 0.5) || ((ddd_dup_end - ddd_pos_a + 1 - ddd_temp_low_count_3) / ((double)ddd_dup_end - (double)ddd_pos_a + 1.0)) > g_max_rd_low_acgt_or_windows )
			{
			  ddd_dup_end = ddd_pos_a - 1;
			  ddd_stop_while = 1;
			}
			ddd_pos_a -= 1;
		      }
		      ddd_pos = ddd_pos_a;
		    }
		  }
		  ddd_pos = ddd_dup_end + 1;
		  ddd_dup_list_start[*ddd_dup_list_index] = ddd_dup_start;
		  ddd_dup_list_end[*ddd_dup_list_index] = ddd_dup_end;
		  ddd_dup_list_ref[*ddd_dup_list_index] = ddd_dup_ref;
		  ddd_dup_list_stdev[*ddd_dup_list_index] = ddd_dup_stdevs;
		  *ddd_dup_list_index += 1;
		  ddd_dup_start = 0;
		  ddd_dup_end = 0;
		  ddd_dup_ref = 0;
		  ddd_dup_stdevs = 0;
		  ddd_dup_last_good = 0;
		  ddd_dup_begin = 0;

		}



		else if( ddd_stop_base == 1 )
		{
		  ddd_pos = ddd_temp_pos;
		}
	      }
	      ddd_pos += 1;

	    }
	  }
	  

	long ddd_longest_del = 0;
	long ddd_longest_dup = 0;
	long ddd_del_ploidy_count, ddd_dup_ploidy_count;
	long ddd_del_ploidy_count_start, ddd_del_ploidy_count_end;
	long ddd_dup_ploidy_count_start, ddd_dup_ploidy_count_end;
	double ddd_del_ploidy, ddd_dup_ploidy;


	  for(ddd_a_loop=0;ddd_a_loop<*ddd_del_list_index;ddd_a_loop++)
	  {
	    if( (ddd_del_list_end[ddd_a_loop] - ddd_del_list_start[ddd_a_loop]) > ddd_longest_del )
	    {
	      ddd_longest_del = (ddd_del_list_end[ddd_a_loop] - ddd_del_list_start[ddd_a_loop]);
	    }
	  }
	  for(ddd_a_loop=0;ddd_a_loop<*ddd_dup_list_index;ddd_a_loop++)
	  {
	    if( (ddd_dup_list_end[ddd_a_loop] - ddd_dup_list_start[ddd_a_loop]) > ddd_longest_dup )
	    {
	      ddd_longest_dup = (ddd_dup_list_end[ddd_a_loop] - ddd_dup_list_start[ddd_a_loop]);
	    }
	  }

	double *ddd_del_ploidy_list = (double *) malloc((ddd_longest_del+1) * sizeof(double));
	if( ddd_del_ploidy_list == NULL )
	{
	  printf("452 NULL\n");
	  exit(0);
	}
	double *ddd_dup_ploidy_list = (double *) malloc((ddd_longest_dup+1) * sizeof(double));
	if( ddd_dup_ploidy_list == NULL )
	{
	  printf("453 NULL\n");
	  exit(0);
	}
	long ddd_temp_start;
	long ddd_temp_end;

	for(ddd_a_loop=0;ddd_a_loop<*ddd_del_list_index;ddd_a_loop++)
	{

		ddd_del_ploidy = 0.0;
		ddd_del_ploidy_count = 0;

		ddd_temp_start = ddd_del_list_start[ddd_a_loop];
		ddd_temp_end = ddd_del_list_end[ddd_a_loop];


		for(ddd_b_loop=ddd_temp_start;ddd_b_loop<ddd_temp_end;ddd_b_loop++)
		{
			if( ddd_rd_low_acgt_or_windows_list[ddd_b_loop] == 0 )
			{
				if( ddd_rd_mq_list[ddd_b_loop] >= g_rd_min_mapq )
				{
					if( ddd_rd_ave_by_gc[0][ddd_one_base_rd_gc_weighted[ddd_b_loop]] > 0 )
					{
						ddd_del_ploidy_list[ddd_del_ploidy_count] = (double) (ddd_rd_rd_list[ddd_b_loop] + ddd_rd_low_mq_rd_list[ddd_b_loop]) / ddd_rd_ave_by_gc[0][ddd_one_base_rd_gc_weighted[ddd_b_loop]];
						ddd_del_ploidy_count += 1;
					}
				}
				else
				{
					if( ddd_rd_ave_by_gc[1][ddd_one_base_rd_gc_weighted[ddd_b_loop]] > 0 )
					{
						ddd_del_ploidy_list[ddd_del_ploidy_count] = (double) (ddd_rd_rd_list[ddd_b_loop] + ddd_rd_low_mq_rd_list[ddd_b_loop]) / ddd_rd_ave_by_gc[1][ddd_one_base_rd_gc_weighted[ddd_b_loop]];
						ddd_del_ploidy_count += 1;
					}
				}

			}
		}

		if( ddd_del_ploidy_count > 0 )
		{
			qsort(ddd_del_ploidy_list, ddd_del_ploidy_count, sizeof(double), cmpfunc);
			ddd_del_ploidy_count_start = 0.1 * ddd_del_ploidy_count;

			ddd_del_ploidy_count_end = ddd_del_ploidy_count - ddd_del_ploidy_count_start;

			for(ddd_c_loop=ddd_del_ploidy_count_start;ddd_c_loop<ddd_del_ploidy_count_end;ddd_c_loop++)
			{
				ddd_del_ploidy += ddd_del_ploidy_list[ddd_c_loop];
			}

			if( (ddd_del_ploidy_count_end-ddd_del_ploidy_count_start) > 0 )
			{
				ddd_del_list_cn[ddd_a_loop] = (ddd_del_ploidy / (ddd_del_ploidy_count_end-ddd_del_ploidy_count_start)) * ddd_ploidy;

			  ddd_del_list_cn_stdev[ddd_a_loop] = 0;
			  for(ddd_c_loop=0;ddd_c_loop<ddd_del_ploidy_count;ddd_c_loop++)
			  {
				  ddd_del_list_cn_stdev[ddd_a_loop] += pow((ddd_ploidy * ddd_del_ploidy_list[ddd_c_loop] - ddd_del_list_cn[ddd_a_loop]), 2);
			  }
			  ddd_del_list_cn_stdev[ddd_a_loop] = sqrt(ddd_del_list_cn_stdev[ddd_a_loop]/ddd_del_ploidy_count);
			}
			else
			{
				ddd_del_list_cn[ddd_a_loop] = - 1;
				ddd_del_list_cn_stdev[ddd_a_loop] = 0;
			}

		}
		else
		{
			ddd_del_list_cn[ddd_a_loop] = - 1;
			ddd_del_list_cn_stdev[ddd_a_loop] = 0;
		}

	}

	for(ddd_a_loop=0;ddd_a_loop<*ddd_dup_list_index;ddd_a_loop++)
	{

		ddd_dup_ploidy = 0.0;
		ddd_dup_ploidy_count = 0;
		for(ddd_b_loop=ddd_dup_list_start[ddd_a_loop];ddd_b_loop<ddd_dup_list_end[ddd_a_loop];ddd_b_loop++)
		{
			if( ddd_rd_low_acgt_or_windows_list[ddd_b_loop] == 0 )
			{
				if( ddd_rd_mq_list[ddd_b_loop] >= g_rd_min_mapq )
				{
					if( ddd_rd_ave_by_gc[0][ddd_one_base_rd_gc_weighted[ddd_b_loop]] > 0 )
					{
						ddd_dup_ploidy_list[ddd_dup_ploidy_count] = (double) (ddd_rd_rd_list[ddd_b_loop] + ddd_rd_low_mq_rd_list[ddd_b_loop]) / ddd_rd_ave_by_gc[0][ddd_one_base_rd_gc_weighted[ddd_b_loop]];
						ddd_dup_ploidy_count += 1;
					}
				}
				else
				{
					if( ddd_rd_ave_by_gc[1][ddd_one_base_rd_gc_weighted[ddd_b_loop]] > 0 )
					{
						ddd_dup_ploidy_list[ddd_dup_ploidy_count] = (double) (ddd_rd_rd_list[ddd_b_loop] + ddd_rd_low_mq_rd_list[ddd_b_loop]) / ddd_rd_ave_by_gc[1][ddd_one_base_rd_gc_weighted[ddd_b_loop]];
						ddd_dup_ploidy_count += 1;
					}
				}

			}
		}
		if( ddd_dup_ploidy_count > 0 )
		{
			qsort(ddd_dup_ploidy_list, ddd_dup_ploidy_count, sizeof(double), cmpfunc);
			ddd_dup_ploidy_count_start = 0.1 * ddd_dup_ploidy_count;
			ddd_dup_ploidy_count_end = ddd_dup_ploidy_count - ddd_dup_ploidy_count_start;

			for(ddd_c_loop=ddd_dup_ploidy_count_start;ddd_c_loop<ddd_dup_ploidy_count_end;ddd_c_loop++)
			{
				ddd_dup_ploidy += ddd_dup_ploidy_list[ddd_c_loop];
			}

			if( (ddd_dup_ploidy_count_end-ddd_dup_ploidy_count_start) > 0 )
			{
				ddd_dup_list_cn[ddd_a_loop] = (ddd_dup_ploidy / (ddd_dup_ploidy_count_end-ddd_dup_ploidy_count_start)) * ddd_ploidy;

			  ddd_dup_list_cn_stdev[ddd_a_loop] = 0;
			  for(ddd_c_loop=0;ddd_c_loop<ddd_dup_ploidy_count;ddd_c_loop++)
			  {
				  ddd_dup_list_cn_stdev[ddd_a_loop] += pow((ddd_ploidy*ddd_dup_ploidy_list[ddd_c_loop] - ddd_dup_list_cn[ddd_a_loop]), 2);
			  }
			  ddd_dup_list_cn_stdev[ddd_a_loop] = sqrt(ddd_dup_list_cn_stdev[ddd_a_loop]/ddd_dup_ploidy_count);

			}
			else
			{
				ddd_dup_list_cn[ddd_a_loop] = - 1;
				ddd_dup_list_cn_stdev[ddd_a_loop] = 0;
			}
		}
		else
		{
			ddd_dup_list_cn[ddd_a_loop] = - 1;
			ddd_dup_list_cn_stdev[ddd_a_loop] = 0;
		}

	}

	free(ddd_del_ploidy_list);
	free(ddd_dup_ploidy_list);

	free(ddd_stdev_list);
	free(ddd_rd_low_acgt_or_windows_list);

	return;

}

int bisect_list(int *bl_list, int bl_pos, int bl_start, int bl_end, int bl_type)
{
  int bl_range = bl_end/64;
  if( bl_range < 4 )
  {
    bl_range = 4;
  }
  else if( bl_range > 64 )
  {
    bl_range = 64;
  }
  int bl_low_index = round((double) bl_pos * (double) bl_end / (double) bl_list[bl_end]) - bl_range;
  int bl_high_index = round((double) bl_pos * (double) bl_end / (double) bl_list[bl_end]) + bl_range;
  if( bl_low_index < bl_start || bl_low_index >= bl_end )
  {
    bl_low_index = bl_start;
  }
  else if( bl_list[bl_low_index] > bl_pos )
  {
    bl_high_index = bl_low_index;
    bl_low_index = bl_start;
  }
  if( bl_high_index > bl_end || bl_high_index < bl_start )
  {
    bl_high_index = bl_end;
  }
  else if( bl_list[bl_high_index] < bl_pos )
  {
    bl_low_index = bl_high_index;
    bl_high_index = bl_end;
  }
  int bl_index = bl_low_index + (bl_high_index - bl_low_index)/2;
  int bl_index_found = 0;

  while( bl_index_found == 0 )
  {
    
	if( bl_pos < bl_list[bl_index] )
    {
      bl_high_index = bl_index;
      bl_index = bl_low_index + (bl_index-bl_low_index)/2;
      if( bl_high_index == bl_index )
      {
	bl_index_found = 1;
      }
    }
	
    else if( bl_pos > bl_list[bl_index] )
    {
      bl_low_index = bl_index;
      bl_index = bl_index + (bl_high_index - bl_index)/2;
      if( bl_low_index == bl_index )
      {
	bl_index_found = 1;
      }
    }
    else
    {
      bl_index_found = 1;
    }
  }

  if( bl_type == 0 && bl_pos > bl_list[bl_index] && bl_index < bl_end )
  {
    bl_index += 1;
  }
  else if( bl_type == 1 && bl_pos < bl_list[bl_index] && bl_index > bl_start )
  {
    bl_index -= 1;
  }

  return bl_index;
}


void find_disc_svs(char *fds_bam_file_name, FILE *fds_fasta_handle, char *fds_results_file_name, char *fds_tumor_sv_file_name, int fds_tumor_len, char *fds_fasta_file_name)
{
  samfile_t *fds_bam_file = NULL;

  int fds_one_base_index_start = g_one_base_rd_len/4 + 1;

  if( temp_chr_fasta == NULL )
  {
    printf("455 NULL\n");
    exit(0);
  }
  char fds_fasta_line[1000];
  long fds_a_loop, fds_b_loop;
  long fds_c_loop, fds_bam_a;
  long fds_chr_fasta_len = 0;

  int fds_fasta_line_len = 0;
  int fds_fasta_line_alpha_len = 0;
  int fds_fasta_while;


  char *fds_target_name;
  char *fds_test_chr = "chr";
  int fds_chr_match = -1;
  char fds_bam_name[g_max_chr_names];
  int fds_bam_name_len;
  int fds_int_a;
  long fds_num_chr;

  char partial_results_file_name[1024];
  int loop_start, loop_limit;



  fds_bam_file = samopen(fds_bam_file_name, "rb", 0);
  if(fds_bam_file == NULL)
  {
    printf("\nCould not open %s\n", fds_bam_file_name);
    return;
  }



  FILE *fds_results_file = NULL;
  FILE *fds_results_file_snv = NULL;
  FILE *fds_results_file_ind = NULL;
  FILE *fds_results_file_sv = NULL;
  FILE *fds_results_file_ctx = NULL;
  char fds_results_file_name_snv[1024];
  char fds_results_file_name_ind[1024];
  char fds_results_file_name_sv[1024];

  char fds_results_file_name_ctx[1024];
  char partial_results_file_name_ctx[1024];
  char fds_snv_string[4];
  strcpy(fds_snv_string, "SNV");
  char fds_ind_string[6];
  strcpy(fds_ind_string, "INDEL");
  char fds_sv_string[3];
  strcpy(fds_sv_string, "SV");
  char fds_bnd_string[4];
  strcpy(fds_bnd_string, "BND");

  char fds_ctx_string[4];
  strcpy(fds_ctx_string, "ctx");

  if (g_one_chromosome < 0) {
      fds_results_file = fopen(fds_results_file_name, "w");

      if( strlen(fds_results_file_name) > 4 && fds_results_file_name[strlen(fds_results_file_name)-4] == '.' && fds_results_file_name[strlen(fds_results_file_name)-3] == 'v' && fds_results_file_name[strlen(fds_results_file_name)-2] == 'c' && fds_results_file_name[strlen(fds_results_file_name)-1] == 'f' )
      {
			char copy_fds[1024];
			char *ext;

			ext = strrchr(fds_results_file_name, '.');
			snprintf(copy_fds, ext - fds_results_file_name + 1, "%s", fds_results_file_name);

			fds_results_file_name = copy_fds;
			sprintf(fds_results_file_name_snv, "%s.%s.vcf", fds_results_file_name, fds_snv_string);
			sprintf(fds_results_file_name_ind, "%s.%s.vcf", fds_results_file_name, fds_ind_string);
			sprintf(fds_results_file_name_sv, "%s.%s.vcf", fds_results_file_name, fds_sv_string);
			sprintf(fds_results_file_name_bnd, "%s.%s.vcf", fds_results_file_name, fds_bnd_string);
			sprintf(fds_results_file_name_ctx, "%s.%s.vcf", fds_results_file_name, fds_ctx_string);
      }
      else
      {
		if( g_vcf == 1 )
		{
			sprintf(fds_results_file_name_snv, "%s.%s.vcf", fds_results_file_name, fds_snv_string);
			sprintf(fds_results_file_name_ind, "%s.%s.vcf", fds_results_file_name, fds_ind_string);
			sprintf(fds_results_file_name_sv, "%s.%s.vcf", fds_results_file_name, fds_sv_string);
			sprintf(fds_results_file_name_bnd, "%s.%s.vcf", fds_results_file_name, fds_bnd_string);
		}
		sprintf(fds_results_file_name_ctx, "%s.%s.txt", fds_results_file_name, fds_ctx_string);

      }


		if (g_troubleshooting == 1)
		{
			printf("Output files created\n");
		}
		fds_results_file_snv = fopen(fds_results_file_name_snv, "w");
        fds_results_file_ind = fopen(fds_results_file_name_ind, "w");
	    fds_results_file_sv = fopen(fds_results_file_name_sv, "w");
        fds_results_file_ctx = fopen(fds_results_file_name_ctx, "w");

		if (g_troubleshooting == 1)
		{
			printf("Output files opened\n");
		}

    if (fds_results_file == NULL)
    {
      printf("Error opening fds results file %s\n", fds_results_file_name);
      exit(1);
    }
    else
    {

      if( g_vcf == 1 )
      {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	fprintf(fds_results_file_snv, "##fileformat=VCFv4.2\n");

	if (g_troubleshooting == 1)
	{
		printf("Header snv file can fprintf\n");
	}
	fprintf(fds_results_file_snv, "##fileDate=%d%d%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	fprintf(fds_results_file_snv, "##source=%s", g_version_name);
	fprintf(fds_results_file_snv, "##reference=%s\n", fds_fasta_file_name);

	fprintf(fds_results_file_ind, "##fileformat=VCFv4.2\n");
	fprintf(fds_results_file_ind, "##fileDate=%d%d%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	fprintf(fds_results_file_ind, "##source=%s", g_version_name);
	fprintf(fds_results_file_ind, "##reference=%s\n", fds_fasta_file_name);

	fprintf(fds_results_file_sv, "##fileformat=VCFv4.2\n");
	fprintf(fds_results_file_sv, "##fileDate=%d%d%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	fprintf(fds_results_file_sv, "##source=%s", g_version_name);
	fprintf(fds_results_file_sv, "##reference=%s\n", fds_fasta_file_name);

	long fgl_a_loop;
	for(fgl_a_loop=0;fgl_a_loop<g_chr_names_index;fgl_a_loop++)
	{
		fprintf(fds_results_file_snv, "##CONTIG=ID=%s>\n", g_chr_names[fgl_a_loop]);
		fprintf(fds_results_file_ind, "##CONTIG=ID=%s>\n", g_chr_names[fgl_a_loop]);
		fprintf(fds_results_file_sv, "##CONTIG=ID=%s>\n", g_chr_names[fgl_a_loop]);
	}

	if (g_troubleshooting == 1)
	{
		printf("Contigs written to header\n");
	}


	fprintf(fds_results_file_snv, "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=AF,Number=1,Type=Float,Description=\"Allele frequency (high mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=PR,Number=1,Type=Float,Description=\"Probability of SNV evidence occurring by chance\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=AMQ,Number=1,Type=Integer,Description=\"A nucleotides (high mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=CMQ,Number=1,Type=Integer,Description=\"C nucleotides (high mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=GMQ,Number=1,Type=Integer,Description=\"G nucleotides (high mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=TMQ,Number=1,Type=Integer,Description=\"T nucleotides (high mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=ALMQ,Number=1,Type=Integer,Description=\"A nucleotides (low mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=CLMQ,Number=1,Type=Integer,Description=\"C nucleotides (low mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=GLMQ,Number=1,Type=Integer,Description=\"G nucleotides (low mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=TLMQ,Number=1,Type=Integer,Description=\"T nucleotides (low mapping quality reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=BQ,Number=1,Type=Float,Description=\"Average base quality (all reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=MQ,Number=1,Type=Float,Description=\"Average mapping quality (all reads)\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=PIR,Number=1,Type=Float,Description=\"Average distance of SNV from DNA fragment end\">\n");
	fprintf(fds_results_file_snv, "##FORMAT=<ID=FS,Number=1,Type=Float,Description=\"Fraction of SNV reads mapped to forward strand\">\n");


	fprintf(fds_results_file_ind, "##INFO=<ID=TYPE,Number=1,Type=String,Description=\"Type of variant\">\n");
	fprintf(fds_results_file_ind, "##INFO=<ID=END,Number=1,Type=Integer,Description=\"End position of the structural variant\">\n");
	fprintf(fds_results_file_ind, "##INFO=<ID=INDEL_INS,Description=\"Indel Insertion\">\n");
	fprintf(fds_results_file_ind, "##INFO=<ID=INDEL_DEL,Description=\"Indel Deletion\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=SPR,Number=1,Type=Float,Description=\"Probability of start breakpoint evidence occurring by chance\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=EPR,Number=1,Type=Float,Description=\"Probability of end breakpoint evidence occurring by chance\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=SEV,Number=1,Type=Integer,Description=\"Evidence supporting variant at start breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=EEV,Number=1,Type=Integer,Description=\"Evidence supporting variant at end breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=SRD,Number=1,Type=Integer,Description=\"Physical read depth at start breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=ERD,Number=1,Type=Integer,Description=\"Physical read depth at end breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=SCO,Number=1,Type=Integer,Description=\"Concordant pairs at start breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=ECO,Number=1,Type=Integer,Description=\"Concordant pairs at end breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=SOT,Number=1,Type=Integer,Description=\"Count of distinct SVs with evidence at start breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=EOT,Number=1,Type=Integer,Description=\"Count of distinct SVs with evidence at end breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=SSC,Number=1,Type=Integer,Description=\"Soft-clipped reads at start breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=ESC,Number=1,Type=Integer,Description=\"Soft-clipped at end breakpoint\">\n");
	fprintf(fds_results_file_ind, "##FORMAT=<ID=HP,Number=1,Type=Integer,Description=\"Homopolymer\">\n");


	fprintf(fds_results_file_sv, "##ALT=<ID=DEL,Description=\"Deletion\">\n");
	fprintf(fds_results_file_sv, "##ALT=<ID=DUP,Description=\"Duplication\">\n");
	fprintf(fds_results_file_sv, "##ALT=<ID=INS,Description=\"Insertion\">\n");
	fprintf(fds_results_file_sv, "##ALT=<ID=INV,Description=\"Inversion\">\n");
	fprintf(fds_results_file_sv, "##INFO=<ID=SVTYPE,Number=1,Type=String,Description=\"Type of structural variant\">\n");
	fprintf(fds_results_file_sv, "##INFO=<ID=END,Number=1,Type=Integer,Description=\"End position of the structural variant\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=SPR,Number=1,Type=Float,Description=\"Probability of start breakpoint evidence occurring by chance\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=EPR,Number=1,Type=Float,Description=\"Probability of end breakpoint evidence occurring by chance\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=SEV,Number=1,Type=Integer,Description=\"Evidence supporting variant at start breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=EEV,Number=1,Type=Integer,Description=\"Evidence supporting variant at end breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=SRD,Number=1,Type=Integer,Description=\"Physical read depth at start breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=ERD,Number=1,Type=Integer,Description=\"Physical read depth at end breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=SCO,Number=1,Type=Integer,Description=\"Concordant pairs at start breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=ECO,Number=1,Type=Integer,Description=\"Concordant pairs at end breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=SOT,Number=1,Type=Integer,Description=\"Count of distinct SVs with evidence at start breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=EOT,Number=1,Type=Integer,Description=\"Count of distinct SVs with evidence at end breakpoint\">\n");

	fprintf(fds_results_file_sv, "##FORMAT=<ID=SFR,Number=1,Type=Integer,Description=\"Position of first read supporting start breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=SLR,Number=1,Type=Integer,Description=\"Position of last read supporting start breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=EFR,Number=1,Type=Integer,Description=\"Position of first read supporting end breakpoint\">\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=ELR,Number=1,Type=Integer,Description=\"Position of last read supporting end breakpoint\">\n");

	fprintf(fds_results_file_sv, "##FORMAT=<ID=SD,Number=1,Type=Float,Description=\"CNV standard deviation\"\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=Z,Number=1,Type=Float,Description=\"CNV probability score\"\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=CN,Number=1,Type=Float,Description=\"CNV copy number\"\n");
	fprintf(fds_results_file_sv, "##FORMAT=<ID=CS,Number=1,Type=Float,Description=\"CNV copy number standard deviation\"\n");


	fprintf(fds_results_file_snv, "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\t%s\n", sample_id);
	fprintf(fds_results_file_ind, "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\t%s\n", sample_id);
	fprintf(fds_results_file_sv, "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\t%s\n", sample_id);



	  if (g_troubleshooting == 1)
	  {
		printf("Headers written\n");
	  }

	}
      else
      {
	fprintf(fds_results_file, "%d\t%d\t%d\t%d\n", g_insert_mean, g_insert_min_size, g_insert_max_size, g_lseq);

	fprintf(fds_results_file, "SV\t");
	fprintf(fds_results_file, "Chromosome\t");
	fprintf(fds_results_file, "Start (Tumor)\t");
	fprintf(fds_results_file, "End (Tumor)\t");
	fprintf(fds_results_file, "Length (Tumor)\t");
	fprintf(fds_results_file, "P-val (Start, Tumor)\t");
	fprintf(fds_results_file, "P-val (End, Tumor)\t");
	fprintf(fds_results_file, "Concordant Pairs (Start, Tumor)\t");
	fprintf(fds_results_file, "Concordant Pairs (End, Tumor)\t");
	fprintf(fds_results_file, "Start or End?\t");
	fprintf(fds_results_file, "Read Depth (High MapQ, Normal)\t");
	fprintf(fds_results_file, "Read Depth (Low MapQ, Normal)\t");
	fprintf(fds_results_file, "Concordant Pairs (Normal)\t");
	fprintf(fds_results_file, "INS (Normal)\t");
	fprintf(fds_results_file, "DEL (For, Normal)\t");
	fprintf(fds_results_file, "DEL (Rev, Normal)\t");
	fprintf(fds_results_file, "DEL (For, Length, Normal)\t");
	fprintf(fds_results_file, "DEL (Rev, Length, Normal)\t");
	fprintf(fds_results_file, "DUP (Rev, Normal)\t");
	fprintf(fds_results_file, "DUP (For, Normal)\t");
	fprintf(fds_results_file, "DUP (Rev, Length, Normal)\t");
	fprintf(fds_results_file, "DUP (For, Length, Normal)\t");
	fprintf(fds_results_file, "INV (For, Start, Normal)\t");
	fprintf(fds_results_file, "INV (Rev, Start, Normal)\t");
	fprintf(fds_results_file, "INV (For, End, Normal)\t");
	fprintf(fds_results_file, "INV (Rev, End, Normal)\t");
	fprintf(fds_results_file, "INV (For, Start, Length, Normal)\t");
	fprintf(fds_results_file, "INV (Rev, Start, Length, Normal)\t");
	fprintf(fds_results_file, "INV (For, End, Length, Normal)\t");
	fprintf(fds_results_file, "INV (Rev, End, Length, Normal)\t");
	fprintf(fds_results_file, "Unmapped Mate (For, Normal)\t");
	fprintf(fds_results_file, "Unmapped Mate (Rev, Normal)\t");
	fprintf(fds_results_file, "Soft-clipping (Left, Normal)\t");
	fprintf(fds_results_file, "Soft-clipping (Right, Normal)\t");
	fprintf(fds_results_file, "Soft-clipping Read Depth (Left, Normal)\t");
	fprintf(fds_results_file, "Soft-clipping Read Depth (Right, Normal)\t");
	fprintf(fds_results_file, "Soft-clipping Read Depth (Left+Right, Normal)\t");
	fprintf(fds_results_file, "INS Indel (Normal)\t");
	fprintf(fds_results_file, "DEL Indel (Start, Normal)\t");
	fprintf(fds_results_file, "DEL Indel (End, Normal)\t");
	fprintf(fds_results_file, "DEL Indel (Start, Length, Normal)\t");
	fprintf(fds_results_file, "DEL Indel (End, Length, Normal)\t");
	fprintf(fds_results_file, "CTX Soft-clipping (Left, Normal)\t");
	fprintf(fds_results_file, "CTX Soft-clipping (Right, Normal)\t");
	fprintf(fds_results_file, "CTX Soft-clipping Read Depth (Left, Normal)\t");
	fprintf(fds_results_file, "CTX Soft-clipping Read Depth (Right, Normal)\t");
	fprintf(fds_results_file, "CTX Soft-clipping Read Depth (Left+Right, Normal)\t");
	fprintf(fds_results_file, "Indel Soft-clipping (Left, Normal)\t");
	fprintf(fds_results_file, "Indel Soft-clipping (Right, Normal)\t");
	fprintf(fds_results_file, "Indel Soft-clipping Read Depth (Left, Normal)\t");
	fprintf(fds_results_file, "Indel Soft-clipping Read Depth (Right, Normal)\t");
	fprintf(fds_results_file, "Indel Soft-clipping Read Depth (Left+Right, Normal)\t");
	fprintf(fds_results_file, "Soft-clipping (Left Max including CTX, Normal)\t");
	fprintf(fds_results_file, "Soft-clipping (Right Max including CTX, Normal)\t");



	fprintf(fds_results_file, "Other (Number of Non-Empty, Normal)\t");
	fprintf(fds_results_file, "CTX (For, Normal)\t");
	fprintf(fds_results_file, "CTX (Rev, Normal)\t");
	fprintf(fds_results_file, "SV Overlap (Normal)\t");
	fprintf(fds_results_file, "Other (Number of Non-Empty, Tumor)\t");

	fprintf(fds_results_file, "Read Start (Start, Tumor)\t");
	fprintf(fds_results_file, "Read End (Start, Tumor)\t");
	fprintf(fds_results_file, "Read Start (End, Tumor)\t");
	fprintf(fds_results_file, "Read End (End, Tumor)\t");
	fprintf(fds_results_file, "DEL Read Start (For/Rev, Normal)\t");
	fprintf(fds_results_file, "DEL Read End (For/Rev, Normal)\t");
	fprintf(fds_results_file, "DUP Read Start (Rev/For, Normal)\t");
	fprintf(fds_results_file, "DUP Read End (Rev/For, Normal)\t");
	fprintf(fds_results_file, "INV Read Start (For, Normal)\t");
	fprintf(fds_results_file, "INV Read End (For, Normal)\t");
	fprintf(fds_results_file, "INV Read Start (Rev, Normal)\t");
	fprintf(fds_results_file, "INV Read End (Rev, Normal)\t");
	fprintf(fds_results_file, "CTX Read Start (For, Normal)\t");
	fprintf(fds_results_file, "CTX Read End (For, Normal)\t");
	fprintf(fds_results_file, "CTX Read Start (Rev, Normal)\t");
	fprintf(fds_results_file, "CTX Read End (Rev, Normal)\t");

	fprintf(fds_results_file, "Mate Chr (CTX only, Tumor)\t");
	fprintf(fds_results_file, "Mate Pos (CTX only, Tumor)\t");

	fprintf(fds_results_file, "Mate Chr (For, Normal)\t");
	fprintf(fds_results_file, "Mate Pos (For, Normal)\t");
	fprintf(fds_results_file, "Mate Chr (Rev, Normal)\t");
	fprintf(fds_results_file, "Mate Pos (Rev, Normal)\t");

	fprintf(fds_results_file, "Reference Base\t");
	fprintf(fds_results_file, "SNV Base (Tumor)\t");
	fprintf(fds_results_file, "SNV Ratio (Tumor)\t");
	fprintf(fds_results_file, "SNV Count (A, Tumor)\t");
	fprintf(fds_results_file, "SNV Count (C, Tumor)\t");
	fprintf(fds_results_file, "SNV Count (G, Tumor)\t");
	fprintf(fds_results_file, "SNV Count (T, Tumor)\t");
	fprintf(fds_results_file, "SNV Count (A, Normal)\t");
	fprintf(fds_results_file, "SNV Count (C, Normal)\t");
	fprintf(fds_results_file, "SNV Count (G, Normal)\t");
	fprintf(fds_results_file, "SNV Count (T, Normal)\t");

	fprintf(fds_results_file, "\n");
      }

    }
  }

  else {
      sprintf(partial_results_file_name, "%s.%s-%d", fds_results_file_name, fds_bam_file->header->target_name[g_one_chromosome], g_sub_child_number);
      fds_results_file = fopen(partial_results_file_name, "w");
      if (fds_results_file == NULL)
      {
	printf("Error opening file %s\n", partial_results_file_name);
	exit(1);
      }

      sprintf(partial_results_file_name_ctx, "%s.%s-%d.%s", fds_results_file_name, fds_bam_file->header->target_name[g_one_chromosome], g_sub_child_number, fds_ctx_string);
      fds_results_file_ctx = fopen(partial_results_file_name_ctx, "w");
      if (fds_results_file_ctx == NULL)
      {
	printf("Error opening file %s\n", partial_results_file_name_ctx);
	exit(1);
      }

  }



  long fdd_a_loop;


  double fdd_p = 0.3275911;
  double fdd_a1 = 0.254829592;
  double fdd_a2 = -0.284496736;
  double fdd_a3 = 1.421413741;
  double fdd_a4 = -1.453152027;
  double fdd_a5 = 1.061405429;
  double fdd_x, fdd_t, fdd_erf, fdd_prob;

  double fdd_sd;
  double fdd_sd_max = 10.0;
  int fdd_sd_loop;

  int fdd_pval2sd_list_len = (int) (fdd_sd_max/g_stdev_step + 0.5);
  fdd_pval2sd_list_len += 1;

  double *fdd_pval2sd_pval_list, *fdd_pval2sd_sd_list;
  fdd_pval2sd_pval_list  = (double *) malloc(fdd_pval2sd_list_len * sizeof(double));
  if( fdd_pval2sd_pval_list == NULL )
  {
    printf("456 NULL\n");
    exit(0);
  }
  fdd_pval2sd_sd_list  = (double *) malloc(fdd_pval2sd_list_len * sizeof(double));
  if( fdd_pval2sd_sd_list == NULL )
  {
    printf("457 NULL\n");
    exit(0);
  }


  for(fdd_sd_loop=0;fdd_sd_loop<fdd_pval2sd_list_len;fdd_sd_loop++)
  {
	  fdd_sd = fdd_sd_max - fdd_sd_loop * g_stdev_step;
	  if( fdd_sd < 0 )
	  {
		  fdd_sd = 0;
	  }
	  fdd_x = fdd_sd / sqrt_2;
	  fdd_t = 1.0/(1.0 + fdd_p * fdd_x);
	  fdd_erf = 1.0 - ((fdd_a1*fdd_t + fdd_a2 * pow(fdd_t,2) + fdd_a3 * pow(fdd_t,3) + fdd_a4 * pow(fdd_t,4) + fdd_a5 * pow(fdd_t,5)) * exp(-pow(fdd_x,2)));
	  fdd_prob = (1.0-fdd_erf)/2.0;
	  fdd_pval2sd_pval_list[fdd_sd_loop] = fdd_prob;
	  fdd_pval2sd_sd_list[fdd_sd_loop] = fdd_sd;
  }


  long fdd_low_mq_index[g_num_gc_bins];
  long fdd_high_mq_index[g_num_gc_bins];
  long fdd_low_mq_index_all[g_num_gc_bins];
  long fdd_high_mq_index_all[g_num_gc_bins];
  for(fdd_a_loop=0;fdd_a_loop<g_num_gc_bins;fdd_a_loop++)
  {
	  fdd_low_mq_index[fdd_a_loop] = 0;
	  fdd_high_mq_index[fdd_a_loop] = 0;
	  fdd_low_mq_index_all[fdd_a_loop] = 0;
	  fdd_high_mq_index_all[fdd_a_loop] = 0;
  }
  int **fdd_sample_high_mq_rd_list;
  int **fdd_sample_low_mq_rd_list;
  int **fdd_sample_repeat_rd_list;
  fdd_sample_high_mq_rd_list = (int**) malloc(g_num_gc_bins * sizeof(int*));
  if( fdd_sample_high_mq_rd_list == NULL )
  {
    printf("458 NULL\n");
    exit(0);
  }
  fdd_sample_low_mq_rd_list = (int**) malloc(g_num_gc_bins * sizeof(int*));
  if( fdd_sample_low_mq_rd_list == NULL )
  {
    printf("459 NULL\n");
    exit(0);
  }
  fdd_sample_repeat_rd_list = (int**) malloc(g_repeat_segments * sizeof(int*));
  if( fdd_sample_repeat_rd_list == NULL )
  {
    printf("460 NULL\n");
    exit(0);
  }
  for(fdd_a_loop=0;fdd_a_loop<g_num_gc_bins;fdd_a_loop++)
  {
	  fdd_sample_high_mq_rd_list[fdd_a_loop] = (int*) malloc(g_sample_lists_len * sizeof(int));
	  if( fdd_sample_high_mq_rd_list[fdd_a_loop] == NULL )
	  {
	    printf("461 NULL\n");
	    exit(0);
	  }
	  fdd_sample_low_mq_rd_list[fdd_a_loop] = (int*) malloc(g_sample_lists_len * sizeof(int));
	  if( fdd_sample_low_mq_rd_list[fdd_a_loop] == NULL )
	  {
	    printf("462 NULL\n");
	    exit(0);
	  }
  }
  for(fdd_a_loop=0;fdd_a_loop<g_repeat_segments;fdd_a_loop++)
  {
	  fdd_sample_repeat_rd_list[fdd_a_loop] = (int*) malloc(g_sample_lists_len * sizeof(int));
	  if( fdd_sample_repeat_rd_list[fdd_a_loop] == NULL )
	  {
	    printf("463 NULL\n");
	    exit(0);
	  }
  }


  fds_num_chr = fds_bam_file->header->n_targets;

  loop_start = 0;
  loop_limit = fds_num_chr;
  if (g_parallel_mode > 1) {
      allcate_children(fds_num_chr);
  }
  else if (g_one_chromosome >= 0) {
      loop_start = g_one_chromosome;
      loop_limit = g_one_chromosome+1;
  }
  for(fds_a_loop=loop_start;fds_a_loop<loop_limit;fds_a_loop++)
  {
    fds_bam_a = fds_a_loop;
    if( g_parallel_mode != 0 )
    {
      fds_chr_match = -1;
      for( fds_c_loop=0;fds_c_loop<fds_num_chr;fds_c_loop++)
      {
	fds_target_name = fds_bam_file->header->target_name[fds_c_loop];
	fds_bam_name_len = strlen(fds_target_name);

	for(fds_int_a=0;fds_int_a<fds_bam_name_len;fds_int_a++)
	{

		if (contig_lower_case == 1) {
			fds_bam_name[fds_int_a] = tolower(fds_target_name[fds_int_a]);
		} else {
			fds_bam_name[fds_int_a] = fds_target_name[fds_int_a];
		}
	}

	fds_int_a = fds_bam_name_len - 1;
	while( fds_int_a > 0 )
	{
	  if( isgraph(fds_bam_name[fds_int_a]) == 0 )
	  {
	    fds_bam_name_len = fds_int_a;
	  }
	  fds_int_a -= 1;
	}

	for(fds_b_loop=g_parallel_fasta_chr_index[fds_a_loop];fds_b_loop<g_parallel_fasta_chr_index[fds_a_loop]+1;fds_b_loop++)
	{
	  if( fds_bam_name_len == g_chr_names_len[fds_b_loop] && strncmp(fds_bam_name, g_chr_names[fds_b_loop], g_chr_names_len[fds_b_loop]) == 0 )
	  {
	    fds_chr_match = fds_b_loop;
	    break;
	  }
	  else if( (fds_bam_name_len-g_chr_name_start+1) == g_chr_names_len[fds_b_loop] && strncmp(fds_bam_name, fds_test_chr, (g_chr_name_start-1)) == 0 )
	  {

	    char *fds_temp_string = malloc(strlen(fds_test_chr)+strlen(g_chr_names[fds_b_loop])+1);
	    strcpy(fds_temp_string, fds_test_chr);
	    strcat(fds_temp_string, g_chr_names[fds_b_loop]);
	    if( strncmp(fds_bam_name, fds_temp_string, fds_bam_name_len) == 0 )
	    {
	      fds_chr_match = fds_b_loop;
	      break;
	    }
	    free(fds_temp_string);
	  }

	  else if( (fds_bam_name_len+g_chr_name_start-1) == g_chr_names_len[fds_b_loop] && strncmp(g_chr_names[fds_b_loop], fds_test_chr, (g_chr_name_start-1)) == 0 )
	  {

	    char *fds_temp_string = malloc(strlen(fds_test_chr)+strlen(fds_bam_name)+1);
	    strcpy(fds_temp_string, fds_test_chr);
	    strcat(fds_temp_string, fds_bam_name);
	    if( strncmp(g_chr_names[fds_b_loop], fds_temp_string, g_chr_names_len[fds_b_loop]) == 0 )
	    {
	      fds_chr_match = fds_b_loop;
	      break;
	    }
	    free(fds_temp_string);
	  }
	}
	if( fds_chr_match != -1 )
	{
	  fds_bam_a = fds_c_loop;
	  break;
	}
      }
    }
    else
    {
      fds_chr_match = -1;
      fds_target_name = fds_bam_file->header->target_name[fds_a_loop];
      fds_bam_name_len = strlen(fds_target_name);

      for(fds_int_a=0;fds_int_a<fds_bam_name_len;fds_int_a++)
      {

		if (contig_lower_case == 1) {
			fds_bam_name[fds_int_a] = tolower(fds_target_name[fds_int_a]);
		} else {
			fds_bam_name[fds_int_a] = fds_target_name[fds_int_a];
		}
      }

      fds_int_a = fds_bam_name_len - 1;
      while( fds_int_a > 0 )
      {
	if( isgraph(fds_bam_name[fds_int_a]) == 0 )
	{
	  fds_bam_name_len = fds_int_a;
	}
	fds_int_a -= 1;
      }

      for(fds_b_loop=0;fds_b_loop<g_chr_names_index;fds_b_loop++)
      {
	if( fds_bam_name_len == g_chr_names_len[fds_b_loop] && strncmp(fds_bam_name, g_chr_names[fds_b_loop], g_chr_names_len[fds_b_loop]) == 0 )
	{
	  fds_chr_match = fds_b_loop;
	  break;
	}
	else if( (fds_bam_name_len-g_chr_name_start+1) == g_chr_names_len[fds_b_loop] && strncmp(fds_bam_name, fds_test_chr, (g_chr_name_start-1)) == 0 )
	{

	  char *fds_temp_string = malloc(strlen(fds_test_chr)+strlen(g_chr_names[fds_b_loop])+1);
	  strcpy(fds_temp_string, fds_test_chr);
	  strcat(fds_temp_string, g_chr_names[fds_b_loop]);
	  if( strncmp(fds_bam_name, fds_temp_string, fds_bam_name_len) == 0 )
	  {
	    fds_chr_match = fds_b_loop;
	    break;
	  }
	  free(fds_temp_string);
	}
	else if( (fds_bam_name_len+g_chr_name_start-1) == g_chr_names_len[fds_b_loop] && strncmp(g_chr_names[fds_b_loop], fds_test_chr, (g_chr_name_start-1)) == 0 )
	{

	  char *fds_temp_string = malloc(strlen(fds_test_chr)+strlen(fds_bam_name)+1);
	  strcpy(fds_temp_string, fds_test_chr);
	  strcat(fds_temp_string, fds_bam_name);
	  if( strncmp(g_chr_names[fds_b_loop], fds_temp_string, g_chr_names_len[fds_b_loop]) == 0 )
	  {
	    fds_chr_match = fds_b_loop;
	    break;
	  }
	  free(fds_temp_string);
	}

      }
    }


    if( fds_bam_name_len == g_chry_len && strncmp(fds_bam_name, g_chry, g_chry_len) == 0 && g_gender == 0 )
    {
      fds_chr_match = -1;
    }
    else if( fds_bam_name_len == g_y_len && strncmp(fds_bam_name, g_y, g_y_len) == 0 && g_gender == 0 )
    {
      fds_chr_match = -1;
    }

	if( fds_chr_match >= 0 && g_chr_len[fds_chr_match] > 0 && g_chr_len[fds_chr_match] >= g_min_chr_fasta_len && g_chr_len[fds_chr_match] <= g_max_chr_fasta_len)
    {
      if (g_parallel_mode > 1) {
	  launch_one_chromosome(fds_bam_a, fds_bam_name, g_fasta_file_position[g_parallel_fasta_chr_index[fds_a_loop]+1] - g_fasta_file_position[g_parallel_fasta_chr_index[fds_a_loop]]);
      }
      else {
	if( g_internal == 1 )
	{
	  printf("fds_chr_match, chr len, bam name, fasta name %d %ld %s %s\n", fds_chr_match, g_chr_len[fds_chr_match], fds_target_name, g_chr_names[fds_chr_match]);
	}

	fds_chr_fasta_len = 0;

	fseek(fds_fasta_handle, g_fasta_file_position[fds_chr_match], SEEK_SET);


	while(fgets(fds_fasta_line, sizeof(fds_fasta_line), fds_fasta_handle) && fds_fasta_line[0] != '>' )
	{

	  if( fds_chr_fasta_len == 0 || strlen(fds_fasta_line) != fds_fasta_line_len )
	  {
	    fds_fasta_line_len = strlen(fds_fasta_line);
	    fds_fasta_while = fds_fasta_line_len - 1;
	    while( isalpha(fds_fasta_line[fds_fasta_while]) == 0 && fds_fasta_while > 0 )
	    {
	      fds_fasta_while -= 1;
	    }
	    fds_fasta_line_alpha_len = fds_fasta_while + 1;
	    if( g_internal == 1 )
	    {
	      printf("%d %d\n", fds_fasta_line_len, fds_fasta_line_alpha_len);
	    }
	  }

	  if( fds_chr_fasta_len + fds_fasta_line_alpha_len <= g_max_chr_fasta_len )
	  {
	    for(fds_b_loop=fds_chr_fasta_len;fds_b_loop<fds_chr_fasta_len+fds_fasta_line_alpha_len;fds_b_loop++)
	    {

	      temp_chr_fasta[fds_b_loop] = fds_fasta_line[fds_b_loop-fds_chr_fasta_len];
	    }
	  }
	  else
	  {
	    printf("ERROR: Reference chromosome length exceeds maximum allowed chromosome size (%ld)\n", g_max_chr_fasta_len);
	  }
	  fds_chr_fasta_len += fds_fasta_line_alpha_len;

	}
#ifdef DO_TIMING
	timers[10+ccount*2] = gettime();
#endif
	if( fds_chr_fasta_len > fds_one_base_index_start + g_overlap_mult*g_insert_max_size )
	{
	  my_before_reading(&g_single);
    #pragma omp parallel
	  {
	      int t = omp_get_thread_num();
	      if (t == 0) {
		  count_discordant_pairs(fds_bam_file, fds_bam_file_name, &temp_chr_fasta[0], fds_chr_fasta_len, g_chr_names[fds_chr_match], g_chr_names_len[fds_chr_match], fds_results_file, fds_tumor_sv_file_name, fds_tumor_len, fds_results_file_ctx, fdd_sample_high_mq_rd_list, fdd_sample_low_mq_rd_list, fdd_sample_repeat_rd_list, &fdd_low_mq_index[0], &fdd_high_mq_index[0], &fdd_low_mq_index_all[0], &fdd_high_mq_index_all[0], fdd_pval2sd_pval_list, fdd_pval2sd_sd_list, fdd_pval2sd_list_len, fds_results_file_name, fds_results_file_snv, fds_results_file_ind, fds_results_file_sv);


		  my_stop_reading(&g_single);
	      }
	      if (t == 1)
		  single_chromosome_read(&g_single, fds_a_loop);
	  }
	}
#ifdef DO_TIMING
	timers[11+ccount*2] = gettime();
#endif
	ccount++;
	if( g_internal == 1 )
	{
	  printf("after calc_ave_for_gc\n");
	}
      }

    }
    else if( g_internal == 1 )
    {
      printf("fds_chr_match %d\n", fds_chr_match);
    }

  }



  if( g_internal == 1 )
  {
    printf("after a_loop, fds_a_loop %ld\n", fds_a_loop);
  }


  if( g_troubleshooting == 1 )
  {
	  printf("before close files\n");
	  if (fds_results_file == NULL)
	  {
		  printf("results=null\n");
	  }
	  if (fds_results_file_snv == NULL)
	  {
		  printf("results snv=null\n");
	  }
	  if (fds_results_file_ind == NULL)
	  {
		  printf("results ind=null\n");
	  }
	  if (fds_results_file_sv == NULL)
	  {
		  printf("results sv=null\n");
	  }
	  if (fds_results_file_ctx == NULL)
	  {
		  printf("results ctx=null\n");
	  }
  }

  fclose(fds_results_file);
  if (fds_results_file_snv != NULL)
  {
	  fclose(fds_results_file_snv);
  }
  if (fds_results_file_ind != NULL)
  {
	  fclose(fds_results_file_ind);
  }
  if (fds_results_file_sv != NULL)
  {
	  fclose(fds_results_file_sv);
  }

  fclose(fds_results_file_ctx);

  if( g_troubleshooting == 1 )
  {
	  printf("after close files\n");
  }

  for(fdd_a_loop=0;fdd_a_loop<g_repeat_segments;fdd_a_loop++)
  {
	  free(fdd_sample_repeat_rd_list[fdd_a_loop]);
  }
  for(fdd_a_loop=0;fdd_a_loop<g_num_gc_bins;fdd_a_loop++)
  {
	  free(fdd_sample_low_mq_rd_list[fdd_a_loop]);
	  free(fdd_sample_high_mq_rd_list[fdd_a_loop]);
  }
  free(fdd_sample_repeat_rd_list);
  free(fdd_sample_high_mq_rd_list);
  free(fdd_sample_low_mq_rd_list);

  free(fdd_pval2sd_pval_list);
  free(fdd_pval2sd_sd_list);

  if (g_parallel_mode > 1) {
      for (fds_a_loop = 0; fds_a_loop < fds_num_chr; fds_a_loop++) {
	  add_one_chromosome_result(fds_a_loop, fds_bam_file->header->target_name[fds_a_loop],
				    fds_results_file_name, g_fasta_file_position[fds_a_loop+1] - g_fasta_file_position[fds_a_loop]);
      }
  }
  return;
}


void read_binom_tables(char *rbt_exec_path)
{
	FILE *rbt_binom_handle;


	char rbt_stdev_str[10], rbt_trials_str[10];

	char rbt_binom_line[100000];
	int rbt_binom_row;
	char *rbt_binom_str;
	int rbt_binom_loop;

	double rbt_p, rbt_a1, rbt_a2, rbt_a3, rbt_a4, rbt_a5;
	double rbt_x, rbt_t, rbt_erf, rbt_pvalue;
	long rbt_n, rbt_successes, rbt_k_factorial, rbt_k;
	double rbt_prob, rbt_lambda, rbt_cdf;
	double rbt_stdev, rbt_mean, rbt_num_stdevs;
	long rbt_n_minus_k, rbt_combinations;


	rbt_p = 0.3275911;
	rbt_a1 = 0.254829592;
	rbt_a2 = -0.284496736;
	rbt_a3 = 1.421413741;
	rbt_a4 = -1.453152027;
	rbt_a5 = 1.061405429;

	for(rbt_binom_row=0;rbt_binom_row<g_max_trials+1;rbt_binom_row++)
	{
	  for(rbt_binom_loop=0;rbt_binom_loop<g_max_trials+1;rbt_binom_loop++)
	  {
	    g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop] = (float) 0;
	    g_mq_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop] = (float) 0;
	  }
	}


	strcpy(g_hez_prob_binom_cdf_file, configdir);
	strcat(g_hez_prob_binom_cdf_file, "/");
	strcat(g_hez_prob_binom_cdf_file, g_hez_prob_binom_cdf_file_base);



	sprintf(rbt_trials_str, "%d", g_max_trials);
	strcat(g_hez_prob_binom_cdf_file, rbt_trials_str);
	strcat(g_hez_prob_binom_cdf_file, g_binom_file_end);

	  printf("g_hez_prob_binom_cdf_file is %s\n", g_hez_prob_binom_cdf_file);

	rbt_binom_handle = fopen(g_hez_prob_binom_cdf_file, "r");


	if(rbt_binom_handle == NULL)
	{

	  printf("\nCould not open %s\n", g_hez_prob_binom_cdf_file);
	  printf("Computing hez table\n");

	  rbt_prob = 0.5;
	  for(rbt_n=1;rbt_n<g_max_trials+1;rbt_n++)
	  {
	    for(rbt_successes=0;rbt_successes<rbt_n+1;rbt_successes++)
	    {
	      if( rbt_n == rbt_n )
	      {
		if( (rbt_n >= 20 && rbt_prob <= 0.05) || (rbt_n >= 100 && rbt_n * rbt_prob <= 10) )
		{
		  rbt_lambda = rbt_n * rbt_prob;
		  rbt_cdf = 0;
		  rbt_k_factorial = 1;
		  for(rbt_k=0;rbt_k<rbt_successes;rbt_k++)
		  {
		    if( rbt_k > 1 )
		    {
		      rbt_k_factorial = rbt_k_factorial * rbt_k;
		    }
		    rbt_cdf += pow(rbt_lambda,rbt_k) * exp(-rbt_lambda) / (double) rbt_k_factorial;
		  }
		}

		else if( rbt_n * rbt_prob * (1 - rbt_prob) >= 5 && rbt_successes >= 17 )
		{
		  rbt_stdev = sqrt(rbt_n * rbt_prob * (1.0-rbt_prob));
		  rbt_mean = rbt_n * rbt_prob;
		  rbt_cdf = 0;

		  rbt_num_stdevs = (rbt_mean - rbt_successes + 0.5) / rbt_stdev;
		  if( rbt_num_stdevs >= 0 )
		  {
		    rbt_x = rbt_num_stdevs / sqrt_2;
		    rbt_t = 1.0/(1.0 + rbt_p*rbt_x);
		    rbt_erf = 1.0 - (rbt_a1*rbt_t + rbt_a2*pow(rbt_t,2) + rbt_a3*pow(rbt_t,3) + rbt_a4*pow(rbt_t,4) + rbt_a5*pow(rbt_t,5)) * exp(-pow(rbt_x,2));
		    rbt_pvalue = (1.0 - rbt_erf)/2.0;
		  }
		  else
		  {
		    rbt_x = rbt_num_stdevs / sqrt_2;
		    rbt_t = 1.0/(1.0 + rbt_p*rbt_x);
		    rbt_erf = 1.0 - (rbt_a1*rbt_t + rbt_a2*pow(rbt_t,2) + rbt_a3*pow(rbt_t,3) + rbt_a4*pow(rbt_t,4) + rbt_a5*pow(rbt_t,5)) * exp(-pow(rbt_x,2));
		    rbt_pvalue = 1 - (rbt_erf + (1.0 - rbt_erf)/2.0);
		  }
		  rbt_cdf = rbt_pvalue;
		}
		else
		{
		  rbt_cdf = 0;
		  rbt_n_minus_k = rbt_n;
		  rbt_combinations = 1;
		  for(rbt_k=0;rbt_k<rbt_successes;rbt_k++)
		  {
		    rbt_cdf += rbt_combinations * pow(rbt_prob,rbt_k) * pow((1-rbt_prob),rbt_n_minus_k);
		    if( rbt_k > 0 )
		    {
		      rbt_combinations = (rbt_combinations / (rbt_k+1.0)) * rbt_n_minus_k;
		    }
		    else
		    {
		      rbt_combinations = rbt_combinations * rbt_n_minus_k;
		    }
		    rbt_n_minus_k -= 1;
		  }
		}
		if( rbt_cdf < 0 )
		{
		  rbt_cdf = 0;
		}
		if( rbt_cdf > 1 )
		{
		  rbt_cdf = 1;
		}
		g_hez_prob_binom_cdf_table[rbt_n][rbt_successes] = (1.0 - rbt_cdf);
		if( g_internal == 1 )
		{
		  printf("rbt_n, rbt_successes, g_hez_prob_binom_cdf_table[rbt_n][rbt_successes] %ld %ld %e\n", rbt_n, rbt_successes, g_hez_prob_binom_cdf_table[rbt_n][rbt_successes]);
		}
	      }
	    }
	  }

	  for(rbt_binom_row=0;rbt_binom_row<g_max_trials;rbt_binom_row++)
	  {
	    for(rbt_binom_loop=0;rbt_binom_loop<g_max_trials;rbt_binom_loop++)
	    {
	      g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop] = 1.0 - g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop+1];
	      if( g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop] < 0 )
	      {
		g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop] = 0;
	      }
	      if( rbt_binom_loop > 0 && g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop-1] == 1 )
	      {
		g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop] = 1;
	      }
	    }
	    g_hez_prob_binom_cdf_table[rbt_binom_row][g_max_trials] = 1.0;
	  }



	  if( g_hez_prob_binom_cdf_file != NULL )
	  {
	    rbt_binom_handle = fopen(g_hez_prob_binom_cdf_file, "w");

	    if(rbt_binom_handle == NULL)
	    {
		    printf("\nCould not open %s for writing\n", g_hez_prob_binom_cdf_file);
	    }
	    else
	    {
	      for(rbt_binom_row=0;rbt_binom_row<g_max_trials+1;rbt_binom_row++)
	      {
		for(rbt_binom_loop=0;rbt_binom_loop<g_max_trials+1;rbt_binom_loop++)
		{
		  fprintf(rbt_binom_handle, "%e", g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop]);
		  if( rbt_binom_loop < g_max_trials )
		  {
		    fprintf(rbt_binom_handle, "\t");
		  }
		}
		fprintf(rbt_binom_handle, "\n");
	      }
	      fclose(rbt_binom_handle);
	    }
	  }

	}
	else
	{

	  rbt_binom_row = 0;
	  while( fgets(rbt_binom_line, sizeof(rbt_binom_line), rbt_binom_handle) )
	  {
	    rbt_binom_str = strtok(rbt_binom_line, g_binom_separator);
	    g_hez_prob_binom_cdf_table[rbt_binom_row][0] = atof(rbt_binom_str);
	    for(rbt_binom_loop=0;rbt_binom_loop<g_max_trials;rbt_binom_loop++)
	    {
	      rbt_binom_str = strtok(NULL, g_binom_separator);
	      g_hez_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop+1] = atof(rbt_binom_str);
	    }
	    rbt_binom_row += 1;

	  }
	  fclose(rbt_binom_handle);
	}

	strcpy(g_mq_prob_binom_cdf_file, configdir);
	strcat(g_mq_prob_binom_cdf_file, "/");
	strcat(g_mq_prob_binom_cdf_file, g_mq_prob_binom_cdf_file_base);

	if( g_min_mapq >= 0 )
	{
	  sprintf(rbt_stdev_str, "%d", g_min_mapq);
	}
	else
	{
	  sprintf(rbt_stdev_str, "%d", 0);
	}

	strcat(g_mq_prob_binom_cdf_file, rbt_stdev_str);
	strcat(g_mq_prob_binom_cdf_file, "_");
	sprintf(rbt_trials_str, "%d", g_max_trials);
	strcat(g_mq_prob_binom_cdf_file, rbt_trials_str);
	strcat(g_mq_prob_binom_cdf_file, g_binom_file_end);
	if( g_internal == 1 )
	{
	  printf("g_mq_prob_binom_cdf_file is %s\n", g_mq_prob_binom_cdf_file);
	}
	rbt_binom_handle = fopen(g_mq_prob_binom_cdf_file, "r");

	if(rbt_binom_handle == NULL)
	{
	  printf("\nCould not open %s\n", g_mq_prob_binom_cdf_file);


	  rbt_prob = g_mq_prob;
	  for(rbt_n=1;rbt_n<g_max_trials+1;rbt_n++)
	  {
	    for(rbt_successes=0;rbt_successes<rbt_n+1;rbt_successes++)
	    {
	      if( (rbt_successes > 0 && g_mq_prob_binom_cdf_table[rbt_n][rbt_successes-1] == 0) || (rbt_successes > 1 && g_mq_prob_binom_cdf_table[rbt_n][rbt_successes-1] == g_mq_prob_binom_cdf_table[rbt_n][rbt_successes-2]) )
	      {
		g_mq_prob_binom_cdf_table[rbt_n][rbt_successes] = 0;
	      }
	      else
	      {
		if( (rbt_n >= 20 && rbt_prob <= 0.05) || (rbt_n >= 100 && rbt_n * rbt_prob <= 10) )
		{
		  rbt_lambda = rbt_n * rbt_prob;
		  rbt_cdf = 0;
		  rbt_k_factorial = 1;
		  for(rbt_k=0;rbt_k<rbt_successes;rbt_k++)
		  {
		    if( rbt_k > 1 )
		    {
		      rbt_k_factorial = rbt_k_factorial * rbt_k;
		    }
		    rbt_cdf += pow(rbt_lambda,rbt_k) * exp(-rbt_lambda) / (double) rbt_k_factorial;
		  }
		}
		else if( rbt_n * rbt_prob * (1 - rbt_prob) >= 5 && rbt_successes >= 20 )
		{
		  rbt_stdev = sqrt(rbt_n * rbt_prob * (1.0-rbt_prob));
		  rbt_mean = rbt_n * rbt_prob;
		  rbt_cdf = 0;

		  rbt_num_stdevs = (rbt_mean - rbt_successes + 0.5) / rbt_stdev;
		  if( rbt_num_stdevs >= 0 )
		  {
		    rbt_x = rbt_num_stdevs / sqrt_2;
		    rbt_t = 1.0/(1.0 + rbt_p*rbt_x);
		    rbt_erf = 1.0 - (rbt_a1*rbt_t + rbt_a2*pow(rbt_t,2) + rbt_a3*pow(rbt_t,3) + rbt_a4*pow(rbt_t,4) + rbt_a5*pow(rbt_t,5)) * exp(-pow(rbt_x,2));
		    rbt_pvalue = (1.0 - rbt_erf)/2.0;
		  }
		  else
		  {
		    rbt_x = rbt_num_stdevs / sqrt_2;
		    rbt_t = 1.0/(1.0 + rbt_p*rbt_x);
		    rbt_erf = 1.0 - (rbt_a1*rbt_t + rbt_a2*pow(rbt_t,2) + rbt_a3*pow(rbt_t,3) + rbt_a4*pow(rbt_t,4) + rbt_a5*pow(rbt_t,5)) * exp(-pow(rbt_x,2));
		    rbt_pvalue = 1 - (rbt_erf + (1.0 - rbt_erf)/2.0);
		  }
		  rbt_cdf = rbt_pvalue;
		}
		else
		{
		  rbt_cdf = 0;
		  rbt_n_minus_k = rbt_n;
		  rbt_combinations = 1;
		  for(rbt_k=0;rbt_k<rbt_successes;rbt_k++)
		  {
		    rbt_cdf += rbt_combinations * pow(rbt_prob,rbt_k) * pow((1-rbt_prob),rbt_n_minus_k);
		    if( rbt_k > 0 )
		    {
		      rbt_combinations = (rbt_combinations / (rbt_k+1.0)) * rbt_n_minus_k;
		    }
		    else
		    {
		      rbt_combinations = rbt_combinations * rbt_n_minus_k;
		    }
		    rbt_n_minus_k -= 1;
		  }
		}
		if( rbt_cdf < 0 )
		{
		  rbt_cdf = 0;
		}
		if( rbt_cdf > 1 )
		{
		  rbt_cdf = 1;
		}
		g_mq_prob_binom_cdf_table[rbt_n][rbt_successes] = (1.0 - rbt_cdf);
		if( g_internal == 1 )
		{
		  printf("rbt_n, rbt_successes, g_mq_prob_binom_cdf_table[rbt_n][rbt_successes] %ld %ld %e\n", rbt_n, rbt_successes, g_mq_prob_binom_cdf_table[rbt_n][rbt_successes]);
		}
	      }
	    }
	  }




	  if( g_mq_prob_binom_cdf_file != NULL )
	  {
	    rbt_binom_handle = fopen(g_mq_prob_binom_cdf_file, "w");

	    if(rbt_binom_handle == NULL)
	    {
		    printf("\nCould not open %s for writing\n", g_mq_prob_binom_cdf_file);
	    }
	    else
	    {
	      for(rbt_binom_row=0;rbt_binom_row<g_max_trials+1;rbt_binom_row++)
	      {
		for(rbt_binom_loop=0;rbt_binom_loop<g_max_trials+1;rbt_binom_loop++)
		{
		  fprintf(rbt_binom_handle, "%e", g_mq_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop]);
		  if( rbt_binom_loop < g_max_trials )
		  {
		    fprintf(rbt_binom_handle, "\t");
		  }
		}
		fprintf(rbt_binom_handle, "\n");
	      }
	      fclose(rbt_binom_handle);
	    }
	  }

	}
	else
	{

	  rbt_binom_row = 0;
	  while( fgets(rbt_binom_line, sizeof(rbt_binom_line), rbt_binom_handle) )
	  {
	    rbt_binom_str = strtok(rbt_binom_line, g_binom_separator);
	    g_mq_prob_binom_cdf_table[rbt_binom_row][0] = atof(rbt_binom_str);
	    for(rbt_binom_loop=0;rbt_binom_loop<g_max_trials;rbt_binom_loop++)
	    {
	      rbt_binom_str = strtok(NULL, g_binom_separator);
	      g_mq_prob_binom_cdf_table[rbt_binom_row][rbt_binom_loop+1] = atof(rbt_binom_str);
	    }
	    rbt_binom_row += 1;

	  }
	  fclose(rbt_binom_handle);
	}


	return;
}


void calculate_normal_binom_constants()
{

	g_p = 0.3275911;
	g_a1 = 0.254829592;
	g_a2 = -0.284496736;
	g_a3 = 1.421413741;
	g_a4 = -1.453152027;
	g_a5 = 1.061405429;
	g_xc = g_insert_num_st_devs / sqrt(2);
	g_t = 1.0/(1.0 + g_p * g_xc);
	g_erf = 1.0 - (g_a1*g_t + g_a2*pow(g_t,2) + g_a3*pow(g_t,3) + g_a4*pow(g_t,4) + g_a5*pow(g_t,5)) * exp(-pow(g_xc,2));

	g_prob2 = (1.0 - g_erf)/2.0;

	g_prob = g_prob2 / 2.0;
	if( g_internal == 1 )
	{

	  printf("g_prob is: %e\n", g_prob);
	}




	g_mq_prob = pow(10,(-g_min_mapq/10.0));
	g_mq_prob_half = g_mq_prob / 2.0;

	if( g_internal == 1 )
	{
	  printf("g_mq_prob, g_mq_prob_half are %e %e\n", g_mq_prob, g_mq_prob_half);
	}


	return;
}


long bisect_left(int *bl_list, int bl_rd, long bl_start, long bl_end)
{
	int bl_index_found = 0;
	long bl_index = bl_start + (bl_end - bl_start)/2;
	long bl_low_index = bl_start;
	long bl_high_index = bl_end;
	while( bl_index_found == 0 )
	{
		if( bl_index <= bl_start )
		{
			if( bl_rd <= bl_list[bl_start] )
			{
				bl_index = bl_start;
			}
			else
			{
				bl_index = bl_start + 1;
			}
			bl_index_found = 1;
		}
		else if( bl_index >= bl_end - 1 )
		{
			if( bl_rd <= bl_list[bl_end-1] )
			{
				bl_index = bl_end - 1;
			}
			else
			{
				bl_index = bl_end;
			}
			bl_index_found = 1;
		}

		else if( bl_rd <= bl_list[bl_index] )
		{
			bl_high_index = bl_index;
			bl_index = bl_low_index + (bl_index-bl_low_index)/2;
			if( bl_high_index == bl_index )
			{
				bl_index_found = 1;
				bl_index += 1;
			}
		}

		else if( bl_rd > bl_list[bl_index] )
		{
			bl_low_index = bl_index;
			bl_index = bl_index + (bl_high_index - bl_index)/2;
			if( bl_low_index == bl_index )
			{
				bl_index_found = 1;
				bl_index += 1;
			}
		}
	}

	return bl_index;
}



long bisect_right(int *br_list, int br_rd, long br_start, long br_end)
{
	int br_index_found = 0;
	long br_index = br_start + (br_end - br_start)/2;
	long br_low_index = br_start;
	long br_high_index = br_end;
	while( br_index_found == 0 )
	{
		if( br_index <= br_start )
		{
			if( br_rd < br_list[br_start] )
			{
				br_index = br_start;
			}
			else
			{
				br_index = br_start + 1;
			}
			br_index_found = 1;
		}
		else if( br_index >= br_end - 1 )
		{
			if( br_rd < br_list[br_end-1] )
			{
				br_index = br_end - 1;
			}
			else
			{
				br_index = br_end;
			}
			br_index_found = 1;
		}

		else if( br_rd < br_list[br_index] )
		{
			br_high_index = br_index;
			br_index = br_low_index + (br_index-br_low_index)/2;
			if( br_high_index == br_index )
			{
				br_index_found = 1;
				br_index += 1;
			}
		}

		else if( br_rd >= br_list[br_index] )
		{
			br_low_index = br_index;
			br_index = br_index + (br_high_index - br_index)/2;
			if( br_low_index == br_index )
			{
				br_index_found = 1;
				br_index += 1;
			}
		}
	}

	return br_index;
}


long bisect_left_double(double *bld_list, double bld_prob, long bld_start, long bld_end)
{
	int bld_index_found = 0;
	long bld_index = bld_start + (bld_end - bld_start)/2;
	long bld_low_index = bld_start;
	long bld_high_index = bld_end;
	while( bld_index_found == 0 )
	{
		if( bld_index <= bld_start )
		{
			if( bld_prob <= bld_list[bld_start] )
			{
				bld_index = bld_start;
			}
			else
			{
				bld_index = bld_start + 1;
			}
			bld_index_found = 1;
		}
		else if( bld_index >= bld_end - 1 )
		{
			if( bld_prob <= bld_list[bld_end-1] )
			{
				bld_index = bld_end - 1;
			}
			else
			{
				bld_index = bld_end;
			}
			bld_index_found = 1;
		}

		else if( bld_prob <= bld_list[bld_index] )
		{
			bld_high_index = bld_index;
			bld_index = bld_low_index + (bld_index-bld_low_index)/2;
			if( bld_high_index == bld_index )
			{
				bld_index_found = 1;
				bld_index += 1;
			}
		}

		else if( bld_prob > bld_list[bld_index] )
		{
			bld_low_index = bld_index;
			bld_index = bld_index + (bld_high_index - bld_index)/2;
			if( bld_low_index == bld_index )
			{
				bld_index_found = 1;
				bld_index += 1;
			}
		}
	}

	return bld_index;
}


long bisect_right_double(double *brd_list, double brd_prob, long brd_start, long brd_end)
{
	int brd_index_found = 0;
	long brd_index = brd_start + (brd_end - brd_start)/2;
	long brd_low_index = brd_start;
	long brd_high_index = brd_end;
	while( brd_index_found == 0 )
	{
		if( brd_index <= brd_start )
		{
			if( brd_prob < brd_list[brd_start] )
			{
				brd_index = brd_start;
			}
			else
			{
				brd_index = brd_start + 1;
			}
			brd_index_found = 1;
		}
		else if( brd_index >= brd_end - 1 )
		{
			if( brd_prob < brd_list[brd_end-1] )
			{
				brd_index = brd_end - 1;
			}
			else
			{
				brd_index = brd_end;
			}
			brd_index_found = 1;
		}

		else if( brd_prob < brd_list[brd_index] )
		{
			brd_high_index = brd_index;
			brd_index = brd_low_index + (brd_index-brd_low_index)/2;
			if( brd_high_index == brd_index )
			{
				brd_index_found = 1;
				brd_index += 1;
			}
		}

		else if( brd_prob >= brd_list[brd_index] )
		{
			brd_low_index = brd_index;
			brd_index = brd_index + (brd_high_index - brd_index)/2;
			if( brd_low_index == brd_index )
			{
				brd_index_found = 1;
				brd_index += 1;
			}
		}
	}

	return brd_index;
}

int delete_files()
{
    int status;

	status = remove(results_file_name);

	if (g_troubleshooting)
	{
		if (status == 0)
		printf("%s file deleted successfully.\n", results_file_name);
		else
		{
			printf("Unable to delete the file\n");
			perror("The following error occurred");
		}
	}

	char ctx_name[1024];
	char copy[1024];
	char *ext;

	ext = strrchr(results_file_name, '.');
	snprintf(copy, ext - results_file_name + 1, "%s", results_file_name);

	results_file_name = copy;
	sprintf(ctx_name, "%s.%s.vcf", results_file_name, "ctx");

	status = remove(ctx_name);

	if (g_troubleshooting)
	{
		if (status == 0)
		printf("%s file deleted successfully.\n", ctx_name);
		else
		{
			printf("Unable to delete the file\n");
			perror("The following error occurred");
		}
	}

	return 0;

}

void write_config_file(char *filename)
{
	FILE *file = fopen (filename, "w");

	if (file != NULL)
	{
		fprintf(file, "%s\n", CONFIG_HEADER_COMMENT1);
		fprintf(file, "%s\n", CONFIG_HEADER_COMMENT2);
		fprintf(file, "%s\n", CONFIG_HEADER_COMMENT3);
		fprintf(file, "%s\n", CONFIG_HEADER_COMMENT4);
		fprintf(file, "%s=%d %s\n", VERBOSE_DESCRIPTION, verbose, VERBOSE_COMMENT);
		fprintf(file, "%s= %s\n", INPUT_FILE_DESCRIPTION, INPUT_FILE_COMMENT);
		fprintf(file, "%s= %s\n", REF_FILE_DESCRIPTION, REF_FILE_COMMENT);	
		fprintf(file, "%s= %s\n", RESULTS_FILE_DESCRIPTION, RESULTS_FILE_COMMENT);
		fprintf(file, "%s=%d %s\n", DUP_FILTERING_DESCRIPTION, g_rmdup, DUP_FILTERING_COMMENT);
		fprintf(file, "%s=%d %s\n", GENDER_DESCRIPTION, g_gender, GENDER_COMMENT);		
		fprintf(file, "%s=%d %s\n", PLOIDY_DESCRIPTION, g_ploidy, PLOIDY_COMMENT);
		fprintf(file, "%s=%d %s\n", PROCESSES_DESCRIPTION, g_parallel_mode, PROCESSES_COMMENT);		
		fprintf(file, "%s=%d %s\n", BASE_PHRED_QUAL_DESCRIPTION, g_min_base_qual, BASE_PHRED_QUAL_COMMENT);
		fprintf(file, "%s=%d %s\n", MAP_QUAL_THRESHOLD_DESCRIPTION, g_min_mapq, MAP_QUAL_THRESHOLD_COMMENT);
		fprintf(file, "%s=%.5f %s\n", PROB_THRESHOLD_DESCRIPTION, g_pval_threshold, PROB_THRESHOLD_COMMENT);
		fprintf(file, "%s=%.12f %s\n", PROB_THRESHOLD_INS_DESCRIPTION, g_pval_insertion, PROB_THRESHOLD_INS_COMMENT);
		fprintf(file, "%s=%.12f %s\n", PROB_THRESHOLD_CNV_DESCRIPTION, g_rd_pval_threshold, PROB_THRESHOLD_CNV_COMMENT);
		fprintf(file, "%s=%ld %s\n", WIN_SAMP_FACTOR_DESCRIPTION, g_windows_sampling_factor, WIN_SAMP_FACTOR_COMMENT);
		fprintf(file, "%s=%ld %s\n", MAX_CHR_LEN_DESCRIPTION, g_max_chr_fasta_len, MAX_CHR_LEN_COMMENT);
		fprintf(file, "%s=%ld %s\n", MIN_CHR_LEN_DESCRIPTION, g_min_chr_fasta_len, MIN_CHR_LEN_COMMENT);
		fprintf(file, "%s=%ld %s\n", MIN_REPEAT_DESCRIPTION, g_min_repeat, MIN_REPEAT_COMMENT);
		fprintf(file, "%s=%.3f %s\n", MIN_REPEAT_STDEV_DESCRIPTION, g_min_repeat_stdev, MIN_REPEAT_STDEV_COMMENT);
		fprintf(file, "%s=%d %s\n", RANK_DESCRIPTION, g_ranks_stdev, RANK_COMMENT);		
		fprintf(file, "%s=%ld %s\n", DUP_COV_THRESHOLD_DESCRIPTION, g_dup_threshold_factor, DUP_COV_THRESHOLD_COMMENT);
		fprintf(file, "%s=%d %s\n", TURN_OFF_SPLIT_READ_DESCRIPTION, g_splitread, TURN_OFF_SPLIT_READ_COMMENT);
		fprintf(file, "%s=%d %s\n", EXCESS_COV_THRESHOLD_DESCRIPTION, g_chr_rd_threshold_factor, EXCESS_COV_THRESHOLD_COMMENT);		
		fprintf(file, "%s=%ld %s\n", WINDOW_MIN_SIZE_DESCRIPTION, g_min_rd_window_len, WINDOW_MIN_SIZE_COMMENT);
		fprintf(file, "%s=%ld %s\n", WINDOW_MAX_SIZE_DESCRIPTION, g_max_rd_window_len, WINDOW_MAX_SIZE_COMMENT);
		fprintf(file, "%s=%ld %s\n", MIN_NUM_BLOCKS_DESCRIPTION, g_min_blocks, MIN_NUM_BLOCKS_COMMENT);
		fprintf(file, "%s=%ld %s\n", BLOCK_SIZE_DESCRIPTION, g_block_min, BLOCK_SIZE_COMMENT);
		fprintf(file, "%s=%.4f %s\n", MIN_SNV_RATIO_DESCRIPTION, g_min_snv_ratio, MIN_SNV_RATIO_COMMENT);
		fprintf(file, "%s=%d %s\n", MIN_READS_SUPPORT_BREAKPOINT_DESCRIPTION, g_min_disc, MIN_READS_SUPPORT_BREAKPOINT_COMMENT);		
		fprintf(file, "%s=%.4f %s\n", MIN_SV_RATIO_DESCRIPTION, g_min_sv_ratio, MIN_SV_RATIO_COMMENT);
		fprintf(file, "%s=%d %s\n", MAX_HOMOPOLYMER_THRESHOLD_DESCRIPTION, g_max_homopolymer, MAX_HOMOPOLYMER_THRESHOLD_COMMENT);	
		fprintf(file, "%s=%.4f %s\n", MIN_INDEL_RATIO_DESCRIPTION, g_min_indel_ratio, MIN_INDEL_RATIO_COMMENT);
		fprintf(file, "%s=%d %s\n", MIN_SNV_READS_SUPP_VAR_DESCRIPTION, g_min_snv, MIN_SNV_READS_SUPP_VAR_COMMENT);
		fprintf(file, "%s=%.3f %s\n", MIN_SD_DISCORDANCE_DESCRIPTION, g_insert_num_st_devs, MIN_SD_DISCORDANCE_COMMENT);
		fprintf(file, "%s=%.4f %s\n", MAX_EVIDENCE_RATIO_DESCRIPTION, g_max_evidence_ratio, MAX_EVIDENCE_RATIO_COMMENT);
		fprintf(file, "%s=%d %s\n", MAX_INS_BREAKPOINT_THRESHOLD_DESCRIPTION, g_max_ins_range, MAX_INS_BREAKPOINT_THRESHOLD_COMMENT);
		fprintf(file, "%s=%.2f %s\n", MIN_AVG_BASE_QUAL_THRESHOLD_DESCRIPTION, g_min_ave_bq, MIN_AVG_BASE_QUAL_THRESHOLD_COMMENT);
		fprintf(file, "%s=%d %s\n", MAX_UNMAPPED_GAP_SR_DESCRIPTION, g_max_split_loss, MAX_UNMAPPED_GAP_SR_COMMENT);
		fprintf(file, "%s=%d %s\n", MIN_SR_MAPPED_LEN_DESCRIPTION, g_min_sr_len, MIN_SR_MAPPED_LEN_COMMENT);
		fprintf(file, "%s=%d %s\n", IGNORE_CONTIG_CASE_DIFF_DESCRIPTION, contig_lower_case, IGNORE_CONTIG_CASE_DIFF_COMMENT);
		fprintf(file, "%s= %s\n", SAMPLE_ID_VCF_DESCRIPTION, SAMPLE_ID_VCF_COMMENT);
		fprintf(file, "%s=0 %s\n", TAB_TXT_OUTPUT_DESCRIPTION, TAB_TXT_OUTPUT_COMMENT);
	} else {
		printf("Unable to write config file\n");
		perror("The following error occurred");
	}
}

void check_config_descriptor(char *entry, char *default_descr)
{
	char *descr;
	descr = strtok(entry, DELIM);
	if (strcmp(descr, default_descr) != 0)
	{
		printf("%s line is corrupted in the config file.\n", default_descr);
		exit(1);
	}
}

void check_if_boolean(char *entry, char *default_descr)
{
	if (strcmp(entry, "0") == 0 || strcmp(entry, "1") == 0)
	{
		
	} else {
		printf("Invalid %s entry in config file.\n", default_descr);
		exit(1);
	}
}

int valid_integer_entry(char *entry)
{
	int length, i;
	int valid = 0;	
	length = strlen(entry);
	for (i=0;i<length; i++)
	{
		if (isdigit(entry[i]))
		{
			valid = 1;
		} else {
			valid = 0;
			break;
		}
	}

	return valid;	
}

struct config get_config(char *filename)
{
	struct config configstruct;
	FILE *file = fopen (filename, "r");

	if (file != NULL)
	{
		char line[MAXBUF];
		int i = 0;

		while(fgets(line, sizeof(line), file) != NULL)
		{
			char *entry;
			entry = strtok(line, " ");
			char *cfline;
			cfline = strstr((char *)entry,DELIM);
			cfline = cfline + strlen(DELIM);

			if (i == VERBOSE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, VERBOSE_DESCRIPTION);
				memcpy(configstruct.verbose,cfline,strlen(cfline));
				check_if_boolean(configstruct.verbose, VERBOSE_DESCRIPTION);
				verbose = atoi(configstruct.verbose);
				if (verbose == 1)
				{
					printf("%s=1\n", line);
				}
			} else if (i == INPUT_FILE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, INPUT_FILE_DESCRIPTION);
				memcpy(configstruct.input_filename,cfline,strlen(cfline));
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.input_filename);
				}
			} else if (i == REF_FILE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, REF_FILE_DESCRIPTION);
				memcpy(configstruct.ref_filename,cfline,strlen(cfline));
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.ref_filename);
				}
			} else if (i == RESULTS_FILE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, RESULTS_FILE_DESCRIPTION);
				memcpy(configstruct.results_filename,cfline,strlen(cfline));					
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.results_filename);
				}		
			} else if (i == DUP_FILTERING_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, DUP_FILTERING_DESCRIPTION);
				memcpy(configstruct.duplicate_filtering_on,cfline,strlen(cfline));				
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.duplicate_filtering_on);
				}
			} else if (i == GENDER_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, GENDER_DESCRIPTION);
				memcpy(configstruct.gender,cfline,strlen(cfline));							
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.gender);	
				}	
			} else if (i == PLOIDY_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, PLOIDY_DESCRIPTION);
				memcpy(configstruct.ploidy,cfline,strlen(cfline));							
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.ploidy);	
				}
			} else if (i == PROCESSES_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, PROCESSES_DESCRIPTION);
				memcpy(configstruct.processes,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.processes);
				}	
			} else if (i == BASE_PHRED_QUAL_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, BASE_PHRED_QUAL_DESCRIPTION);
				memcpy(configstruct.min_base_phred_qual,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_base_phred_qual);
				}
			} else if (i == MAP_QUAL_THRESHOLD_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MAP_QUAL_THRESHOLD_DESCRIPTION);
				memcpy(configstruct.maq_qual_threshold,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.maq_qual_threshold);	
				}
			} else if (i == PROB_THRESHOLD_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, PROB_THRESHOLD_DESCRIPTION);
				memcpy(configstruct.prob_threshold,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.prob_threshold);
				}
			} else if (i == PROB_THRESHOLD_INS_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, PROB_THRESHOLD_INS_DESCRIPTION);
				memcpy(configstruct.prob_threshold_ins,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.prob_threshold_ins);	
				}	
			} else if (i == PROB_THRESHOLD_CNV_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, PROB_THRESHOLD_CNV_DESCRIPTION);
				memcpy(configstruct.prob_threshold_cnv,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.prob_threshold_cnv);
				}
			} else if (i == WIN_SAMP_FACTOR_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, WIN_SAMP_FACTOR_DESCRIPTION);
				memcpy(configstruct.win_samp_factor,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.win_samp_factor);	
				}	
			} else if (i == MAX_CHR_LEN_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MAX_CHR_LEN_DESCRIPTION);
				memcpy(configstruct.max_chr_len,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.max_chr_len);
				}
			} else if (i == MIN_CHR_LEN_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_CHR_LEN_DESCRIPTION);
				memcpy(configstruct.min_chr_len,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_chr_len);
				}
			} else if (i == MIN_REPEAT_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_REPEAT_DESCRIPTION);
				memcpy(configstruct.min_rep,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_rep);
				}
			} else if (i == MIN_REPEAT_STDEV_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_REPEAT_STDEV_DESCRIPTION);
				memcpy(configstruct.min_rep_stdev,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_rep_stdev);	
				}
			} else if (i == RANK_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, RANK_DESCRIPTION);
				memcpy(configstruct.rank,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.rank);	
				}
			} else if (i == DUP_COV_THRESHOLD_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, DUP_COV_THRESHOLD_DESCRIPTION);
				memcpy(configstruct.dup_cov_threshold,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.dup_cov_threshold);
				}
			} else if (i == TURN_OFF_SPLIT_READ_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, TURN_OFF_SPLIT_READ_DESCRIPTION);
				memcpy(configstruct.turn_off_split_read,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.turn_off_split_read);
				}				
			} else if (i == EXCESS_COV_THRESHOLD_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, EXCESS_COV_THRESHOLD_DESCRIPTION);
				memcpy(configstruct.exc_cov_threshold,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.exc_cov_threshold);
				}	
			} else if (i == WINDOW_MIN_SIZE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, WINDOW_MIN_SIZE_DESCRIPTION);
				memcpy(configstruct.win_min_size,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.win_min_size);
				}
			} else if (i == WINDOW_MAX_SIZE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, WINDOW_MAX_SIZE_DESCRIPTION);
				memcpy(configstruct.win_max_size,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.win_max_size);
				}
			} else if (i == MIN_NUM_BLOCKS_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_NUM_BLOCKS_DESCRIPTION);
				memcpy(configstruct.min_num_blocks,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_num_blocks);
				}
			} else if (i == BLOCK_SIZE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, BLOCK_SIZE_DESCRIPTION);
				memcpy(configstruct.block_size,cfline,strlen(cfline));									
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.block_size);
				}
			} else if (i == MIN_SNV_RATIO_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_SNV_RATIO_DESCRIPTION);
				memcpy(configstruct.min_snv_ratio,cfline,strlen(cfline));							
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_snv_ratio);	
				}
			} else if (i == MIN_READS_SUPPORT_BREAKPOINT_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_READS_SUPPORT_BREAKPOINT_DESCRIPTION);
				memcpy(configstruct.min_reads_supp_breakpoint,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_reads_supp_breakpoint);
				}			
			} else if (i == MIN_SV_RATIO_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_SV_RATIO_DESCRIPTION);
				memcpy(configstruct.min_sv_ratio,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_sv_ratio);
				}					
			} else if (i == MAX_HOMOPOLYMER_THRESHOLD_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MAX_HOMOPOLYMER_THRESHOLD_DESCRIPTION);
				memcpy(configstruct.max_hp_threshold,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.max_hp_threshold);
				}					
			} else if (i == MIN_INDEL_RATIO_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_INDEL_RATIO_DESCRIPTION);
				memcpy(configstruct.min_indel_ratio,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_indel_ratio);
				}					
			} else if (i == MIN_SNV_READS_SUPP_VAR_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_SNV_READS_SUPP_VAR_DESCRIPTION);
				memcpy(configstruct.min_snv_reads_supp_var,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_snv_reads_supp_var);
				}					
			} else if (i == MIN_SD_DISCORDANCE_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_SD_DISCORDANCE_DESCRIPTION);
				memcpy(configstruct.min_sd_discordance,cfline,strlen(cfline));							
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_sd_discordance);	
				}					
			} else if (i == MAX_EVIDENCE_RATIO_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MAX_EVIDENCE_RATIO_DESCRIPTION);
				memcpy(configstruct.max_evidence_ratio,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.max_evidence_ratio);
				}					
			} else if (i == MAX_INS_BREAKPOINT_THRESHOLD_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MAX_INS_BREAKPOINT_THRESHOLD_DESCRIPTION);
				memcpy(configstruct.max_ins_breakpoint_thr,cfline,strlen(cfline));							
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.max_ins_breakpoint_thr);	
				}
			} else if (i == MIN_AVG_BASE_QUAL_THRESHOLD_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_AVG_BASE_QUAL_THRESHOLD_DESCRIPTION);
				memcpy(configstruct.min_avg_base_qual_thr,cfline,strlen(cfline));							
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_avg_base_qual_thr);	
				}
			} else if (i == MAX_UNMAPPED_GAP_SR_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MAX_UNMAPPED_GAP_SR_DESCRIPTION);
				memcpy(configstruct.max_unmapped_gap_sr,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.max_unmapped_gap_sr);
				}					
			} else if (i == MIN_SR_MAPPED_LEN_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, MIN_SR_MAPPED_LEN_DESCRIPTION);
				memcpy(configstruct.min_sr_mapped_len,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.min_sr_mapped_len);
				}					
			} else if (i == IGNORE_CONTIG_CASE_DIFF_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, IGNORE_CONTIG_CASE_DIFF_DESCRIPTION);
				memcpy(configstruct.ignore_contig_case_diff,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.ignore_contig_case_diff);
				}					
			} else if (i == SAMPLE_ID_VCF_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, SAMPLE_ID_VCF_DESCRIPTION);
				memcpy(configstruct.sample_id_vcf,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.sample_id_vcf);
				}					
			} else if (i == TAB_TXT_OUTPUT_LINE + CONFIG_HEADER_LENGTH){
				check_config_descriptor(entry, TAB_TXT_OUTPUT_DESCRIPTION);
				memcpy(configstruct.tab_txt_output,cfline,strlen(cfline));								
				if (verbose == 1)
				{
					printf("%s=", line);
					printf("%s\n",configstruct.tab_txt_output);
				}					
			}     
			
			i++;
		}
		fclose(file);
	} else {
		printf("%s\n", CONFIG_NOT_FOUND_MESSAGE);
		write_config_file(filename);
		exit(1);
	}

	return configstruct;

}

int main(int argc, char *argv[])
{

	FILE *fasta_handle;
	FILE *results_file_ctx_handle;
	
	char *bam_file_name = NULL;
	char *fasta_file_name = NULL;	

	configdir = getenv("HOME");

	if (configdir != NULL) {
		printf("Home dir in enviroment=");
		printf("%s\n", configdir);
	}

	strcpy(configname, "/gromconfig");
	strcat(configdir, configname);

	strcpy(config_file_path, configdir);
	strcat(config_file_path, "/");
	strcat(config_file_path, CONFIG_FILENAME);
	printf("config_path=%s\n", config_file_path);


	DIR *dirptr;

    if (access (configdir, F_OK) != -1) {
        if ((dirptr = opendir(configdir)) != NULL) {
            closedir (dirptr);
			printf("config directory %s exists\n", configdir);
			struct config configstruct;

			configstruct = get_config(config_file_path);

			if (strlen(configstruct.input_filename) > 0)
			{					
				bam_file_name = configstruct.input_filename;
			}
			if (strlen(configstruct.ref_filename) > 0)
			{
				fasta_file_name = configstruct.ref_filename;
			}
			if (strlen(configstruct.results_filename) > 0)
			{
				results_file_name = configstruct.results_filename;
			}	
			check_if_boolean(configstruct.duplicate_filtering_on, DUP_FILTERING_DESCRIPTION);
			g_rmdup = atoi(configstruct.duplicate_filtering_on);
			check_if_boolean(configstruct.gender, GENDER_DESCRIPTION);
			g_gender = atoi(configstruct.gender);
			if (valid_integer_entry(configstruct.ploidy) == 1)
			{
				g_ploidy = atoi(configstruct.ploidy);
			} else {
				printf("Invalid %s entry in config file.\n", PLOIDY_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.processes) == 1)
			{
				g_parallel_mode = atoi(configstruct.processes);
			} else {
				printf("Invalid %s entry in config file.\n", PROCESSES_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.min_base_phred_qual) == 1)
			{
				g_min_base_qual = atoi(configstruct.min_base_phred_qual);
			} else {
				printf("Invalid %s entry in config file.\n", BASE_PHRED_QUAL_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.maq_qual_threshold) == 1)
			{
				g_min_mapq = atoi(configstruct.maq_qual_threshold);
			} else {
				printf("Invalid %s entry in config file.\n", MAP_QUAL_THRESHOLD_DESCRIPTION);
				exit(1);
			}
			

			if (strlen(configstruct.prob_threshold) > 0)
			{
				char *err;
				double pr_thr = strtod(configstruct.prob_threshold, &err);
				if (*err == 0) 
				{
					g_pval_threshold = pr_thr;
				} else {
					printf("Invalid %s entry in config file.\n", PROB_THRESHOLD_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", PROB_THRESHOLD_DESCRIPTION);
				exit(1);
			}	
			if (strlen(configstruct.prob_threshold_ins) > 0)
			{
				char *err;
				double pr_thr_ins = strtod(configstruct.prob_threshold_ins, &err);
				if (*err == 0) 
				{
					g_pval_insertion = pr_thr_ins;
				} else {
					printf("Invalid %s entry in config file.\n", PROB_THRESHOLD_INS_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", PROB_THRESHOLD_INS_DESCRIPTION);
				exit(1);
			}	
			if (strlen(configstruct.prob_threshold_cnv) > 0)
			{
				char *err;
				double pr_thr_cnv = strtod(configstruct.prob_threshold_cnv, &err);
				if (*err == 0) 
				{
					g_rd_pval_threshold = pr_thr_cnv;
				} else {
					printf("Invalid %s entry in config file.\n", PROB_THRESHOLD_CNV_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", PROB_THRESHOLD_CNV_DESCRIPTION);
				exit(1);
			}	
			if (valid_integer_entry(configstruct.win_samp_factor) == 1)
			{
				g_windows_sampling_factor = atoi(configstruct.win_samp_factor);
			} else {
				printf("Invalid %s entry in config file.\n", WIN_SAMP_FACTOR_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.max_chr_len) == 1)
			{
				g_max_chr_fasta_len = atoi(configstruct.max_chr_len);
			} else {
				printf("Invalid %s entry in config file.\n", MAX_CHR_LEN_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.min_chr_len) == 1)
			{
				g_min_chr_fasta_len = atoi(configstruct.min_chr_len);
			} else {
				printf("Invalid %s entry in config file.\n", MIN_CHR_LEN_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.min_rep) == 1)
			{
				g_min_repeat = atoi(configstruct.min_rep);
			} else {
				printf("Invalid %s entry in config file.\n", MIN_REPEAT_DESCRIPTION);
				exit(1);
			}
			if (strlen(configstruct.min_rep_stdev) > 0)
			{
				char *err;
				double min_rep_sd = strtod(configstruct.min_rep_stdev, &err);
				if (*err == 0) 
				{
					g_min_repeat_stdev = min_rep_sd;
				} else {
					printf("Invalid %s entry in config file.\n", MIN_REPEAT_STDEV_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", MIN_REPEAT_STDEV_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.rank) == 1)
			{
				g_ranks_stdev = atoi(configstruct.rank);
			} else {
				printf("Invalid %s entry in config file.\n", RANK_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.dup_cov_threshold) == 1)
			{
				g_dup_threshold_factor = atoi(configstruct.dup_cov_threshold);
			} else {
				printf("Invalid %s entry in config file.\n", DUP_COV_THRESHOLD_DESCRIPTION);
				exit(1);
			}
			check_if_boolean(configstruct.turn_off_split_read, TURN_OFF_SPLIT_READ_DESCRIPTION);
			g_splitread = atoi(configstruct.turn_off_split_read);
			
			if (valid_integer_entry(configstruct.exc_cov_threshold) == 1)
			{
				g_chr_rd_threshold_factor = atoi(configstruct.exc_cov_threshold);
			} else {
				printf("Invalid %s entry in config file.\n", EXCESS_COV_THRESHOLD_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.win_min_size) == 1)
			{
				g_min_rd_window_len = atoi(configstruct.win_min_size);
			} else {
				printf("Invalid %s entry in config file.\n", WINDOW_MIN_SIZE_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.win_max_size) == 1)
			{
				g_max_rd_window_len = atoi(configstruct.win_max_size);
			} else {
				printf("Invalid %s entry in config file.\n", WINDOW_MAX_SIZE_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.min_num_blocks) == 1)
			{
				g_min_blocks = atoi(configstruct.min_num_blocks);
			} else {
				printf("Invalid %s entry in config file.\n", MIN_NUM_BLOCKS_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.block_size) == 1)
			{
				g_block_min = atoi(configstruct.block_size);
			} else {
				printf("Invalid %s entry in config file.\n", BLOCK_SIZE_DESCRIPTION);
				exit(1);
			}
			if (strlen(configstruct.min_snv_ratio) > 0)
			{
				char *err;
				double min_snv_r = strtod(configstruct.min_snv_ratio, &err);
				if (*err == 0) 
				{
					g_min_snv_ratio = min_snv_r;
				} else {
					printf("Invalid %s entry in config file.\n", MIN_SNV_RATIO_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", MIN_SNV_RATIO_DESCRIPTION);
				exit(1);
			}	
			if (valid_integer_entry(configstruct.min_reads_supp_breakpoint) == 1)
			{
				g_min_disc = atoi(configstruct.min_reads_supp_breakpoint);
			} else {
				printf("Invalid %s entry in config file.\n", MIN_READS_SUPPORT_BREAKPOINT_DESCRIPTION);
				exit(1);
			}			
			if (strlen(configstruct.min_sv_ratio) > 0)
			{
				char *err;
				double min_sv_r = strtod(configstruct.min_sv_ratio, &err);
				if (*err == 0) 
				{
					g_min_sv_ratio = min_sv_r;
				} else {
					printf("Invalid %s entry in config file.\n", MIN_SV_RATIO_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", MIN_SV_RATIO_DESCRIPTION);
				exit(1);
			}	
			if (valid_integer_entry(configstruct.max_hp_threshold) == 1)
			{
				g_max_homopolymer = atoi(configstruct.max_hp_threshold);
			} else {
				printf("Invalid %s entry in config file.\n", MAX_HOMOPOLYMER_THRESHOLD_DESCRIPTION);
				exit(1);
			}
			if (strlen(configstruct.min_indel_ratio) > 0)
			{
				char *err;
				double min_indel_r = strtod(configstruct.min_indel_ratio, &err);
				if (*err == 0) 
				{
					g_min_indel_ratio = min_indel_r;
				} else {
					printf("Invalid %s entry in config file.\n", MIN_INDEL_RATIO_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", MIN_INDEL_RATIO_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.min_snv_reads_supp_var) == 1)
			{
				g_min_snv = atoi(configstruct.min_snv_reads_supp_var);
			} else {
				printf("Invalid %s entry in config file.\n", MIN_SNV_READS_SUPP_VAR_DESCRIPTION);
				exit(1);
			}
			if (strlen(configstruct.min_sd_discordance) > 0)
			{
				char *err;
				double min_sd_disc = strtod(configstruct.min_sd_discordance, &err);
				if (*err == 0) 
				{
					g_insert_num_st_devs = min_sd_disc;
				} else {
					printf("Invalid %s entry in config file.\n", MIN_SD_DISCORDANCE_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", MIN_SD_DISCORDANCE_DESCRIPTION);
				exit(1);
			}
			if (strlen(configstruct.max_evidence_ratio) > 0)
			{
				char *err;
				double max_evid_r = strtod(configstruct.max_evidence_ratio, &err);
				if (*err == 0) 
				{
					g_max_evidence_ratio = max_evid_r;
				} else {
					printf("Invalid %s entry in config file.\n", MAX_EVIDENCE_RATIO_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", MAX_EVIDENCE_RATIO_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.max_ins_breakpoint_thr) == 1)
			{
				g_max_ins_range = atoi(configstruct.max_ins_breakpoint_thr);
			} else {
				printf("Invalid %s entry in config file.\n", MAX_INS_BREAKPOINT_THRESHOLD_DESCRIPTION);
				exit(1);
			}
			if (strlen(configstruct.min_avg_base_qual_thr) > 0)
			{
				char *err;
				double min_avg_bq_thr = strtod(configstruct.min_avg_base_qual_thr, &err);
				if (*err == 0) 
				{
					g_min_ave_bq = min_avg_bq_thr;
				} else {
					printf("Invalid %s entry in config file.\n", MIN_AVG_BASE_QUAL_THRESHOLD_DESCRIPTION);
					exit(1);
				}
			} else {
				printf("Invalid %s entry in config file.\n", MIN_AVG_BASE_QUAL_THRESHOLD_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.max_unmapped_gap_sr) == 1)
			{
				g_max_split_loss = atoi(configstruct.max_unmapped_gap_sr);
			} else {
				printf("Invalid %s entry in config file.\n", MAX_UNMAPPED_GAP_SR_DESCRIPTION);
				exit(1);
			}
			if (valid_integer_entry(configstruct.min_sr_mapped_len) == 1)
			{
				g_min_sr_len = atoi(configstruct.min_sr_mapped_len);
			} else {
				printf("Invalid %s entry in config file.\n", MIN_SR_MAPPED_LEN_DESCRIPTION);
				exit(1);
			}
			check_if_boolean(configstruct.ignore_contig_case_diff, IGNORE_CONTIG_CASE_DIFF_DESCRIPTION);
			contig_lower_case = atoi(configstruct.ignore_contig_case_diff);
			if (strlen(configstruct.sample_id_vcf) > 0)
			{
				sample_id = configstruct.sample_id_vcf;
				sample_id_cmd_arg_present = 1;
			}	
			check_if_boolean(configstruct.tab_txt_output, TAB_TXT_OUTPUT_DESCRIPTION);
			int use_tab_txt_value = atoi(configstruct.tab_txt_output);
			if (use_tab_txt_value == 1)
			{
				g_vcf = 0;
			} else {
				g_vcf = 1;
			}			
        }
    } else {

		mkdir(configdir, 0755);
		printf("%s\n", CONFIG_NOT_FOUND_MESSAGE);
		write_config_file(config_file_path);
		exit(1);
    }

	temp_chr_fasta = malloc(g_max_chr_fasta_len + 1);
	sqrt_2 = sqrt(2.0);

#ifdef DO_TIMING
	unsigned long long start_t;
	unsigned long long end_t;
#endif
	long m_a_loop;
	int m_b_loop, m_c_loop;
	int m_temp_chr, m_max_chr;
	long m_max_chr_len;

	char exec_path[1024];
	
	char *tumor_sv_file_name = NULL;

	samfile_t *bam_file = NULL;

	int m_tumor_len = 1;

	g_max_distance_since_last_del_good = g_max_rd_window_len + 500;
	g_lowvar_block_start_list[0] = 0;

	strcpy(infinity_qual_entry, "Inf");
	strcpy(unknown_gt_entry, "./.");
	strcpy(heterozygous_gt_entry, "1/0");
	strcpy(homozygous_gt_entry, "1/1");

	setlinebuf(stdout);
	g_save_argc = argc;
	g_save_argv = argv;

	int opt = 0;
	while ((opt = getopt (argc, argv, "Z:W:X:Q:A:Y:D:E:K:V:U:L:F:SP:c:R:MG:O:i:r:o:p:q:s:v:g:l:d:b:n:a:C:B:y:z:e:fj:k:m:u:w:x:Ih")) != -1)
	{
 		switch (opt)
		{

			case 'S':
				g_splitread = 0;
				break;


			case 'P':
				g_parallel_mode = atoi(optarg);
				if (g_parallel_mode < 0 || g_parallel_mode > CHROMOSOMES_IN_PARALLEL)
				    g_parallel_mode = 0;
				break;
			case 'c':
				sscanf(optarg, "%d,%d,%d,%d", &g_one_chromosome, &g_sub_child_number, &g_sub_region_start, &g_sub_region_end);
				break;
			case 'R':
				g_sub_region_size = atoi(optarg) * 1000000;
				break;


			case 'G':
				g_sv_list_len = atoi(optarg);
				g_sv_list2_len = g_sv_list_len / 10;
				break;


			case 'M':
				g_rmdup = 1;
				break;

			case 'O':
				sample_id_cmd_arg_present = 1;
				sample_id = optarg;
				break;
			case 'i':
				bam_file_name = optarg;
				if (sample_id_cmd_arg_present == 0)
				{
					sample_id = basename(optarg);
					if (strchr(sample_id, '.') != NULL)
					{
						char copy[1024];
						char *ext;

						ext = strrchr(sample_id, '.');
						snprintf(copy, ext - sample_id + 1, "%s", sample_id);

						sample_id = copy;
					}
				}
				break;
			case 'r':
				fasta_file_name = optarg;
				break;
			case 'o':
				results_file_name = optarg;
				break;

			case 'Z':
				g_block_min = atol(optarg);
				break;
			case 'W':
				g_min_rd_window_len = atol(optarg);
				break;
			case 'X':
				g_max_rd_window_len = atol(optarg);
				break;
			case 'Q':
				g_rd_min_mapq = atoi(optarg);
				break;
			case 'A':
				g_windows_sampling_factor = atol(optarg);
				break;
			case 'Y':
				g_min_blocks = atol(optarg);
				break;
			case 'D':
				g_min_repeat = atol(optarg);
				break;
			case 'E':
				g_min_repeat_stdev = atof(optarg);
				break;
			case 'K':
				g_ranks_stdev = atoi(optarg);
				break;

			case 'V':
				g_rd_pval_threshold = atof(optarg);
				break;
			case 'U':
				g_chr_rd_threshold_factor = atoi(optarg);
				break;
			case 'L':
				g_dup_threshold_factor = atol(optarg);
				break;
			case 'F':
				g_mapq_factor = atof(optarg);
				break;

			case 'p':
				g_ploidy = atoi(optarg);
				break;
			case 'q':
				g_min_mapq = atoi(optarg);
				break;
			case 's':
				g_insert_num_st_devs = atof(optarg);
				break;

  

			case 'v':
				g_pval_threshold = atof(optarg);
				break;
			case 'g':
				g_gender = atoi(optarg);
				break;
			case 'l':
				g_overlap_mult = atoi(optarg);
				break;
			case 'd':
				g_min_disc = atoi(optarg);
				break;
			case 'b':
				g_min_base_qual = atoi(optarg);
				break;
			case 'n':
				g_min_snv = atoi(optarg);
				break;
			case 'a':
				g_min_snv_ratio = atof(optarg);
				break;
			case 'C':
				g_min_chr_fasta_len = atoi(optarg);
				break;
			case 'B':
				g_max_chr_fasta_len = atoi(optarg);
				break;

			case 'y':
				g_max_split_loss = atoi(optarg);
				break;
			case 'z':
				g_min_sr_len = atoi(optarg);
				break;

			case 'e':
				g_pval_insertion = atof(optarg);
				break;
			case 'f':
				g_vcf = 0;
				break;


			case 'j':
				g_min_sv_ratio = atof(optarg);
				break;
			case 'k':
				g_max_homopolymer = atoi(optarg);
				break;
			case 'm':
				g_min_indel_ratio = atof(optarg);
				break;
			case 'u':
				g_max_evidence_ratio = atof(optarg);
				break;
			case 'w':
				g_max_ins_range = atoi(optarg);
				break;
			case 'x':
				g_min_ave_bq = atof(optarg);
				break;
			case 'I':
				contig_lower_case = 1;
				break;
			case 'h':
				print_help();
				return 0;

			case '?':
				if (  optopt == 'G' || optopt == 'P' || optopt == 'C' || optopt == 'R' || optopt == 'i' || optopt == 'r' || optopt == 'o' || optopt == 'p' || optopt == 'q' || optopt == 's' || optopt == 'v' || optopt == 'g' || optopt == 'l' || optopt == 'd' || optopt == 'b' || optopt == 'n' || optopt == 'a' || optopt == 'c' || optopt == 'y' || optopt == 'z' || optopt == 'e' || optopt == 'j' || optopt == 'k' || optopt == 'm' || optopt == 'u' || optopt == 'w' || optopt == 'x' || optopt == 'O' )
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
			default:
				abort ();
		}
	}

	g_pval_threshold1 = g_pval_threshold;

	g_rd_min_mapq = g_min_mapq;
	if (g_one_chromosome >= 0)
	    g_parallel_mode = 1;
	omp_set_num_threads(g_parallel_mode ? 2 : 1);

	if (g_vcf == 1)
	{
		char *ext1 = strrchr(results_file_name, '.');

		if (ext1 == NULL)
		{
			results_file_name1 = strcat(results_file_name, ".vcf");
		} else {

			if (strcmp(ext1, ".vcf") != 0)
			{
				results_file_name1 = strcat(results_file_name, ".vcf");
			}
		}
		results_file_name1 = results_file_name;
	}

	printf("bam %s\n", bam_file_name);
	printf("ref %s\n", fasta_file_name);
	printf("results %s\n", results_file_name);


	if( bam_file_name != NULL )
	{
		bam_file = samopen(bam_file_name, "rb", 0);

		if(bam_file == NULL)
		{
			printf("\nCould not open %s\n", bam_file_name);
			return 1;
		}


		bam_index_t *bam_idx;

		bam_idx = bam_index_load(bam_file_name);
		if (bam_idx == 0)
		{
			printf("Could not open BAM indexing file\n");
			return 1;
		}
		bam_index_destroy(bam_idx);


	}
	else
	{
		printf("ERROR: No bam file specified.\n");
		return 1;
	}




	if (g_one_chromosome < 0) {
	  if( results_file_name != NULL )
	  {
		  fasta_handle = fopen(results_file_name, "w");

		  if(fasta_handle == NULL)
		  {
			  printf("\nCould not open %s\n", results_file_name);
			  return 1;
		  }
		  else
		  {
			  fclose(fasta_handle);
		  }
	  }
	  else
	  {
		  printf("ERROR: No output file specified.\n");
		  return 1;
	  }
	}





	if( fasta_file_name != NULL )
	{
		fasta_handle = fopen(fasta_file_name, "r");
		if( fasta_handle == NULL )
		{
			printf("\nCould not open %s\n", fasta_file_name);
			return 1;
		}
	}
	else
	{
		printf("ERROR: No reference file specified.\n");
		return 1;
	}


	calculate_normal_binom_constants();



	strcpy(exec_path, argv[0]);
	printf("execute path is %s\n", exec_path);


	printf("binomial path is %s\n", configdir);
	read_binom_tables(configdir);


#ifdef DO_TIMING
	timers[1] = gettime();
#endif

	if (g_parallel_mode)
	    my_bam_open(&g_single, bam_file_name);


	if( g_tumor_sv >= 0 )
	{
	  if (g_one_chromosome < 0 || load_insert_mean(bam_file_name, &g_insert_mean, &g_lseq, &g_insert_min_size, &g_insert_max_size, &g_mapped_reads) <= 0) {
	    g_insert_mean = find_insert_mean(bam_file, &g_lseq, &g_insert_min_size, &g_insert_max_size);
	    save_insert_mean(bam_file_name, g_insert_mean, g_lseq, g_insert_min_size, g_insert_max_size, g_mapped_reads);
	  }
	}
	if( g_insert_mean < g_lseq )
	{
	  g_insert_mean = g_lseq;
	}
	g_one_base_window_size = 2*g_insert_mean - 1;
	g_one_base_window_size_total = g_insert_mean;
	for(m_a_loop=0;m_a_loop<g_insert_mean-1;m_a_loop++)
	{
		g_one_base_window_size_total += 2*(m_a_loop + 1);
	}
#ifdef DO_TIMING
	timers[2] = gettime();
#endif
	printf("insert mean, insert minimum, insert maximum: %d %d %d\n", g_insert_mean, g_insert_min_size, g_insert_max_size);
	printf("median read length: %d\n", g_lseq);
	if( g_internal == 1 )
	{
	  printf("mapped reads: %ld\n", g_mapped_reads);
	}

	g_one_base_rd_len = g_overlap_mult*8*(2*g_insert_mean - 1);
	if( g_overlap_mult*8*(g_insert_max_size + 1) > g_one_base_rd_len )
	{
	  g_one_base_rd_len = g_overlap_mult*8*(g_insert_max_size + 1);
	}
	g_half_one_base_rd_len = g_one_base_rd_len;
	g_34_one_base_rd_len = g_half_one_base_rd_len + g_half_one_base_rd_len/2;
	g_14_one_base_rd_len = g_34_one_base_rd_len - g_half_one_base_rd_len;
	g_one_base_rd_len = 2*g_one_base_rd_len;

	if( g_internal == 1 )
	{
	  printf("g_one_base_rd_len is %d\n", g_one_base_rd_len);
	  printf("g_overlap_mult is %d\n", g_overlap_mult);
	}

	samclose(bam_file);


	    find_genome_length(fasta_handle);


	for(m_a_loop=0;m_a_loop<g_chr_names_index;m_a_loop++)
	{
	  g_parallel_fasta_chr_index[m_a_loop] = m_a_loop;
	}

	if( g_one_chromosome < 0 )
	{
	  for(m_a_loop=0;m_a_loop<g_chr_names_index-1;m_a_loop++)
	  {
	    m_max_chr_len = g_chr_len[g_parallel_fasta_chr_index[m_a_loop]];
	    m_max_chr = m_a_loop;
	    for(m_b_loop=m_a_loop;m_b_loop<g_chr_names_index;m_b_loop++)
	    {
	      if( g_chr_len[g_parallel_fasta_chr_index[m_b_loop]] > m_max_chr_len )
	      {
		m_max_chr_len = g_chr_len[g_parallel_fasta_chr_index[m_b_loop]];
		m_max_chr = m_b_loop;
	      }
	    }
	    m_temp_chr = g_parallel_fasta_chr_index[m_a_loop];
	    g_parallel_fasta_chr_index[m_a_loop] = g_parallel_fasta_chr_index[m_max_chr];
	    g_parallel_fasta_chr_index[m_max_chr] = m_temp_chr;
	  }
	}
	for(m_a_loop=0;m_a_loop<g_chr_names_index;m_a_loop++)
	{
	  printf("%ld fasta index %d\n", m_a_loop, g_parallel_fasta_chr_index[m_a_loop]);
	}

	printf("mappable genome length: %ld\n", g_mappable_genome_length);


#ifdef DO_TIMING
	timers[3] = gettime();
#endif

#ifdef DO_TIMING
	start_t = rdtsc();
#endif
	find_disc_svs(bam_file_name, fasta_handle, results_file_name, tumor_sv_file_name, m_tumor_len, fasta_file_name);
#ifdef DO_TIMING
	end_t = rdtsc();
	timers_ss[10] += end_t - start_t;
	printf("timer 10 %lld\n", timers_ss[10]);
#endif
	if (g_parallel_mode)
	    my_bam_close(&g_single);
#ifdef DO_TIMING
	timers[4] = gettime();
#endif
	fclose(fasta_handle);

#ifdef DO_TIMING
	printf("%d-%d: GROM total runtime = %.3f   parellel = %d\n", g_one_chromosome, g_sub_child_number, gettime() - timers[0], g_parallel_mode);
	if( g_internal == 1 )
	{
	  printf("%d-%d: Init, read predictions and binom time = %.3f\n", g_one_chromosome, g_sub_child_number, timers[1]-timers[0]);
	  printf("%d-%d: find_insert_mean total time = %.3f\n", g_one_chromosome, g_sub_child_number, timers[2]-timers[1]);
	  printf("%d-%d: find_genome_length total time = %.3f\n", g_one_chromosome, g_sub_child_number, timers[3]-timers[2]);
	  printf("%d-%d: find_disc_svs total time = %.3f\n", g_one_chromosome, g_sub_child_number, timers[4]-timers[3]);
	  printf("\n");
	  printf("%d-%d: chromosome processing times : ", g_one_chromosome, g_sub_child_number);
	  for (ix = 0; ix < ccount; ix++) printf(" %.3f", timers[11+ix*2]-timers[10+ix*2]);
	}
	printf("\n");
#endif

	char results_file_name_trim_ctx[1024];

	if( g_one_chromosome < 0 )
	{
	  long m_num_chr;
	  char *m_target_name;
	  int m_bam_name_len;

	  bam_file = samopen(bam_file_name, "rb", 0);
	  if(bam_file == NULL)
	  {
	    printf("\nCould not open %s\n", bam_file_name);
	    exit(1);
	  }



	  m_num_chr = bam_file->header->n_targets;
	  char **m_bam_chr_name_list;
	  m_bam_chr_name_list = malloc(m_num_chr*sizeof(char*));
	  for(m_a_loop=0;m_a_loop<m_num_chr;m_a_loop++)
	  {
	    m_bam_chr_name_list[m_a_loop] = malloc(g_max_chr_name_len * sizeof(int));
	  }

	  for(m_a_loop=0;m_a_loop<m_num_chr;m_a_loop++)
	  {
	    m_target_name = bam_file->header->target_name[m_a_loop];
	    m_bam_name_len = strlen(m_target_name);
	    if( g_internal == 1 )
	    {
	      printf("m_a_loop, m_target_name %ld %s\n", m_a_loop, m_target_name);
	    }
	    for(m_b_loop=0;m_b_loop<m_bam_name_len;m_b_loop++)
	    {

			if (contig_lower_case == 1) {
				m_bam_chr_name_list[m_a_loop][m_b_loop] = tolower(m_target_name[m_b_loop]);
			} else {
				m_bam_chr_name_list[m_a_loop][m_b_loop] = m_target_name[m_b_loop];
			}
	    }
	    m_bam_chr_name_list[m_a_loop][m_b_loop] = '\0';
	    printf("%s\n", m_bam_chr_name_list[m_a_loop]);
	  }


	  char m_ctx_string[4] = "ctx";
	  if( strlen(results_file_name) > 4 && results_file_name[strlen(results_file_name)-4] == '.' && results_file_name[strlen(results_file_name)-3] == 'v' && results_file_name[strlen(results_file_name)-2] == 'c' && results_file_name[strlen(results_file_name)-1] == 'f' )
	  {
	    sprintf(results_file_name_trim_ctx, "%s.%s", results_file_name, m_ctx_string);
	    results_file_name_trim_ctx[strlen(results_file_name)-3] = 'c';
	    results_file_name_trim_ctx[strlen(results_file_name)-2] = 't';
	    results_file_name_trim_ctx[strlen(results_file_name)-1] = 'x';
	    results_file_name_trim_ctx[strlen(results_file_name)+1] = 'v';
	    results_file_name_trim_ctx[strlen(results_file_name)+2] = 'c';
	    results_file_name_trim_ctx[strlen(results_file_name)+3] = 'f';
	  }
	  else
	  {
	    sprintf(results_file_name_trim_ctx, "%s.%s", results_file_name, m_ctx_string);
	  }

	  results_file_ctx_handle = fopen(results_file_name_trim_ctx, "r");
	  if (results_file_ctx_handle == NULL)
	  {
	    printf("Error opening results ctx file %s\n", results_file_name_ctx);
	    exit(1);
	  }
	  if( g_internal == 1 )
	  {
	    printf("Trim ctx\n");
	  }

	  char *m_ctx_str;
	  char m_ctx_str_lower[g_max_chr_name_len];
	  char m_ctx_line[100000];
	  int m_ctx_len = 1;
	  int m_ctx_chr_len = 0;
	  while(fgets(m_ctx_line, sizeof(m_ctx_line), results_file_ctx_handle) )
	  {
	    m_ctx_len += 1;
	  }


	  int *m_ctx_type = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_chr = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_pos = malloc(m_ctx_len * sizeof(int));
	  double *m_ctx_binom_cdf = malloc(m_ctx_len * sizeof(double));
	  double *m_ctx_sv_evidence = malloc(m_ctx_len * sizeof(double));
	  int *m_ctx_rd = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_conc = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_other_len = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_mchr = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_mpos = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_read_start = malloc(m_ctx_len * sizeof(int));
	  int *m_ctx_read_end = malloc(m_ctx_len * sizeof(int));
	  double *m_ctx_hez_binom_cdf = malloc(m_ctx_len * sizeof(double));
	  int *m_ctx_mateid = malloc(m_ctx_len * sizeof(int));

	  int *m_ctx_index = malloc(m_ctx_len * sizeof(int));

	  fseek(results_file_ctx_handle, 0, SEEK_SET);

	  while(fgets(m_ctx_line, sizeof(m_ctx_line), results_file_ctx_handle) )
	  {

	      m_ctx_str = strtok(m_ctx_line, g_tumor_separator);
	      for(m_a_loop=0;m_a_loop<g_sv_types_len;m_a_loop++)
	      {
		if( strcmp(m_ctx_str, g_sv_types[m_a_loop]) == 0 )
		{
		  m_ctx_type[m_ctx_chr_len] = m_a_loop;
		  break;
		}
	      }
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      for(m_b_loop=0;m_b_loop<strlen(m_ctx_str);m_b_loop++)
	      {

			if (contig_lower_case == 1) {
				m_ctx_str_lower[m_b_loop] = tolower(m_ctx_str[m_b_loop]);
			} else {
				m_ctx_str_lower[m_b_loop] = m_ctx_str[m_b_loop];
			}
	      }
	      m_ctx_str_lower[strlen(m_ctx_str)] = '\0';
	      for(m_a_loop=0;m_a_loop<m_num_chr;m_a_loop++)
	      {
		if( strcmp(m_bam_chr_name_list[m_a_loop], m_ctx_str_lower) == 0 )
		{
		  m_ctx_chr[m_ctx_chr_len] = m_a_loop;
		  break;
		}
	      }

	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_pos[m_ctx_chr_len] = atoi(m_ctx_str) - 1;
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_binom_cdf[m_ctx_chr_len] = atof(m_ctx_str);
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_sv_evidence[m_ctx_chr_len] = atof(m_ctx_str);
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_rd[m_ctx_chr_len] = atoi(m_ctx_str);
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_conc[m_ctx_chr_len] = atoi(m_ctx_str);
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_other_len[m_ctx_chr_len] = atoi(m_ctx_str);
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_mchr[m_ctx_chr_len] = atoi(m_ctx_str);
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_mpos[m_ctx_chr_len] = atoi(m_ctx_str) - 1;
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_read_start[m_ctx_chr_len] = atoi(m_ctx_str) - 1;
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_read_end[m_ctx_chr_len] = atoi(m_ctx_str) - 1;
	      m_ctx_str = strtok(NULL, g_tumor_separator);
	      m_ctx_hez_binom_cdf[m_ctx_chr_len] = atof(m_ctx_str);

	      m_ctx_chr_len += 1;

	  }
	  fclose(results_file_ctx_handle);


	  int sort_bnd_count = 0;

	  vcf_chr_pos_data_t* vcf_bnd_data_list = malloc(sizeof(vcf_chr_pos_data_t) * g_sv_list_len);
	  if( vcf_bnd_data_list == NULL )
      {
          printf("428 NULL\n");
          exit(0);
      }



	  for(m_b_loop=0;m_b_loop<m_ctx_chr_len;m_b_loop++)
	  {

		m_ctx_index[m_b_loop] = 0;

	    m_ctx_mateid[m_b_loop] = -1;
	  }
	  for(m_b_loop=0;m_b_loop<m_ctx_chr_len;m_b_loop++)
	  {
	    for(m_c_loop=0;m_c_loop<m_ctx_chr_len;m_c_loop++)
	    {
	      if( m_ctx_chr[m_b_loop] == m_ctx_mchr[m_c_loop] && m_ctx_chr[m_c_loop] == m_ctx_mchr[m_b_loop] )
	      {
		if( abs(m_ctx_pos[m_b_loop] - abs(m_ctx_mpos[m_c_loop])) < g_insert_max_size - 2*g_lseq && abs(m_ctx_pos[m_c_loop] - abs(m_ctx_mpos[m_b_loop])) < g_insert_max_size - 2*g_lseq )
		{
		  if( ((m_ctx_type[m_b_loop] == 6 && m_ctx_mpos[m_c_loop] >= 0) || (m_ctx_type[m_b_loop] == 7 && m_ctx_mpos[m_c_loop] < 0)) && ((m_ctx_type[m_c_loop] == 6 && m_ctx_mpos[m_b_loop] >= 0) || (m_ctx_type[m_c_loop] == 7 && m_ctx_mpos[m_b_loop] < 0)) )
		  {
		    m_ctx_index[m_b_loop] = 1;
		    m_ctx_mateid[m_b_loop] = m_c_loop;
		    if( m_ctx_mpos[m_b_loop] < 0 )
		    {
		      m_ctx_mpos[m_b_loop] = -m_ctx_pos[m_c_loop];
		    }
		    else
		    {
		      m_ctx_mpos[m_b_loop] = m_ctx_pos[m_c_loop];
		    }
 		  }
		}
	      }
	    }
	  }
	  for(m_b_loop=0;m_b_loop<m_ctx_chr_len;m_b_loop++)
	  {
	    for(m_c_loop=0;m_c_loop<m_ctx_chr_len;m_c_loop++)
	    {
	      if( m_b_loop != m_c_loop && m_ctx_chr[m_b_loop] == m_ctx_chr[m_c_loop] && m_ctx_mchr[m_b_loop] == m_ctx_mchr[m_c_loop] )
	      {
		if( abs(m_ctx_pos[m_b_loop] - m_ctx_pos[m_c_loop]) < g_insert_max_size - 2*g_lseq && abs(abs(m_ctx_mpos[m_b_loop]) - abs(m_ctx_mpos[m_c_loop])) < g_insert_max_size - 2*g_lseq )
		{
		  if( m_ctx_index[m_b_loop] == 1 && m_ctx_index[m_c_loop] == 1 && (m_ctx_binom_cdf[m_b_loop] > m_ctx_binom_cdf[m_c_loop] || (m_ctx_binom_cdf[m_b_loop] == m_ctx_binom_cdf[m_c_loop] && m_b_loop > m_c_loop)) )
		  {
		    m_ctx_index[m_b_loop] = 0;
		    if( m_ctx_mateid[m_b_loop] >= 0 )
		    {
		      m_ctx_index[m_ctx_mateid[m_b_loop]] = 0;
		    }
		  }
		}
	      }
	    }
	  }


	  printf("Translocations before filter: %d\n", m_ctx_chr_len);


	  results_file_ctx_handle = fopen(results_file_name_trim_ctx, "w");
	  if (results_file_ctx_handle == NULL)
	  {
	    printf("Error opening file %s\n", results_file_name_trim_ctx);
	    exit(1);
	  }



	  if( g_vcf == 1 )
	  {
		results_file_ctx_handle = fopen(fds_results_file_name_bnd, "w");
	    if (results_file_ctx_handle == NULL)
	    {
	        printf("Error opening results bnd file %s\n", fds_results_file_name_bnd);
	        exit(1);
	    }
	    time_t t = time(NULL);
	    struct tm tm = *localtime(&t);
	    fprintf(results_file_ctx_handle, "##fileformat=VCFv4.2\n");
	    fprintf(results_file_ctx_handle, "##fileDate=%d%d%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
		fprintf(results_file_ctx_handle, "##source=%s", g_version_name);
	    fprintf(results_file_ctx_handle, "##reference=%s\n", fasta_file_name);
		long fgl_a_loop;
		for(fgl_a_loop=0;fgl_a_loop<g_chr_names_index;fgl_a_loop++)
		{
			fprintf(results_file_ctx_handle, "##CONTIG=ID=%s>\n", g_chr_names[fgl_a_loop]);
		}
	    fprintf(results_file_ctx_handle, "##ALT=<ID=BND,Description=\"Breakend replacement\">\n");
		fprintf(results_file_ctx_handle, "##INFO=<ID=SVTYPE,Number=1,Type=String,Description=\"Type of structural variant\">\n");
	    fprintf(results_file_ctx_handle, "##INFO=<ID=MATEID,Number=1,Type=String,Description=\"ID column entry of mate\">\n");
		fprintf(results_file_ctx_handle, "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n");
		fprintf(results_file_ctx_handle, "##FORMAT=<ID=SPR,Number=1,Type=Float,Description=\"Probability of start breakpoint evidence occurring by chance\">\n");
	    fprintf(results_file_ctx_handle, "##FORMAT=<ID=SEV,Number=1,Type=Integer,Description=\"Evidence supporting variant at start breakpoint\">\n");
	    fprintf(results_file_ctx_handle, "##FORMAT=<ID=SRD,Number=1,Type=Integer,Description=\"Physical read depth at start breakpoint\">\n");
	    fprintf(results_file_ctx_handle, "##FORMAT=<ID=SCO,Number=1,Type=Integer,Description=\"Concordant pairs at start breakpoint\">\n");
	    fprintf(results_file_ctx_handle, "##FORMAT=<ID=SOT,Number=1,Type=Integer,Description=\"Count of distinct SVs with evidence at start breakpoint\">\n");
	    fprintf(results_file_ctx_handle, "##FORMAT=<ID=SFR,Number=1,Type=Integer,Description=\"Position of first read supporting start breakpoint\">\n");
	    fprintf(results_file_ctx_handle, "##FORMAT=<ID=SLR,Number=1,Type=Integer,Description=\"Position of last read supporting start breakpoint\">\n");
		fprintf(results_file_ctx_handle, "##FORMAT=<ID=SHPR,Number=1,Type=Float,Description=\"Unknown\">\n");
		if (sample_id_cmd_arg_present == 0)
		{
			ctx_sample_id = basename(bam_file_name);
			if (strchr(ctx_sample_id, '.') != NULL)
			{
				char copy[1024];
				char *ext;

				ext = strrchr(ctx_sample_id, '.');
				snprintf(copy, ext - ctx_sample_id + 1, "%s", ctx_sample_id);

				ctx_sample_id = copy;
			}
			char temp_id[1024];
			strcpy(temp_id, ctx_sample_id);
			ctx_sample_id = temp_id;
			fprintf(results_file_ctx_handle, "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\t%s\n", ctx_sample_id);
		} else {
			fprintf(results_file_ctx_handle, "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\t%s\n", sample_id);
		}


	  }

	  else
	  {
	    fprintf(results_file_ctx_handle, "SV\t");  
	    fprintf(results_file_ctx_handle, "Chromosome\t");  
	    fprintf(results_file_ctx_handle, "Start\t");  
	    fprintf(results_file_ctx_handle, "ID\t");
	    fprintf(results_file_ctx_handle, "Mate ID\t");
	    fprintf(results_file_ctx_handle, "Binom Prob (Start)\t");  
	    fprintf(results_file_ctx_handle, "CTX evidence\t");  
	    fprintf(results_file_ctx_handle, "Read Depth (High MapQ)\t");  
	    fprintf(results_file_ctx_handle, "Concordant Pairs\t");  
	    fprintf(results_file_ctx_handle, "Other (Number of Non-Empty)\t");  
	    fprintf(results_file_ctx_handle, "Mate Chr\t");  
	    fprintf(results_file_ctx_handle, "Mate Pos\t");  
	    fprintf(results_file_ctx_handle, "Read Start\t");  
	    fprintf(results_file_ctx_handle, "Read End\t");  
	    fprintf(results_file_ctx_handle, "Hez binom prob");  

	    fprintf(results_file_ctx_handle, "\n");
	  }



	  int m_ctx_chr_len2 = 0;
	  for(m_b_loop=0;m_b_loop<m_ctx_chr_len;m_b_loop++)
	  {
	    if( m_ctx_index[m_b_loop] == 1 )
	    {
	      m_ctx_chr_len2 += 1;
	      if( g_vcf == 1 )
	      {
		char m_ctx_bnd_str[g_max_chr_name_len];
		if( m_ctx_type[m_b_loop] == 6 && m_ctx_mpos[m_b_loop] < 0 )
		{
		  sprintf(m_ctx_bnd_str, "%c[%s:%d[", toupper(temp_chr_fasta[m_ctx_pos[m_b_loop] - 1]), m_bam_chr_name_list[m_ctx_mchr[m_b_loop]], abs(m_ctx_mpos[m_b_loop]));
		}
		else if( m_ctx_type[m_b_loop] == 6 && m_ctx_mpos[m_b_loop] >= 0 )
		{
		  sprintf(m_ctx_bnd_str, "%c]%s:%d]", toupper(temp_chr_fasta[m_ctx_pos[m_b_loop] - 1]), m_bam_chr_name_list[m_ctx_mchr[m_b_loop]], abs(m_ctx_mpos[m_b_loop]));
		}
		else if( m_ctx_type[m_b_loop] == 7 && m_ctx_mpos[m_b_loop] < 0 )
		{
		  sprintf(m_ctx_bnd_str, "[%s:%d[%c", m_bam_chr_name_list[m_ctx_mchr[m_b_loop]], abs(m_ctx_mpos[m_b_loop]), toupper(temp_chr_fasta[m_ctx_pos[m_b_loop] - 1]));
		}
		else
		{
		  sprintf(m_ctx_bnd_str, "]%s:%d]%c", m_bam_chr_name_list[m_ctx_mchr[m_b_loop]], abs(m_ctx_mpos[m_b_loop]), toupper(temp_chr_fasta[m_ctx_pos[m_b_loop] - 1]));
		}

		if (m_ctx_binom_cdf[m_b_loop] != 0)
		{
			int qual_entry = (int) -10 * log10(m_ctx_binom_cdf[m_b_loop]);
			vcf_bnd_data_list[sort_bnd_count].chr_name = m_bam_chr_name_list[m_ctx_chr[m_b_loop]];
			vcf_bnd_data_list[sort_bnd_count].pos = m_ctx_pos[m_b_loop];
			sprintf(vcf_bnd_data_list[sort_bnd_count].vcf_data, "tbnd_%d\t%c\t%s\t%d\t.\tSVTYPE=BND;MATEID=bnd_%d\tGT:SPR:SEV:SRD:SCO:SOT:SFR:SLR:SHPR\t%s:%.2e:%.1f:%d:%d:%d:%d:%d:%.2e\n", m_b_loop, toupper(temp_chr_fasta[m_ctx_pos[m_b_loop] - 1]), m_ctx_bnd_str, qual_entry, m_ctx_mateid[m_b_loop], unknown_gt_entry, m_ctx_binom_cdf[m_b_loop], m_ctx_sv_evidence[m_b_loop], m_ctx_rd[m_b_loop], m_ctx_conc[m_b_loop], m_ctx_other_len[m_b_loop], m_ctx_read_start[m_b_loop] + 1, m_ctx_read_end[m_b_loop] + 1, m_ctx_hez_binom_cdf[m_b_loop]);
			sort_bnd_count += 1;

		} else {
			vcf_bnd_data_list[sort_bnd_count].chr_name = m_bam_chr_name_list[m_ctx_chr[m_b_loop]];
			vcf_bnd_data_list[sort_bnd_count].pos = m_ctx_pos[m_b_loop];
			sprintf(vcf_bnd_data_list[sort_bnd_count].vcf_data, "tbnd_%d\t%c\t%s\t%s\t.\tSVTYPE=BND;MATEID=bnd_%d\tGT:SPR:SEV:SRD:SCO:SOT:SFR:SLR:SHPR\t%s:%.2e:%.1f:%d:%d:%d:%d:%d:%.2e\n", m_b_loop, toupper(temp_chr_fasta[m_ctx_pos[m_b_loop] - 1]), m_ctx_bnd_str, infinity_qual_entry, m_ctx_mateid[m_b_loop], unknown_gt_entry, m_ctx_binom_cdf[m_b_loop], m_ctx_sv_evidence[m_b_loop], m_ctx_rd[m_b_loop], m_ctx_conc[m_b_loop], m_ctx_other_len[m_b_loop], m_ctx_read_start[m_b_loop] + 1, m_ctx_read_end[m_b_loop] + 1, m_ctx_hez_binom_cdf[m_b_loop]);			
			sort_bnd_count += 1;

		}
	      }
	      else
	      {
			fprintf(results_file_ctx_handle, "%s\t%s\t%d\t%d\t%d\t%e\t%.1f\t%d\t%d\t%d\t%s\t%d\t%d\t%d\t%e\n", g_sv_types[m_ctx_type[m_b_loop]], m_bam_chr_name_list[m_ctx_chr[m_b_loop]], m_ctx_pos[m_b_loop] + 1, m_b_loop, m_ctx_mateid[m_b_loop], m_ctx_binom_cdf[m_b_loop], m_ctx_sv_evidence[m_b_loop], m_ctx_rd[m_b_loop], m_ctx_conc[m_b_loop], m_ctx_other_len[m_b_loop], m_bam_chr_name_list[m_ctx_mchr[m_b_loop]], m_ctx_mpos[m_b_loop] + 1, m_ctx_read_start[m_b_loop] + 1, m_ctx_read_end[m_b_loop] + 1, m_ctx_hez_binom_cdf[m_b_loop]);
	      }
	    }
	  }
	  printf("Translocations after filter: %d\n", m_ctx_chr_len2);


	  int sort_bnd_index;
	  for (sort_bnd_index = 0; sort_bnd_index < sort_bnd_count; sort_bnd_index++)
	  {

		  fprintf(results_file_ctx_handle, "%s\t%d\t%s", vcf_bnd_data_list[sort_bnd_index].chr_name, vcf_bnd_data_list[sort_bnd_index].pos, vcf_bnd_data_list[sort_bnd_index].vcf_data);
	  }

	  free(vcf_bnd_data_list);



	  for(m_a_loop=0;m_a_loop<m_num_chr;m_a_loop++)
	  {
	    free(m_bam_chr_name_list[m_a_loop]);
	  }
	  free(m_bam_chr_name_list);

	  free(m_ctx_index);

	  free(m_ctx_mateid);
	  free(m_ctx_hez_binom_cdf);
	  free(m_ctx_read_end);
	  free(m_ctx_read_start);
	  free(m_ctx_mpos);
	  free(m_ctx_mchr);
	  free(m_ctx_other_len);
	  free(m_ctx_conc);
	  free(m_ctx_rd);
	  free(m_ctx_sv_evidence);
	  free(m_ctx_binom_cdf);
	  free(m_ctx_pos);
	  free(m_ctx_chr);
	  free(m_ctx_type);


	  free(temp_chr_fasta);

	  fclose(results_file_ctx_handle);
	  samclose(bam_file);

	  if (g_vcf == 1)
	  {
		  delete_files();
	  }

	}

	return 0;

}


