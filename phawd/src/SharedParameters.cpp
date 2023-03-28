/*!
 * PHAWD - Parameters Handler and Waveform Display
 * Licensed under the GNU GPLv3 license. See LICENSE for more details.
 * @author HuNing-He
 * @date 2022-2-26
 * @version 1.0.1
 * @email 2689112371@qq.com
 * @copyright (c) 2022 HuNing-He
 * @file SharedParameter.cpp
 */

#include <SharedParameter.h>
using namespace phawd;

GamepadCommand::GamepadCommand(){
    down = false;
    left = false;
    up = false;
    right = false;
    LB = false;
    RB = false;
    back = false;
    start = false;
    A = false;
    B = false;
    X = false;
    Y = false;

    LT = 0;
    RT = 0;

    axisLeftX = 0;
    axisLeftY = 0;

    axisRightX = 0;
    axisRightY = 0;
}

void GamepadCommand::init() {
    std::memset(this, 0, sizeof(GamepadCommand));
}

SharedParameters::SharedParameters(){
    connected = 0;
    numControlParams = 0;
    numWaveParams = 0;
    gameCommand.init();
}

SharedParameters::SharedParameters(const SharedParameters &p): SharedParameters() {
    *this = p;
}

SharedParameters::SharedParameters(SharedParameters &&p) noexcept: SharedParameters() {
    *this = std::move(p);
}

SharedParameters &SharedParameters::operator=(const SharedParameters &p) {
    if (this != &p) {
        size_t count_real = p.numControlParams + p.numWaveParams;
        if(realloc(this, sizeof(SharedParameters) + count_real * sizeof(Parameter)) == nullptr){
            throw std::runtime_error("realloc error in operator=!");
        }
        connected = p.connected;
        numControlParams = p.numControlParams;
        numWaveParams = p.numWaveParams;
        std::memcpy(&gameCommand, &p.gameCommand, sizeof(GamepadCommand));

        for (size_t i = 0; i < count_real; ++i) {
            parameters[i] = p.parameters[i];
        }
    }
    return *this;
}

SharedParameters &SharedParameters::operator=(SharedParameters &&p) noexcept{
    if (this != &p) {
        size_t count_real = p.numControlParams + p.numWaveParams;
        realloc(this, sizeof(SharedParameters) + count_real * sizeof(Parameter));

        connected = p.connected;
        numControlParams = p.numControlParams;
        numWaveParams = p.numWaveParams;
        std::memcpy(&gameCommand, &p.gameCommand, sizeof(GamepadCommand));

        for (size_t i = 0; i < count_real; ++i) {
            parameters[i] = p.parameters[i];
        }
        free(&p);
    }
    return *this;
}

SharedParameters *SharedParameters::create(int num_control_params, int num_wave_params) {
    if (num_control_params < 0 || num_wave_params < 0){
        throw std::runtime_error("Both numControlParams and numWaveParams are negative!");
    }
    if (num_control_params == 0 && num_wave_params == 0){
        throw std::runtime_error("Both numControlParams and numWaveParams are zero!");
    }
    auto sp = (SharedParameters*) malloc(sizeof(SharedParameters) + sizeof(Parameter) * (num_wave_params + num_control_params));
    if (sp == nullptr){
        throw std::runtime_error("malloc error in SharedParameters::create!");
    }
    sp->numControlParams = num_control_params;
    sp->numWaveParams = num_wave_params;
    sp->connected = 0;
    sp->gameCommand.init();
    for (int i = 0; i < num_wave_params + num_control_params; ++i) {
        sp->parameters[i] = Parameter();
    }
    return sp;
}

void SharedParameters::destroy(SharedParameters *p){
    free(p);
    p = nullptr;
}

void SharedParameters::setParameters(py::list &param_list, bool set_control_params){
    size_t size = param_list.size();
    std::vector<Parameter> params(size);
    params = py::cast<std::vector<Parameter>>(param_list);
    if (set_control_params) {
        size_t range = size <= numControlParams? size: numControlParams;
        for (size_t i = 0; i < range; ++i) {
            parameters[i] = params[i];
        }
    } else {
        size_t range = size <= numWaveParams? size: numWaveParams;
        for (size_t i = numControlParams; i < numControlParams + range; ++i) {
            parameters[i] = params[i - numControlParams];
        }
    }
}

py::list SharedParameters::getParameters(bool get_control_params){
    py::list result;
    if (get_control_params) {
        for (int i = 0; i < numControlParams; ++i) {
            result.append(parameters[i]);
        }
    } else {
        for (size_t i = numControlParams; i < numControlParams + numWaveParams; ++i) {
            result.append(parameters[i]);
        }
    }
    return result;
}

void SharedParameters::collectParameters(ParameterCollection *pc) {
    for (size_t i = 0; i < numControlParams + numWaveParams; ++i) {
        pc->addParameter(&parameters[i]);
    }
}

SocketFromPhawd::SocketFromPhawd(){
    numControlParams = 0;
    gameCommand.init();
}

SocketFromPhawd::SocketFromPhawd(const SocketFromPhawd &p): SocketFromPhawd() {
    *this = p;
}

SocketFromPhawd::SocketFromPhawd(SocketFromPhawd &&p) noexcept: SocketFromPhawd() {
    *this = std::move(p);
}

