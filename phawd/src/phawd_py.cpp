#include <iostream>
#include <cstring>
#include "Parameter.h"
#include "SocketConnect.h"
#include "SharedMemory.h"
#include "SharedParameter.h"

#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(phawd, m) {
    m.doc() = "phawd stands for Parameter Handler And Waveform Displayer. This python binding, "
              "mainly includes socket and shared memory communication with the software phawd.";
    py::enum_<phawd::ParameterKind>(m, "ParameterKind")
    .value("DOUBLE", phawd::ParameterKind::DOUBLE)
    .value("S64", phawd::ParameterKind::S64)
    .value("VEC3_DOUBLE", phawd::ParameterKind::VEC3_DOUBLE);

    py::class_<phawd::ParameterValue>(m, "ParameterValue")
    .def(py::init<>())
    .def_readwrite("d", &phawd::ParameterValue::d, "Double value")
    .def_readwrite("i", &phawd::ParameterValue::i, "Long int value")
    .def_property("vec3d", &phawd::ParameterValue::getVec3d, &phawd::ParameterValue::setVec3d, "3 dim vector of double value");

    py::class_<phawd::Parameter>(m, "Parameter")
    .def(py::init())
    .def(py::init<const std::string&, phawd::ParameterKind &>(), py::arg("name"), py::arg("kind"))
    .def(py::init<const std::string&, phawd::ParameterKind &, phawd::ParameterValue &>(), py::arg("name"), py::arg("kind"), py::arg("value"))
    .def(py::init<const std::string &, double>(), py::arg("name"), py::arg("double"))
    .def(py::init<const std::string &, long int>(), py::arg("name"), py::arg("s64"))
    .def(py::init<const std::string &, const py::array_t<double>&>(), py::arg("name"), py::arg("vec3d"))
    .def("getName", &phawd::Parameter::getName)
    .def("getValueKind", &phawd::Parameter::getValueKind)
    .def("getValue", &phawd::Parameter::getValue)
    .def("getDouble", &phawd::Parameter::getDouble)
    .def("getS64", &phawd::Parameter::getS64)
    .def("getVec3d", &phawd::Parameter::getVec3d)
    .def("setName", &phawd::Parameter::setName, py::arg("name"))
    .def("setValueKind", &phawd::Parameter::setValueKind, py::arg("kind"))
    .def("isSet", &phawd::Parameter::isSet)
    .def("setValue", static_cast<void (phawd::Parameter::*)(const py::array_t<double> &)>(&phawd::Parameter::setValue), py::arg("vec3d"))
    .def("setValue", static_cast<void (phawd::Parameter::*)(long int)>(&phawd::Parameter::setValue), py::arg("s64"))
    .def("setValue", static_cast<void (phawd::Parameter::*)(double)>(&phawd::Parameter::setValue), py::arg("double"))
    .def("setValue", static_cast<void (phawd::Parameter::*)(phawd::ParameterKind, const phawd::ParameterValue&)>(&phawd::Parameter::setValue), py::arg("kind"), py::arg("value"))
    .def("__sizeof__", [](){return sizeof(phawd::Parameter);});

    py::class_<phawd::ParameterCollection>(m, "ParameterCollection")
    .def(py::init<const std::string &>(), py::arg("name") = "")
    .def("addParameter", &phawd::ParameterCollection::addParameter, "Add one parameter to collection")
    .def("lookup", &phawd::ParameterCollection::lookup, "Find a parameter in collection", py::return_value_policy::reference)
    .def("checkIfAllSet", &phawd::ParameterCollection::checkIfAllSet, "Check whether all parameter values in the collection are set")
    .def("clearAllSet", &phawd::ParameterCollection::clearAllSet, "Clear set of all parameters")
    .def("clearAllParameters", &phawd::ParameterCollection::clearAllParameters, "delete all parameters in collection");

    py::class_<phawd::GamepadCommand>(m, "GamepadCommand")
    .def(py::init<>())
    .def_readwrite("X", &phawd::GamepadCommand::X)
    .def_readwrite("Y", &phawd::GamepadCommand::Y)
    .def_readwrite("A", &phawd::GamepadCommand::A)
    .def_readwrite("B", &phawd::GamepadCommand::B)
    .def_readwrite("LB", &phawd::GamepadCommand::LB)
    .def_readwrite("RB", &phawd::GamepadCommand::RB)
    .def_readwrite("LT", &phawd::GamepadCommand::LT)
    .def_readwrite("RT", &phawd::GamepadCommand::RT)
    .def_readwrite("UP", &phawd::GamepadCommand::up)
    .def_readwrite("DOWN", &phawd::GamepadCommand::down)
    .def_readwrite("LEFT", &phawd::GamepadCommand::left)
    .def_readwrite("RIGHT", &phawd::GamepadCommand::right)
    .def_readwrite("BACK", &phawd::GamepadCommand::back)
    .def_readwrite("START", &phawd::GamepadCommand::start)
    .def_readwrite("axisLeftX", &phawd::GamepadCommand::axisLeftX)
    .def_readwrite("axisLeftY", &phawd::GamepadCommand::axisLeftY)
    .def_readwrite("axisRightX", &phawd::GamepadCommand::axisRightX)
    .def_readwrite("axisRightY", &phawd::GamepadCommand::axisRightY)
    .def("init", &phawd::GamepadCommand::init);

    py::class_<phawd::SharedParameters>(m, "SharedParameters")
    .def_readwrite("connected", &phawd::SharedParameters::connected, "Number of connected objects, and whenever an object is connected, the value should manually increment 1")
    .def_readwrite("numControlParams", &phawd::SharedParameters::numControlParams)
    .def_readwrite("numWaveParams", &phawd::SharedParameters::numWaveParams)
    .def_readwrite("gamepadCommand", &phawd::SharedParameters::gameCommand)
    .def_static("create", &phawd::SharedParameters::create, "create instance of SharedParameters", py::arg("num_control_params"), py::arg("num_wave_params"), py::return_value_policy::automatic_reference)
    .def_static("destroy", &phawd::SharedParameters::destroy, "destroy instance of SharedParameters", py::arg("instance"))
    .def("__sizeof__", [](){return sizeof(phawd::SharedParameters);})
    .def("getParameters", &phawd::SharedParameters::getParameters, py::arg("get_control_params") = true)
    .def("setParameters", &phawd::SharedParameters::setParameters, py::arg("param_list"), py::arg("set_control_params") = false)
    .def("collectParameters", &phawd::SharedParameters::collectParameters, py::arg("parameter_collection"));

    py::class_<phawd::SharedMemory<phawd::SharedParameters>>(m, "SharedMemory").def(py::init<>())
    .def("createNew", &phawd::SharedMemory<phawd::SharedParameters>::createNew, "Create new shared memory.", py::arg("name"), py::arg("size"), py::arg("allowOverwrite") = true)
    .def("attach", &phawd::SharedMemory<phawd::SharedParameters>::attach, "Attach to a shared memory.", py::arg("name"), py::arg("size"))
    .def("closeNew", &phawd::SharedMemory<phawd::SharedParameters>::closeNew, "Close shared memory of new created.")
    .def("detach", &phawd::SharedMemory<phawd::SharedParameters>::detach, "Detach with the shared memory")
    .def("__call__", &phawd::SharedMemory<phawd::SharedParameters>::operator(), py::return_value_policy::automatic_reference)
    .def("get", &phawd::SharedMemory<phawd::SharedParameters>::get, "get shared parameters in shared memory.", py::return_value_policy::automatic_reference);

    py::class_<phawd::SocketFromPhawd>(m, "SocketFromPhawd")
    .def_readwrite("numControlParams", &phawd::SocketFromPhawd::numControlParams)
    .def_readwrite("gamepadCommand", &phawd::SocketFromPhawd::gameCommand)
    .def_static("create", &phawd::SocketFromPhawd::create, "create instance of SocketFromPhawd", py::arg("num_params"),py::return_value_policy::automatic_reference)
    .def_static("destroy", &phawd::SocketFromPhawd::destroy, "destroy instance of SocketFromPhawd", py::arg("instance"))
    .def("__sizeof__", [](){return sizeof(phawd::SocketFromPhawd);})
    .def_property("parameters", &phawd::SocketFromPhawd::getParameters, &phawd::SocketFromPhawd::setParameters, py::return_value_policy::automatic_reference)
    .def("collectParameters", &phawd::SocketFromPhawd::collectParameters, py::arg("parameter_collection"));

    py::class_<phawd::SocketToPhawd>(m, "SocketToPhawd")
    .def_readwrite("numWaveParams", &phawd::SocketToPhawd::numWaveParams)
    .def_static("create", &phawd::SocketToPhawd::create, "create instance of SocketToPhawd", py::arg("num_params"), py::return_value_policy::automatic_reference)
    .def_static("destroy", &phawd::SocketToPhawd::destroy, "destroy instance of SocketToPhawd" ,py::arg("instance"))
    .def("__sizeof__", [](){return sizeof(phawd::SocketToPhawd);})
    .def_property("parameters", &phawd::SocketToPhawd::getParameters, &phawd::SocketToPhawd::setParameters, py::return_value_policy::automatic_reference)
    .def("collectParameters", &phawd::SocketToPhawd::collectParameters, py::arg("parameter_collection"));

    py::class_<phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>>(m, "SocketConnect")
    .def(py::init<>())
    .def("init", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::Init, py::arg("send_size"), py::arg("read_size"), py::arg("is_server") = false)
    .def("connectToServer", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::connectToServer, py::arg("server_ip"), py::arg("port"), py::arg("milliseconds") = 30)
    .def("listenToClient", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::listenToClient, py::arg("port"), py::arg("listenQueueLength") = 2, py::arg("milliseconds") = 60)
    .def("send", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::Send, py::arg("verbose") = false)
    .def("read", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::Read, py::arg("verbose") = false)
    .def("close", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::Close, "Close socket connection")
    .def("getSend", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::getSend, "Return send data object", py::return_value_policy::automatic_reference)
    .def("getRead", &phawd::SocketConnect<phawd::SocketToPhawd, phawd::SocketFromPhawd>::getRead, "Return read data object", py::return_value_policy::automatic_reference);
}