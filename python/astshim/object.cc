/*
 * LSST Data Management System
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 * See the COPYRIGHT file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */
#include <pybind11/pybind11.h>

#include "astshim/base.h"
#include "astshim/Stream.h"
#include "astshim/Channel.h"
#include "astshim/Object.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace ast {
namespace {

/*
Make an Object from a string output by Object.show()

Used to support pickling Object and all of its subclasses

Note: I would prefer to use a function but defining the obvious function
and returning that as the first item of Object.__reduce__ resulted in
Python rejecting it, claiming the function was not pickleable. I know how
to make a class pickleable, I don't know how to make a function pickleable.
*/
class ObjectMaker {
public:
    ObjectMaker() = default;
    ~ObjectMaker() = default;
    std::shared_ptr<Object> operator()(std::string const &state) {
        ast::StringStream stream(state);
        ast::Channel chan(stream);
        return chan.read();
    }
};

PYBIND11_PLUGIN(object) {
    py::module mod("object", "Python wrapper for Object");

    py::module::import("astshim.base");

    py::class_<ObjectMaker, std::shared_ptr<ObjectMaker>> makerCls(mod, "ObjectMaker");
    makerCls.def(py::init<>());
    makerCls.def("__call__", &ObjectMaker::operator());
    makerCls.def("__reduce__",
                       [makerCls](ObjectMaker const &self) { return py::make_tuple(makerCls, py::tuple()); });

    py::class_<Object, std::shared_ptr<Object>> cls(mod, "Object");

    cls.def_static("fromString", &Object::fromString);
    // do not wrap fromAstObject because it uses a bare AST pointer

    cls.def("__str__", &Object::getClassName);
    cls.def("__repr__", [](Object const &self) { return "astshim." + self.getClassName(); });
    cls.def("__eq__", &Object::operator==, py::is_operator());
    cls.def("__ne__", &Object::operator!=, py::is_operator());

    cls.def_property_readonly("className", &Object::getClassName);
    cls.def_property("id", &Object::getID, &Object::setID);
    cls.def_property("ident", &Object::getIdent, &Object::setIdent);
    cls.def_property_readonly("objSize", &Object::getObjSize);
    cls.def_property("useDefs", &Object::getUseDefs, &Object::setUseDefs);

    cls.def("copy", &Object::copy);
    cls.def("clear", &Object::clear, "attrib"_a);
    cls.def("hasAttribute", &Object::hasAttribute, "attrib"_a);
    cls.def("getNObject", &Object::getNObject);
    cls.def("getRefCount", &Object::getRefCount);
    cls.def("lock", &Object::lock, "wait"_a);
    cls.def("same", &Object::same, "other"_a);
    // do not wrap the ostream version of show, since there is no obvious Python equivalent to ostream
    cls.def("show", (std::string(Object::*)(bool) const) & Object::show, "showComments"_a = true);

    cls.def("test", &Object::test, "attrib"_a);
    cls.def("unlock", &Object::unlock, "report"_a = false);
    // do not wrap getRawPtr, since it returns a bare AST pointer

    // add pickling support
    cls.def("__reduce__", [makerCls](Object const &self) {
        std::string state = self.show(false);
        auto unpickleArgs = py::make_tuple(state);
        return py::make_tuple(makerCls(), unpickleArgs);
    });

    return mod.ptr();
}

}  // namespace
}  // namespace ast
