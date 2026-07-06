## Quick Trial
If you want to quickly try out Pika, pre-built binary versions for CentOS 5, CentOS 6, and Debian (Ubuntu 16) are available on the [release page](https://github.com/Qihoo360/pika/releases) — look for files named `pikaX.Y.Z_xxx_bin.tar.gz`.

```bash
1. Unzip the file
$ tar zxf pikaX.Y.Z_xxx_bin.tar.gz
2. Change working directory to output
note: we should be in this directory because the RPATH is ./lib;
$ cd output
3. Run pika:
$ ./bin/pika -c conf/pika.conf
```

## Compiling and Installing
### CentOS (Fedora, Redhat)
* Install required libraries

```bash
$ sudo yum install gflags-devel snappy-devel glog-devel protobuf-devel
```

* Optional libraries
```
$ sudo yum install zlib-devel lz4-devel libzstd-devel
```

* Install gcc

```
$ sudo yum install gcc-c++
```
* If your machine's gcc version is lower than 4.8, you need to switch to gcc 4.8 or above. The following commands temporarily switch to gcc 4.8:

```
$ sudo wget -O /etc/yum.repos.d/slc6-devtoolset.repo http://linuxsoft.cern.ch/cern/devtoolset/slc6-devtoolset.repo
$ sudo yum install --nogpgcheck devtoolset-2
$ scl enable devtoolset-2 bash
```
* Get the project source code

```
$ git clone https://github.com/OpenAtomFoundation/pika.git
```
* Update dependent subprojects

```
$ cd pika
$ git submodule update --init
```

* Switch to the latest release version

```
a. Run git tag to view the latest release tag (e.g., v2.3.1)
b. Run git checkout TAG to switch to the latest version (e.g., git checkout v2.3.1)
```

* Compile
 
```
$ make
```

> note: If the compilation process prompts that a required library is not installed, follow the prompt to install it, then recompile.

### Debian (Ubuntu)
* Install required libraries

```bash
$ sudo apt-get install libgflags-dev libsnappy-dev
$ sudo apt-get install libprotobuf-dev protobuf-compiler
$ sudo apt install libgoogle-glog-dev
```


* Get the project source code

```bash
$ git clone https://github.com/OpenAtomFoundation/pika.git
$ cd pika
```
* Switch to the latest release version

```
a. Run git tag to view the latest release tag (e.g., v2.3.1)
b. Run git checkout TAG to switch to the latest version (e.g., git checkout v2.3.1)
```

* Compile
```
$ make
```

> note: If the compilation process prompts that a required library is not installed, follow the prompt to install it, then recompile.

### [Static Compilation Method](https://github.com/OpenAtomFoundation/pika/issues/1148)


## Usage
```
$ ./output/bin/pika -c ./conf/pika.conf
```