SocketFromPhawd &SocketFromPhawd::operator=(const SocketFromPhawd &p) {
    if (this != &p) {
        size_t count_real = p.numControlParams;
        if(realloc(this, sizeof(SocketFromPhawd) + count_real * sizeof(Parameter)) == nullptr){
            throw std::runtime_error("realloc error in operator=!");
        }
        numControlParams = p.numControlParams;
        std::memcpy(&gameCommand, &p.gameCommand, sizeof(GamepadCommand));

        for (size_t i = 0; i < count_real; ++i) {
            parameters[i] = p.parameters[i];
        }
    }
    return *this;
}

SocketFromPhawd &SocketFromPhawd::operator=(SocketFromPhawd &&p) noexcept{
    if (this != &p) {
        size_t count_real = p.numControlParams;
        realloc(this, sizeof(SocketFromPhawd) + count_real * sizeof(Parameter));

        numControlParams = p.numControlParams;
        std::memcpy(&gameCommand, &p.gameCommand, sizeof(GamepadCommand));

        for (size_t i = 0; i < count_real; ++i) {
            parameters[i] = p.parameters[i];
        }
        free(&p);
    }
    return *this;
}

SocketFromPhawd *SocketFromPhawd::create(int num_params){
    if (num_params <= 0){
        throw std::runtime_error("num_params is negative or zero!");
    }

    auto sp = (SocketFromPhawd*) malloc(sizeof(SocketFromPhawd) + sizeof(Parameter) * num_params);
    if (sp == nullptr){
        throw std::runtime_error("malloc error in SharedParameters::create!");
    }
    sp->numControlParams = num_params;
    sp->gameCommand.init();
    for (int i = 0; i < num_params; ++i) {
        sp->parameters[i] = Parameter();
    }
    return sp;
}

void SocketFromPhawd::destroy(SocketFromPhawd *p){
    free(p);
    p = nullptr;
}

void SocketFromPhawd::setParameters(py::list &param_list){
    size_t size = param_list.size();
    std::vector<Parameter> params(size);
    params = py::cast<std::vector<Parameter>>(param_list);

    size_t range = size <= numControlParams? size: numControlParams;
    for (size_t i = 0; i < range; ++i) {
        parameters[i] = params[i];
    }
}

py::list SocketFromPhawd::getParameters(){
    py::list result;
    for (int i = 0; i < numControlParams; ++i) {
        result.append(parameters[i]);
    }
    return result;
}

void SocketFromPhawd::collectParameters(ParameterCollection *pc){
    for (size_t i = 0; i < numControlParams; ++i) {
        pc->addParameter(&parameters[i]);
    }
}

SocketToPhawd::SocketToPhawd(){
    numWaveParams = 0;
}

SocketToPhawd::SocketToPhawd(const SocketToPhawd &p) : SocketToPhawd(){
    *this = p;
}

SocketToPhawd::SocketToPhawd(SocketToPhawd &&p) noexcept : SocketToPhawd() {
    *this = std::move(p);
}

SocketToPhawd &SocketToPhawd::operator=(const SocketToPhawd &p){
    if (this != &p) {
        size_t count_real = p.numWaveParams;
        if(realloc(this, sizeof(SocketToPhawd) + count_real * sizeof(Parameter)) == nullptr){
            throw std::runtime_error("realloc error in operator=!");
        }
        numWaveParams = p.numWaveParams;
        for (size_t i = 0; i < count_real; ++i) {
            parameters[i] = p.parameters[i];
        }
    }
    return *this;
}

SocketToPhawd &SocketToPhawd::operator=(SocketToPhawd &&p) noexcept{
    if (this != &p) {
        size_t count_real = p.numWaveParams;
        realloc(this, sizeof(SocketToPhawd) + count_real * sizeof(Parameter));

        numWaveParams = p.numWaveParams;

        for (size_t i = 0; i < count_real; ++i) {
            parameters[i] = p.parameters[i];
        }
        free(&p);
    }
    return *this;
}

SocketToPhawd *SocketToPhawd::create(int num_params){
    if (num_params <= 0){
        throw std::runtime_error("num_params is negative or zero!");
    }

    auto sp = (SocketToPhawd*) malloc(sizeof(SocketToPhawd) + sizeof(Parameter) * num_params);
    if (sp == nullptr){
        throw std::runtime_error("malloc error in SharedParameters::create!");
    }
    sp->numWaveParams = num_params;
    for (int i = 0; i < num_params; ++i) {
        sp->parameters[i] = Parameter();
    }
    return sp;
}

void SocketToPhawd::destroy(SocketToPhawd *p){
    free(p);
    p = nullptr;
}

void SocketToPhawd::setParameters(py::list &param_list){
    size_t size = param_list.size();
    std::vector<Parameter> params(size);
    params = py::cast<std::vector<Parameter>>(param_list);
    size_t range = size <= numWaveParams? size: numWaveParams;
    for (size_t i = 0; i < range; ++i) {
        parameters[i] = params[i];
    }
}

py::list SocketToPhawd::getParameters(){
    static py::list result(numWaveParams);
    for (int i = 0; i < numWaveParams; ++i) {
        result.append(parameters[i]);
    }
    return result;
}

void SocketToPhawd::collectParameters(ParameterCollection *pc){
    for (size_t i = 0; i < numWaveParams; ++i) {
        pc->addParameter(&parameters[i]);
    }
}