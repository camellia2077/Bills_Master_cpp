// reprocessing/wrapper.cpp

#include <pybind11/pybind11.h>
#include "Reprocessor.h" // 包含我们想要绑定的核心类

// pybind11 的命名空间别名，方便使用
namespace py = pybind11;

// PYBIND11_MODULE 是一个特殊的宏，它创建了一个函数，当 Python 执行 `import reprocessor_lib` 时会调用这个函数。
// 
// - 第一个参数 (reprocessor_lib) 是 Python 模块的名称。
// - 第二个参数 (m) 是一个 py::module_ 对象，它代表了这个模块本身。我们可以向这个对象添加绑定。
//
PYBIND11_MODULE(reprocessor_lib, m) {
    // 为整个模块添加一个文档字符串，在 Python 中可以通过 help(reprocessor_lib) 查看
    m.doc() = "Python bindings for the C++ bill reprocessing library, created with pybind11.";

    // **绑定 Reprocessor 类**
    // py::class_ 会创建一个 C++ 类的 Python 包装器。
    // - 模板参数 <Reprocessor> 是我们要绑定的 C++ 类。
    // - 第一个参数 (m) 是我们正在构建的模块对象。
    // - 第二个参数 ("Reprocessor") 是这个类在 Python 中将要使用的名字。
    py::class_<Reprocessor>(m, "Reprocessor")

        // **绑定构造函数**
        // .def(py::init<...>());
        // 我们需要告诉 pybind11 如何创建 Reprocessor 对象。
        // C++ 构造函数是 Reprocessor(const std::string& config_dir_path)。
        // pybind11 会自动将 Python 的字符串类型转换为 C++ 的 std::string。
        // py::arg("config_dir_path") 为参数指定了名字，这样在 Python 中可以使用关键字参数，提高可读性。
        .def(py::init<const std::string &>(), py::arg("config_dir_path"), "Constructor for Reprocessor")

        // **绑定 validate_bill 方法**
        // .def("python_method_name", &CppClassName::cpp_method_name, "docstring");
        // - "validate_bill" 是 Python 中调用的方法名。
        // - &Reprocessor::validate_bill 是一个指向 C++ 成员函数的指针。
        // - "docstring..." 是这个方法在 Python 中的文档字符串。
        .def("validate_bill", &Reprocessor::validate_bill,
             "Validates a bill file using config/Validator_Config.json.",
             py::arg("bill_path"))

        // **绑定 modify_bill 方法**
        // 同样地，我们绑定 modify_bill 方法。它接受两个字符串参数。
        .def("modify_bill", &Reprocessor::modify_bill,
             "Modifies a bill file using config/Modifier_Config.json.",
             py::arg("input_bill_path"),
             py::arg("output_bill_path"));
}