#ifndef TK2K_CLI_H
#define TK2K_CLI_H

namespace cli
{

namespace args
{
enum ArgType
{
    mode=0,
    alg=1,
    archive=2,
    output=3,
    fileToAdd=4,
};
} // namespace args

void handleArgs(int argc, char *argv[]);
} // namespace cli

#endif