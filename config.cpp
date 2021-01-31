#include "config.h"

#include <assert.h>
#include <iostream>


Config::Config()
{
    parse();
}


Config::~Config()
{
    if (config_file.is_open()) config_file.close();
}


void Config::parse()
{
    if (config_file.is_open()) config_file.close();

    if (std::filesystem::exists(std::filesystem::current_path() / "config.ini")) {

        config_file.open( std::filesystem::current_path() / "config.ini", std::ios::in );

        std::string buffer;
        uint32_t property_counter = 0;
        while (!config_file.eof()) {
            std::getline(config_file, buffer);

            if (buffer[0] == '#' and buffer[1] == '#') continue;    // my comments start with ##
            if (buffer.length() == 0) continue;                     // if line is empty, skip it

            switch( property_counter )
            {
            case 0: extraction_path = buffer; break;

            case 1: filesize_scaling = (buffer[0] == '1'); break;

            case 2: dark_mode = (buffer[0] == '1'); break;

            default: assert(false);
            }

            property_counter++;
        }
    }
    else { // if there's no config file
        save_default();
        parse();
    }



    if (config_file.is_open()) config_file.close();
}


void Config::save()
{
    if (config_file.is_open()) config_file.close();

    config_file.open( std::filesystem::current_path() / "config.ini", std::ios::out );
    assert(config_file.is_open());

    config_file << "## Configuration file for Turbo Kompresor 2000\n"
                << "## Saved extraction path:\n"
                << extraction_path.string() << '\n'
                << "## Are file sizes scaled?\n"
                << "## 0 - no, 1 - yes\n"
                << (int)filesize_scaling << '\n'
                << "## Is dark mode on?\n"
                << "## 0 - no, 1 - yes\n"
                << (int)dark_mode << '\n';

    if (config_file.is_open()) config_file.close();
}


void Config::save_default()
{
    if (config_file.is_open()) config_file.close();

    config_file.open( std::filesystem::current_path() / "config.ini", std::ios::out );
    assert(config_file.is_open());

    config_file << "## Configuration file for Turbo Kompresor 2000\n"
                << "## Saved extraction path:\n"
                << std::filesystem::current_path().string() << '\n'
                << "## Are file sizes scaled?\n"
                << "## 0 - no, 1 - yes\n"
                << 1 << '\n'
                << "## Is dark mode on?\n"
                << "## 0 - no, 1 - yes\n"
                << 0 << '\n';



    if (config_file.is_open()) config_file.close();
}
