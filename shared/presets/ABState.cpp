#include "presets/ABState.h"

namespace ovni::presets
{
void ABState::toggle()
{
    if (fresh) (onA ? b : a) = apvts.copyState();   // Periscope: the other slot starts from the current state
    fresh = false;
    (onA ? a : b) = apvts.copyState();              // guardar el slot activo
    onA = ! onA;
    apvts.replaceState ((onA ? a : b).createCopy()); // cargar el otro
    if (onStateReplaced) onStateReplaced();
}

void ABState::copyActiveToOther()
{
    (onA ? b : a) = apvts.copyState();              // el otro arranca desde el activo
    fresh = false;
}
}
