//
// Created by pc on 03.11.2020.
//

#ifndef TURBO_KOMPRESOR_2000_INTEGRITY_VALIDATION_H
#define TURBO_KOMPRESOR_2000_INTEGRITY_VALIDATION_H
#include <string>

class integrity_validation {
    std::string SHA1;
public:
    integrity_validation();
    bool is_your_machine_big_endian();
    std::string get_SHA1( const std::string& path_to_file, bool& aborting_var );
};


#endif //TURBO_KOMPRESOR_2000_INTEGRITY_VALIDATION_H
