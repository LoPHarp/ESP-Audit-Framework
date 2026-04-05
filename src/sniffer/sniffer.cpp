#include "sniffer.h"

#include <format>

using namespace std;

string MacAddress::toString() const
{
    return format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}