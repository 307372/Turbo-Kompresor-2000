#ifndef PROJECT_EXCEPTIONS_H
#define PROJECT_EXCEPTIONS_H

#include <exception>
#include <string>


class NotImplementedException : public std::exception {
public:
    explicit NotImplementedException(const char *error = "Functionality not yet implemented!");
    [[nodiscard]] const char * what() const noexcept;
private:
    std::string errorMessage;
};



class FlagReservedException : public std::exception {
public:
    explicit FlagReservedException(const char *error = "This flag is reserved for future use, and does nothing yet. Something went wrong!");
    [[nodiscard]] const char * what() const noexcept;

private:
    std::string errorMessage;
};



class VariableTypeException : public std::exception {
public:
    explicit VariableTypeException(const char *error = "Your version of this variable is different than mine. This algorithm probably won't work with it.");
    [[nodiscard]] const char * what() const noexcept;

private:
    std::string errorMessage;
};

class NothingLeftToReadException : public std::exception {
public:
    explicit NothingLeftToReadException(const char *error = "All the necessary bits have already been read. This shouldn't be called.");
    [[nodiscard]] const char * what() const noexcept;

private:
    std::string errorMessage;
};

#endif //EXPERIMENTAL_PROJECT_EXCEPTIONS_H
