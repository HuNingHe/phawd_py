import sys
from phawd import ParameterKind, ParameterValue, Parameter
from phawd import SharedParameters, SharedMemory
from phawd import SocketToPhawd, SocketFromPhawd, SocketConnect
from phawd import GamepadCommand, ParameterCollection
sys.path.insert(0, "./")


def test_parameter_kind():
    assert int(ParameterKind.DOUBLE) == 1
    assert int(ParameterKind.S64) == 2
    assert int(ParameterKind.VEC3_DOUBLE) == 4


def test_parameter_value():
    v = ParameterValue()
    assert v.d == 0
    assert v.i == 0
    assert all(v.vec3d == [0, 0, 0])

    v.d = 123
    assert v.d == 123
    v.i = 980
    assert v.i == 980
    v.vec3d = [1, 2, 3]
    assert all(v.vec3d == [1, 2, 3])


def test_parameter():
    # default constructor
    p = Parameter()
    assert not p.isSet()
    assert p.getDouble() == 0
    assert len(p.getName()) == 0
    assert p.getValueKind() == ParameterKind.DOUBLE

    p = Parameter("p0", ParameterKind.VEC3_DOUBLE)
    assert p.isSet()
    assert all(p.getVec3d() == [0, 0, 0])
    assert p.getName() == "p0"
    assert p.getValueKind() == ParameterKind.VEC3_DOUBLE

    p.setName("p1")
    assert p.getName() == "p1"
    p.setValue([1, 2, 3])
    assert all(p.getVec3d() == [1, 2, 3])
    p.setValueKind(ParameterKind.S64)
    assert p.getValueKind() == ParameterKind.S64
    p.setValue(123)
    assert p.getS64() == 123

    pv = ParameterValue()
    pv.vec3d = [0, 1, 2]
    p = Parameter("p0", ParameterKind.VEC3_DOUBLE, pv)
    assert p.getValueKind() == ParameterKind.VEC3_DOUBLE
    assert all(p.getVec3d() == [0, 1, 2])
    assert p.getName() == "p0"

    p = Parameter("p_d", 12.31)
    assert p.getValueKind() == ParameterKind.DOUBLE
    assert p.getDouble() == 12.31
    assert p.getName() == "p_d"

    p = Parameter("p_s64", 1231)
    assert p.getValueKind() == ParameterKind.S64
    assert p.getS64() == 1231
    assert p.getName() == "p_s64"

    p = Parameter("p_vec3", [12, 23, 34])
    assert p.getValueKind() == ParameterKind.VEC3_DOUBLE
    assert all(p.getVec3d() == [12, 23, 34])
    assert p.getName() == "p_vec3"

    assert Parameter.__sizeof__() == 48


def test_parameter_collection():
    p0 = Parameter("p0", 12.13)
    p1 = Parameter("p1", 1213)
    p2 = Parameter("p2", [1, 2, 3])
    pc = ParameterCollection("pc")
    pc.addParameter(p0)
    pc.addParameter(p1)
    pc.addParameter(p2)

    p_tmp = pc.lookup("p0")
    p_tmp.setName("p4")
    assert p0.getName() == "p4"
    assert id(p_tmp) == id(p0)

    p_tmp = pc.lookup("p1")
    p_tmp.setValue(3121)
    assert p1.getS64() == 3121
    assert id(p_tmp) == id(p1)

    p_tmp = pc.lookup("p2")
    p_tmp.setValueKind(ParameterKind.DOUBLE)
    assert p2.getValueKind() == ParameterKind.DOUBLE
    assert id(p_tmp) == id(p2)

    assert pc.checkIfAllSet()
    pc.clearAllParameters()


def test_gamepad_command():
    gp = GamepadCommand()
    assert not gp.A
    assert not gp.B
    assert not gp.X
    assert not gp.Y
    assert not gp.UP
    assert not gp.DOWN
    assert not gp.LEFT
    assert not gp.RIGHT
    assert not gp.LB
    assert not gp.RB
    assert not gp.BACK
    assert not gp.START
    assert gp.LT == 0
    assert gp.RT == 0
    assert gp.axisLeftX == 0
    assert gp.axisLeftY == 0
    assert gp.axisRightX == 0
    assert gp.axisRightY == 0
    gp.init()


def test_shared_parameters():
    pc = ParameterCollection()
    sp = SharedParameters.create(2, 1)
    p0 = Parameter("p0", 12.13)
    p1 = Parameter("p1", 1213)
    p2 = Parameter("p2", [1, 2, 3])

    sp.setParameters([p2])
    sp.setParameters([p0, p1], set_control_params=True)

    p_tmp = sp.getParameters()
    assert p_tmp[0].getDouble() == 12.13
    assert p_tmp[0].getName() == "p0"
    assert p_tmp[1].getS64() == 1213
    assert p_tmp[1].getName() == "p1"

    p_tmp = sp.getParameters(get_control_params=False)
    assert all(p_tmp[0].getVec3d() == [1, 2, 3])
    assert p_tmp[0].getName() == "p2"

    sp.collectParameters(pc)

    assert pc.lookup("p0").getDouble() == 12.13
    assert pc.lookup("p1").getS64() == 1213
    assert all(pc.lookup("p2").getVec3d() == [1, 2, 3])

    assert SharedParameters.__sizeof__() == 88
    SharedParameters.destroy(sp)


def test_shared_memory():
    p0 = Parameter("p0", 12.13)
    p1 = Parameter("p1", 1213)
    p2 = Parameter("p2", [1, 2, 3])

    p3 = Parameter("p3", 3.1415926)
    p4 = Parameter("p4", 45)

    shm2 = SharedMemory()
    shm1 = SharedMemory()

    p_size = Parameter.__sizeof__()
    sp_size = SharedParameters.__sizeof__()
    num_control_params = 3
    num_wave_params = 2
    shm_size = sp_size + (num_control_params + num_wave_params) * p_size

    shm1.createNew("hun", shm_size)
    sp1 = shm1.get()
    sp1.numControlParams = 3
    sp1.numWaveParams = 2
    sp1.setParameters([p0, p1, p2], set_control_params=True)

    shm2.attach("hun", shm_size)
    sp2 = shm2.get()
    sp2.setParameters([p3, p4])
    sp2.connected += 1
    assert sp2.numControlParams == 3
    assert sp2.numWaveParams == 2

    wave_params = sp1.getParameters(get_control_params=False)
    ctr_params = sp2.getParameters()

    assert ctr_params[0].getName() == "p0"
    assert ctr_params[1].getName() == "p1"
    assert ctr_params[2].getName() == "p2"
    assert ctr_params[0].getDouble() == 12.13
    assert ctr_params[1].getS64() == 1213
    assert all(ctr_params[2].getVec3d() == [1, 2, 3])

    assert wave_params[0].getName() == "p3"
    assert wave_params[1].getName() == "p4"
    assert wave_params[0].getDouble() == 3.1415926
    assert wave_params[1].getS64() == 45


def test_socket():
    send_params_num = 2
    read_params_num = 1
    send_size = SocketToPhawd.__sizeof__() + send_params_num * Parameter.__sizeof__()
    read_size = SocketFromPhawd.__sizeof__() + read_params_num * Parameter.__sizeof__()

    client = SocketConnect()

    client.init(send_size, read_size)
