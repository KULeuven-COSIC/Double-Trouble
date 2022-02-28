# Install intel-cmt-cat for Cache Allocation

Clone the [Intel Cache Allocation Technology](https://github.com/intel/intel-cmt-cat) repository and compile. 

Use the compilation path for replacing the `CATLIBDIR` variable in [`sw/ddio_reverse_eng/Makefile`](../sw/ddio_reverse_eng/Makefile). 

___

This library allows to fix the LLC ways used by a CPU core. It is used only for the experiment where we reverse engineer the DDIO structure, with the relevant function calls defined in [`sw/ddio_reverse_eng/cat.c`](../sw/ddio_reverse_eng/cat.c).

You will need `sudo` for the experiments that make use of this tool.
