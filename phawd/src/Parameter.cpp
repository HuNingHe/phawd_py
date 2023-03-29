/*!
 * PHAWD - Parameters Handler and Waveform Display
 * Licensed under the GNU GPLv3 license. See LICENSE for more details.
 * @author HuNing-He
 * @date 2022-2-24
 * @version 0.2
 * @email 2689112371@qq.com
 * @copyright (c) 2022 HuNing-He
 * @file Parameters.cpp
 * @brief definition of parameter in phawd and parameters collection
 */

#include "Parameter.h"

using namespace phawd;

/*!
 * Check if the unordered map contain the given element
 */
template<typename T1, typename T2>
bool mapContains(const std::map<T1, T2> &set, T1 key) {
    return set.find(key) != set.end();
}

ParameterValue::ParameterValue() {
    d = 0;
    std::memset(this, 0, sizeof(ParameterValue));
}

py::array_t<double> ParameterValue::getVec3d() {
    py::array_t<double> result(3);
    auto buf_info = result.request();
    auto *data = static_cast<double *>(buf_info.ptr);
    for (int j = 0; j < 3; ++j) {
        data[j] = vec3d[j];
    }
    return result;
}

void ParameterValue::setVec3d(const py::array_t<double>& value) {
    auto size = value.size();
    if (size >= 3) {
        for (int j = 0; j < 3; ++j) {
            vec3d[j] = value.at(j);
        }
    } else {
        for (int j = 2; j > 2 - (3 - size); --j) {
            vec3d[j] = 0;
        }
        for (int j = 0; j < size; ++j) {
            vec3d[j] = value.at(j);
        }
    }
}

Parameter::Parameter() {
    std::memset(m_name, 0, sizeof(m_name));
    std::memset(&m_value, 0, sizeof(ParameterValue));
    m_set = false;
    m_kind = ParameterKind::DOUBLE;
}

Parameter::Parameter(const std::string& name, ParameterKind &kind) {
    std::memset(&m_value, 0, sizeof(ParameterValue));
    if(!this->setName(name)) return;
    m_set = true;
    m_kind = kind;
}

Parameter::Parameter(const std::string& name, ParameterKind &kind, ParameterValue &value) {
    if(!this->setName(name)) return;
    std::memcpy(&m_value, &value, sizeof(ParameterValue));
    m_kind = kind;
    m_set = true;
}

Parameter::Parameter(const std::string& name, double value) {
    if(!this->setName(name)) return;
    m_set = true;
    m_value.d = value;
    m_kind = ParameterKind::DOUBLE;
}

Parameter::Parameter(const std::string& name, long int value) {
    if(!this->setName(name)) return;
    m_set = true;
    m_value.i = value;
    m_kind = ParameterKind::S64;
}

Parameter::Parameter(const std::string& name, const py::array_t<double>& value) {
    if(!this->setName(name)) return;
    auto size = value.size();
    if (size >= 3) {
        for (int j = 0; j < 3; ++j) {
            m_value.vec3d[j] = value.at(j);
        }
    } else {
        for (int j = 2; j > 2 - (3 - size); --j) {
            m_value.vec3d[j] = 0;
        }
        for (int j = 0; j < size; ++j) {
            m_value.vec3d[j] = value.at(j);
        }
    }
    m_set = true;
    m_kind = ParameterKind::VEC3_DOUBLE;
}

void Parameter::setValueKind(ParameterKind kind) {
    m_kind = kind;
}

ParameterKind Parameter::getValueKind(){
    return m_kind;
}

void Parameter::setValue(double value){
    m_kind = ParameterKind::DOUBLE;
    m_value.d = value;
    m_set = true;
}

void Parameter::setValue(long int value){
    m_kind = ParameterKind::S64;
    m_value.i = value;
    m_set = true;
}

void Parameter::setValue(const py::array_t<double>& value) {
    m_kind = ParameterKind::VEC3_DOUBLE;
    auto size = value.size();
    if (size >= 3) {
        for (int j = 0; j < 3; ++j) {
            m_value.vec3d[j] = value.at(j);
        }
    } else {
        for (int j = 2; j > 2 - (3 - size); --j) {
            m_value.vec3d[j] = 0;
        }
        for (int j = 0; j < size; ++j) {
            m_value.vec3d[j] = value.at(j);
        }
    }
    m_set = true;
}

