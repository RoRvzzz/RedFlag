#include "../../../../main.h"
#include "../../../../../misc/Umodule/Umodule.hpp"
#include "../../../Classes/offsets/offsets.hpp"
void RBX::Instance::SetIntValue(int value)
{
    Umodule::write<int>(this->address +  Offsets::Value, value);
}
int RBX::Instance::getIntFromValue() const
{
    return    Umodule::read<int>(this->address +  Offsets::Value);
}