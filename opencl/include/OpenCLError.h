//
// Created by David Young on 2024/05/03.
//

#ifndef EEE4120F_YODA_OPENCLERROR_H
#define EEE4120F_YODA_OPENCLERROR_H

#include <exception>
#include <string>

class OpenCLError : public std::exception {
private:
    std::string message;
public:
    OpenCLError(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override {
            return message.c_str();
    }
};

#endif //EEE4120F_YODA_OPENCLERROR_H
