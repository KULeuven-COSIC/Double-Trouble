#include <stdio.h>

#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "mmap.h"
#include "fpga_utils.h"
#include "cache_info.h"
#include "cache_utils.h"
#include "statistics.h"
#include "eviction_hw.h"
#include "cat.h"

#define ASSERT(x) assert(x != -1)

#define MSRTOOLS_PATH "~/tools/intel/msr-tools/"

////////////////////////////////////////////////////////////////////////////////
// Reverse-Engineering Configuration

#define EXPERIMENT_DDIO       1 // Decide between DDIO (1) and DDIO+ (0)
#define ASSOCIATIVITY_TEST    1
#define DDIOP_ASSOCIATIVITY   2
#define SW_CONTENTION         1
#define USE_VICTIM            1
#define PRINT_STATISTICS      0
#define TRY_MANY_CAT_WIDTHS   0
#define MAX_D                11
#define TEST_COUNT          100 

////////////////////////////////////////////////////////////////////////////////
// Function and Shared Variable Declerations

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) < (y)) ? (y) : (x))

uint64_t shared_mem_iova;
extern volatile uint64_t *shared_mem;
extern volatile uint64_t *syncronization;
extern volatile uint64_t *syncronization_params;
extern char *evict_mem;

int setDDIO(int count);

////////////////////////////////////////////////////////////////////////////////

