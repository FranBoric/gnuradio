/*
 * Copyright 2021 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(rfnoc_fir_filter.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(f37eb9d3caef81291878a7a63798d6c1)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/uhd/rfnoc_fir_filter.h>
// pydoc.h is automatically generated in the build directory
#include <rfnoc_fir_filter_pydoc.h>

void bind_rfnoc_fir_filter(py::module& m)
{

    using rfnoc_fir_filter = ::gr::uhd::rfnoc_fir_filter;


    py::class_<rfnoc_fir_filter,
               gr::uhd::rfnoc_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<rfnoc_fir_filter>>(
        m, "rfnoc_fir_filter", D(rfnoc_fir_filter))


        .def(py::init(&rfnoc_fir_filter::make),
             py::arg("graph"),
             py::arg("block_args"),
             py::arg("device_select"),
             py::arg("instance"),
             D(rfnoc_fir_filter, make))


        .def("set_coefficients",
             py::overload_cast<const std::vector<float>&, const size_t>(
                 &rfnoc_fir_filter::set_coefficients),
             py::arg("coeffs"),
             py::arg("chan") = 0,
             D(rfnoc_fir_filter, set_coefficients))


        .def("set_coefficients",
             py::overload_cast<const std::vector<int16_t>&, const size_t>(
                 &rfnoc_fir_filter::set_coefficients),
             py::arg("coeffs"),
             py::arg("chan") = 0,
             D(rfnoc_fir_filter, set_coefficients))


        .def("get_max_num_coefficients",
             &rfnoc_fir_filter::get_max_num_coefficients,
             py::arg("chan") = 0,
             D(rfnoc_fir_filter, get_max_num_coefficients))


        .def("get_coefficients",
             &rfnoc_fir_filter::get_coefficients,
             py::arg("chan") = 0,
             D(rfnoc_fir_filter, get_coefficients))

        ;
}
