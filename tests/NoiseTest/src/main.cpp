#include <columns/buildandrun.hpp> // Initialization function for PetaVision
#include <columns/PV_Init.hpp>     // Needed to register custom classes
#include <layers/NoiseLayer.hpp>   // Our custom layer to add noise to input

int main(int argc, char *argv[])
{
    PV_Init pv_initObj(&argc, &argv, false /*do not allow unrecognized arguments*/);
    pv_initObj.registerKeyword("NoiseLayer", Factory::create<NoiseLayer>);
    int status = buildandrun(&pv_initObj, NULL, NULL);
    return status == PV_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
