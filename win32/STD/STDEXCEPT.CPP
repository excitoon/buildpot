#include <stdexcept>

#include <common.h>


logic_error::logic_error(const char * what_arg)
{
}

unsigned long logic_error::code()
{
    return 0x0eab723f & 0xefffffff;
}

domain_error::domain_error(const char * what_arg)
{
}

unsigned long domain_error::code()
{
    return 0x8e1ec084 & 0xefffffff;
}

invalid_argument::invalid_argument(const char * what_arg)
{
}

unsigned long invalid_argument::code()
{
    return 0xdef6e260 & 0xefffffff;
}

length_error::length_error(const char * what_arg)
{
}

unsigned long length_error::code()
{
    return 0xa2324897 & 0xefffffff;
}

out_of_range::out_of_range(const char * what_arg)
{
}

unsigned long out_of_range::code()
{
    return 0x174110aa & 0xefffffff;
}

runtime_error::runtime_error(const char * what_arg)
{
}

unsigned long runtime_error::code()
{
    return 0x4f7420bc & 0xefffffff;
}

overflow_error::overflow_error(const char * what_arg)
{
}

unsigned long overflow_error::code()
{
    return 0xdce315b5 & 0xefffffff;
}

underflow_error::underflow_error(const char * what_arg)
{
}

unsigned long underflow_error::code()
{
    return 0x763900a6 & 0xefffffff;
}
