
// #pragma once
// #include "ztest_base.hpp"
// #include "ztest_utils.hpp"
// #include <any>
// #include <exception>
// #include <functional>
// class ZTestDataLoader {};
// template <typename T> class ZTestDataManager {
// private:
//   vector<T> _data;
//   int _cur = -1;

// public:
//   ZTestDataManager() {}
//   T yield() {
//     if (_cur < _data.size())
//       return _data[++_cur];
//     else
//       throw "No more data to yield";
//   }
//   void reset() { _cur = -1; }
//   bool isEmpty() { return _data.empty(); }
//   int getCur() { return _cur; }
//   bool hasNext() { return _cur < _data.size() - 1; }
// };
// template <typename T> class ZTestParameterized : public ZTestBase {
// private:
//   ZTestDataManager<T> data_manager = ZTestDataManager<T>();
//   function<any> _test_func;

// public:
//   ZTestParameterized(string name, ZType type, string description)
//       : ZTestBase(name, type, description) {}
//   virtual ZState run() override {
//     data_manager.reset();
//     while (data_manager.hasNext()) {
//       T data = data_manager.yield();
//       _test_func(data);
//       setState();
//       if (getState() == ZState::z_failed)
//         return ZState::z_failed;
//     }
//     return ZState::z_failed;
//   }
// };