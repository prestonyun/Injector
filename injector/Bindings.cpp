#include "pch.h"
#include <pybind11/pybind11.h>
#include "Injector.hpp"

namespace py = pybind11;

PYBIND11_MODULE(injector, m) {
    py::class_<Injector>(m, "Injector")
        .def("inject", &Injector::Inject);
}
