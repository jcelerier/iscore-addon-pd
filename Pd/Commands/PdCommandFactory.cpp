#include "PdCommandFactory.hpp"
#include <iscore/command/SerializableCommand.hpp>

const CommandParentFactoryKey& Pd::CommandFactoryName() {
    static const CommandParentFactoryKey key{"Pd"};
    return key;
}
