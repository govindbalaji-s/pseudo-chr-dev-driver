================================================================================
==================================== CS3523 ====================================
++++++++++++++++++++++++++++++ HWP Assignment 3 ++++++++++++++++++++++++++++++++

Govind Balaji S (cs18btech11015)
K Havya Sree (cs18btech11022)

============================= Building and running =============================
To build the driver and the userspace utility:
 # cd pseudo-chr-dev-driver/
 # make

To load the device driver module:
 # insmod kernel/mykmod.ko

Check the major number of the device driver:
 # grep mykmod /proc/devices
 243 mykmod

Create a device special file:
 # mknod /tmp/mydevfile c 243 10
Use this file as argument to the utility program.

Run the utility program at ./util/memutil
Usage: ./util/memutil [options] <devname>
Options:
--operation <optype> : Where optype can be mapread, mapwrite
--message <message>  : Message to be written/read-compare to/from the device
                        memory
--paging <ptype>     : Where ptype can be prefetch or demand
--help               : Show this help

Remove the device special files and unload the driver:
 # rm -f /tmp/mydev*
 # rmmod mykmod

============================= Sample input outputs =============================
Note: The dmesg logs are printed at the end of each function.

Prefetch(Read):

 # mknod /tmp/mydevfile c 243 10
 # ./util/memutil /tmp/mydev_pR6 --pt prefetch --op mapread
 # dmesg | grep -e mykmod_vm_open -e mykmod_vm_close
[18174.522401] mykmod_vm_open: vma=ffff8d84f5becbd0 npagefaults:0
[18174.526732] mykmod_vm_close: vma=ffff8d84f5becbd0 npagefaults:256

Demand paging (Read):

 # mknod /tmp/mydev_JZl c 243 11
 # ./util/memutil /tmp/mydev_JZl --pt demand --op mapread
 # dmesg | grep -e mykmod_vm_open -e mykmod_vm_close
[ 1233.348664] mykmod_vm_open: vma=ffff89b0b6253360 npagefaults:0
[ 1233.394809] mykmod_vm_close: vma=ffff89b0b6253360 npagefaults:256

Prefetch paging (Write & Read):

 # mknod /tmp/mydev_fBc c 243 20
 # ./util/memutil /tmp/mydev_fBc --pt prefetch --op mapwrite --op mapread --mes test2
 # dmesg | grep -e mykmod_vm_open -e mykmod_vm_close
[ 1415.207203] mykmod_vm_open: vma=ffff89b1385c4ca8 npagefaults:0
[ 1415.243550] mykmod_vm_close: vma=ffff89b1385c4ca8 npagefaults:256
[ 1415.243570] mykmod_vm_open: vma=ffff89b1385c4ca8 npagefaults:0
[ 1415.255539] mykmod_vm_close: vma=ffff89b1385c4ca8 npagefaults:256

Demand paging (Write & Read):

 # mknod /tmp/mydev_Ln5 c 243 21
 # ./util/memutil /tmp/mydev_Ln5 --pt demand --op mapwrite --op mapread --mes test2
 # dmesg | grep -e mykmod_vm_open -e mykmod_vm_close
[ 1501.923579] mykmod_vm_open: vma=ffff89b135a76d80 npagefaults:0
[ 1501.947487] mykmod_vm_close: vma=ffff89b135a76d80 npagefaults:256
[ 1501.947527] mykmod_vm_open: vma=ffff89b135a76d80 npagefaults:0
[ 1501.993956] mykmod_vm_close: vma=ffff89b135a76d80 npagefaults:256
