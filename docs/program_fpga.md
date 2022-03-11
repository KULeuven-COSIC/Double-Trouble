# Programming the FPGAs

An already synthesized bitstream is given for [Intel A10 PAC](https://www.intel.com/content/www/us/en/programmable/products/boards_and_kits/dev-kits/altera/acceleration-card-arria-10-gx.html), `hw/gbs/afu_a10.gbs`. In fact, we provide the `.gbs` file under compression due to GitHub's file size limitations.

You can use the Intel-provided `fpgaconf` app to program the bitstream, for example:

```
$ cd ~/Double-Trouble/hw/gbs/
$ tar -xzvf afu_a10.tar.gz
$ sudo fpgaconf -v afu_a10.gbs -B 0x3b
```

Note that you may need to change the slot parameter (`-B 0x3b`) according to your system's configuration. 

## Synthesize New Bitstream

If you have a different acceleration card, you can sythesize the bitstream using the `rtl` files given in the `hw` folder. 

Be careful that the implementation excludes the `mpf` implementations for `csr`'s. It uses them from [Intel FPGA Basic Building Blocks](https://github.com/OPAE/intel-fpga-bbb.git) repository.
Cloning this repository to the `hw` folder is enough for synthesis. [`./hw/get_bbb.sh`](../hw/get_bbb.sh) is provided for handling this cloning.

The following set of commands are provided to illustrate the synthesis steps on a [Intel Labs Academic Compute Environment (ACE)](https://wiki.intel-research.net):

```
source /export/fpga/bin/setup-fpga-env fpga-pac-a10

cd ./hw
./get_bbb.sh
afu_synth_setup -s rtl/sources.txt build_fpga
cd build_fpga
qsub-synth

# For monitoring the synthesis log
tail -f build.log
```
