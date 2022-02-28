`include "cci_mpf_if.vh"
`include "csr_mgr.vh"
`include "afu_json_info.vh"
`include "afu_params.vh"

module app_afu (
  input  logic clk,

  // Connection toward the host.  Reset comes in here.
  cci_mpf_if.to_fiu fiu,

  // CSR connections
  app_csrs.app csrs,

  // MPF tracks outstanding requests.  These will be true as long as
  // reads or unacknowledged writes are still in flight.
  input  logic c0NotEmpty,
  input  logic c1NotEmpty
  );

  // Local reset to reduce fan-out
  logic reset = 1'b1;
  always @(posedge clk)
  begin
    reset <= fiu.reset;
  end

  // ===========================================================================
  //
  //  CSRs (simple connections to the external CSR management engine)
  //
  // ===========================================================================

                                        //  +-------> Control Status Reg Number
                                        //  |   +---> DIR - In:  CPU  -> FPGA
                                        //  |   |         - Out: FPGA -> CPU
                                        //  |   |   Explanation
                                        // ---|---|-----------------------------
  logic         op_start;               //  0 | I | Flag at CSR write
  logic [ 2:0]  op_params;              //  0 | I |
  logic         op_en_test;             //  0 | I |
  logic         op_en_test_multi;       //  0 | I |
  logic         op_en_test_only_first;  //  0 | I |
  logic         op_pagesize;            //  0 | I |
  logic [ 3:0]  op_recipe;              //  0 | I |
  logic         op_done;                //  0 | O |
  logic [63:0]  op_adr_target;          //  1 | I |
  logic [ 9:0]  op_timing;              //  1 | O |
  logic [63:0]  op_adr_search;          //  2 | I |
  logic [ 9:0]  op_threshold;           //  3 | I |
  logic [ 3:0]  op_ddio_count;          //  4 | I |
  logic [ 7:0]  op_wrap_count;          //  4 | I |
  logic [ 3:0]  op_test_count;          //  4 | I |
  logic [63:0]  op_evs_recipe_cnt;      //  4 | O |
  logic [ 7:0]  op_evs_addr;            //  5 | I |
  logic [63:0]  op_evs_data;            //  5 | O |
  logic [63:0]  op_evs_recipe;          //  6 | I |
  logic         op_evs_recipe_wen;      //  6 | I |
  logic [63:0]  op_debug0;              //  6 | O |
  logic [ 9:0]  op_wait;                //  7 | I |
  logic [63:0]  op_debug1;              //  7 | O |


  afu_regs regs (.*);

  // ===========================================================================
  //
  //  Datapath Circuits
  //
  // ===========================================================================

  // A guess address generator /////////////////////////////////////////////////
  //

  logic gen_guessaddr;
  logic [26:0] counter_addr;

  always_ff @(posedge clk)
    if (op_start)
      counter_addr <= 27'd0;
    else if(gen_guessaddr)
      counter_addr <= counter_addr + 27'd1;

  wire  [ 7:0] counter_wrap;
  assign counter_wrap = (op_pagesize) ? counter_addr[26:19] :
                                        counter_addr[21:14] ;

  // A Mux for selecting target or test address ////////////////////////////////
  //

  t_sel_memaddr sel_memaddr;
  t_ccip_clAddr address;
  t_ccip_clAddr address_guess;
  t_ccip_clAddr address_guess_r;
  t_ccip_clAddr address_first_found;
  t_ccip_clAddr address_recipe;
  t_ccip_clAddr address_recipe_r;
  
  assign address_guess = (op_pagesize) ?
    op_adr_search + {43'b0, counter_addr[14:0],        6'b00_0000} :
    op_adr_search + {43'b0, counter_addr[ 9:0], 11'b000_0000_0000} ;

  always_ff @(posedge clk) begin
    address_recipe_r <= address_recipe;
    address_guess_r  <= address_guess;
  end

  // always_ff @(posedge clk)
  always_comb
    case (sel_memaddr)
      ADDR_TARGET  : address <= op_adr_target;
      ADDR_GUESS   : address <= address_guess;
      ADDR_FRSTFND : address <= address_first_found;
      ADDR_RECIPE  : address <= address_recipe_r;
    endcase

  // ===========================================================================
  //
  //   Memory Operations
  //
  // ===========================================================================

  t_wr_commands wr_command;
  logic         wr_touch;
  logic         wr_done;

  t_rd_commands rd_command;
  logic         rd_done;

  t_ccip_clAddr memaddress;
  
  logic [9:0]   counter_memop;

  app_memops memops (.*);

  assign memaddress = address;

  // ===========================================================================
  //
  //   Main AFU logic
  //
  // ===========================================================================

  t_state state;
  t_state next_state;

  logic [9:0]  op_threshold_r;
  logic [9:0]  counter_memop_r;
  always_ff @(posedge clk) begin
    counter_memop_r <= counter_memop;
    op_threshold_r  <= op_threshold ;
  end

  logic        c_ge_t;
  logic        c_s_t;  
  always_ff @(posedge clk) begin
    c_ge_t <= (counter_memop_r >= op_threshold_r);
    c_s_t  <= (counter_memop_r <  op_threshold_r);
  end

  // A flag that goes high when a congurent address is found ///////////////////
  //
  logic flag_congruent_found;

  assign flag_congruent_found =
    (state==S_RD_TARGET) &&
    rd_done &&
    c_ge_t;

  // A flag that goes high when congurence test fails //////////////////////////
  //
  logic flag_test_failed;

  assign flag_test_failed =
    // both for single and multiple test modes
    (state==S_RDT1_TARGET || state==S_RDT3_GUESS) &&
    rd_done &&
    c_s_t;

  // A counter for test loops //////////////////////////////////////////////////
  //
  logic [3:0] counter_test;
  always_ff @(posedge clk)
    if (state == S_WRT3_GUESS && wr_done) begin
      counter_test <= counter_test + 4'd1;
      $display("counter_test <= %d", counter_test + 4'd1);
    end
    // else if (state==S_WR_GUESS || flag_test_failed) begin
    else if (gen_guessaddr || flag_test_failed) begin
      
      counter_test <= 4'd0;
      $display("counter_test <= 0");
    end
    
  // A flag that goes high when congurence test passes /////////////////////////
  //
  logic flag_test_passed;

  assign flag_test_passed = (!op_en_test_multi)
    ? ( // if single test mode
      (state==S_RDT1_TARGET) &&
      rd_done &&
      c_ge_t
    )
    : ( // if multiple test mode
      (state==S_RDT3_GUESS) &&
      rd_done &&
      (counter_test == op_test_count) &&
      c_ge_t
    );

  // A register that stays high for the duration of first congruency test //////
  //
  logic flag_first_test_passed;

  always_ff @(posedge clk)
    if (op_start) begin
      flag_first_test_passed <= 1'b0;
      $display("flag_first_test_passed is reset");
    end
    else if (flag_test_passed) begin
      flag_first_test_passed <= 1'b1;
      $display("flag_first_test_passed is set");
    end

  // A register that goes high when the first congruent address is found ///////
  //
  logic flag_first_found;

  always_ff @(posedge clk)
    if (op_start || (!flag_first_test_passed && flag_test_failed)) begin
      flag_first_found <= 1'b0;
      $display("flag_first_found is reset");
    end
    else if (flag_congruent_found) begin
      flag_first_found <= 1'b1;
      $display("flag_first_found is set");
    end
    
  // Trigger a new address_guess generation ////////////////////////////////////
  //

  logic state_wr_guess_reg;
  always_ff @(posedge clk)
    state_wr_guess_reg <= (state==S_WR_GUESS);

  assign gen_guessaddr = (state==S_WR_GUESS) && !state_wr_guess_reg;

  // A counter for keeping track of number of found congruent addresses ////////
  //
  logic [7:0] counter_evs;

  always_ff @(posedge clk)
    if (op_start) begin
      counter_evs <= 8'b0;
      $display("counter_evs rst %h", 8'b0);
    end
    else if (flag_congruent_found) begin
      counter_evs <= counter_evs + 8'b1;
      $display("counter_evs inc %h", counter_evs + 8'b1);
    end
    else if (flag_test_failed && flag_first_test_passed) begin
      counter_evs <= counter_evs - 8'b1;
      $display("counter_evs dec %h", counter_evs - 8'b1);
    end
    else if (flag_test_failed && !flag_first_test_passed) begin
      counter_evs <= 8'b0;
      $display("counter_evs rst %h", 8'b0);
    end

  // A flag for enabling test route ////////////////////////////////////////////
  //
  logic flag_en_test;

  assign flag_en_test =
    op_en_test & ~(op_en_test_only_first & flag_first_test_passed);

  // A counter for keeping track of number of failed tests for each address ////
  //

`ifdef DEBUG
  logic [7:0] counter_fails;

  always_ff @(posedge clk)
    if (op_start || flag_test_passed) begin
      counter_fails <= 8'b0;
      $display("counter_fails rst %h", 8'b0);
    end
    else if (flag_test_failed) begin
      counter_fails <= counter_fails + 8'b1;
      $display("counter_fails inc %h", counter_fails + 8'b1);
    end
`endif

  // A loop counter for debugging  /////////////////////////////////////////////
  //

`ifdef DEBUG
  /*
  logic [7:0] counter_loop;
  always_ff @(posedge clk)
    if (  op_start ||
        ( flag_en_test && (
                           (flag_congruent_found && !flag_first_found) ||
                            flag_test_passed
                          )
        ) ||
        ( !flag_en_test && state == S_WR_TARGET)
    ) begin
      counter_loop <= 8'd0;
      $display("counter_loop reset");
    end
    else if (state == S_RD_TARGET && next_state == S_WR_GUESS) begin
      counter_loop <= counter_loop + 8'd1;
      $display("counter_loop %d", counter_loop + 8'd1);
    end
    */
`endif

  // A counter for total execution time ////////////////////////////////////////
  //

`ifdef DEBUG
  /*
  logic [19:0] counter_total;
  always_ff @(posedge clk)
    if (op_start) begin
      counter_total <= 20'b0;
      $display("counter_total reset");
    end
    else if (state != S_INIT)
      counter_total <= counter_total + 20'd1;
  */
`endif

  // A BRAM for storing the eviction set ///////////////////////////////////////
  //

`ifdef DEBUG
  reg   [63:0] mem_ev_set [0:EV_SET_SIZE-1];
  reg   [63:0] mem_debug  [0:EV_SET_SIZE-1];

  always_ff @(posedge clk) begin

    if (flag_congruent_found) begin
      mem_ev_set[counter_evs] <= address_guess_r;
      $display("EVSET Add %h", address_guess_r);

      mem_debug[counter_evs] <= {8'b0                  , //  8-bit
                                20'b0                  , // 20-bit counter_total
                                 8'b0                  , //  8-bit counter_loop
                                 counter_fails[7:0]    , //  8-bit
                          {2'b00,counter_memop_r}      , // 12-bit
                                 counter_evs          }; //  8-bit

      $display("mem_debug[%d] <= %h",
        counter_evs, {   8'b0                 , //  8-bit
                        20'b0                 , // 20-bit counter_total
                         8'b0                 , //  8-bit counter_loop
                         counter_fails[7:0]   , //  8-bit
                {2'b00, counter_memop_r}      , // 12-bit
                         counter_evs}           //  8-bit
        );
    end
    op_evs_data <= mem_ev_set[op_evs_addr];
    op_debug0   <= {24'b0, counter_wrap, 24'b0, counter_evs};
    op_debug1   <= mem_debug[op_evs_addr];
  end

`else
  reg [63:0] mem_ev_set [0:EV_SET_SIZE-1];

  always_ff @(posedge clk) begin
    if (flag_congruent_found) begin
      mem_ev_set[counter_evs] <= address_guess_r;
      $display("EVSET Add %h", address_guess_r);
    end
    op_evs_data <= mem_ev_set[op_evs_addr];
    op_debug0   <= '0;
    op_debug1   <= '0;
  end
`endif

  // A BRAM for storing the eviction set recipes ///////////////////////////////
  //
  (* ramstyle = "logic" *) reg [63:0] mem_recipe_evs [0:EV_SET_RECIPE_SIZE-1];
  (* ramstyle = "logic" *) reg [63:0] mem_recipe     [0:EV_SET_RECIPE_SIZE-1];

  reg   [63:0] recipe_chosen;
  reg   [63:0] recipe;

  always_ff @(posedge clk) begin
    if (flag_congruent_found) begin
      mem_recipe_evs[counter_evs] <= address_guess_r;
      $display("RECIPE_EVSET Add %h @ %d", address_guess_r, counter_evs);
    end
    
    if (op_evs_recipe_wen) begin
      mem_recipe[op_evs_addr] <= op_evs_recipe;
      $display("RECIPE Add %h @ %d", op_evs_recipe, op_evs_addr);
    end

    recipe_chosen <= mem_recipe[op_recipe];
    if (op_start) begin
      recipe <= recipe_chosen;
      $display("recipe %h @ %d", recipe_chosen, op_recipe);
    end
    else if ( (state == S_RECIPE_WR && next_state == S_RECIPE_WR_WAIT) ||
              (state == S_RECIPE_RD && next_state == S_RECIPE_RD_NEXT) ) begin
      recipe <= {4'hF, recipe[63:4]};
      $display("recipe %h", {4'hF, recipe[63:4]});
    end
  end

  wire [3:0] recipe_index = recipe[3:0];
  assign address_recipe   = mem_recipe_evs[recipe_index];

  // A counter for the number of evictions of the reads of the recipe //////////
  //
  logic flag_recipe_count_en;
  assign flag_recipe_count_en =
    (state==S_RECIPE_RD) &&
    rd_done &&
    (counter_memop_r>op_threshold_r);
  
  always_ff @(posedge clk)
    // if (state==S_INIT && 
    //     (next_state==S_RECIPE_WR || next_state==S_RECIPE_RD)) 
    if (op_start) 
                                   op_evs_recipe_cnt <= 'b0;
    else if (flag_recipe_count_en) op_evs_recipe_cnt <= op_evs_recipe_cnt + 1;
  
  // A register to store timing info of test functions /////////////////////////
  //
  always_ff @(posedge clk)
    if (wr_done || rd_done)
      op_timing <= counter_memop_r;

  // A wait counter ////////////////////////////////////////////////////////////
  //
  logic flag_waited;

  logic [9:0] counter_wait;
  always_ff @(posedge clk)
    if (state==S_INIT          ||
        state==S_WR_TARGET     ||
        state==S_WR_FRSTFND    ||
        state==S_WR_FRSTFND2   ||
        state==S_WR_GUESS      ||
        state==S_WRT1_TARGET   ||
        state==S_WRT1_FRSTFND  ||
        state==S_WRT1_FRSTFND2 ||
        state==S_WRT1_GUESS    ||
        state==S_WRT3_GUESS    ||
        state==S_WRT3_FRSTFND  ||
        state==S_WRT3_FRSTFND2 ||
        state==S_WRT3_TARGET   ||
        state==S_WR_WTEST      ||
        state==S_RECIPE_WR      )
      counter_wait <= 10'b0;
    else
      counter_wait <= counter_wait + 10'b1;

  assign flag_waited = (counter_wait == op_wait);

  // A logic to control multiple first found congruent addresses ///////////////
  //
  logic [3:0] count_first_found;
  logic [3:0] count_first_found_r;
  
  assign count_first_found = op_ddio_count - 4'd1;
  always_ff @(posedge clk)
    count_first_found_r <= count_first_found;


  (* ramstyle = "logic" *) logic [63:0] mem_found [0:10];
  logic [ 3:0] mem_found_rd_index;
  logic [ 3:0] mem_found_wr_index;

  logic flag_found_wrtn;

  assign flag_found_wrtn = (mem_found_rd_index == mem_found_wr_index);

  always_ff @(posedge clk)
    if ((state==S_WR_TARGET && wr_done) || (flag_waited && flag_found_wrtn)) begin
      mem_found_rd_index <= 'd0;
      $display("mem_ff[%d] =>", 0);
    end
    else if (sel_memaddr==ADDR_FRSTFND && wr_done) begin
      mem_found_rd_index <= mem_found_rd_index + 'd1;
      $display("mem_ff[%d] =>", mem_found_rd_index + 'd1);
    end
  
  // Write a guess address to mem_found, if it is found to be congruent
  always_ff @(posedge clk)
    if (op_start) begin
      mem_found_wr_index <= 'b0;
    end
    else if (flag_congruent_found && !flag_first_found) begin
      mem_found_wr_index <= 'b1;
      mem_found[0] <= address_guess_r;
      $display("mem_ff[%d] <= %h", 0, address_guess_r);
    end
    else if ((flag_congruent_found && flag_first_found) &&
             mem_found_wr_index!=count_first_found_r) begin
      mem_found_wr_index <= mem_found_wr_index + 'd1;
      mem_found[mem_found_wr_index] <= address_guess_r;
      $display("mem_ff[%d] <= %h", mem_found_wr_index, address_guess_r);
    end
    else if (flag_test_failed && !flag_first_test_passed) begin
      mem_found_wr_index <= 'b0;
    end

  // Read indexed address from mem_found
  assign address_first_found = (rd_command == RD_NONE) ?
    t_ccip_clAddr'(mem_found[mem_found_rd_index]) :
    t_ccip_clAddr'(mem_found[0]) ;

  // A register to hold the operation //////////////////////////////////////////
  //
  logic [2:0] operation;
  always_ff @(posedge clk)
    if (op_start) operation <= op_params;

  // The (Mealy) state machine /////////////////////////////////////////////////
  //
  assign wr_touch = (operation==OP_WRITE_UPDATE);

  always_comb begin
    op_done       <= 1'b0;
    sel_memaddr   <= ADDR_TARGET;
    wr_command    <= WR_NONE;
    rd_command    <= RD_NONE;
    next_state    <= S_INIT;

    case (state)
      S_INIT: begin
        if (op_start)
          case (op_params)
            OP_CREATE_EVS   : next_state <= S_WR_TARGET ;
            OP_WRITE_NONBL  : next_state <= S_WR_TEST   ;
            OP_WRITE_BLOCK  : next_state <= S_WR_WTEST  ;
            OP_READ         : next_state <= S_RD_TEST   ;
            OP_WRITE_UPDATE : next_state <= S_RD_TEST   ;
            OP_RECIPE_WRITE : next_state <= S_RECIPE_WR ;
            OP_RECIPE_READ  : next_state <= S_RECIPE_RD ;
            default         : next_state <= S_INIT      ;
          endcase
        else                  next_state <= S_INIT      ;

        op_done       <= 1'b1;
      end

      S_WR_TARGET: begin
        next_state    <= wr_done ? S_WR_TARGET_WAIT : S_WR_TARGET;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_TARGET;
      end

      S_WR_TARGET_WAIT: begin
        case ({flag_waited, flag_first_found})
          2'b10  : next_state <= S_WR_GUESS;
          2'b11  : next_state <= S_WR_FRSTFND;
          default: next_state <= S_WR_TARGET_WAIT;
        endcase       
      end

      S_WR_FRSTFND: begin
        next_state    <= wr_done ? S_WR_FRSTFND_WAIT : S_WR_FRSTFND;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_FRSTFND;
      end

      S_WR_FRSTFND_WAIT: begin
        // next_state    <= flag_waited ? S_WR_FRSTFND2 : S_WR_FRSTFND_WAIT;
        next_state    <= (flag_waited &&  flag_found_wrtn) ? S_WR_FRSTFND2 :
                         (flag_waited && !flag_found_wrtn) ? S_WR_FRSTFND  :
                                                             S_WR_FRSTFND_WAIT;
      end

      S_WR_FRSTFND2: begin
        next_state    <= wr_done ? S_WR_FRSTFND_WAIT2 : S_WR_FRSTFND2;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_FRSTFND;
      end

      S_WR_FRSTFND_WAIT2: begin
        // next_state    <= flag_waited ? S_WR_GUESS : S_WR_FRSTFND_WAIT2;
        next_state    <= (flag_waited &&  flag_found_wrtn) ? S_WR_GUESS :
                         (flag_waited && !flag_found_wrtn) ? S_WR_FRSTFND2  :
                                                             S_WR_FRSTFND_WAIT2;
      end

      S_WR_GUESS: begin
        next_state    <= wr_done ? S_WR_GUESS_WAIT : S_WR_GUESS;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_GUESS;
      end

      S_WR_GUESS_WAIT: begin
        next_state    <= flag_waited ? S_RD_TARGET : S_WR_GUESS_WAIT;
      end

      S_RD_TARGET: begin

        if (!rd_done)
          next_state  <= S_RD_TARGET;

        // Put an upper bound on how many new addresses will be considered
        else if (counter_wrap == op_wrap_count)
          next_state  <= S_INIT;

        // Loop with a new address until finding a congruent addresses
        else if (!flag_congruent_found)
          next_state  <= S_WR_GUESS;

        else if (!flag_en_test) begin // Without test

          // Repeat the loop till finding requested # of congruent addresses
          if ((counter_evs + 8'b1) < op_evs_addr)
            next_state  <= S_WR_TARGET;

          // If enough addresses are found, end the loop
          else
            next_state  <= S_INIT;

        end
        else begin // With test

          // If all the first-founds are found, test if the address is congurent
          if (mem_found_wr_index == count_first_found)
            next_state  <= S_WRT1_TARGET;

          // Otherwise, iterate the loop to find the next ones
          else
            next_state  <= S_WR_TARGET;

        end

        rd_command    <= RD_BLOCKING;
        sel_memaddr   <= ADDR_TARGET;
      end

      //////////////////////////////////////////////////////////////////////////
      // Congurency Test 1

      S_WRT1_TARGET: begin
        next_state    <= wr_done ? S_WRT1_TARGET_WAIT : S_WRT1_TARGET;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_TARGET;
      end

      S_WRT1_TARGET_WAIT: begin
        next_state    <= flag_waited ? S_WRT1_FRSTFND : S_WRT1_TARGET_WAIT;
      end

      S_WRT1_FRSTFND: begin
        next_state    <= wr_done ? S_WRT1_FRSTFND_WAIT : S_WRT1_FRSTFND;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_FRSTFND;
      end

      S_WRT1_FRSTFND_WAIT: begin
        next_state    <= (flag_waited &&  flag_found_wrtn) ? S_WRT1_FRSTFND2 :
                         (flag_waited && !flag_found_wrtn) ? S_WRT1_FRSTFND :
                                                             S_WRT1_FRSTFND_WAIT;
      end

      S_WRT1_FRSTFND2: begin
        next_state    <= wr_done ? S_WRT1_FRSTFND_WAIT2 : S_WRT1_FRSTFND2;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_FRSTFND;
      end

      S_WRT1_FRSTFND_WAIT2: begin
        next_state    <= (flag_waited &&  flag_found_wrtn) ? S_WRT1_GUESS :
                         (flag_waited && !flag_found_wrtn) ? S_WRT1_FRSTFND2 :
                                                             S_WRT1_FRSTFND_WAIT2;
      end

      S_WRT1_GUESS: begin
        next_state    <= wr_done ? S_WRT1_GUESS_WAIT : S_WRT1_GUESS;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_GUESS;
      end

      S_WRT1_GUESS_WAIT: begin
        next_state    <= flag_waited ? S_RDT1_TARGET : S_WRT1_GUESS_WAIT;
      end

      S_RDT1_TARGET: begin
        if (!rd_done)
          next_state  <= S_RDT1_TARGET;

        // If multi test is enabled
        else if (op_en_test_multi) begin

          // If test fails, loop again
          if (flag_test_failed)
            next_state  <= S_WR_TARGET;

          // Otherwise, go for next test
          else
            next_state  <= S_WRT3_GUESS;
        end

        // Otherwise, repeat the loop till finding requested number of addresses
        else if (flag_test_failed || counter_evs < op_evs_addr)
          next_state  <= S_WR_TARGET;

        // If enough addresses are found, end the loop
        else
          next_state  <= S_INIT;

        rd_command    <= RD_BLOCKING;
        sel_memaddr   <= ADDR_TARGET;
      end

      //////////////////////////////////////////////////////////////////////////
      // Congurency Test 2

      S_WRT3_GUESS: begin
        next_state    <= wr_done ? S_WRT3_GUESS_WAIT : S_WRT3_GUESS;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_TARGET; //ADDR_GUESS;
      end

      S_WRT3_GUESS_WAIT: begin
        next_state    <= flag_waited ? S_WRT3_FRSTFND : S_WRT3_GUESS_WAIT;
      end

      S_WRT3_FRSTFND: begin
        next_state    <= wr_done ? S_WRT3_FRSTFND_WAIT : S_WRT3_FRSTFND;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_FRSTFND; //ADDR_FRSTFND;
      end

      S_WRT3_FRSTFND_WAIT: begin
        next_state    <= (flag_waited &&  flag_found_wrtn) ? S_WRT3_FRSTFND2 :
                         (flag_waited && !flag_found_wrtn) ? S_WRT3_FRSTFND :
                                                             S_WRT3_FRSTFND_WAIT;
      end

      S_WRT3_FRSTFND2: begin
        next_state    <= wr_done ? S_WRT3_FRSTFND_WAIT2 : S_WRT3_FRSTFND2;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_FRSTFND; //ADDR_FRSTFND;
      end

      S_WRT3_FRSTFND_WAIT2: begin
        next_state    <= (flag_waited &&  flag_found_wrtn) ? S_WRT3_TARGET :
                         (flag_waited && !flag_found_wrtn) ? S_WRT3_FRSTFND2 :
                                                             S_WRT3_FRSTFND_WAIT2;
      end

      S_WRT3_TARGET: begin
        next_state    <= wr_done ? S_WRT3_TARGET_WAIT : S_WRT3_TARGET;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_GUESS; //ADDR_TARGET;
      end

      S_WRT3_TARGET_WAIT: begin
        next_state    <= flag_waited ? S_RDT3_GUESS : S_WRT3_TARGET_WAIT;
      end

      S_RDT3_GUESS: begin
        if (!rd_done)
          next_state  <= S_RDT3_GUESS;

        // Repeat the loop, if test is failed
        else if (flag_test_failed)
          next_state  <= S_WR_TARGET;
        
        // Repeat test if multiple tests are requested
        else if (counter_test < op_test_count)
          next_state  <= S_WRT1_TARGET;

        // Repeat the loop, till finding requested number of congruent addresses
        else if (counter_evs < op_evs_addr)
          next_state  <= S_WR_TARGET;

        // If enough addresses are found, end the loop
        else
          next_state  <= S_INIT;

        rd_command    <= RD_BLOCKING;
        sel_memaddr   <= ADDR_TARGET; //ADDR_GUESS;
      end

      //////////////////////////////////////////////////////////////////////////

      S_WR_TEST: begin
        next_state    <= wr_done ? S_INIT : S_WR_TEST;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_TARGET;
      end

      S_WR_WTEST: begin
        next_state    <= wr_done ? S_WAIT_WTEST : S_WR_WTEST;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_TARGET;
      end

      S_WAIT_WTEST: begin
        next_state    <= flag_waited ? S_INIT : S_WAIT_WTEST;
      end

      S_RD_TEST: begin
        if (rd_done)
          case (operation)
            OP_READ         : next_state <= S_INIT     ;
            OP_WRITE_UPDATE : next_state <= S_WR_WTEST ;
            default         : next_state <= S_INIT     ;
          endcase
        else                  next_state <= S_RD_TEST  ;

        rd_command    <= RD_BLOCKING;
        sel_memaddr   <= ADDR_TARGET;
      end

      S_RECIPE_WR: begin
        next_state    <= wr_done ? S_RECIPE_WR_WAIT : S_RECIPE_WR;
        wr_command    <= WR_BLOCKING;
        sel_memaddr   <= ADDR_RECIPE;
      end

      S_RECIPE_WR_WAIT: begin
        next_state    <= flag_waited ?
                          ( (recipe_index==4'hF) ? S_INIT
                                                 : S_RECIPE_WR
                          )                      : S_RECIPE_WR_WAIT;
      end

      S_RECIPE_RD: begin
        next_state    <= rd_done ? S_RECIPE_RD_NEXT : S_RECIPE_RD;
        rd_command    <= RD_BLOCKING;
        sel_memaddr   <= ADDR_RECIPE;
      end

      S_RECIPE_RD_NEXT: begin
        next_state    <= (recipe_index==4'hF) ? S_INIT : S_RECIPE_RD;
      end

    endcase
  end

  always_ff @(posedge clk) begin
    if (reset) state <= S_INIT;
    else       state <= next_state;

    if (state != next_state) begin
        $display("state %d -> %d", state, next_state);

        if (next_state == S_INIT)
          $display("counter_memop_r %d", counter_memop_r);
    end
  end

endmodule // app_afu