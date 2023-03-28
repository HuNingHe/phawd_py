/*!
 * PHAWD - Parameters Handler and Waveform Display
 * Licensed under the GNU GPLv3 license. See LICENSE for more details.
 * @author HuNing-He
 * @date 2022-2-24
 * @version 0.2
 * @email 2689112371@qq.com
 * @copyright (c) 2022 HuNing-He
 * @file Parameters.h
 * @brief definition of parameter in phawd and parameters collection
 */

#pragma once
#include <map>
#include <string>
#include <cstring>
#include <utility>
#include <iostream>
#include <stdexcept>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace phawd {
/*!
 * Value of a control parameter
 */
union ParameterValue {
    double d;
    long int i;
    double vec3d[3];

    ParameterValue();

    void setVec3d(const py::array_t<double>& value);

    py::array_t<double> getVec3d();
};

/*!
 * Data types supported for control parameters
 * we use enum class, in case that namespace pollution.
 * like FLOAT and DOUBLE
 */
enum class ParameterKind: unsigned short int {
    FLOAT = 0,
    DOUBLE = 1,
    S64 = 2,
    VEC3_FLOAT = 3,
    VEC3_DOUBLE = 4
};

class Parameter {
private:
    bool m_set;
    char m_name[16] = {};
    ParameterKind m_kind;
    ParameterValue m_value;
public:
    Parameter();
    Parameter(const Parameter& parameter);
    Parameter(Parameter&& parameter) noexcept;
    Parameter &operator=(const Parameter &p);
    Parameter &operator=(Parameter &&p) noexcept;


    /*!
     * Used to init a parameter but do not set value for this parameter
     * @param name : name of parameter
     * @param kind : type of parameter
     */
    Parameter(const std::string& name, ParameterKind &kind);

    /*!
	 * @param name : name of parameter
	 * @param kind : type of parameter
	 * @param value : parameter value
	 */
    Parameter(const std::string& name, ParameterKind &kind, ParameterValue &value);

    /*!
     * Construct control parameter for a double
     * @param name : name of parameter
     * @param value : double value
     */
    Parameter(const std::string &name, double value);

    /*!
        * Construct control parameter for an s64
     * @param name : name of parameter
     * @param value : s64 value
     */
    Parameter(const std::string &name, long int value);

    /*!
     * Construct control parameter for a list of 3 floats
     * @param name : name of parameter
     * @param value : vec3d value
     */
    Parameter(const std::string &name, const py::array_t<double>& value);

    bool setName(const std::string& name);
    void setValue(double value);
    void setValue(long int value);
    void setValue(const py::array_t<double>& value);
    void setValue(ParameterKind kind, const ParameterValue& value);
    void setValueKind(ParameterKind kind);

    /*!
     * Get the value of a control parameter.  Does type checking - you must provide
     * the correct type.
     * @param kind : the kind of the control parameter
     * @return the value of the control parameter
     */
    ParameterValue getValue(ParameterKind kind);
    ParameterKind getValueKind();
    std::string getName();
    double getDouble();
    long int getS64();
    py::array_t<double> getVec3d();

    bool& isSet();
    void set(bool set);
};

/*!
 * ControlParameterCollections contains a map of all the control parameters which facilitates Read Parameters
 * Mainly used in webots robot program
 */
class ParameterCollection {
private:
    std::string m_name;
public:
    std::map<std::string, Parameter *> m_map;
    explicit ParameterCollection(std::string name)
        : m_name(std::move(name))
    {}
    /*!
     * Use this to add a parameter for the first time in the
     * Throws exception if you try to add a parameter twice.
     */
    void addParameter(Parameter *param);

    /*!
     * Lookup a control parameter by its name.
     * This does not modify the set field of the control parameter!
     *
     * Throws exception if parameter isn't found
     */
    Parameter &lookup(const std::string &name);

    //!< are all the control parameters initialized?
    bool checkIfAllSet();

    /*!
     * Mark all parameters as not set
     */
    void clearAllSet();

    /*!
     * Remove all parameters
     */
    void clearAllParameters();
};
}