#pragma once

namespace selfchecksum {

class self_checksum
{
public:
    self_checksum() = default;

public:
    void run(const std::string& binary_name, unsigned connectivity);
    void run(const std::string& binary_name, const std::string& module_name, unsigned connectivity);
}; // class self_checksum

} // namespace selfchecksum

