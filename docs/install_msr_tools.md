# Install msr-tools for DDIO Configuration

Clone the [Intel msr-tools](https://github.com/intel/msr-tools) repository and compile. 

Use the installation path to replace the `MSRTOOLS_PATH` variable in the experiment code (if that variable is needed).

___

This app allows to set CPU MSR registers. In our specific case, we use it to tweak the DDIO configuration. The default configuration allows PCIe-attached secondary devices to write-allocate to at most 2 ways of the LLC. By writing to the appropriate MSR, one can allow DDIO to consume more LLC ways.

After compilation, you should first modprobe the kernel module:

```
sudo modprobe msr
```

Afterwards, you can use the `wrmsr` and `rdmsr` apps, respectively to write and read CPU registers. In the example below, the CPU register for DDIO configuration (at `0xc8b`) is accessed. The last argument of the first line is for the configuration you want to set.

```
sudo <MSRTOOLS_PATH>/msr-tools/wrmsr 0xc8b 0x600
sudo <MSRTOOLS_PATH>/msr-tools/rdmsr 0xc8b 
```

On our 11-way LLC, when the most-significant 10th and 9th bits are set (`0x600`), DDIO can access only 2 cache ways. This is the default value.

## How it is used in our experiments?

You do not need to explicitly control the MSR yourself with `wrmsr` and `rdmsr`, the code takes care of this. Replacing the `MSRTOOLS_PATH` variable in the relevant experiment's code with `msr-tools` installation path is sufficient.

You will need `sudo` for the experiments that use this tool.
After running the experiment, it may make sense to revert the DDIO configuration to its default value.

Note that we only use `msr-tools` for reverse-engineering experiments, and for evaluating our designs. Normally, changing the CPU's DDIO configuration is a privilege of the system admin (having root privileges). We do not assume such an ability for the attacker, and most of our hardware should work for any configuration.
