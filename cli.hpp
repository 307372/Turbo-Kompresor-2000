#ifndef TK2K_CLI_H
#define TK2K_CLI_H

namespace cli
{

namespace args
{
enum ArgType
{
    mode=0,
    alg,
    archive,
    output,
    fileToAdd,
    blockSize
};
} // namespace args

void handleArgs(int argc, char *argv[]);
} // namespace cli

#endif