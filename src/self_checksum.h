#pragma once

namespace selfchecksum {

class self_checksum
{
public:
    self_checksum() = default;

public:
    void run(const std::string& binary_name, unsigned connectivity);
};

}

