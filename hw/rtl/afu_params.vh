`ifndef AFU_PARAMS_H
`define AFU_PARAMS_H

`define DEBUG 1
// `define SIMULATION 1

localparam EV_SET_SIZE = 128;
localparam EV_SET_RECIPE_SIZE = 16; 
  // hence "op_recipe" is 4-bits
  // and "recipe" is processed in 4-bit shifts

localparam OP_CREATE_EVS   = 3'b000;
localparam OP_WRITE_NONBL  = 3'b001;
localparam OP_WRITE_BLOCK  = 3'b010;
localparam OP_READ         = 3'b100;
localparam OP_WRITE_UPDATE = 3'b110;
localparam OP_RECIPE_WRITE = 3'b101;
localparam OP_RECIPE_READ  = 3'b111;

typedef enum logic [1:0] {
  WR_NONE,
  WR_BLOCKING,
  WR_NONBLOCKING
} t_wr_commands;

typedef enum logic [1:0] {
  RD_NONE,
  RD_BLOCKING,
  RD_NONBLOCKING
} t_rd_commands;

typedef enum logic [5:0] {

  S_WR_TARGET,            //  0
  S_WR_TARGET_WAIT,       //  1
  S_WR_FRSTFND,           //  2
  S_WR_FRSTFND_WAIT,      //  3
  S_WR_FRSTFND2,          //  4
  S_WR_FRSTFND_WAIT2,     //  5
  S_WR_GUESS,             //  6
  S_WR_GUESS_WAIT,        //  7

  S_WRT1_TARGET,          //  8
  S_WRT1_TARGET_WAIT,     //  9
  S_WRT1_FRSTFND,         // 10
  S_WRT1_FRSTFND_WAIT,    // 11
  S_WRT1_FRSTFND2,        // 12
  S_WRT1_FRSTFND_WAIT2,   // 13
  S_WRT1_GUESS,           // 14
  S_WRT1_GUESS_WAIT,      // 15

  S_WRT3_GUESS,           // 16
  S_WRT3_GUESS_WAIT,      // 17
  S_WRT3_FRSTFND,         // 18
  S_WRT3_FRSTFND_WAIT,    // 19
  S_WRT3_FRSTFND2,        // 20
  S_WRT3_FRSTFND_WAIT2,   // 21
  S_WRT3_TARGET,          // 22
  S_WRT3_TARGET_WAIT,     // 23
  
  S_INIT,                 // 24
  S_RD_TARGET,            // 25
  S_RDT_TARGET,           // 26
  S_RDT1_TARGET,          // 27
  S_RDT3_GUESS,           // 28

  S_WR_TEST,              // 29
  S_WR_WTEST,             // 30
  S_WAIT_WTEST,           // 31
  S_RD_TEST,              // 32

  S_RECIPE_WR,            // 33
  S_RECIPE_WR_WAIT,       // 34
  S_RECIPE_RD,            // 35
  S_RECIPE_RD_NEXT        // 36

} t_state;

typedef enum logic [1:0] {
  ADDR_TARGET,
  ADDR_GUESS,
  ADDR_FRSTFND,
  ADDR_RECIPE
} t_sel_memaddr;

`endif