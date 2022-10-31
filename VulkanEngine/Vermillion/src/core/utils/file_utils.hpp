#pragma once

std::vector<std::string> get_directories(std::string relPath)
{
    size_t padSize = std::filesystem::current_path().append(relPath).string().size();
    std::vector<std::string> dirs;
    for (auto& p : std::filesystem::directory_iterator(std::filesystem::current_path().append(relPath))) {
        if (p.is_directory()) {
            std::string str = p.path().string();
            str.erase(0, padSize + 1);
            dirs.push_back(str);
        }
    }
    return dirs;
}

bool VectorOfStringGetter(void* data, int n, const char** out_text)
{
    const std::vector<std::string>* v = (std::vector<std::string>*)data;
    *out_text = (*v)[n].c_str();
    return true;
}