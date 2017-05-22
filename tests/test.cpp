#include <iostream>

void dummy_function()
{
    std::cout << "dummy function\n";    
}

int main()
{
    int i = 0;
    if (i == 0) {
        i = 3;
    } else {
        i = 7;
    }
    dummy_function();
    std::cout << i << "\n";
    return 0;
}

