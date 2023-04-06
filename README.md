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
RobotName: shm_demo
Type: SharedMemory
WaveParamNum: 3
DOUBLE:
  p_d: 3.14159
S64:
  p_s64: 31
VEC3_DOUBLE:
  p_vec3d: [1, 2, 3]
```

&emsp;&emsp;Read this file using phawd, and click OK button. Then run the code demo as below.

### 3.2 code demo

&emsp;&emsp;A robot controller program example using sharedmemory to communicate with phawd software:

```python
# robot controller
from phawd import SharedMemory, SharedParameters, ParameterCollection, Parameter
if __name__ == '__main__':
    num_ctr_params = 3
    num_wave_params = 3
    shm_size = SharedParameters.__sizeof__() + (num_ctr_params + num_wave_params) * Parameter.__sizeof__()
    shm = SharedMemory()
    shm.attach(name="shm_demo", size=shm_size)
    sp = shm.get()
    params = sp.getParameters()

    print('numControlParams: {}'.format(sp.numControlParams))
    print('numWaveParams: {}'.format(sp.numWaveParams))
    print('p_d name: {}; ValueKind: {}; Value: {}'.format(params[0].getName(), params[0].getValueKind(), params[0].getDouble()))
    print('p_s64 name: {}; ValueKind: {}; Value: {}'.format(params[1].getName(), params[1].getValueKind(), params[1].getS64()))
    print('p_vec3d name: {}; ValueKind: {}; Value: {}'.format(params[2].getName(), params[2].getValueKind(), params[2].getVec3d()))

    pw0 = Parameter("pw0", 5)
    pw1 = Parameter("pw1", 3.14)
    pw2 = Parameter("pw2", [1, 2, 3])

    sp.setParameters([pw0, pw1, pw2])
    sp.connected += 1  # This is important, otherwise phawd will not be able to detect the writing of data

    # If you have a lot of parameters, and it is inconvenient to process by index, you may wish to try ParameterCollection
    pc = ParameterCollection()
    sp.collectParameters(pc)

    print("p_d value in ParameterCollection: {}".format(pc.lookup("p_d").getDouble()))
    print("p_s64 value in ParameterCollection: {}".format(pc.lookup("p_s64").getS64()))
    print("p_vec3d value in ParameterCollection: {}".format(pc.lookup("p_vec3d").getVec3d()))
    sp.connected -= 1  # suggest to do this
```

### 3.3 Result

- Print control parameter informations at console
- You can select curves to add in phawd

## 4 Socket example

### 4.1 Prerequisites in phawd

&emsp;&emsp;phawd's yaml file is as follows:

```yaml
RobotName: 5230
Type: Socket
WaveParamNum: 3
DOUBLE:
  p_d: 3.14159
S64:
  p_s64: 31
VEC3_DOUBLE:
  p_vec3d: [1, 2, 3]
```

&emsp;&emsp;Read this file using phawd, and click OK button. Then run the code demo as below.

### 4.2 code demo

&emsp;&emsp;A robot controller program example using Socket to communicate with phawd software:

```python
# robot controller
from phawd import SocketToPhawd, SocketFromPhawd, SocketConnect, Parameter

if __name__ == '__main__':
    num_ctr_params = 3
    num_wave_params = 3
    send_size = Parameter.__sizeof__() * num_wave_params + SocketToPhawd.__sizeof__()
    read_size = Parameter.__sizeof__() * num_ctr_params + SocketFromPhawd.__sizeof__()
    sc = SocketConnect()
    sc.init(send_size, read_size, False)
    ret = sc.connectToServer("127.0.0.1", 5230, 30)
    iter_c = 0

    pw0 = Parameter("pw0", 5)
    pw1 = Parameter("pw1", 3.14)
    pw2 = Parameter("pw2", [1, 2, 3])

    while iter_c < 500000:
        iter_c += 1
        socket_to_phawd = sc.getSend()
        socket_to_phawd.numWaveParams = 3
        socket_to_phawd.parameters = [pw0, pw1, pw2]
        sc.send()
        ret = sc.read()

        if ret > 0:
            socket_from_phawd = sc.getRead()
            ctrl_params = socket_from_phawd.parameters
            print("numControlParams: {}".format(socket_from_phawd.numControlParams))
            print('p_d name: {}; ValueKind: {}; Value: {}'.format(ctrl_params[0].getName(), ctrl_params[0].getValueKind(), ctrl_params[0].getDouble()))
            print('p_s64 name: {}; ValueKind: {}; Value: {}'.format(ctrl_params[1].getName(), ctrl_params[1].getValueKind(), ctrl_params[1].getS64()))
            print('p_vec3d name: {}; ValueKind: {}; Value: {}'.format(ctrl_params[2].getName(), ctrl_params[2].getValueKind(), ctrl_params[2].getVec3d()))
```

### 4.3 Result

- Once you modify the parameter in phawd software, this program will print parameter informations at console
- You can select curves to add in phawd

## 5 Notation

- Parameters of type FLOAT and VEC3_FLOAT  are not supported in phawd_py
- For other tutorials on phawd_py, you can refer to the tests/test.py

## License

&emsp;&emsp;phawd_py is provided under MIT license that can be found in the LICENSE file. By using, distributing, or contributing to this project, you agree to the terms and conditions of this license.
