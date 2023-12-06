#include "AsconPy.h"

PYBIND11_MODULE(AsconPy, m) {
    py::class_<Ascon>(m, "Ascon")
        .def(py::init<py::array_t<uint8_t>>())
        .def("encrypt", &Ascon::encrypt)
        .def("decrypt", &Ascon::decrypt);
}