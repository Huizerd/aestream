#include "convert.hpp"

#include <torch/extension.h>
#include <torch/script.h>

torch::Tensor
convert_polarity_events(std::vector<AEDAT::PolarityEvent> &polarity_events) {
  size_t size = polarity_events.size();
  std::vector<int64_t> indices(3 * size);
  std::vector<int8_t> values;
  size_t idx = 0;
  for (auto event : polarity_events) {
    indices[idx] = event.timestamp;
    indices[size + idx] = event.x;
    indices[2 * size + idx] = event.y;
    values.push_back(event.polarity ? 1 : -1);
    idx++;
  }

  auto index_options = torch::TensorOptions().dtype(torch::kInt64);
  torch::Tensor ind = torch::from_blob(
      indices.data(), {3, static_cast<uint32_t>(size)}, index_options);

  auto value_options = torch::TensorOptions().dtype(torch::kInt8);
  torch::Tensor val = torch::from_blob(
      values.data(), {static_cast<uint32_t>(values.size())}, value_options);
  auto events = torch::sparse_coo_tensor(ind, val);

  return events.clone();
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  py::class_<AEDAT::PolarityEvent>(m, "PolarityEvent");
  py::class_<AEDAT>(m, "AEDAT")
      .def(py::init<>())
      .def(py::init<const std::string &>())
      .def("load", &AEDAT::load)
      .def_readwrite("polarity_events", &AEDAT::polarity_events)
      .def_readwrite("dynapse_events", &AEDAT::dynapse_events)
      .def_readwrite("imu6_events", &AEDAT::imu6_events)
      .def_readwrite("imu9_events", &AEDAT::imu9_events);
  m.def("convert_polarity_events", &convert_polarity_events,
        "convert_polarity_events");
}