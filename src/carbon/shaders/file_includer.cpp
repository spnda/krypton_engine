#include "file_includer.hpp"

#include <fstream>
#include <sstream>

carbon::FileIncluder::FileIncluder() = default;

shaderc_include_result* carbon::FileIncluder::GetInclude(
        const char *requestedSource, shaderc_include_type type,
        const char *requestingSource, size_t includeDepth) {
    const std::string fullFilePath = findFile(requestedSource, requestingSource);
    if (fullFilePath.empty()) {
        return carbon::FileIncluder::includeErrorResult("Failed to find file: " + std::string(requestedSource));
    }

    std::string fileContents;
    try {
        fileContents = carbon::FileIncluder::readFileAsString(fullFilePath);
    } catch (std::exception& exception) {
        return carbon::FileIncluder::includeErrorResult(exception.what());
    }

    auto* fileInfo = new FileInfo {
        .fullFilePath = fullFilePath,
        .fileContents = {fileContents.begin(),  fileContents.end()}
    };
    allIncludedFiles.insert(fullFilePath);
    return new shaderc_include_result {
        .source_name = fileInfo->fullFilePath.data(),
        .source_name_length = fileInfo->fullFilePath.length(),
        .content = fileInfo->fileContents.data(),
        .content_length = fileInfo->fileContents.size(),
        .user_data = fileInfo,
    };
}

void carbon::FileIncluder::ReleaseInclude(shaderc_include_result *includeResult) {
    auto* fileInfo = static_cast<FileInfo*>(includeResult->user_data);
    delete fileInfo;
    delete includeResult;
}

std::string carbon::FileIncluder::readFileAsString(const std::string& filename) {
    std::ifstream is(filename, std::ios::in);

    if (is.is_open()) {
        std::string shaderCode;
        std::stringstream stringBuffer;
        stringBuffer << is.rdbuf();
        shaderCode = stringBuffer.str();

        return shaderCode;
    } else {
        throw std::runtime_error(std::string("Failed to open shader file: ") + filename);
    }
}

shaderc_include_result* carbon::FileIncluder::includeErrorResult(const std::string &message) {
    return new shaderc_include_result {
        .source_name = "",
        .source_name_length = 0,
        .content = message.c_str(),
        .content_length = message.size(),
    };
}

std::string carbon::FileIncluder::findFile(const std::string &inclusionPath, const std::string &source) {
    // TODO: Improve this function.
    static const std::string baseFolder = "shaders/";
    return baseFolder + inclusionPath;
}
