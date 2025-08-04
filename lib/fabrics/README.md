# Fabrics API

The Fabrics API provides point-to-point memory sharing semantics. This includes Host to Host (inter-host), Host to device, Device to host and Device-to-Device (seperate devices) memory transfers. This differs from the publish/subscribe shared memory semantics which are available through the Flow API.

## Usage
### Inter-Host usage

#### Building the Docker image
```sh
```sh
$ cd .devcontainer
$ docker build -t mxl:ubuntu-24.04 -f Dockerfile . 
```
```


#### Running the Docker image and exposing the RDMA devices
```sh
```
docker run  --rm -it -v <WORKSPACE_PATH_SRC></WORKSPACE>:<WORKSPACE_PATH_DST> -v <MXL_DOMAIN_SRC>:<MXL_DOMAIN_DST> --device /dev/infiband/rdma_cm --device /dev/infinband/<rdma_device> -ulimit=memlock=-1:-1 --cap-add=IPC_LOCK --network host mxl:ubuntu-24.04
```
```
