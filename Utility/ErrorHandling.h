#ifndef ERRORHANDLING_H
#define ERRORHANDLING_H
#include <stdexcept>
#include <string>

namespace BSPParser::Utility
{
    /**
     * \brief Tosses a runtime error but ensures the given memory is deallocated before erroring.
     * \tparam T Type of the pointer to free
     * \param msg Message to set in the runtime error exception
     * \param ptr Reference to the variable holding the pointer
     */
    template<typename T>
    void ThrowSafeError(std::string&& msg, T& ptr)
    {
        free(ptr);
        ptr = nullptr;

        throw std::runtime_error(msg);
    }
}

#endif //ERRORHANDLING_H
