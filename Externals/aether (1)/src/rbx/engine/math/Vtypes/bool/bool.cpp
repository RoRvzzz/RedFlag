
#include "../../../../main.h"
#include "../../../../../misc/Umodule/Umodule.hpp"
#include "../../../Classes/offsets/offsets.hpp"
void RBX::Instance::SetBoolFromValue(bool value) const
{
    Umodule::write<bool>(this->address +  Offsets::Value, value);
}
bool RBX::Instance::getBoolFromValue() const
{
   return  Umodule::read<std::uint8_t>(this->address +  Offsets::Value);
}
namespace RBX {
template <typename T>
void Instance::WriteValue( const T& value) {
    Umodule::write<T>(this->address, Offsets::Value, value);
}


template <typename T>
T Instance::ReadValue() {
    return Umodule::read<T>(this->address);
}

}
