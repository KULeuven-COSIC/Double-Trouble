`include "cci_mpf_if.vh"
`include "csr_mgr.vh"
`include "afu_json_info.vh"
`include "afu_params.vh"

module app_memops (
  cci_mpf_if.to_fiu     fiu         ,

  input   logic         clk         ,
  input   logic         reset       ,

  input   logic [1:0]   wr_command  ,
  input   logic         wr_touch    ,
  output  logic         wr_done     ,

  input   logic [1:0]   rd_command  ,
  output  logic         rd_done     ,

  input   t_ccip_clAddr memaddress  ,
  output  logic [9:0]   counter_memop     
  );

  //////////////////////////////////////////////////////////////////////////////

  logic [1:0]   wr_command_r;
  logic         wr_touch_r;
  logic         wr_done_r;
  logic [1:0]   rd_command_r;
  logic         rd_done_r;
  t_ccip_clAddr memaddress_r;

  always_ff @(posedge clk) begin
    wr_command_r <= wr_command;
    wr_touch_r   <= wr_touch  ;
    wr_done_r    <= wr_done   ;
    rd_command_r <= rd_command;
    rd_done_r    <= rd_done   ;
    memaddress_r <= memaddress;
  end

  //////////////////////////////////////////////////////////////////////////////

  assign fiu.c2Tx.mmioRdValid = 1'b0;

  
  localparam WR_DATA = 64'hA8899ABB_CCDDEEFF;

  t_cci_clData read_data;   // 512-bit
  t_cci_clData write_data;

  assign write_data = (wr_touch_r) ? read_data : {read_data[511:64], WR_DATA};
  
  // =========================================================================
  //
  //   Write Operation
  //
  // =========================================================================

  logic wr_busy;
  logic wr_start;
  logic wr_SentRequest;
  logic wr_ReceivedReply;

  assign wr_done  = ((wr_command_r == WR_NONBLOCKING) &   wr_SentRequest   ) ||
                    ((wr_command_r == WR_BLOCKING   ) &   wr_ReceivedReply ) ;

  assign wr_start = ( wr_command_r != WR_NONE       ) & ~(wr_done | wr_done_r);


  always_ff @(posedge clk)
  begin
    if (reset)           wr_busy <= 1'b0;
    else if (wr_start)   wr_busy <= 1'b1;
    else if (wr_done)    wr_busy <= 1'b0;
    else                 wr_busy <= wr_busy;
  end

  // Schedule Write Request ////////////////////////////////////////////////////
  //

  // Back pressure may prevent an immediate write request,
  // we must record whether a write is needed and
  // hold it until the request can be sent to the FIU.
  //
  logic wr_needed;

  always_ff @(posedge clk)
  begin
    if (reset) begin
      wr_needed <= 1'b0;
    end
    else begin
      // If writes are allowed this cycle,
      // then we can safely clear any previously requested writes.

      if (wr_needed) begin
        wr_needed <= fiu.c1TxAlmFull;
      end
      else begin
        wr_needed <= wr_start & ~wr_busy;
      end

      if (wr_needed)
        $display("   Write request is needed");

    end
  end

  // Emit write request to the FIU /////////////////////////////////////////////
  //

  // Construct a memory write request header.
  t_cci_mpf_c1_ReqMemHdr wr_hdr;
  assign wr_hdr = cci_mpf_c1_genReqHdr( eREQ_WRLINE_I,
                                        memaddress_r,
                                        t_cci_mdata'(0),
                                        cci_mpf_defaultReqHdrParams());

  // Send write requests to the FIU
  always_ff @(posedge clk)
  begin
    if (reset) begin
      fiu.c1Tx.valid <= 1'b0;
      wr_SentRequest  <= 1'b0;
    end
    else begin
      // Generate a read request when needed and the FIU isn't full
      fiu.c1Tx <= cci_mpf_genC1TxWriteReq(wr_hdr,
                                          write_data,
                                          (wr_needed && !fiu.c1TxAlmFull));

      if (wr_needed && !fiu.c1TxAlmFull) begin
        wr_SentRequest <= 1'b1;
        $display("   Write request is sent to FIU for %h", memaddress_r);
      end
      else begin
        wr_SentRequest <= 1'b0;
      end
    end
  end

  // Write Response Handling ///////////////////////////////////////////////////
  //

  always_ff @(posedge clk)
  begin
    if (reset) begin
      wr_ReceivedReply <= 1'b0;
    end
    // Receive data (write response).
    else if (cci_c1Rx_isWriteRsp(fiu.c1Rx)) begin
      $display("   Write response received.");
      wr_ReceivedReply <= 1'b1;
    end
    else begin
      wr_ReceivedReply <= 1'b0;
    end
  end

  // =========================================================================
  //
  //   Read operation
  //
  // =========================================================================

  logic rd_busy;
  logic rd_start;
  logic rd_SentRequest;
  logic rd_ReceivedReply;

  assign rd_done  = ((rd_command_r == RD_NONBLOCKING) &   rd_SentRequest   ) ||
                    ((rd_command_r == RD_BLOCKING   ) &   rd_ReceivedReply ) ;

  assign rd_start = ( rd_command_r != RD_NONE       ) & ~(rd_done | rd_done_r);

  always_ff @(posedge clk)
  begin
    if (reset)           rd_busy <= 1'b0;
    else if (rd_start)   rd_busy <= 1'b1;
    else if (rd_done)    rd_busy <= 1'b0;
    else                 rd_busy <= rd_busy;
  end


  // Schedule Read Request /////////////////////////////////////////////////////
  //

  // Back pressure may prevent an immediate read request,
  // we must record whether a read is needed and
  // hold it until the request can be sent to the FIU.

  logic rd_needed;

  always_ff @(posedge clk)
  begin
    if (reset) begin
      rd_needed <= 1'b0;
    end
    else begin
      // If reads are allowed this cycle,
      // then we can safely clear any previously requested reads.

      if (rd_needed) begin
        rd_needed <= fiu.c0TxAlmFull;
      end
      else begin
        rd_needed <= rd_start & ~rd_busy;
      end

      if (rd_needed)
        $display("   Read request is needed");

    end
  end

  // Emit read request to the FIU //////////////////////////////////////////////
  //

  // Construct a memory read request header.
  t_cci_mpf_c0_ReqMemHdr rd_hdr;
  assign rd_hdr = cci_mpf_c0_genReqHdr( eREQ_RDLINE_I,
                                        memaddress_r,
                                        t_cci_mdata'(0),
                                        cci_mpf_defaultReqHdrParams());

  // Send read requests to the FIU
  always_ff @(posedge clk)
  begin
    if (reset) begin
      fiu.c0Tx.valid <= 1'b0;
      rd_SentRequest <= 1'b0;
    end
    else begin
      // Generate a read request when needed and the FIU isn't full
      fiu.c0Tx <= cci_mpf_genC0TxReadReq(rd_hdr, (rd_needed && !fiu.c0TxAlmFull));

      if (rd_needed && !fiu.c0TxAlmFull) begin
        $display("   Read request is sent to FIU for %h", memaddress_r);
        rd_SentRequest <= 1'b1;
      end
      else begin
        rd_SentRequest <= 1'b0;
      end
    end
  end

  // Read Response Handling ////////////////////////////////////////////////////
  //

  always_ff @(posedge clk)
  begin
    if (reset) begin
      rd_ReceivedReply <= 1'b0;
      // read_data <= 64'b0;
    end
    // Receive data (read responses).
    else if (cci_c0Rx_isReadRsp(fiu.c0Rx)) begin
      $display("   Read reply received %h", fiu.c0Rx.data);
      read_data <= fiu.c0Rx.data;
      rd_ReceivedReply <= 1'b1;
    end
    else begin
      rd_ReceivedReply <= 1'b0;
    end
  end

  // =========================================================================
  //
  //   The counter_memop
  //
  // =========================================================================

  logic [9:0] cycle_counter_memop;

  always_ff @(posedge clk)
    if (rd_SentRequest || wr_SentRequest)
          cycle_counter_memop <= 10'b0;
    else  cycle_counter_memop <= cycle_counter_memop + 10'd1;

  assign counter_memop = cycle_counter_memop;

endmodule