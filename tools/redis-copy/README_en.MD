
## README

#### Install
Please ensure `hiredis` is installed on your machine first.

##### CentOS

```
sudo yum install hiredis-devel
```

Modify the Makefile:

Set `HIREDIS_INCLUDE` to the location of the `hiredis` header files.

Set `HIREDIS_LIB` to the location of the `hiredis` static library.

#### USAGE:

DESCRIPTION:
- Redis monitor copy tool: monitor redis server indicated by src_host, src_port, src_auth and send to des server
  Parameters:
  -s: source server
  -d: destination server
  -v: show more information
  -h: help
  Example:
- ./redis-copy -s abc@xxx.xxx.xxx.xxx:6379 -d cba@xxx.xxx.xxx.xxx:6379 -v
