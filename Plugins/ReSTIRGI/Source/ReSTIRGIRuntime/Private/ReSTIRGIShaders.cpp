#include "ReSTIRGIShaders.h"

IMPLEMENT_GLOBAL_SHADER(FReSTIRGIInitialSampleCS, "/Plugin/ReSTIRGI/ReSTIRGI.usf", "InitialSampleCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FReSTIRGITemporalResamplingCS, "/Plugin/ReSTIRGI/ReSTIRGI.usf", "TemporalResamplingCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FReSTIRGISpatialResamplingCS, "/Plugin/ReSTIRGI/ReSTIRGI.usf", "SpatialResamplingCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FReSTIRGIFinalCompositeCS, "/Plugin/ReSTIRGI/ReSTIRGI.usf", "FinalCompositeCS", SF_Compute);
