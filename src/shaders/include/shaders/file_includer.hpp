#pragma once

#include <unordered_set>

#include <shaderc/shaderc.hpp>

namespace krypton::shaders {
    class FileIncluder : public shaderc::CompileOptions::IncluderInterface {
        struct FileInfo {
            const std::string fullFilePath;
            std::vector<char> fileContents;
        };

        std::unordered_set<std::string> allIncludedFiles;

        static shaderc_include_result* includeErrorResult(const std::string& message);
        static std::string readFileAsString(const std::string& filename);
        /** Simple gets the file path inside the shaders/ directory. */
        static std::string findFile(const std::string& inclusionPath, const std::string& source);

    public:
        explicit FileIncluder();

        shaderc_include_result* GetInclude(const char* requestedSource,
                                           shaderc_include_type type,
                                           const char* requestingSource,
                                           size_t includeDepth) override;

        void ReleaseInclude(shaderc_include_result* includeResult) override;
    };
}
