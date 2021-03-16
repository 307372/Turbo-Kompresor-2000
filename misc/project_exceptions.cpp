#include "project_exceptions.h"


NotImplementedException::NotImplementedException(const char* error) {
    errorMessage = error;
}


[[nodiscard]] const char * NotImplementedException::what() const noexcept {
    return errorMessage.c_str();
}


FlagReservedException::FlagReservedException(const char* error) {
    errorMessage = error;
}


[[nodiscard]] const char* FlagReservedException::what() const noexcept {
    return errorMessage.c_str();
}


VariableTypeException::VariableTypeException(const char* error) {
    errorMessage = error;
}


[[nodiscard]] const char* VariableTypeException::what() const noexcept {
    return errorMessage.c_str();
}


NothingLeftToReadException::NothingLeftToReadException(const char* error) {
    errorMessage = error;
}


[[nodiscard]] const char* NothingLeftToReadException::what() const noexcept {
    return errorMessage.c_str();
}