void attacker(int core) { 

  int seed = time (NULL);
  srand (seed);
  
  ASSERT(fpga_connect_to_accel(AFU_ACCEL_UUID));

  fpga_share_buffer(shared_mem, SHARED_MEM_SIZE, &shared_mem_iova);

  // Delete the old log files
  if(system("rm -f ./log/*.log") == -1)
    printf("Error at the deletion of old log files\n");


////////////////////////////////////////////////////////////////////////////////
// Eviction set creation

  // Setting the msr value for number of DDIO ways, after evset construction,
  // because the construction hw is designed specifically for 2 ways
  if (!setDDIO(NUMDDIOWAYS)) {
    printf("Error: Requested DDIO configuration failed!\n");
    goto error;
  }

  printf("Executing Accuracy Test...\n"); 

  // int target_index = (rand() & 0xFF)*8;
  int target_index = 0;
  printf("target_index %d\n", target_index);

  uint64_t evset_sw[EV_DESIRED];
  uint64_t evset_hw[EV_DESIRED];

  double time; 
  int fail; 
  int evs_len;
  int nb_faulty;

  evs_check_threshold(target_index);

  int flags = 0;
  flags |= FPGA_CREATE_EVS_HUGE;
  // flags |= FPGA_CREATE_EVS_SMALL;
  flags |= FPGA_CREATE_EVS_EN_TEST;
  flags |= FPGA_CREATE_EVS_EN_TEST_MULTI;
  // flags |= FPGA_CREATE_EVS_EN_TEST_ONLYFIRST;

  time      = evs_create(target_index, THRESHOLD, WAIT, 2, 1600, 2, flags);
  fail      = evs_check_error(flags, 3, EVS_NONVERB);
  evs_len   = evs_get(evset_sw, evset_hw, EVS_NONVERB);
  printf("evs_len: %u\n", evs_len);
  nb_faulty = evs_verify(target_index, evset_sw, evs_len, EVS_NONVERB);  

  printf("Timing %.2f ms\n", time);

  if (nb_faulty==0)      printf(GREEN "Success! No fault. \n"NC);
  else if (nb_faulty==1) printf(ORANGE"Good!    One fault.\n"NC);
  else                   printf(RED   "Bad!     %u faults.\n"NC, nb_faulty);

////////////////////////////////////////////////////////////////////////////////
// Create an L2 eviction set

  #define L2_EVS_LEN (L2_WAYS)

  uint64_t evset_sw_l2[L2_EVS_LEN];

  int index=0, i=0;
	
	// Expects L2 to have more sets than L1
  assert(LLC_PERIOD > L2_PERIOD);
	uint64_t mask = (LLC_PERIOD-1) ^ (L2_PERIOD-1);
	
	while (index<L2_EVS_LEN) {
		uint64_t address;
    
    address = ((uint64_t)shared_mem + ((uint64_t)&shared_mem[target_index] & (L2_PERIOD-1)) + i++*L2_PERIOD);
		
		// Check if "address" is beyond the range
		assert((address-(uint64_t)shared_mem)<SHARED_MEM_SIZE);
		
		if (((uint64_t)shared_mem & mask) != (address & mask)) {
      int j, inLLC = 0;
      for (j=0; j<LLC_WAYS; j++)
        if (inLLC==0 && evset_sw[j]==address)
          inLLC = 1;

      if (inLLC==0)
			  evset_sw_l2[index++] = address;
    }
	}

////////////////////////////////////////////////////////////////////////////////
// Test CAT


  // An array for collecting timing measurements
  uint64_t hw_read_addr;
  int t, m, it, itc, j;
  int timeHW;
  int z; // z is used in macros.h

  uint64_t TARGET = (uint64_t)&shared_mem[target_index];

  #include "macros.h"

  int wayshift = 0;
  int CONT[LLC_WAYS-1][LLC_WAYS];
  int CONT_ASS_DDIO[LLC_WAYS];
  int CONT_ASS_DDIOP[LLC_WAYS];
  int D, W=0;
  int CAT;

  int ass_DDIO; int ass_DDIOP;
  
  #if ASSOCIATIVITY_TEST == 1

  printf("\n==================\n");
  printf("Associativity test\n");
  printf("==================\n\n");

  printf("\tQuestion: What is the associativity of the DDIO/DDIO+ regions?\n\n");


  printf("  D | Ass. DDIO | Ass. DDIO+ |\n");
  printf("----|-----------|------------|\n");

  for (D = 2; D <= LLC_WAYS; D++){

    ass_DDIO = LLC_WAYS; ass_DDIOP = LLC_WAYS;

    if (!setDDIO(D)) { 
      printf("Error: Requested DDIO configuration failed!\n");
      goto error;
    }

    for (W = 2; W <= LLC_WAYS; W++){
      for (it=0; it < W; it++){ CONT_ASS_DDIO[it] = 0; }
      for (it=0; it < W; it++){ CONT_ASS_DDIOP[it] = 0; }

      for (t=0; t<TEST_COUNT; t++) {

        for (i=0; i<evs_len; i++)
          flush((void*)evset_sw[i]);

        // Write W lines in DDIO region
        for (it=0; it < W; it++){ HW_WRITE_A(evset_hw[it]); }

        // Read W lines from DDIO region and time them
        for (it=0; it < W; it++){
          HW_READ_A(evset_hw[it]); CONT_ASS_DDIO[it] += (timeHW > THRESHOLD);
        }
      }


      for (t=0; t<TEST_COUNT; t++) {
        for (i=0; i<evs_len; i++)
          flush((void*)evset_sw[i]);

        // Write W lines in DDIO+ region
        for (it=0; it < W; it++){
          HW_WRITE_A  (evset_hw[it]);
          SW_WRITE    (evset_sw[it]);
          HW_WRITE_A  (evset_hw[it]);
        }

        // Read W lines from DDIO+ region and time them
        for (it=0; it < W; it++){
          HW_READ_A(evset_hw[it]); CONT_ASS_DDIOP[it] += (timeHW > THRESHOLD);
        }
      }

      int sum_contention_DDIO = 0; int sum_contention_DDIOP = 0;
      for(i=0; i < W; i++){ sum_contention_DDIO += CONT_ASS_DDIO[i];}
      for(i=0; i < W; i++){ sum_contention_DDIOP += CONT_ASS_DDIOP[i];}
      float associativity_contention_DDIO = ((float) sum_contention_DDIO) / ((float) TEST_COUNT);
      float associativity_contention_DDIOP = (float) sum_contention_DDIOP / (float) TEST_COUNT;

      // If W is the first configuration where at least one (with margin: 0.98) of the DDIO/DDIO+ ways is consistently evicted, that means the associativity of the region under consideration is W-1
      if (associativity_contention_DDIO > 0.98 ){ ass_DDIO = MIN(ass_DDIO, W-1); }
      if (associativity_contention_DDIOP > 0.98 ){ ass_DDIOP = MIN(ass_DDIOP, W-1); }

    }

    printf(" %2u |    %2u     |     %2u     |\n", D, ass_DDIO, ass_DDIOP);

  }

  #endif

    printf("\n======================\n");
    printf("Anatomy of the LLC set\n");
    printf("======================\n\n");

    printf("\tQuestion: Which ways in the LLC set are used for the DDIO/DDIO+ regions?\n\n");

    for (D = 2; D <= MAX_D; D++){

    assert(2*D < EV_DESIRED);

    printf("\n---|------------|-----------------------------------------|\n");
    #if EXPERIMENT_DDIO == 1
      printf(" DDIO  REGION?  | Contention on es[i] (%4u measurements) |\n", TEST_COUNT);
    #else
      printf(" DDIO+ REGION?  | Contention on es[i] (%4u measurements) |\n", TEST_COUNT);
    #endif
    printf(" D |  CAT WAYS  |");
  #if EXPERIMENT_DDIO == 1 
    for (i=0; i<D; i++){
  #else
    for (i=0; i<DDIOP_ASSOCIATIVITY; i++){
  #endif
     printf(" es[%u] |", i);
    }
    printf("\n---|------------|-----------------------------------------|\n");

    if (!setDDIO(D)) {
      printf("Error: Requested DDIO configuration failed!\n");
      goto error;
    }

    #if TRY_MANY_CAT_WIDTHS == 1
    for (CAT = 2; CAT <= D; CAT++){
    #else
      #if EXPERIMENT_DDIO == 1
    for (CAT = D; CAT <= D; CAT++){
      #else
    for (CAT = DDIOP_ASSOCIATIVITY; CAT <= DDIOP_ASSOCIATIVITY; CAT++){
      #endif
    #endif

      int nb_masks = 12-CAT;

      for (j=0; j<LLC_WAYS-1; j++){
        for (i=0; i<D; i++){
          CONT[j][i] = 0;
        }
      }


      for (wayshift = 0; wayshift < nb_masks; wayshift++){

        // SET CAT MASK FOR COS 0 /////////////////////

        mask = 0;
        for (i=0; i<CAT; i++)
          mask |= (0x1<<(LLC_WAYS-1)) >> i;
        mask = mask >> wayshift;
        assert(cat_set_ways(0, mask) == EXIT_SUCCESS);
        ///////////////////////////////////////////////

        for (t=0; t<TEST_COUNT; t++) {
          
          for (i=0; i<100; i++)
            BUSY_WAIT();
          m=0;

          // For testing DDIO

          for (i=0; i<evs_len; i++)
            flush((void*)evset_sw[i]);
          //for (i=0; i<L2_EVS_LEN; i++) 
            //flush((void*)evset_sw_l2[i]); 


          /////////////////////////
          #if EXPERIMENT_DDIO == 1 // Perform experiment to determine DDIO ways
          /////////////////////////

            // PREPARE DDIO REGION ////////////////////////////////
            for (it=0; it < D; it++){
              HW_WRITE_A  (evset_hw[it]);
            }
            ///////////////////////////////////////////////////////

            // CONTENTION BY SW ///////////////////////////////////
                // Generate contention in the two current CAT ways 
            #if SW_CONTENTION == 1
              for (it=0; it<3; it++){
                #if USE_VICTIM == 1
                for (itc = 0; itc < D; itc++){
                  SW_READ     (evset_sw[D+itc]);     VICTIM_READ (evset_sw[D+itc]);
                }
                EVICT_L2    ();
                #else
                for (itc = 0; itc < D; itc++){
                  SW_READ     (evset_sw[D+itc]);
                }
                EVICT_L2    ();
                #endif
              }
            #endif
            ///////////////////////////////////////////////////////

            // MEASURE DDIO REGION ////////////////////////////////
            for (it=0; it < D; it++){
              HW_READ_A  (evset_hw[it]); CONT[wayshift][it] += (timeHW > THRESHOLD);
            }
            ///////////////////////////////////////////////////////

          //////
          #else // Perform experiment to determine DDIO+ ways
          //////

            // PREPARE DDIO+ REGION ////////////////////////////////
            for (it=0; it < DDIOP_ASSOCIATIVITY; it++){
              HW_WRITE_A  (evset_hw[it]);
              SW_WRITE    (evset_sw[it]);
              HW_WRITE_A  (evset_hw[it]);
            }
            ///////////////////////////////////////////////////////

            // CONTENTION BY SW ///////////////////////////////////
                // Generate contention in the two current CAT ways 
            #if SW_CONTENTION == 1
              for (it=0; it<3; it++){
                #if USE_VICTIM == 1
                for (itc = 0; itc < D; itc++){
                  SW_READ     (evset_sw[D+itc]);     VICTIM_READ (evset_sw[D+itc]);
                }
                EVICT_L2    ();
                #else
                for (itc = 0; itc < D; itc++){
                  SW_READ     (evset_sw[D+itc]);     VICTIM_READ (evset_sw[D+itc]);
                }
                EVICT_L2    ();
                #endif
              }
            #endif
            ///////////////////////////////////////////////////////

            // MEASURE DDIO+ REGION ///////////////////////////////
            for (it=0; it < DDIOP_ASSOCIATIVITY; it++){
              HW_READ_A  (evset_hw[it]); CONT[wayshift][it] += (timeHW > THRESHOLD);
            }
            ///////////////////////////////////////////////////////

          #endif

        }


        printf(" %u |  %2u - %2u   |", D, 11-wayshift-CAT, 10-wayshift);
        #if EXPERIMENT_DDIO == 1
          for(i=0; i < D; i++){
            printf("  %4u |", CONT[wayshift][i]);
          }
        #else
          for(i=0; i < DDIOP_ASSOCIATIVITY; i++){
            printf("  %4u |", CONT[wayshift][i]);
          }
        #endif
        printf("\n");


      }

      printf("\n");

    }

}



error:
  *syncronization = -1;
  BUSY_WAIT();
  fpga_close();

  //assert(cache_allocate(1, 0x7ff) == EXIT_SUCCESS);

  // Restore default CAT configuration /////////////////
  for (core=0; core<16; core++)
    assert(cat_set_ways(core,  0x7ff) == EXIT_SUCCESS);
  //////////////////////////////////////////////////////

  // Restore the DDIO way configuration ////////////////
  assert(setDDIO(2));
  //////////////////////////////////////////////////////
}

int setDDIO(int count) {

  int requestedConfig=0, observedConfig;

  // Calculate the hex value for the number of DDIO ways requestes

  int i;
  for (i=0; i<count; i++)
    requestedConfig |= (0x1<<(LLC_WAYS-1)) >> i;

  // Write the msr value with the requested DDIO way configuration
  char wrmsr_cmd[128];
  sprintf(wrmsr_cmd, "%s/wrmsr 0xc8b 0x%x", MSRTOOLS_PATH, requestedConfig);
  assert(system(wrmsr_cmd) == 0);

  // Read the msr value after the configuration
  char rdmsr_cmd[128];
  sprintf(rdmsr_cmd, "%s/rdmsr 0xc8b", MSRTOOLS_PATH);
  FILE *cmd = popen(rdmsr_cmd, "r");
  char result[24]={0x0};
  while (fgets(result, sizeof(result), cmd)!=NULL);
  pclose(cmd);
  observedConfig = (int)strtol(result, NULL, 16);

  // Return with a comparison of the written and read configurations

  return (observedConfig==requestedConfig);
}