void Parameter::setValue(ParameterKind kind, const ParameterValue& value) {
    if(m_kind != kind) {
        printf("[ Parameter]: The parameter type is different with setting type.");
        throw std::runtime_error(" parameter type mismatch in set");
    }
    switch(m_kind) {
        case ParameterKind::DOUBLE:
            m_value.d = value.d;
            break;
        case ParameterKind::S64:
            m_value.i = value.i;
            break;
        case ParameterKind::VEC3_DOUBLE:{
            for(int i = 0; i < 3; i++){
                m_value.vec3d[i] = value.vec3d[i];
            }
            break;
        }
        default:return;
    }
    m_set = true;
}

/*!
* Get the value of a  parameter.  Does type checking - you must provide
* the correct type.
* @param kind : the kind of the  parameter
* @return the value of the  parameter
*/
ParameterValue Parameter::getValue(ParameterKind kind) {
    ParameterValue value;
    if (kind != m_kind) {
        printf("[ Parameter]: The parameter type is different with setting type.");
        throw std::runtime_error(" parameter type mismatch in get");
    }
    switch (m_kind) {
        case ParameterKind::DOUBLE:
            value.d = m_value.d;
            break;
        case ParameterKind::S64:
            value.i = m_value.i;
            break;
        case ParameterKind::VEC3_DOUBLE:{
            value.vec3d[0] = m_value.vec3d[0];
            value.vec3d[1] = m_value.vec3d[1];
            value.vec3d[2] = m_value.vec3d[2];
            break;
        }
        default:
        throw std::runtime_error(" parameter invalid kind in get");
    }
    return value;
}

double Parameter::getDouble() {
    if (m_kind != ParameterKind::DOUBLE){
        throw std::runtime_error("Parameter::getDouble(): type error");
    }
    return m_value.d;
}

long int Parameter::getS64(){
    if (m_kind != ParameterKind::S64){
        throw std::runtime_error("Parameter::getS64(): type error");
    }
    return m_value.i;
}

bool Parameter::setName(const std::string& name) {
    if(name.length() > 16 || name.empty()){
        printf("[Parameter]: The parameter name size is invalid when construct it. should be in range[1, 16]");
        return false;
    } else {
        strcpy(m_name, name.c_str());
        return true;
    }
}

std::string Parameter::getName() {
    return m_name;
}

py::array_t<double> Parameter::getVec3d() {
    py::array_t<double> result(3);
    auto buf_info = result.request();
    auto *data = static_cast<double *>(buf_info.ptr);
    for (int j = 0; j < 3; ++j) {
        data[j] = m_value.vec3d[j];
    }
    return result;
}

Parameter::Parameter(const Parameter &parameter) : Parameter() {
    *this = parameter;
}

Parameter::Parameter(Parameter &&parameter) noexcept : Parameter() {
    *this = std::move(parameter);
}

Parameter &Parameter::operator=(const Parameter &parameter) {
    if (this != &parameter) {
        m_set = parameter.m_set;
        m_kind = parameter.m_kind;
        std::memcpy(m_name, parameter.m_name, sizeof(m_name));
        std::memcpy(&m_value, &parameter.m_value, sizeof(ParameterValue));
    }
    return *this;
}

Parameter &Parameter::operator=(Parameter &&parameter) noexcept{
    if (this != &parameter) {
        m_set = parameter.m_set;
        m_kind = parameter.m_kind;
        std::memcpy(m_name, parameter.m_name, sizeof(m_name));
        std::memcpy(&m_value, &parameter.m_value, sizeof(ParameterValue));
    }
    return *this;
}

bool& Parameter::isSet(){
    return m_set;
}

void Parameter::set(bool set){
    m_set = set;
}

void ParameterCollection::addParameter(Parameter* param) {
    std::string name = param->getName();
    if (mapContains(m_map, name)) {
        // printf("[ERROR] ParameterCollection %s: tried to add parameter %s twice!\n", m_name.c_str(), name.c_str());
        // throw std::runtime_error(" parameter error [" + m_name + "]: parameter " + name + " appears twice!");
        return;
    }
    m_map[name] = param;
}

Parameter& ParameterCollection::lookup(const std::string& name) {
    if(mapContains(m_map, name)){
        return *m_map[name];
    } else {
        throw std::runtime_error(" parameter " + name + " wasn't found in parameter collection " + m_name);
    }
}

bool ParameterCollection::checkIfAllSet() {
    return std::all_of(m_map.begin(), m_map.end(), [](const std::pair<std::string, Parameter*>& kv){return kv.second->isSet();});
}

void ParameterCollection::clearAllSet() {
    for (auto& kv : m_map) {
        kv.second->set(false);
    }
}

void ParameterCollection::clearAllParameters() {
    m_map.clear();
}
