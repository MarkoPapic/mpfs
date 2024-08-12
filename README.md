# Trying out

Install the kernel module:
```
sudo insmod mpfs.ko
```

Check the registered file systems:
```
cat /proc/filesystems | grep mpfs
```

Check the logs:
```
sudo dmesg | tail -4
```

Create an empty file:
```
dd if=/dev/zero of=dev/fake_device bs=1024 count=104857600
```

Build the `mkfs` tool:
```
make mkfs_mpfs
```

Run `mkfs`:
```
./mkfs_mpfs dev/fake_device
```

Create a `mpfs` directory in `~/media`

Mount the file system:
```
sudo mount -t mpfs -o loop dev/fake_device ~/media/mpfs
```

# Troubleshooting

## Module verification failed: signature and/or required key missing - tainting kernel

If you see the following line in your `make build` output, that is probably what is causing it:

```
Skipping BTF generation for /home/marko/code/mpfs/src/mpfs.ko due to unavailability of vmlinux
```

You can fix it by installing `dwarves`
```
apt-get install dwarves
```

check if `vmlinux` is now present

```
ls /sys/kernel/btf
```

and copy it to where it is expected to be found

```
cp /sys/kernel/btf/vmlinux /usr/lib/modules/$(uname -r)/build/
```

An alternative is to sign your modules manually.

## pr_debug logs not visible in dmesg

Add `CFLAGS_mpfs.o := -DDEBUG` to your `Kbuild` file.

An alternative would be to use `pr_warn` instead.

## VSCode intellisense reports a bunch of errors even though the build succeeds

It boils down to `c_cpp_properties.json` configuration. Probably one of the following:
* `includePath`
* `defines`
* `browse->path`
* `compilerPath`