#include "PdCommandFactory.hpp"
#include <iscore/command/SerializableCommand.hpp>

const CommandGroupKey& Pd::CommandFactoryName() {
    static const CommandGroupKey key{"Pd"};
    return key;
}
