# phawd_py: Python binding for phawd

&emsp;&emsp;[phawd](https://github.com/HuNingHe/phawd) is a lightweight and cross-platform software based on QT5, mainly used for robot simulation, programming and debugging, which is the abbreviation of Parameter Handler And Waveform Displayer. 

&emsp;&emsp;Here is the python binding of phawd core functions, mainly contains the interface between the robot controller written in Python and phawd software.

## 0 Installation

&emsp;&emsp;On Windows10 or Linux:

```shell
pip install phawd
```

&emsp;&emsp;MacOS is not supported for now.

## 1 Build from source

### 1.1 Prerequisites

* A compiler with C++11 support (gcc/g++ is recommended under Linux, MSVC is mandatory under Windows)

* CMake >= 3.14 (Make sure that you can use cmake on the command line)

* Pip 10+

### 1.2 command

&emsp;&emsp;Just clone this repository and pip install. Note the `--recursive` option which is needed for the pybind11 submodule:

```bash
git clone --recursive https://github.com/HuNingHe/phawd_py.git
pip install ./phawd_py
```

## 2 Using cibuildwheel 

&emsp;&emsp;[cibuildwheel](https://cibuildwheel.readthedocs.io) used for building python wheels across **Mac, Linux, Windows**, on **multiple versions of Python**. The steps are as follows: 

```shell
pip install cibuildwheel
git clone --recursive https://github.com/HuNingHe/phawd_py.git
cd phawd_py

# on windows
cibuildwheel --platform windows

# on linux
cibuildwheel --platform linux
```

&emsp;&emsp;Then you will get 36 python wheels. For example, you can then install phawd_py by:

```shell
cd wheelhouse
# this depends on your system and python version
pip install phawd-0.3-cp311-cp311-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
```

## 3 SharedMemory example

### 3.1 Prerequisites in phawd

&emsp;&emsp;phawd's yaml file is as follows:

```yaml
  RobotName: demo
  Type: SharedMemory
  WaveParamNum: 3
  DOUBLE:
    pd: [2]
  S64:
    ps64: [123]
  VEC3_DOUBLE:
    pvec3d: [1, 2, 3]
```

&emsp;&emsp;Read this file using phawd, and click OK button. Then run the code demo as below.

### 3.2 code demo

&emsp;&emsp;A robot controller program example using sharedmemory to communicate with phawd software:

```python
# robot controller
from phawd import SharedMemory, SharedParameters, ParameterCollection, Parameter

if __name__ == '__main__':
    p0 = Parameter("pw_d", 12.13)
    p1 = Parameter("pw_s64", 1213)
    p2 = Parameter("pw_vec3d", [-1.4, 2, 3])

    shm = SharedMemory()
    pc = ParameterCollection()

    p_size = Parameter.__sizeof__()
    sp_size = SharedParameters.__sizeof__()
    num_control_params = 3
    num_wave_params = 3
    shm_size = sp_size + (num_control_params + num_wave_params) * p_size

    shm.attach("demo", shm_size)
    sp = shm.get()
    sp.setParameters([p0, p1, p2])
    sp.connected += 1
    ctr_params = sp.getParameters()
    sp.collectParameters(pc)

    run_iter = 0

    while run_iter < 500000:
        run_iter += 1
        p0.setValue(pc.lookup("pd").getDouble())
        p1.setValue(pc.lookup("ps64").getS64())
        p2.setValue(pc.lookup("pvec3d").getVec3d())
        sp.setParameters([p0, p1, p2])

        if run_iter % 20 == 0:
            print(pc.lookup("ps64").getName(), ":")
            print(pc.lookup("ps64").getS64())
            print(pc.lookup("pd").getName(), ":")
            print(pc.lookup("pd").getDouble())
            print(pc.lookup("pvec3d").getName(), ":")
            print(pc.lookup("pvec3d").getVec3d())

    sp.connected -= 1
```

### 3.3 Result

- Print control parameter informations at console
- You can select all 3 curves to add in phawd
- Modify any parameter value in the phawd parameter setting interface,
- and the corresponding curve will change to the value you set

## 4 Socket example

### 4.1 Prerequisites in phawd

&emsp;&emsp;phawd's yaml file is as follows:

```yaml
RobotName: 5230
Type: Socket
WaveParamNum: 3
DOUBLE:
  pd: [2]
S64:
  ps64: [123]
VEC3_DOUBLE:
  pvec3d: [1, 2, 3]
```

&emsp;&emsp;Read this file using phawd, and click OK button. Then run the code demo as below.

### 4.2 code demo

&emsp;&emsp;A robot controller program example using Socket to communicate with phawd software:

```python
# robot controller
from phawd import SocketToPhawd, SocketFromPhawd, SocketConnect, Parameter

if __name__ == '__main__':
    p0 = Parameter("pw_d", 1.213)
    p1 = Parameter("pw_s64", 12)
    p2 = Parameter("pw_vec3d", [-1.4, 2, 3])

    send_params_num = 3
    read_params_num = 3
    send_size = SocketToPhawd.__sizeof__() + send_params_num * Parameter.__sizeof__()
    read_size = SocketFromPhawd.__sizeof__() + read_params_num * Parameter.__sizeof__()

    client = SocketConnect()

    client.init(send_size, read_size)
    client.connectToServer("127.0.0.1", 5230)
    run_iter = 0

    socket_to_phawd = client.getSend()
    socket_to_phawd.numWaveParams = 3
    socket_to_phawd.parameters = [p0, p1, p2]

    while run_iter < 5000000:
        run_iter += 1
        socket_to_phawd = client.getSend()

        ret = client.read()

        if ret > 0:
            socket_from_phawd = client.getRead()

            p0.setValue(socket_from_phawd.parameters[0].getDouble())
            p1.setValue(socket_from_phawd.parameters[1].getS64())
            p2.setValue(socket_from_phawd.parameters[2].getVec3d())
            socket_to_phawd.parameters = [p0, p1, p2]
            client.send()

            if run_iter % 20 == 0:
                print(socket_from_phawd.parameters[0].getName(), ":")
                print(socket_from_phawd.parameters[0].getDouble())
                print(socket_from_phawd.parameters[1].getName(), ":")
                print(socket_from_phawd.parameters[1].getS64())
                print(socket_from_phawd.parameters[2].getName(), ":")
                print(socket_from_phawd.parameters[2].getVec3d())
```

### 4.3 Result

- Once you modify the parameter in phawd software, this program will print parameter informations at console
- You can select all 3 curves to add in phawd
- Modify any parameter value in the phawd parameter setting interface,
- and the corresponding curve will change to the value you set

## 5 Notation

- Parameters of type FLOAT and VEC3_FLOAT  are not supported in phawd_py
- For other tutorials on phawd_py, you can refer to the tests/test.py

## License

&emsp;&emsp;phawd_py is provided under MIT license that can be found in the LICENSE file. By using, distributing, or contributing to this project, you agree to the terms and conditions of this license.
