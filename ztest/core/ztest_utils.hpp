// #pragma once
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <stdexcept>
// #include <string>
// #include <vector>

// /**
//  * @class CSVStream
//  * @brief A class for handling CSV file operations with type-safe data access
//  */
// class CSVStream {
// public:
//   /// @brief CSV metadata container
//   struct CSVInfo {
//     size_t totalRows; ///< Total number of rows including headers
//     size_t dataRows;  ///< Data rows (excluding header if present)
//     size_t columns;   ///< Number of columns
//     std::vector<std::string> headers; ///< Column headers (if hasHeader=true)
//   };

//   /**
//    * @brief Construct a new CSVStream object
//    *
//    * @param filename Path to CSV file
//    * @param delimiter Field delimiter (default: ",")
//    */
//   explicit CSVStream(const std::string &filename,
//                      const std::string &delimiter = ",")
//       : filename_(filename), delimiter_(delimiter), mode_("overwrite") {}

//   // File mode configuration
//   CSVStream &setMode(const std::string &mode) {
//     mode_ = mode;
//     return *this;
//   }

//   // Core I/O operations
//   CSVStream &operator>>(std::vector<std::vector<std::string>> &data);
//   CSVStream &operator<<(const std::vector<std::vector<std::string>> &data);

//   // Data analysis
//   CSVInfo analyzeCSV(const std::vector<std::vector<std::string>> &data,
//                      bool hasHeader = false) const;

//   // Type-safe data accessors
//   template <typename T>
//   std::vector<T>
//   getRow(size_t rowIndex,
//          const std::vector<std::vector<std::string>> &data) const;

//   template <typename T>
//   std::vector<T>
//   getColumn(size_t colIndex,
//             const std::vector<std::vector<std::string>> &data) const;

// private:
//   // Internal enum for type-safe mode
//   enum class FileOpenMode { Append, Overwrite };

//   // Internal helpers
//   FileOpenMode getOpenMode() const;
//   void validateData(const std::vector<std::vector<std::string>> &data) const;

//   std::string filename_;
//   std::string delimiter_;
//   std::string mode_;
// };

// // Implementation of core methods
// inline CSVStream &
// CSVStream::operator>>(std::vector<std::vector<std::string>> &data) {
//   std::ifstream file(filename_);
//   if (!file.is_open()) {
//     throw std::runtime_error("Failed to open file for reading: " +
//     filename_);
//   }

//   data.clear();
//   std::string line;
//   while (std::getline(file, line)) {
//     std::vector<std::string> row;
//     std::stringstream ss(line);
//     std::string cell;

//     while (std::getline(ss, cell, delimiter_[0])) {
//       row.push_back(cell);
//     }
//     data.push_back(row);
//   }

//   return *this;
// }

// inline CSVStream &
// CSVStream::operator<<(const std::vector<std::vector<std::string>> &data) {

//   std::ofstream file;
//   auto openMode = getOpenMode();

//   if (openMode == FileOpenMode::Append) {
//     file.open(filename_, std::ios::app);
//   } else {
//     file.open(filename_, std::ios::out | std::ios::trunc);
//   }

//   if (!file.is_open()) {
//     throw std::runtime_error("Failed to open file for writing: " +
//     filename_);
//   }

//   for (const auto &row : data) {
//     for (size_t i = 0; i < row.size(); ++i) {
//       file << row[i];
//       if (i < row.size() - 1) {
//         file << delimiter_;
//       }
//     }
//     file << "\n";
//   }

//   file.close();
//   return *this;
// }

// // Private helper implementations
// inline CSVStream::FileOpenMode CSVStream::getOpenMode() const {
//   if (mode_ == "append") {
//     return FileOpenMode::Append;
//   } else if (mode_ == "overwrite") {
//     return FileOpenMode::Overwrite;
//   }
//   throw std::invalid_argument("Invalid mode: must be 'append' or
//   'overwrite'");
// }

// inline void CSVStream::validateData(
//     const std::vector<std::vector<std::string>> &data) const {
//   if (data.empty())
//     return;

//   size_t expectedColumns = data[0].size();
//   for (size_t i = 1; i < data.size(); ++i) {
//     if (data[i].size() != expectedColumns) {
//       throw std::runtime_error("Inconsistent column count in CSV data at row
//       " +
//                                std::to_string(i + 1));
//     }
//   }
// }

// // Data analysis implementation
// inline CSVStream::CSVInfo
// CSVStream::analyzeCSV(const std::vector<std::vector<std::string>> &data,
//                       bool hasHeader) const {

//   CSVInfo info;
//   info.totalRows = data.size();

//   if (data.empty()) {
//     info.columns = 0;
//     info.dataRows = 0;
//     return info;
//   }

//   validateData(data);
//   info.columns = data[0].size();
//   info.dataRows = hasHeader ? info.totalRows - 1 : info.totalRows;

//   if (hasHeader && info.totalRows > 0) {
//     info.headers = data[0];
//   }

//   return info;
// }

// // Template method implementations
// template <typename T>
// std::vector<T>
// CSVStream::getRow(size_t rowIndex,
//                   const std::vector<std::vector<std::string>> &data) const {

//   if (rowIndex >= data.size()) {
//     throw std::out_of_range("Row index out of range");
//   }

//   std::vector<T> result;
//   for (const auto &cell : data[rowIndex]) {
//     std::stringstream ss(cell);
//     T value;
//     ss >> value;
//     if (ss.fail()) {
//       throw std::runtime_error("Conversion failed for value: " + cell);
//     }
//     result.push_back(value);
//   }
//   return result;
// }

// template <typename T>
// std::vector<T>
// CSVStream::getColumn(size_t colIndex,
//                      const std::vector<std::vector<std::string>> &data) const
//                      {

//   std::vector<T> result;
//   for (size_t i = 0; i < data.size(); ++i) {
//     if (colIndex >= data[i].size()) {
//       throw std::out_of_range("Column index out of range in row " +
//                               std::to_string(i + 1));
//     }

//     std::stringstream ss(data[i][colIndex]);
//     T value;
//     ss >> value;
//     if (ss.fail()) {
//       throw std::runtime_error("Conversion failed in row " +
//                                std::to_string(i + 1));
//     }
//     result.push_back(value);
//   }
//   return result;
// }