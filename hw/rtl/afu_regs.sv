`include "cci_mpf_if.vh"
`include "csr_mgr.vh"
`include "afu_json_info.vh"
`include "afu_params.vh"

module afu_regs (
  app_csrs.app        csrs            ,
  input  logic        clk             ,
  input  logic        reset           ,

                                          // +-------> Control Status Reg Number
                                          // |   +---> DIR: In  : CPU  -> FPGA
                                          // |   |          Out : FPGA -> CPU
                                          // |   |   Explanation
                                          //---|---|-----------------------------
  output wire         op_start          , // 0 | I | Flag at CSR write
  output wire [ 2:0]  op_params         , //   | I | Data received at CSR write
  output reg          op_en_test        , //   | I | Data received at CSR write
  output reg          op_en_test_multi  , //   | I |
  output reg          op_en_test_only_first ,
  output reg          op_pagesize       , //   | I | Data received at CSR write
  output wire [ 3:0]  op_recipe         , //   | I | Recipe index 
  input  wire         op_done           , //   | O | Data send at CSR read
  output reg  [63:0]  op_adr_target     , // 1 | I |
  input  wire [ 9:0]  op_timing         , // 1 | O |
  output reg  [63:0]  op_adr_search     , // 2 | I |
  output reg  [ 9:0]  op_threshold      , // 3 | I |
  output reg  [ 3:0]  op_ddio_count     , // 4 | I |
  output reg  [ 7:0]  op_wrap_count     , // 4 | I |
  output reg  [ 3:0]  op_test_count     , // 4 | I |
  input  wire [63:0]  op_evs_recipe_cnt , // 4 | O |                                        
  output reg  [ 7:0]  op_evs_addr       , // 5 | I |
  input  wire [63:0]  op_evs_data       , // 5 | O |
  output reg  [63:0]  op_evs_recipe     , // 6 | I |
  output reg          op_evs_recipe_wen , // 6 | I |
  input  wire [63:0]  op_debug0         , // 6 | O |
  output reg  [ 9:0]  op_wait           , // 7 | I |
  input  wire [63:0]  op_debug1           // 7 | O |
  );
  
  logic        op_done_reg;
  logic [ 9:0] op_timing_reg;
  logic [63:0] op_evs_recipe_cnt_reg;
  logic [63:0] op_evs_data_reg;
  logic [63:0] op_debug0_reg;
  logic [63:0] op_debug1_reg;

  always_ff @(posedge clk) begin
    op_done_reg           <= op_done;
    op_timing_reg         <= op_timing;
    op_evs_recipe_cnt_reg <= op_evs_recipe_cnt;
    op_evs_data_reg       <= op_evs_data;
    op_debug0_reg         <= op_debug0;
    op_debug1_reg         <= op_debug1;
  end

  always_comb
  begin
    // The AFU ID is a unique ID for a given program.
    csrs.afu_id = `AFU_ACCEL_UUID;

    // Default
    for (int i = 0; i < NUM_APP_CSRS; i = i + 1)
      csrs.cpu_rd_csrs[i].data = 64'(0);

    // Non-Default
    csrs.cpu_rd_csrs[0].data = 64'(op_done_reg          );
    csrs.cpu_rd_csrs[1].data = 64'(op_timing_reg        );
    csrs.cpu_rd_csrs[4].data = 64'(op_evs_recipe_cnt_reg);
    csrs.cpu_rd_csrs[5].data = 64'(op_evs_data_reg      );
    csrs.cpu_rd_csrs[6].data = 64'(op_debug0_reg        );
    csrs.cpu_rd_csrs[7].data = 64'(op_debug1_reg        );
  end

  // Implement the CSR writes //////////////////////////////////////////////////
  //

  assign op_start  = csrs.cpu_wr_csrs[0].en;
  assign op_params = csrs.cpu_wr_csrs[0].data;
  assign op_recipe = csrs.cpu_wr_csrs[0].data[10:7];

  always_ff @(posedge clk)
  begin
    if (reset) begin
      op_en_test              <= 'b0;
      op_en_test_multi        <= 'b0;
      op_en_test_only_first   <= 'b0;
      op_pagesize             <= 'b0;
      op_adr_target           <= 'b0;
      op_adr_search           <= 'b0;
      op_threshold            <= 'b0;
      op_ddio_count           <= 'd2;
      op_wrap_count           <= 'd8;
      op_test_count           <= 'd1; 
      op_evs_addr             <= 'b0;
      op_evs_recipe           <= 'b0;
      op_evs_recipe_wen       <= 'b0;
      op_wait                 <= 'b0;
    end
    else begin

      if (csrs.cpu_wr_csrs[0].en) begin
        op_en_test            <= csrs.cpu_wr_csrs[0].data[3];
        op_en_test_multi      <= csrs.cpu_wr_csrs[0].data[4];
        op_en_test_only_first <= csrs.cpu_wr_csrs[0].data[5];
        op_pagesize           <= csrs.cpu_wr_csrs[0].data[6];        
        $display("REG0: En Test %h", csrs.cpu_wr_csrs[0].data[3]);
        $display("REG0: En Multi Test %h", csrs.cpu_wr_csrs[0].data[4]);
        $display("REG0: En Only First Test %h", csrs.cpu_wr_csrs[0].data[5]);
        $display("REG0: Page Size %h", csrs.cpu_wr_csrs[0].data[6]);
      end

      if (csrs.cpu_wr_csrs[1].en) begin
        op_adr_target <= csrs.cpu_wr_csrs[1].data;
        $display("REG1: Target Address %h", csrs.cpu_wr_csrs[1].data);
      end

      if (csrs.cpu_wr_csrs[2].en) begin
        op_adr_search <= csrs.cpu_wr_csrs[2].data;
        $display("REG2: Search Address %h", csrs.cpu_wr_csrs[2].data);
      end

      if (csrs.cpu_wr_csrs[3].en) begin
        op_threshold <= csrs.cpu_wr_csrs[3].data;
        $display("REG3: HW Read Threshold %d", csrs.cpu_wr_csrs[3].data);
      end

      if (csrs.cpu_wr_csrs[4].en) begin
        op_ddio_count <= csrs.cpu_wr_csrs[4].data[ 3: 0];
        op_wrap_count <= csrs.cpu_wr_csrs[4].data[15: 8];
        op_test_count <= csrs.cpu_wr_csrs[4].data[29:16];
        $display("REG4: Configurations DDIO %d Wrap %d Test %d", 
          csrs.cpu_wr_csrs[4].data[ 3: 0],
          csrs.cpu_wr_csrs[4].data[15: 8],
          csrs.cpu_wr_csrs[4].data[29:16]);
      end

      if (csrs.cpu_wr_csrs[5].en) begin
        op_evs_addr <= csrs.cpu_wr_csrs[5].data;
        $display("REG5: Eviction Set Address %h", csrs.cpu_wr_csrs[5].data);
      end

      if (csrs.cpu_wr_csrs[6].en) begin
        op_evs_recipe <= csrs.cpu_wr_csrs[6].data;
        op_evs_recipe_wen <= 1'b1;
        $display("REG6: Eviction Set Recipe %h", csrs.cpu_wr_csrs[6].data);
      end
      else 
        op_evs_recipe_wen <= 1'b0; 

      if (csrs.cpu_wr_csrs[7].en) begin
        op_wait <= csrs.cpu_wr_csrs[7].data;
        $display("REG7: Wait %d", csrs.cpu_wr_csrs[7].data);
      end

    end
  end

endmodule