#include "cli.hpp"

#include <exception>


namespace cli
{
namespace args
{
std::vector<ArgType> options =
    {
        ArgType::mode,
        ArgType::alg,
        ArgType::archive,
        ArgType::output,
        ArgType::fileToAdd
    };

std::vector<std::string> enumToString =
    {
        "mode",
        "alg",
        "archive",
        "output",
        "fileToAdd"
    }; 

std::map<std::string, ArgType> strToEnum =
    {
        {"mode", ArgType::mode},
        {"alg", ArgType::alg},
        {"archive", ArgType::archive},
        {"output", ArgType::output},
        {"fileToAdd", ArgType::fileToAdd},
    }; 

std::string strToParam(std::string text)
{
    return "--" + text + "=";
}

std::string enumToParam(ArgType arg)
{
    return strToParam(enumToString[arg]);
}

} // namespace args

std::string getArgTypeue(args::ArgType arg, Args args, int argInd)
{
    std::string paramSubstr = enumToParam(arg);
    return args[argInd].substr(paramSubstr.length());
}

size_t findArg(args::ArgType arg, Args args)
{
    std::string paramSubstr = enumToParam(arg);
    for (size_t i=0; i < args.size(); ++i)
    {
        size_t pos = args[i].find(paramSubstr);
        if (pos != std::string::npos)
        {
            return i;
        }
    }
    return std::string::npos;
}

std::string parseMandatoryString(args::ArgType argType, Args args)
{
    size_t argInd = findArg(argType, args);
    if (argInd == std::string::npos)
    {
        throw std::runtime_error("Error: No " + args::enumToString[argType] + " given");
    }
    std::string argVal = getArgTypeue(argType, args, argInd);
    if (argVal.size() == 0)
    {
        throw std::runtime_error("Error: " + args::enumToString[argType] + " too short!");
    }
    return argVal;
}

multithreading::mode parseOperationMode(Args args)
{
    std::string argVal = parseMandatoryString(args::ArgType::mode, args);
    if (argVal == "compress")
    {
        return multithreading::mode::compress;
    }
    else if (argVal == "decompress")
    {
        return multithreading::mode::decompress;
    }
    throw std::runtime_error("Error: unknown operation mode");
}

std::bitset<16> parseAlgorithmFlags(Args args)
{
    std::string argVal = parseMandatoryString(args::ArgType::alg, args);
    std::bitset<16> flags{0};
    if (argVal == "1")
    {
        // flags.set(15); // SHA-1
        flags.set(0); // BWT (DC3)
        flags.set(1); // MTF
        flags.set(2); // RLE
        flags.set(3); // AC (naive)
        //flags.set(4); // AC (better)
        return flags;
    }
    else if (argVal == "2")
    {
        // flags.set(15); // SHA-1
        flags.set(0); // BWT (DC3)
        flags.set(1); // MTF
        flags.set(2); // RLE
        //flags.set(3); // AC (naive)
        flags.set(4); // AC (better)
        return flags;
    }
    throw std::runtime_error("Error: unknown alg id");
}

std::string parseArchivePath(Args args)
{
    return parseMandatoryString(args::ArgType::archive, args);
}

std::string parseOutputPath(Args args)
{
    return parseMandatoryString(args::ArgType::output, args);
}

std::string parseFileToAddPath(Args args)
{
    return parseMandatoryString(args::ArgType::fileToAdd, args);
}


void createArchiveWithSingleCompressedFile(const std::bitset<16>& flags, std::string fileToAddPath, std::string archivePath)
{
    Archive archive;
    std::cout << "bitset:" << flags << std::endl;
    uint16_t flags_num = (uint16_t) flags.to_ulong();
    archive.add_file_to_archive_model(std::ref(archive.root_folder), fileToAddPath, flags_num);
    bool fakeAbortingVar = false;
    archive.save(archivePath, fakeAbortingVar);
    archive.close();
}


void unpackArchiveWithSingleCompressedFile(std::string unpackedPath, std::string archivePath)
{
    Archive archive;
    archive.load(archivePath);
    bool fakeAbortingVar = false;
    archive.unpack_whole_archive(unpackedPath, archive.archive_file, fakeAbortingVar);
    archive.close();
}


void parseArgs(Args args)
{
    multithreading::mode opMode = parseOperationMode(args);
    std::string archivePath = parseArchivePath(args);

    if (opMode == multithreading::mode::compress)
    {
        std::bitset<16> flags = parseAlgorithmFlags(args);
        std::string fileToAddPath = parseFileToAddPath(args);
        createArchiveWithSingleCompressedFile(flags, fileToAddPath, archivePath);
    }
    else
    {
        std::string outputPath = parseOutputPath(args);
        unpackArchiveWithSingleCompressedFile(outputPath, archivePath);
    }
}


void handleArgs(int argc, char *argv[])
{
    Args args(argv + 1, argv + argc);
    parseArgs(args);
}

} // namespace cli

